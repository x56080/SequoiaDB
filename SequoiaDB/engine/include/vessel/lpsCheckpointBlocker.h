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

   Source File Name = lpsCheckpointBlocker.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPS_CHECKPOINT_BLOCKER_H_
#define VESSEL_LPS_CHECKPOINT_BLOCKER_H_

#include "vessel/vesselFileDef.h"
#include "vessel/vesselIdDef.h"
#include "ossRWMutex.hpp"

namespace engine
{
namespace vessel
{
   class lpsCheckpointBlocker : public SDBObject
   {
      public:
         lpsCheckpointBlocker(){}
         ~lpsCheckpointBlocker();
         lpsCheckpointBlocker(const lpsCheckpointBlocker &) = delete;
         lpsCheckpointBlocker &operator=(const lpsCheckpointBlocker &) = delete;

      public:
         OSS_INLINE BOOLEAN isBlocking()const
         {
            return NULL != _mutex;
         }

         void fini();

         INT32 block(SPACE_ID sid, 
                     SPACE_TYPE type,
                     ossRWMutex *mutex);

         INT32 tryToBlock(SPACE_ID sid, 
                          SPACE_TYPE type,
                          ossRWMutex *mutex,
                          BOOLEAN &blocked);

         void unblock();

         BOOLEAN isSameBlocker(SPACE_ID sid, 
                               SPACE_TYPE type,
                               ossRWMutex *mutex)const;

      private:
         ossRWMutex *_mutex = NULL;
         SPACE_ID _sid = INVALID_SPACE_ID;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         UINT32 _count = 0;
   };//class lpsCheckpointBlocker
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPS_CHECKPOINT_BLOCKER_H_