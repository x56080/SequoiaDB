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

#include "dmsCB.hpp"
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
    : _cb(cb), _dmsCB(pmdGetKRCB()->getDMSCB()),
      _dpsCB(pmdGetKRCB()->getDPSCB()), _transCB(sdbGetTransCB()),
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
      PD_LOG(PDERROR, "Failed to inti rollback manager [rc=%d]", rc);
      return rc;
   }
   // Call the main loop
   if ((rc = _readLogAndRollback()))
   {
      PD_LOG(PDERROR, "Error during rollback loop [rc=%d]", rc);
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
      PD_LOG(PDERROR, "Rollback test failed during execute [rc=%d]", rc);
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
      if ( _cb->isInterrupted() )
      {
         PD_LOG(PDERROR, "Rollback interrupted");
         return (rc = SDB_APP_INTERRUPT);
      }
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
         PD_LOG(PDERROR, "Rollback failed at LSN [%llu] [rc=%d]", _cursor, rc);
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
   DPS_LSN dpsLsn;
   dpsLsn.offset = _cursor;
   _mb.clear(); // clean up the tmp storage
   // Search for the record given the LSN and load it
   if ((rc = _dpsCB->search(dpsLsn, &_mb)) ||
       (rc = record->load(_mb.offset(0))))
   {
      PD_LOG(PDERROR, "Get record failed [LSN:%llu] [rc=%d]", _cursor, rc);
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
   if (!_shouldUndo(record))
   {
      return rc; // not an error
   }
   if ((rc = _canUndo(record)))
   {
      PD_LOG(PDERROR, "Cannot undo record [rc=%d]", rc);
      return rc;
   }
   if ((rc = _undo()))
   {
      PD_LOG(PDERROR, "Failed to undo record [rc=%d]", rc);
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
      PD_LOG(PDERROR, "Replayer failed to rollback record [rc=%d]", rc);
      return rc;
   }
   return rc;
}

//
// rtnPITRollbackManager
//

rtnPITRollbackManager::rtnPITRollbackManager(pmdEDUCB *cb, UINT64 targetTime,
                                             const DPS_TRANS_ID &transID)
    : _rtnRollbackManager(cb), _rollbackTime(), _undoTransMap(),
      _recordTransID(), _continue(TRUE), _remainingLogSpace(0),
      _rollbackRecCount(0), _logLimitTime(0), _transID(transID),
      _dmsLocked(FALSE), _undoCount(0)
{
   // Set the target time from the input message
   _targetTime = stpLogicalTimeUS();
   _targetTime.setTime(targetTime);
}

rtnPITRollbackManager::~rtnPITRollbackManager()
{
   if(_dmsLocked)
   {
      _dmsCB->restoreDown(_cb);
   }
}

INT32 rtnPITRollbackManager::countRollbackRecords()
{
   return _rollbackRecCount;
}

UINT64 rtnPITRollbackManager::getLogLimitTime()
{
   return _logLimitTime;
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

   if ((rc = _dmsCB->registerRestore(_cb)))
   {
      PD_LOG(PDERROR, "Failed to lock the storage engine.");
      return rc;
   }
   _dmsLocked = TRUE;

   if ((rc = _drainTrans()))
   {
      PD_LOG(PDERROR, "Failed to terminate all existing transactions. Unsafe "
                      "to continue rollback. [rc=%d]", rc);
      return rc;
   }

   // Start at the end of the log
   _cursor = _dpsCB->getCurrentLsnOffset();

   // Get the total log space
   _remainingLogSpace = _dpsCB->getLogFileNum() * _dpsCB->getLogFileSz();

   if (!_testOnly)
   {
      // Need to be wrapped in a transaction. The coord would have passed in a
      // transID if this is a global transaction. Otherwise, assume this is a
      // local restoreToTime operation (not recommended!!!).
      if (!_transID.isValid())
      {
         PD_LOG(PDERROR, "Invalid transID for restore operation");
         return (rc = SDB_SYS);
      }
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
                "Failed to begin global transaction for restoreToTime "
                "[id=%s] [rc=%d]",
                dpsTransIDToString(_transID).c_str(), rc);
         return rc;
      }
   }

   return rc;
}

// Perform error case cleanup
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_ABORT, "rtnPITRollbackManager::_abort" )
void rtnPITRollbackManager::_abort()
{
}

