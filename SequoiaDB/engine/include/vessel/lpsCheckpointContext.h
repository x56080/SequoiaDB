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
            CREATING_FLUSH_LIST = 1,
         };

      public:
         void fini();
         void setCheckpoint(const LPS_CHECKPOINT &checkpoint);
         void updateDirtyLsn(DPS_LSN_OFFSET lsn);
         const LPS_CHECKPOINT &getCheckpoint()const
         {
            return _checkpoint;
         }
         ossRWMutex *getLatch()
         {
            return &_checkpointLatch;
         }
      private:
         ossRWMutex _checkpointLatch;
         CHECKPOINT_STATUS _status = NONE;
         LPS_CHECKPOINT _checkpoint;
         DPS_LSN_OFFSET _minDirtyLsn = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _maxDirtyLsn = DPS_INVALID_LSN_OFFSET;
   };//class lpsCheckpointContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPS_CHECKPOINT_CONTEXT_H_