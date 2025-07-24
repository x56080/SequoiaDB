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

   Source File Name = spaceIDLocker.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
                             OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      ossRWMutex *latch = NULL;

      if (OSS_UNLIKELY(!_array.isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
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

      if (SHARED == mode)
      {
         latch->lock_r();
      }
      else
      {
         latch->lock_w();
      }

   done:
      return rc;
   error:
      goto done;
   }

   void spaceIDLocker::unlock(SPACE_ID sid, OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(_array.isInitialized(), "must be inited");


      ossRWMutex *latch = NULL;
      INT32 rc = _array.get(sid, &latch);
      if (SDB_OK != rc)
      {
         SDB_ASSERT(FALSE, "impossible");
      }
      else if (SHARED == mode)
      {
         latch->release_r();
      }
      else
      {
         latch->release_w();
      }
      return;
   }

   INT32 spaceIDLocker::tryLock(SPACE_ID sid,
                                OSS_LATCH_MODE mode,
                                BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;
      ossRWMutex *latch = NULL;
      if (OSS_UNLIKELY(!_array.isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
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

      if (SHARED == mode)
      {
         locked = latch->try_lock_r();
      }
      else
      {
         locked = latch->try_lock_w();
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine