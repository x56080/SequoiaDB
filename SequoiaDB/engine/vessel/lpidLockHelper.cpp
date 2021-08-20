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

   Source File Name = lpidLockHelper.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpidLockHelper.h"
#include "vessel/requestContext.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   INT32 lpidLockHelper::lock(requestContext *context,
                              SPACE_TYPE type,
                              PAGE_ID lpid,
                              OSS_SHARED_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY((NULL == context ||
                       INVALID_SPACE_TYPE == type ||
                       INVALID_PAGE_ID == lpid ||
                       OSS_SHARED_LATCH_MODE_NONE == mode)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(isLocked()))
      {
         SDB_ASSERT(FALSE, "should not be locked");
         unlock();
      }

      rc = context->lockLpid(type, lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      _context = context;
      _type = type;
      _lpid = lpid;
      _mode = mode;
   done:
      return rc;
   error:
      goto done;
   }

   void lpidLockHelper::unlock()
   {
      if (isLocked())
      {
         _context->unlockLpid(_type, _lpid);
         _context = NULL;
         _type = INVALID_SPACE_TYPE;
         _lpid = INVALID_PAGE_ID;
         _mode = OSS_SHARED_LATCH_MODE_NONE;
      }
      return;
   }

   INT32 lpidLockHelper::lockLpidFromUpgrade()
   {
      INT32 rc = SDB_OK;
      if (OSS_SHARED_LATCH_MODE_UPGRADE != _mode)
      {
         SDB_ASSERT(FALSE, "lock upgrade first");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _context->lockLpidFromUpgrade(_type, _lpid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _mode = OSS_SHARED_LATCH_MODE_EXCLUSIVE;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine