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

   Source File Name = spaceIDLocker.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SPACE_ID_LOCKER_H_
#define VESSEL_SPACE_ID_LOCKER_H_

#include "vessel/vesselIdDef.h"
#include "vessel/lazyArray.hpp"
#include "ossRWMutex.hpp"

namespace engine
{  
namespace vessel
{
   class spaceIDLocker : public SDBObject
   {
      public:
         spaceIDLocker();
         ~spaceIDLocker();
         spaceIDLocker(const spaceIDLocker &) = delete;
         spaceIDLocker &operator=(const spaceIDLocker &) = delete;

      public:
         INT32 init();
         void fini();
         INT32 lock(SPACE_ID sid, OSS_LATCH_MODE mode);
         INT32 tryLock(SPACE_ID sid,
                       OSS_LATCH_MODE mode,
                       BOOLEAN &locked);
         void unlock(SPACE_ID sid, OSS_LATCH_MODE mode);

      private:
         lazyArray<ossRWMutex> _array;
   };//class spaceIDLocker
}//namespace vessel
}//namespace engine

#endif//VESSEL_SPACE_ID_LOCKER_H_