// Perform success case cleanup
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_FINISH, "rtnPITRollbackManager::_finish" )
INT32 rtnPITRollbackManager::_finish()
{
   if (_testOnly)
   {
      PD_LOG(PDEVENT,
             "Rollback manager test run identified %llu records for rollback",
             _undoCount);
   }
   else
   {
      PD_LOG(PDEVENT, "Rollback manager performed rollback on %llu records",
             _undoCount);
   }
   return SDB_OK;
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
      PD_LOG(PDERROR, "Failed to get transaction time from record [rc=%d]", rc);
      return rc;
   }
   if (recordTransTime.getTime() > _rollbackTime.getTime())
   {
      // this is the max time (so far), so use it for the new trx time
      _rollbackTime.setTime(recordTransTime.getTime() + 1);
      _rollbackTime.setTimeError(_cb->getTransTimeError());
   }
   if (!(recordTransTime.getTime() > _targetTime.getTime()))
   {
      // This transaction committed at/before the target so skip it
      return rc;
   }

   try
   {
      _undoTransMap.insert(std::pair<DPS_TRANS_ID, stpLogicalTimeUS>(
          _recordTransID, recordTransTime));
   }
   catch (std::exception &e)
   {
      PD_LOG(PDERROR, "Failed to insert into map [exception=%s]",
             e.what());
      return (rc = SDB_OOM);
   }
   return rc;
}

// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_BEGINREC, "rtnPITRollbackManager::_processBeginRecord" )
INT32 rtnPITRollbackManager::_processBeginRecord()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_BEGINREC, &rc);
   // All records for this transaction have been processed. Remove this
   // transaction from the set of transactions to undo.
   _undoTransMap.erase(_recordTransID);
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
   if (record.isCommit() && !record.isPreCommit())
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
   ++_undoCount;
   if (_recordTransID.isFirstOp())
   {
      // Finished an entire transaction
      if ((rc = _processBeginRecord()))
      {
         PD_LOG(PDERROR, "Error processing begin record [rc=%d]", rc);
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
      PD_LOG(PDERROR, "Error checking exit condition [rc=%d]", rc);
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
   if (_undoTransMap.empty() &&
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
   if (!_isTransInUndoTransMap())
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
   // Log space required is double the record being undone: the reverse record
   // to undo the transaction and enough space for the undo of the reverse
   // record, in case the PIT rollback fails and the transaction is rolled back.
   // The undo record also has an additional header.
   UINT64 logSpaceRequired =
       2 * record.head()._length + DPS_TRANS_LOG_UNDO_DELTA;
   if (logSpaceRequired > _remainingLogSpace)
   {
      PD_LOG(PDERROR, "Not enough log space for rollback");
      if ((rc = _setLogLimit()))
      {
         // Unhandled error
         PD_LOG(PDERROR, "Error setting log limit [rc=%d]", rc);
         return rc;
      }
      // Handled error
      return (rc = SDB_DPS_LOG_FILE_OUT_OF_SIZE);
   }
   else
   {
      // Reduce the remaining space by the amount required
      _remainingLogSpace -= logSpaceRequired;
   }
   _rollbackRecCount += 1;
   return rc;
}

// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_LOGLIMIT, "rtnPITRollbackManager::_setLogLimit" )
INT32 rtnPITRollbackManager::_setLogLimit()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_LOGLIMIT, &rc);
   // Find the max commit time still outstanding
   // Do not confuse the "second" member of a map pair with the "_seconds" of
   // a time object
   for (UNDO_TRANS_MAP::const_iterator it = _undoTransMap.begin();
        it != _undoTransMap.end(); ++it)
   {
      _logLimitTime = OSS_MAX(_logLimitTime, it->second.getUpperTime());
   }
   // Also check the summary
   UINT64 summMaxCommitTime;
   if ((rc = _transCB->getMaxCommitTimeBefore(_cursor, summMaxCommitTime)))
   {
      PD_LOG(PDERROR, "Failed to calculate log limit [rc=%d]", rc);
      return (rc = SDB_SYS);
   }
   // Set the member var to the max time + 1 to set all outstanding commits out
   // of range
   _logLimitTime = OSS_MAX(_logLimitTime, summMaxCommitTime + 1);
   PD_LOG(PDINFO, "Log limit reached at global time [%llu]",
          _logLimitTime);
   return rc;
}

BOOLEAN rtnPITRollbackManager::_isTransInUndoTransMap()
{
   return _undoTransMap.find(_recordTransID) != _undoTransMap.end();
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

// Drain in-flight transactions on this node. This protects against stale
// data after the restore. The node will send a session disconnect msg to
// the transaction's coord.
// PD_TRACE_DECLARE_FUNCTION( RTN_PITROLLBACKMGR_DRAIN, "rtnPITRollbackManager::_drainTrans" )
INT32 rtnPITRollbackManager::_drainTrans()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(RTN_PITROLLBACKMGR_DRAIN, &rc);
   // termAllTrans sends a signal to terminate all transactions, but it is not
   // synchronous. Do a loop to check the TransCBSize, which is the number of
   // transactions. Repeatedly call termAllTrans (no harm in trying).
   UINT32 remTrans = 0;
   while ((remTrans = _transCB->getTransCBSize()) > 0 && !_cb->isInterrupted())
   {
      PD_LOG(PDDEBUG, "Waiting for %u transactions to finish", remTrans);
      _transCB->termAllTrans();
      ossSleep(OSS_ONE_SEC);
   }
   if (_cb->isInterrupted())
   {
      PD_LOG(PDERROR, "Rollback drain transactions interrupted");
      return (rc = SDB_APP_INTERRUPT);
   }
   return rc;
}

} // namespace engine
