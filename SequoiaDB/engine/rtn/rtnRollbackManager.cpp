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
#include "dpsTransCB.hpp"
#include "dpsTransID.hpp"
#include "ossUtil.hpp"
#include "pmd.hpp"
#include "rtn.hpp"

namespace engine
{

//
// rtnRollbackManager
//

rtnRollbackManager::rtnRollbackManager(pmdEDUCB *cb)
    : _cb(cb), _dpsCB(pmdGetKRCB()->getDPSCB()), _transCB(sdbGetTransCB()),
      _cursor(DPS_INVALID_LSN_OFFSET),
      _mb(dpsMessageBlock(DPS_MSG_BLOCK_DEF_LEN)), _replayer(TRUE),
      _testOnly(FALSE)
{
}

INT32 rtnRollbackManager::execute()
{
   INT32 rc = SDB_OK;
   if ((rc = _init()))
   {
      return rc;
   }
   if ((rc = _readLogAndRollback()))
   {
      PD_LOG(PDERROR, "Error during rollback loop");
      return rc;
   }
   return (rc = _finalize());
}

INT32 rtnRollbackManager::test()
{
   INT32 rc = SDB_OK;
   _testOnly = TRUE;
   if ((rc = execute()))
   {
      PD_LOG(PDERROR, "Rollback test failed during execute");
      return rc;
   }
   return rc;
}

INT32 rtnRollbackManager::_readLogAndRollback()
{
   INT32 rc = SDB_OK;
   // Read the log and rollback one by one
   while (_cursor != DPS_INVALID_LSN_OFFSET)
   {
      dpsLogRecord record;
      BOOLEAN undone = FALSE;
      if ((rc = _getRecord(&record)) || (rc = _preProcess(record)) ||
          (rc = _rollback(record, &undone)) ||
          (rc = _postProcess(record, undone)) || (rc = _nextRecord(record)))
      {
         // Error case
         PD_LOG(PDERROR, "Rollback failed at LSN [%llu]", _cursor);
         break;
      }
   }
   if (SDB_OK != rc)
   {
      _abort();
   }
   return rc;
}

INT32 rtnRollbackManager::_getRecord(dpsLogRecord *record)
{
   INT32 rc = SDB_OK;
   DPS_LSN dpsLsn;
   dpsLsn.offset = _cursor;
   _mb.clear(); // clean up the tmp storage
   if ((rc = _dpsCB->search(dpsLsn, &_mb)))
   {
      PD_LOG(PDERROR, "LSN search failed (LSN %llu)", _cursor);
      return rc;
   }
   if ((rc = record->load(_mb.offset(0))))
   {
      PD_LOG(PDERROR, "Loading record failed (LSN %llu)", _cursor);
      return rc;
   }
   return rc;
}

INT32 rtnRollbackManager::_rollback(const dpsLogRecord &record, BOOLEAN *undone)
{
   INT32 rc = SDB_OK;
   if (!_shouldUndo(record) || (rc = _checkUndo(record)))
   {
      return rc;
   }
   if ((rc = _undo()))
   {
      PD_LOG(PDERROR, "failed to undo record");
      return rc;
   }
   *undone = TRUE;
   return rc;
}

INT32 rtnRollbackManager::_undo()
{
   INT32 rc = SDB_OK;
   // Set the TransRelatedLSN of the undo record to the record being undone
   _cb->setRelatedTransLSN(_cursor);
   // Perform the undo of the record
   if (!_testOnly &&
       (rc = _replayer.rollback((dpsLogRecordHeader *)_mb.offset(0), _cb)))
   {
      PD_LOG(PDERROR, "Replayer failed to rollback record");
      return rc;
   }
   return rc;
}

//
// rtnPITRollbackManager
//

rtnPITRollbackManager::rtnPITRollbackManager(pmdEDUCB *cb, UINT64 targetTime)
    : rtnRollbackManager(cb), _continue(TRUE), _remainingLogSpace(0)
{
   _targetTime = stpLogicalTimeUS();
   _targetTime.setTime(targetTime);
}

INT32 rtnPITRollbackManager::_init()
{
   INT32 rc = SDB_OK;
   PD_LOG(PDEVENT, "Starting rollback to point-in-time [%llu]. Test only [%d]",
          _targetTime.getTime(), _testOnly);

   // Start at the end of the log
   _cursor = _dpsCB->getCurrentLsn().offset;

   // Start a new transaction for the rollback
   if (!_testOnly && (rc = rtnTransBegin(_cb, FALSE, TRUE)))
   {
      PD_LOG(PDERROR, "Failed in transaction begin");
      return rc;
   }
   return rc;
}

INT32 rtnPITRollbackManager::_finalize()
{
   INT32 rc = SDB_OK;
   // Commit the transaction
   if (!_testOnly && (rc = rtnTransCommit(_cb, _dpsCB, _rollbackTime)))
   {
      PD_LOG(PDERROR, "Failed in transaction commit");
      return rc;
   }
   return rc;
}

void rtnPITRollbackManager::_abort()
{
   // Rollback the rollback transaction
   if (!_testOnly && rtnTransRollback(_cb, _dpsCB))
   {
      // The rc here is just temporary - abort if only called during failure
      PD_LOG(PDERROR, "Failed in transaction abort");
   }
}

INT32 rtnPITRollbackManager::_processCommitRecord(const dpsLogRecord &record)
{
   INT32 rc = SDB_OK;
   // Get the transaction time from the commit record
   stpLogicalTimeUS recordTransTime;
   if ((rc =
            dpsGetTransTimeFromRecord(record, _recordTransID, recordTransTime)))
   {
      PD_LOG(PDERROR, "Failed to get transaction time from record");
      return rc;
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
      return SDB_OK;
   }
   if (_isTransInUndoTransSet())
   {
      // This transaction is already marked for undo, this must be a pre-commit
      SDB_ASSERT(record.isPreCommit(),
                 "Found a final commit record for a transaction already in the "
                 "undo transaction set");
      return SDB_OK;
   }
   _undoTransSet.insert(_recordTransID);
   return rc;
}

INT32 rtnPITRollbackManager::_processBeginRecord()
{
   INT32 rc = SDB_OK;
   // All records for this transaction have been processed. Remove this
   // transaction from the set of transactions to undo.
   _undoTransSet.erase(_recordTransID);
   if (_undoTransSet.empty() && _isTargetTimeReached(&rc))
   {
      // Transaction map is empty and there are no outstanding commits before
      // the target time
      PD_LOG(PDEVENT, "Ending rollback (LSN %llu)", _cursor);
      _continue = FALSE;
   }
   else if (SDB_OK != rc)
   {
      // Error
      return rc;
   }
   return rc;
}

INT32 rtnPITRollbackManager::_preProcess(const dpsLogRecord &record)
{
   INT32 rc = SDB_OK;
   _recordTransID.reset();
   // Extract the transaction ID. Don't check the rc because failure just means
   // it is a non-transactional record, in which case the ID will fail its
   // isValid() check later.
   dpsGetTransIDFromRecord(record, _recordTransID);
   if (record.isCommit())
   {
      rc = _processCommitRecord(record);
   }
   return rc;
}

INT32 rtnPITRollbackManager::_postProcess(const dpsLogRecord &record,
                                          BOOLEAN undone)
{
   INT32 rc = SDB_OK;
   if (!undone)
   {
      return SDB_OK;
   }
   if (_recordTransID.isFirstOp())
   {
      // Finished an entire transaction
      if ((rc = _processBeginRecord()))
      {
         return rc;
      }
   }
   return rc;
}

INT32 rtnPITRollbackManager::_nextRecord(const dpsLogRecord &record)
{
   if (_continue)
   {
      // Set the cursor to the previous contiguous record
      _cursor = record.head()._preLsn;
      return SDB_OK;
   }
   // End of the rollback
   _cursor = DPS_INVALID_LSN_OFFSET;
   return SDB_OK;
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

INT32 rtnPITRollbackManager::_checkUndo(const dpsLogRecord &record)
{
   INT32 rc = SDB_OK;
   // Log space required is double the record being undone: the undo record
   // itself and enough space for the redo in case the PIT rollback fails and
   // the transaction is rolled back
   UINT64 logSpaceRequired = 2 * record.head()._length;
   if (logSpaceRequired > _remainingLogSpace)
   {
      PD_LOG(PDERROR, "Not enough log space for rollback");
      return (rc = SDB_DPS_LOG_FILE_OUT_OF_SIZE);
   }
   return rc;
}

BOOLEAN rtnPITRollbackManager::_isTransInUndoTransSet()
{
   return _undoTransSet.find(_recordTransID) != _undoTransSet.end();
}

BOOLEAN rtnPITRollbackManager::_isRecordTransactional()
{
   return _recordTransID.isValid();
}

BOOLEAN rtnPITRollbackManager::_isTargetTimeReached(INT32 *rc)
{
   // Check if the max commit time before this log record is less than the
   // target time
   UINT64 maxTime;
   if ((*rc = _transCB->getMaxCommitTimeBefore(_cursor, maxTime)))
   {
      PD_LOG(PDERROR, "Failed to get max commit time before record");
      return FALSE;
   }
   return maxTime < _targetTime.getTime();
}

} // namespace engine
