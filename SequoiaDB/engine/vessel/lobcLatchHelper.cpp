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

   Source File Name = lobcLatchHelper.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobcLatchHelper.h"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 lobcLatchHelper::lock(const globalLobChunkKey &key,
                               const ossSharedLatchMode &mode,
                               LOBC_LATCH_MAP::object &o)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(mode.isShared() || mode.isExclusive(), "can not be others");
      SDB_ASSERT(!o.isValid(), "already been locked");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      if (OSS_UNLIKELY(nullptr == tc))
      {
         SDB_ASSERT(FALSE, "thread context not ready");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         LOBC_LATCH_MAP &lm = tc->getEnv()->latchEnv.lobcLatchMap;
         rc = lm.ensure(key, o);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (mode.isShared())
         {
            o.getValue().get_shared();
         }
         else
         {
            o.getValue().get();
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   void lobcLatchHelper::unlock(const ossSharedLatchMode &mode,
                                LOBC_LATCH_MAP::object &o)
   {
      SDB_ASSERT(mode.isShared() || mode.isExclusive(), "can not be others");
      
      if (o.isValid())
      {
         THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
         SDB_ASSERT(nullptr != tc, "invalid thread context");
         if (mode.isShared())
         {
            o.getValue().release_shared();
         }
         else
         {
            o.getValue().release();
         }

         tc->getEnv()->latchEnv.lobcLatchMap.release(o);
      }
   }
} // namespace vessel

} // namespace engine

