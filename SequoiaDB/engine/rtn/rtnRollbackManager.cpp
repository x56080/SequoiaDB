/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   
*******************************************************************************/
#include "rtnRollbackManager.hpp"

#include <string>

#include "dpsLogWrapper.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsOp2Record.hpp"
#include "dpsTransCB.hpp"
#include "dpsTransID.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"

namespace engine
{

//
// _rtnRollbackManager
//

_rtnRollbackManager::_rtnRollbackManager(pmdEDUCB *cb)
    : _cb(cb), _dpsCB(pmdGetKRCB()->getDPSCB()), _transCB(sdbGetTransCB()),
      _cursor(DPS_INVALID_LSN_OFFSET),
      _mb(dpsMessageBlock(DPS_MSG_BLOCK_DEF_LEN)), _replayer(TRUE),
      _testOnly(FALSE)
{
}

// Entrypoint to running the rollback.
// PD_TRACE_DECLARE_FUNCTION( RTN_ROLLBACKMGR_EXE, "_rtnRollbackManager::execute" )
INT32 _rtnRollbackManager::execute()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_ROLLBACKMGR_EXE, &rc);
   // Do setup work.
   if ((rc = _init()))
   {
      return rc;
   }
   // Call the main loop
   if ((rc = _readLogAndRollback()))
   {
      PD_LOG(PDERROR, "Error during rollback loop");
      // Encountered an error. Do cleanup work.
      _abort();
      return rc;
   }
   // Do closure work.
   return (rc = _finish());
}

// Entrypoint to test whether the rollback would succeed.
// PD_TRACE_DECLARE_FUNCTION( RTN_ROLLBACKMGR_TEST, "_rtnRollbackManager::test" )
INT32 _rtnRollbackManager::test()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_ROLLBACKMGR_TEST, &rc);
   // Turn on the test mode and call execute
   _testOnly = TRUE;
   if ((rc = execute()))
   {
      PD_LOG(PDERROR, "Rollback test failed during execute");
      return rc;
   }
   return rc;
}

// The main rollback loop. Reads the record at the lsn, rolls back (if needed),
// moves the lsn to the next record.
// PD_TRACE_DECLARE_FUNCTION( RTN_ROLLBACKMGR_READLOGANDROLLBACK, "_rtnRollbackManager::_readLogAndRollback" )
INT32 _rtnRollbackManager::_readLogAndRollback()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_ROLLBACKMGR_READLOGANDROLLBACK, &rc);
   // Read the log and rollback one by one
   while (_cursor != DPS_INVALID_LSN_OFFSET)
   {
      dpsLogRecord record;
      BOOLEAN undone = FALSE;
      // If any step fails (rc != SDB_OK) then error out
      // Steps are: read record at the LSN cursor, process it, rollback (if
      // needed), post-process it, and move the LSN cursor
      if ((rc = _getRecord(&record)) ||
          (rc = _preProcess(record)) ||
          (rc = _rollback(record, &undone)) ||
          (rc = _postProcess(record, undone)) ||
          (rc = _nextRecord(record)))
      {
         // Error case
         PD_LOG(PDERROR, "Rollback failed at LSN [%llu]", _cursor);
         return rc;
      }
   }
   return rc;
}

// Load the record at the LSN cursor.
// PD_TRACE_DECLARE_FUNCTION( RTN_ROLLBACKMGR_GETRECORD, "_rtnRollbackManager::_getRecord" )
INT32 _rtnRollbackManager::_getRecord(dpsLogRecord *record)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_ROLLBACKMGR_GETRECORD, &rc);
   PD_TRACER(1, PD_PACK_ULONG(_cursor));
   DPS_LSN dpsLsn;
   dpsLsn.offset = _cursor;
   _mb.clear(); // clean up the tmp storage
   // Search for the record given the LSN and load it
   if ((rc = _dpsCB->search(dpsLsn, &_mb)) ||
       (rc = record->load(_mb.offset(0))))
   {
      PD_LOG(PDERROR, "Get record failed (LSN %llu)", _cursor);
      return rc;
   }
   return rc;
}

