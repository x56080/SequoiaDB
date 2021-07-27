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

   Source File Name = lpidLockHelper.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPID_LOCK_HELPER_H_
#define VESSEL_LPID_LOCK_HELPER_H_

#include "ossLatch.hpp"
#include "vessel/vesselFileDef.h"
#include "vessel/pageIdentifier.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class lpidLockHelper : public SDBObject
   {
      public:
         OSS_INLINE lpidLockHelper()
         {}

         OSS_INLINE ~lpidLockHelper()
         {
            unlock();
         }

      public:
         OSS_INLINE PAGE_ID getLpid()const
         {
            return _lpid;
         }

         INT32 lock(requestContext *context,
                  SPACE_TYPE type,
                  PAGE_ID lpid,
                  OSS_SHARED_LATCH_MODE mode); 

         void unlock();

         INT32 unlockUpgradeAndLock();

         OSS_INLINE BOOLEAN isLocked()const
         {
            return OSS_SHARED_LATCH_MODE_NONE != _mode;
         }

         OSS_INLINE OSS_SHARED_LATCH_MODE getLockMode()
         {
            return _mode;
         }

      private:
         requestContext *_context = NULL;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         OSS_SHARED_LATCH_MODE _mode = OSS_SHARED_LATCH_MODE_NONE;
   };//class lpidLockHelper
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPID_LOCK_HELPER_H_