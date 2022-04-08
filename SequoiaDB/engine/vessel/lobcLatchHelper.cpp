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

   Source File Name = lobcLatchHelper.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

