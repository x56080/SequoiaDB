/*******************************************************************************

   Copyright (C) 2011-2020 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

#include "rtnRollbackManager.hpp"

#include <string>

#include "dpsLogWrapper.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsOp2Record.hpp"
#include "dpsTransID.hpp"
#include "ossUtil.hpp"
#include "pmd.hpp"
#include "rtn.hpp"

#include "boost/lexical_cast.hpp"

namespace engine
{

//
// rtnRollbackManager
//

rtnRollbackManager::rtnRollbackManager(_pmdEDUCB *cb)
    : _cb(cb), _dpsCB(pmdGetKRCB()->getDPSCB()),
      _cursor(DPS_INVALID_LSN_OFFSET),
      _mb(dpsMessageBlock(DPS_MSG_BLOCK_DEF_LEN)), _replayer(TRUE)
{
}

void rtnRollbackManager::execute()
{
   _init();
   // Read the log and rollback one by one
   while (_cursor != DPS_INVALID_LSN_OFFSET)
   {
      try
      {
         const dpsLogRecord record = _getRecord();
         _preProcess(record);
         const BOOLEAN undone = _rollback(record);
         _postProcess(record, undone);
         _nextRecord(record);
      }
      catch (std::exception &e)
      {
         _abort();
         throw pdGeneralException(pdGetLastError(), "Rollback failed");
      }
   }
   _finalize();
}

dpsLogRecord rtnRollbackManager::_getRecord()
{
   dpsLogRecord record;
   DPS_LSN dpsLsn;
   dpsLsn.offset = _cursor;
   _mb.clear(); // clean up the tmp storage
   if (INT32 rc = _dpsCB->search(dpsLsn, &_mb))
   {
      PD_LOG(PDERROR, "LSN search failed (LSN %llu)", _cursor);
      throw pdGeneralException(rc, "LSN search failed");
   }
   if (INT32 rc = record.load(_mb.offset(0)))
   {
      PD_LOG(PDERROR, "Loading record failed (LSN %llu)", _cursor);
      throw pdGeneralException(rc, "Loading record failed");
   }
   return record;
}

BOOLEAN rtnRollbackManager::_rollback(const dpsLogRecord &record)
{
   if (!_shouldUndo(record))
   {
      // Do not undo this record
      return FALSE;
   }
   _undo();
   return TRUE;
}

void rtnRollbackManager::_undo()
{
   // Set the TransRelatedLSN of the undo record to the record being undone
   _cb->setRelatedTransLSN(_cursor);
   // Perform the undo of the record
   if (INT32 rc = _replayer.rollback((dpsLogRecordHeader *)_mb.offset(0), _cb))
   {
      throw pdGeneralException(rc, "rollbackTrans failed");
   }
}

//
// rtnPITRollbackManager
//

rtnPITRollbackManager::rtnPITRollbackManager(pmdEDUCB *cb,
                                             const std::string targetTimeString)
    : rtnRollbackManager(cb)
{
   _targetTime = stpLogicalTimeUS();
   _targetTime.setTime(boost::lexical_cast<UINT64>(targetTimeString));
}

void rtnPITRollbackManager::_init()
{
   // Start at the end of the log
   _cursor = _dpsCB->getCurrentLsn().offset;

   // Start a new transaction for the rollback
   if (INT32 rc = rtnTransBegin(_cb, FALSE, TRUE))
   {
      throw pdGeneralException(rc, "rtnTransBegin failed");
   }
}

void rtnPITRollbackManager::_finalize()
{
   // Commit the transaction
   if (INT32 rc = rtnTransCommit(_cb, _dpsCB, _rollbackTime))
   {
      throw pdGeneralException(rc, "rtnTransCommit failed");
   }
}

void rtnPITRollbackManager::_abort()
{
   // Rollback the rollback transaction
   if (INT32 rc = rtnTransRollback(_cb, _dpsCB))
   {
      throw pdGeneralException(rc, "rtnTransRollback failed");
   }
}

void rtnPITRollbackManager::_processCommitRecord(const dpsLogRecord &record)
{
   // Get the transaction time from the commit record
   stpLogicalTimeUS recordTransTime;
   if (INT32 rc =
           dpsGetTransTimeFromRecord(record, _recordTransID, recordTransTime))
   {
      throw pdGeneralException(rc, "dpsGetTransTimeFromRecord failed");
   }
   if (recordTransTime.getTime() > _rollbackTime.getTime())
   {
      // this is the max time (so far), so use it for the new trx time
      _rollbackTime.setTime(recordTransTime.getTime() + 1);
      _rollbackTime.setTimeError(1);
   }
   if (recordTransTime < _targetTime)
   {
      // This transaction committed before the target so skip it
      return;
   }
   if (_isTransInUndoTransSet())
   {
      // This transaction is already marked for undo, this must be a pre-commit
      SDB_ASSERT(record.isPreCommit(),
                 "Found a final commit record for a transaction already in the "
                 "undo transaction set");
      return;
   }
   _undoTransSet.insert(_recordTransID);
}

void rtnPITRollbackManager::_processBeginRecord()
{
   // All records for this transaction have been processed. Remove this
   // transaction from the set of transactions to undo.
   _undoTransSet.erase(_recordTransID);
}

void rtnPITRollbackManager::_preProcess(const dpsLogRecord &record)
{
   // Extract the transaction ID. Don't check the rc because failure just means
   // it is a non-transactional record, in which case the ID will fail its
   // isValid() check later.
   _recordTransID.reset();
   dpsGetTransIDFromRecord(record, _recordTransID);
   if (record.isCommit())
   {
      _processCommitRecord(record);
   }
}

void rtnPITRollbackManager::_postProcess(const dpsLogRecord &record,
                                         BOOLEAN undone)
{
   if (!undone)
   {
      return;
   }
   if (_recordTransID.isFirstOp())
   {
      _processBeginRecord();
   }
   // Trans info for the next undo record should point to this one
   // TODO: is this necessary? I think it gets done automatically
   // _cb->setCurTransLsn(_dpsCB->getCurrentLsn().offset);
}

void rtnPITRollbackManager::_nextRecord(const dpsLogRecord &record)
{
   // Set the cursor to the previous contiguous record
   _cursor = record.head()._preLsn;
}

BOOLEAN rtnPITRollbackManager::_shouldUndo(const dpsLogRecord &record)
{
   if (!_isRecordTransactional())
   {
      // Non-transactional operations cannot be undone
      return FALSE;
   }
   if (record.isCommit())
   {
      // Commit records don't contain any data to undo
      return FALSE;
   }
   if (!_isTransInUndoTransSet())
   {
      // This record is not from a transaction being undone
      return FALSE;
   }
   return TRUE;
}

BOOLEAN rtnPITRollbackManager::_isTransInUndoTransSet()
{
   return _undoTransSet.find(_recordTransID) != _undoTransSet.end();
}

BOOLEAN rtnPITRollbackManager::_isRecordTransactional()
{
   return _recordTransID.isValid();
}

} // namespace engine
