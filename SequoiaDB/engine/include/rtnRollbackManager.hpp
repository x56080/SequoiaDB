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
class _pmdEDUCB;

/// Rollback operation management class.
///
/// Starting from the end of the log, work backwards and undo any qualifying
/// records. This is a base class and cannot be used directly.
class rtnRollbackManager
{
 public:
   rtnRollbackManager(pmdEDUCB *cb);
   virtual ~rtnRollbackManager(){};

   // Perform the rollback
   void execute();

 protected:
   _pmdEDUCB *_cb;
   _dpsLogWrapper *_dpsCB;
   DPS_LSN_OFFSET _cursor;
   dpsMessageBlock _mb;
   clsReplayer _replayer;

   // Load the record
   dpsLogRecord _getRecord();
   // Perform the record rollback
   // @return    True if the record was rolled back, else false
   BOOLEAN _rollback(const dpsLogRecord &record);
   // Perform the undo of the current record
   // @return    True if the record was undone, else false
   void _undo();

   //
   // Child classes need to define the following:
   //

   virtual void _init() = 0;

   // End a successful rollback
   virtual void _finalize() = 0;

   // Abort due to an error during rollback
   virtual void _abort() = 0;

   // Do any processing of the record before/after it is rolled back
   virtual void _preProcess(const dpsLogRecord &record) = 0;
   virtual void _postProcess(const dpsLogRecord &record,
                             const BOOLEAN undone) = 0;

   // Get the next record (earlier record in the log)
   virtual void _nextRecord(const dpsLogRecord &record) = 0;

   // Check if this record should be undone
   virtual BOOLEAN _shouldUndo(const dpsLogRecord &record) = 0;
};

/// Performs the rollback for the restore to point-in-time feature. Unlike the
/// transaction rollback, it reads every record. Also unlike the transaction
/// rollback, it may or may not undo each record it reads.
class rtnPITRollbackManager : public rtnRollbackManager
{
 public:
   rtnPITRollbackManager(pmdEDUCB *cb, const std::string targetTimeString);
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

   virtual void _init();
   virtual void _finalize();
   virtual void _abort();

   // Reads the preceding record in the log
   virtual void _nextRecord(const dpsLogRecord &record);

   // Processes the log records
   virtual void _preProcess(const dpsLogRecord &record);
   virtual void _postProcess(const dpsLogRecord &record, const BOOLEAN undone);
   void _processCommitRecord(const dpsLogRecord &record);
   void _processBeginRecord();

   virtual BOOLEAN _shouldUndo(const dpsLogRecord &record);

   BOOLEAN _isTransInUndoTransSet();
   BOOLEAN _isRecordTransactional();
};

} // namespace engine