// Determines if the record is to be undone and performs the undo
// PD_TRACE_DECLARE_FUNCTION( RTN_ROLLBACKMGR_ROLLBACK, "_rtnRollbackManager::_rollback" )
INT32 _rtnRollbackManager::_rollback(const dpsLogRecord &record,
                                     BOOLEAN *undone)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_ROLLBACKMGR_ROLLBACK, &rc);
   // Determine if the record should and can be undone
   // It is an error if it should be undone but it cannot be undone
   if (!_shouldUndo(record) ||
       (rc = _canUndo(record)))
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

// Does the inverse operation of the current record and logs it
// PD_TRACE_DECLARE_FUNCTION( RTN_ROLLBACKMGR_UNDO, "_rtnRollbackManager::_undo" )
INT32 _rtnRollbackManager::_undo()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_ROLLBACKMGR_UNDO, &rc);
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

rtnPITRollbackManager::rtnPITRollbackManager(pmdEDUCB *cb, UINT64 targetTime,
                                             const DPS_TRANS_ID &transID)
    : _rtnRollbackManager(cb), _continue(TRUE), _remainingLogSpace(0),
      _transID(transID)
{
   // Set the target time from the input message
   _targetTime = stpLogicalTimeUS();
   _targetTime.setTime(targetTime);
}

// PIT rollback starts the end of the log and reads every record. It is also
// wrapped in a transaction - either from the coord (handled externally) or
// begin the local transaction here
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_INIT, "rtnPITRollbackManager::_init" )
INT32 rtnPITRollbackManager::_init()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_INIT, &rc);
   PD_LOG(PDEVENT, "Starting rollback to point-in-time [%llu]. Test only [%d]",
          _targetTime.getTime(), _testOnly);

   // Drain in-flight transactions on this node. This protects against stale
   // data after the restore. The node will send a session disconnect msg to
   // the transaction's coord.
   _transCB->termAllTrans();

   // Start at the end of the log
   _cursor = _dpsCB->getCurrentLsn().offset;

   // Get the total log space
   _remainingLogSpace = _dpsCB->getLogFileNum() * _dpsCB->getLogFileSz();

   if (!_testOnly)
   {
      // Need to be wrapped in a transaction. The coord would have passed in a
      // transID if this is a global transaction. Otherwise, assume this is a
      // local restoreToTime operation (not recommended!!!).
      if (_transID.isValid())
      {
         // Global transaction
         stpLogicalTimeUS beginTime;
         beginTime.setTime(_transID.getLogicalTime());
         // Use the global transaction ID from the coordinator. Note that there
         // are no corresponding rtnTransCommit/rtnTransRollback calls for
         // global transactions in this class. This is because while all three
         // are driven from the coord, the transBegin message is normally
         // packaged with the first real operation of the transaction.
         // restoreToTime is a special case - the transID in the message body is
         // the only indication that a global transaction has begun - so call
         // rtnTransBegin now. rtnTransCommit and rtnTransRollback will be
         // driven by messages from the coord.
         if ((rc = rtnTransBegin(_cb, FALSE, TRUE, _transID, beginTime)))
         {
            PD_LOG(PDERROR,
                   "Failed to begin global transaction for restoreToTime [%s]",
                   dpsTransIDToString(_transID).c_str());
            return rc;
         }
      }
      else
      {
         // Local transaction
         if ((rc = rtnTransBegin(_cb, FALSE, FALSE)))
         {
            PD_LOG(PDERROR, "Failed to begin transaction for restoreToTime");
            return rc;
         }
      }
   }

   return rc;
}

// Perform error case cleanup
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_ABORT, "rtnPITRollbackManager::_abort" )
void rtnPITRollbackManager::_abort()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_ABORT, &rc);
   // A global tranasaction abort would be driven from the coordinator.
   if (!_testOnly && !_cb->getTransID().isGlobTrans())
   {
      // Local transaction. Perform rollback.
      rtnTransRollback(_cb, _dpsCB);
   }
}

