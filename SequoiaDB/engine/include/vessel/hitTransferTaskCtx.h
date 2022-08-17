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
#include "vessel/lpsPteWriteBatch.h"

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
         OSS_INLINE BOOLEAN isDone()const {return _done;}
         OSS_INLINE void setDone() {_done = TRUE;}
         OSS_INLINE void setRC(INT32 rc) {_rc = rc;}
         OSS_INLINE INT32 getRC()const {return _rc;}
         OSS_INLINE BOOLEAN isOK()const {return SDB_OK == _rc;}
         OSS_INLINE hitIndexTransferTask &getTask() {return _task;}
         OSS_INLINE UINT32 getInsertedEntryNum()const {return _insertedEntryNum;}
         OSS_INLINE UINT32 getRemovedEntryNum()const {return _removedEntryNum;}
         OSS_INLINE void incInsertedEntryNum() {++_insertedEntryNum;}
         OSS_INLINE void incRemovedEntryNum() {++_removedEntryNum;}
         OSS_INLINE BOOLEAN isBtreeEntryPageUnstable()const {return _btreeEntryPageUnstable;}
         OSS_INLINE void setBtreeEntryPageUnstable() {_btreeEntryPageUnstable = TRUE;}
         OSS_INLINE void resetBtreeEntryPageUnstabl() {_btreeEntryPageUnstable = FALSE;}
         OSS_INLINE lpsPteWriteBatch *getBatch() {return _batch;}
         OSS_INLINE void setBatch(lpsPteWriteBatch *batch) {_batch = batch;}

      public:
         void reset();
         void resetToRedo();

      private:
         hitIndexTransferTask _task;
         lpsPteWriteBatch *_batch = nullptr;
         BOOLEAN _done = FALSE;
         INT32 _rc = SDB_OK;
         UINT32 _insertedEntryNum = 0;
         UINT32 _removedEntryNum = 0;
         BOOLEAN _btreeEntryPageUnstable = FALSE;
   };//class hitTransferTaskCtx

   using HIT_TRANS_TASK_CTX = std::unique_ptr<hitTransferTaskCtx>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_HIT_TRANSFER_TASK_CTX_H_
