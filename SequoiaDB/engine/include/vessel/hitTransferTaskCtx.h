/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = hitTransferTaskCtx.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_HIT_TRANSFER_TASK_CTX_H_
#define VESSEL_HIT_TRANSFER_TASK_CTX_H_

#include "vessel/hitIndexTransferTask.h"
#include "vessel/indexSpaceAccessCtx.h"

#include <atomic>
#include <memory>

namespace engine
{
namespace vessel
{
   class hitTransferTaskCtx : public SDBObject
   {
      public:
         hitTransferTaskCtx() = default;
         explicit hitTransferTaskCtx(const hitIndexTransferTask &task);
         ~hitTransferTaskCtx() = default;

      public:
         OSS_INLINE BOOLEAN isValid()const {return _task.isValid();}
         OSS_INLINE BOOLEAN isTerminated()const
         {
            return _terminated.load(std::memory_order_relaxed);
         }
         OSS_INLINE void terminate()
         {
            _terminated.store(TRUE, std::memory_order_relaxed);
         }
         OSS_INLINE BOOLEAN isDone()const {return _done;}
         OSS_INLINE void setDone() {_done = TRUE;}
         OSS_INLINE void setRC(INT32 rc) {_rc = rc;}
         OSS_INLINE INT32 getRC()const {return _rc;}
         OSS_INLINE BOOLEAN isOK()const {return SDB_OK == _rc;}
         OSS_INLINE indexSpaceAccessCtx &getSpaceCtx() {return _ac;}
         OSS_INLINE hitIndexTransferTask &getTask() {return _task;}
         OSS_INLINE UINT32 getInsertedEntryNum()const {return _insertedEntryNum;}
         OSS_INLINE UINT32 getRemovedEntryNum()const {return _removedEntryNum;}
         OSS_INLINE void incInsertedEntryNum() {++_insertedEntryNum;}
         OSS_INLINE void incRemovedEntryNum() {++_removedEntryNum;}
         OSS_INLINE BOOLEAN isBtreeEntryPageCreated()const {return _btreeEntryPageCreated;}
         OSS_INLINE void setBtreeEntryPageCreated() {_btreeEntryPageCreated = TRUE;}

      public:
         void reset();

      private:
         hitIndexTransferTask _task;
         std::atomic_bool _terminated{FALSE};
         indexSpaceAccessCtx _ac;
         BOOLEAN _done = FALSE;
         INT32 _rc = SDB_OK;
         UINT32 _insertedEntryNum = 0;
         UINT32 _removedEntryNum = 0;
         BOOLEAN _btreeEntryPageCreated = FALSE;
   };//class hitTransferTaskCtx

   using HIT_TRANS_TASK_CTX = std::unique_ptr<hitTransferTaskCtx>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_HIT_TRANSFER_TASK_CTX_H_