// Perform success case cleanup
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_FINISH, "rtnPITRollbackManager::_finish" )
INT32 rtnPITRollbackManager::_finish()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_FINISH, &rc);
   // A global tranasaction commit would be driven from the coordinator.
   if (!_testOnly && !_cb->getTransID().isGlobTrans() &&
       // Local transaction. Perform commit.
       (rc = rtnTransCommit(_cb, _dpsCB)))
   {
      PD_LOG(PDERROR, "Failed to commit transaction commit for restoreToTime");
      return rc;
   }
   return rc;
}

// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_COMMITREC, "rtnPITRollbackManager::_processCommitRecord" )
INT32 rtnPITRollbackManager::_processCommitRecord(const dpsLogRecord &record)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_COMMITREC, &rc);
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
   if (!(recordTransTime.getTime() > _targetTime.getTime()))
   {
      // This transaction committed at/before the target so skip it
      return rc;
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

// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_BEGINREC, "rtnPITRollbackManager::_processBeginRecord" )
INT32 rtnPITRollbackManager::_processBeginRecord()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_BEGINREC, &rc);
   // All records for this transaction have been processed. Remove this
   // transaction from the set of transactions to undo.
   _undoTransSet.erase(_recordTransID);
   return rc;
}

// Processing of record before rollback
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_PRE, "rtnPITRollbackManager::_preProcess" )
INT32 rtnPITRollbackManager::_preProcess(const dpsLogRecord &record)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_PRE, &rc);
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

// Processing of record after rollback
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_POST, "rtnPITRollbackManager::_postProcess" )
INT32 rtnPITRollbackManager::_postProcess(const dpsLogRecord &record,
                                          BOOLEAN undone)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_POST, &rc);
   if (!undone)
   {
      // The record was not undone
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

// Move the cursor
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_NEXT, "rtnPITRollbackManager::_nextRecord" )
INT32 rtnPITRollbackManager::_nextRecord(const dpsLogRecord &record)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_NEXT, &rc);
   if ((rc = _checkExitCondition()))
   {
      return rc;
   }
   if (_continue)
   {
      // Set the cursor to the previous contiguous record
      _cursor = record.head()._preLsn;
      return rc;
   }
   // End of the rollback
   _cursor = DPS_INVALID_LSN_OFFSET;
   return rc;
}

// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_CHECKEXIT, "rtnPITRollbackManager::_checkExitCondition" )
INT32 rtnPITRollbackManager::_checkExitCondition()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_CHECKEXIT, &rc);
   if (_undoTransSet.empty() &&
       _isLogFileDone() &&
       _isTargetTimeReached(&rc))
   {
      // Transaction map is empty and there are no outstanding commits before
      // the target time
      PD_LOG(PDEVENT, "Ending rollback (LSN %llu)", _cursor);
      _continue = FALSE;
   }
   return rc;
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

// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_CANUNDO, "rtnPITRollbackManager::_canUndo" )
INT32 rtnPITRollbackManager::_canUndo(const dpsLogRecord &record)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_CANUNDO, &rc);
   // Log space required is double the record being undone: the undo record
   // itself and enough space for the redo in case the PIT rollback fails and
   // the transaction is rolled back
   UINT64 logSpaceRequired = 2 * record.head()._length;
   if (logSpaceRequired > _remainingLogSpace)
   {
      PD_LOG(PDERROR, "Not enough log space for rollback");
      return (rc = SDB_DPS_LOG_FILE_OUT_OF_SIZE);
   }
   else
   {
      // Reduce the remaining space by the amount required
      _remainingLogSpace -= logSpaceRequired;
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

BOOLEAN rtnPITRollbackManager::_isLogFileDone()
{
   return (_dpsCB->getStartLsn(FALSE).offset == _cursor);
}

BOOLEAN rtnPITRollbackManager::_isTargetTimeReached(INT32 *pRc)
{
   // Check if the max commit time before this log record is less than the
   // target time
   UINT64 maxTime;
   if ((*pRc = _transCB->getMaxCommitTimeBefore(_cursor, maxTime)))
   {
      PD_LOG(PDERROR, "Failed to get max commit time before record");
      return FALSE;
   }
   return ((maxTime < _targetTime.getTime()) || // maxTime is less than target
           (maxTime == DPS_MAX_TRANS_TIME));    // invalid maxTime (first file)
}

} // namespace engine
