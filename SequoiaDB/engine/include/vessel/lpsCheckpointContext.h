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
#include "ossSpinLatch.hpp"

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
      
         struct STATUS
         {
            static const INT32 NONE = 0;
            static const INT32 APPLYING = 1;
            static const INT32 RUNNING = 2;
            static const INT32 CREATING_NEW_BASE = 3;
            static const INT32 ENDING = 4;
         };//enum CHECKPOINT_STATUS

      public:
         
         OSS_INLINE void setStatus(INT32 s)
         {
            _status.store(s);
         }
         OSS_INLINE INT32 peekStatus()const
         {
            return _status.load(std::memory_order_relaxed);
         }
         OSS_INLINE BOOLEAN isRunning()const
         {
            return STATUS::NONE != peekStatus();
         }
         
         OSS_INLINE const LPS_CHECKPOINT &getCheckpoint()const
         {
            return _checkpoint;
         }
         OSS_INLINE DPS_LSN_OFFSET getMaxDirtyLsn()const
         {
            return _maxDirtyLSN;
         }
         OSS_INLINE BOOLEAN isDirty()const
         {
            return DPS_INVALID_LSN_OFFSET != _maxDirtyLSN;
         }

         OSS_INLINE ossRWMutex *getLatch()
         {
            return &_checkpointLatch;
         }

         OSS_INLINE INT32 getCheckpointTick()const
         {
            return _checkpointTick;
         }
         OSS_INLINE void incCheckpointTick()
         {
            ++_checkpointTick;
         }

      public:
         void fini();
         void setCheckpoint(const LPS_CHECKPOINT &checkpoint);
         void updateDirtyLsn(DPS_LSN_OFFSET lsn);
         void clearDirtyLSN();

         BOOLEAN tryToApplyCheckpoint();
         BOOLEAN tryToSetRunningFromNoneOrApplying();

         DPS_LSN_OFFSET getMinDirtyLsn()const;
      private:
         ossRWMutex _checkpointLatch;
         std::atomic_int _status = {STATUS::NONE};
         LPS_CHECKPOINT _checkpoint;

         DPS_LSN_OFFSET _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _maxDirtyLSN = DPS_INVALID_LSN_OFFSET;
         INT32 _checkpointTick = 0;
   };//class lpsCheckpointContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPS_CHECKPOINT_CONTEXT_H_