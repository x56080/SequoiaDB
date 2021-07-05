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

   Source File Name = spaceIDLocker.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/spaceIDLocker.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   spaceIDLocker::spaceIDLocker()
   {}

   spaceIDLocker::~spaceIDLocker()
   {
      fini();
   }

   INT32 spaceIDLocker::init()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_array.isInitialized(), "do not reinit");
      UINT32 capacity = MAX_SU_COUNT;
      UINT32 chunkSize = 512;

      rc = _array.init(capacity, chunkSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init locker array:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void spaceIDLocker::fini()
   {
      _array.fini();
   }

   INT32 spaceIDLocker::lock(SPACE_ID sid,
                             ossSharedLatch::mode mode)
   {
      INT32 rc = SDB_OK;
      ossSharedLatch *latch = NULL;

      if (OSS_UNLIKELY(!_array.isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                            ossSharedLatch::NONE == mode))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _array.ensure(sid, &latch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure latch obj[%d], rc:%d", sid, rc);
         goto error;
      }

      latch->lockWith(mode);

   done:
      return rc;
   error:
      goto done;
   }

   void spaceIDLocker::unlock(SPACE_ID sid, ossSharedLatch::mode mode)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(ossSharedLatch::NONE != mode, "can not be invalid");
      SDB_ASSERT(_array.isInitialized(), "must be inited");
      ossSharedLatch *latch = NULL;
      INT32 rc = _array.get(sid, &latch);
      if (SDB_OK != rc)
      {
         SDB_ASSERT(FALSE, "impossible");
      }
      else
      {
         latch->unlockWith(mode);
      }
      return;
   }

   INT32 spaceIDLocker::tryLock(SPACE_ID sid,
                                ossSharedLatch::mode mode,
                                BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;
      ossSharedLatch *latch = NULL;
      if (OSS_UNLIKELY(!_array.isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       ossSharedLatch::NONE == mode))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _array.ensure(sid, &latch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure latch obj[%d], rc:%d", sid, rc);
         goto error;
      }

      locked = latch->tryLockWith(mode);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine