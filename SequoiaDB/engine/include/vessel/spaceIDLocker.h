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

   Source File Name = spaceIDLocker.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SPACE_ID_LOCKER_H_
#define VESSEL_SPACE_ID_LOCKER_H_

#include "vessel/vesselIdDef.h"
#include "ossLatch.hpp"
#include "vessel/lazyArray.h"

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
         INT32 lock(SPACE_ID sid, ossSharedLatch::mode mode);
         INT32 tryLock(SPACE_ID sid,
                       ossSharedLatch::mode mode,
                       BOOLEAN &locked);
         void unlock(SPACE_ID sid, ossSharedLatch::mode mode);

      private:
         lazyArray<ossSharedLatch> _array;
   };//class spaceIDLocker
}//namespace vessel
}//namespace engine

#endif//VESSEL_SPACE_ID_LOCKER_H_