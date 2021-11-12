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

   Source File Name = lpsCheckpointContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPS_CHECKPOINT_CONTEXT_H_
#define VESSEL_LPS_CHECKPOINT_CONTEXT_H_

#include "vessel/logicalPageSpaceCheckpoint.h"
#include "ossRWMutex.hpp"
#include <atomic> /// c++11

namespace engine
{
namespace vessel
{
   class lpsCheckpointContext : public SDBObject
   {
      public:
         lpsCheckpointContext();
         ~lpsCheckpointContext();

      public:
         enum CHECKPOINT_STATUS
         {
            NONE = 0,
            RUNNING = 2,
            CREATING_NEW_BASE = 3
         };//enum CHECKPOINT_STATUS

      public:
         
         OSS_INLINE void setStatus(CHECKPOINT_STATUS s)
         {
            _status = s;
         }
         OSS_INLINE CHECKPOINT_STATUS getStatus()const
         {
            return _status;
         }
         
         OSS_INLINE const LPS_CHECKPOINT &getCheckpoint()const
         {
            return _checkpoint;
         }

         OSS_INLINE DPS_LSN_OFFSET getMinDirtyLsn()const
         {
            return _minDirtyLsn;
         }
         OSS_INLINE DPS_LSN_OFFSET peekMinDirtyLsn()const
         {
            return ((const ossAtomic64 *)(&_minDirtyLsn))->peek();
         }
         OSS_INLINE DPS_LSN_OFFSET getMaxDirtyLsn()const
         {
            return _maxDirtyLsn;
         }
         OSS_INLINE BOOLEAN isDirty()const
         {
            return DPS_INVALID_LSN_OFFSET != _minDirtyLsn;
         }

         OSS_INLINE ossRWMutex *getLatch()
         {
            return &_checkpointLatch;
         }

      public:
         void fini();
         void setCheckpoint(const LPS_CHECKPOINT &checkpoint);
         void updateDirtyLsn(DPS_LSN_OFFSET lsn);
         void clearLsn()
         {
            _minDirtyLsn = DPS_INVALID_LSN_OFFSET;
            _maxDirtyLsn = DPS_INVALID_LSN_OFFSET;
         }
         void setMinDirtyLsn(DPS_LSN_OFFSET lsn)
         {
            SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
            SDB_ASSERT(lsn <= _maxDirtyLsn, "impossible");
            _minDirtyLsn = lsn;
         }

         BOOLEAN tryToApplyCheckpoint();
         void clearApplyingCheckpoint();
      private:
         ossRWMutex _checkpointLatch;
         CHECKPOINT_STATUS _status = NONE;
         LPS_CHECKPOINT _checkpoint;
         DPS_LSN_OFFSET _minDirtyLsn = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _maxDirtyLsn = DPS_INVALID_LSN_OFFSET;
         std::atomic_flag _applying = ATOMIC_FLAG_INIT;
   };//class lpsCheckpointContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPS_CHECKPOINT_CONTEXT_H_