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
#ifndef RTN_ROLLBACK_MANAGER_HPP__
#define RTN_ROLLBACK_MANAGER_HPP__

#include <string>

#include "clsReplayer.hpp"
#include "dpsDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsTransID.hpp"
#include "ossMemPool.hpp"
#include "ossTypes.hpp"
#include "stpLogicalTime.hpp"

namespace engine
{

class _dpsLogWrapper;
class dpsTransCB;
class _pmdEDUCB;

/// Rollback operation management class.
///
/// Starting from the end of the log, work backwards and undo any qualifying
/// records. This is a base class and cannot be used directly.
class _rtnRollbackManager : public SDBObject
{
 public:
   _rtnRollbackManager(_pmdEDUCB *cb);
   virtual ~_rtnRollbackManager(){};

   // Perform the rollback
   INT32 execute();

   // Perform a test run (check only, do not perform any undo operations)
   INT32 test();

 protected:
   _pmdEDUCB *_cb;
   _dpsLogWrapper *_dpsCB;
   dpsTransCB *_transCB;
   DPS_LSN_OFFSET _cursor;
   dpsMessageBlock _mb;
   clsReplayer _replayer;
   BOOLEAN _testOnly;

   // Main execution loop
   INT32 _readLogAndRollback();
   // Load the record
   INT32 _getRecord(dpsLogRecord *record);
   // Perform the record rollback
   // @return    True if the record was rolled back, else false
   BOOLEAN _rollback(const dpsLogRecord &record, BOOLEAN *undone);
   // Perform the undo of the current record
   // @return    True if the record was undone, else false
   INT32 _undo();

   //
   // Child classes need to define the following:
   //

   virtual INT32 _init() = 0;
   virtual void _abort() = 0;
   virtual INT32 _finish() = 0;

   // Do any processing of the record before/after it is rolled back
   virtual INT32 _preProcess(const dpsLogRecord &record) = 0;
   virtual INT32 _postProcess(const dpsLogRecord &record,
                              const BOOLEAN undone) = 0;

   // Get the next record (earlier record in the log)
   virtual INT32 _nextRecord(const dpsLogRecord &record) = 0;

   // Check if this record should be undone
   virtual BOOLEAN _shouldUndo(const dpsLogRecord &record) = 0;

   // Check if this record can be undone before performing undo
   virtual INT32 _canUndo(const dpsLogRecord &record) = 0;
};

/// Performs the rollback for the restore to point-in-time feature. Unlike the
/// transaction rollback, it reads every record. Also unlike the transaction
/// rollback, it may or may not undo each record it reads.
class rtnPITRollbackManager : public _rtnRollbackManager
{
 public:
   rtnPITRollbackManager(_pmdEDUCB *cb, UINT64 targetTime,
                         const DPS_TRANS_ID &transID);
   virtual ~rtnPITRollbackManager(){};

 protected:
   // Target consistency point
   stpLogicalTimeUS _targetTime;
   // Time for the PIT rollback transaction
   // This isn't a real global time from the stp servers. It is just the highest
   // commit time in the log + 1. The actual time does not matter as it just
   // needs to be higher than what is in the log and the node is locked during
   // this process so no other transactions can happen.
   stpLogicalTimeUS _rollbackTime;
   // Set of transIDs for transactions that need to be undone
   // DPS_TRANS_ID implements 'operator <' so it can be used in a set
   ossPoolSet<DPS_TRANS_ID> _undoTransSet;
   // Current record transaction ID
   DPS_TRANS_ID _recordTransID;
   BOOLEAN _continue;
   UINT64 _remainingLogSpace;
   // The global transaction ID from the coordinator
   DPS_TRANS_ID _transID;

   virtual INT32 _init();
   virtual void _abort();
   virtual INT32 _finish();

   // Reads the preceding record in the log
   virtual INT32 _nextRecord(const dpsLogRecord &record);
   INT32 _checkExitCondition();

   // Processes the log records
   virtual INT32 _preProcess(const dpsLogRecord &record);
   virtual INT32 _postProcess(const dpsLogRecord &record, const BOOLEAN undone);
   INT32 _processCommitRecord(const dpsLogRecord &record);
   INT32 _processBeginRecord();
   INT32 _exitConditionCheck();

   virtual BOOLEAN _shouldUndo(const dpsLogRecord &record);
   virtual INT32 _canUndo(const dpsLogRecord &record);

   BOOLEAN _isTransInUndoTransSet();
   BOOLEAN _isRecordTransactional();
   BOOLEAN _isLogFileDone();
   BOOLEAN _isTargetTimeReached(INT32 *rc);
};

} // namespace engine

#endif // RTN_ROLLBACK_MANAGER_HPP__
