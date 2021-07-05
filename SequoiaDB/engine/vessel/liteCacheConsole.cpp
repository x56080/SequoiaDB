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

   Source File Name = liteCacheConsole.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/liteCacheConsole.h"
#include "vessel/logicalPageSpace.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   static const UINT32 POOL_NO_32KB = 0;
   static const UINT32 POOL_NO_64KB = 1;

   liteCacheConsole::liteCacheConsole()
   {}

   liteCacheConsole::~liteCacheConsole()
   {
      fini();
   }

   void liteCacheConsole::fini()
   {
      _32KBCache.fini();
      _64KBCache.fini();
   }

   INT32 liteCacheConsole::init32KBCache(const liteCacheOptions &o)
   {
      INT32 rc = SDB_OK;
      _32KBCache.fini();
      rc = _32KBCache.init(0, DMS_PAGE_SIZE32K, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cache:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      _32KBCache.fini();
      goto done;
   }

   INT32 liteCacheConsole::allocate(requestContext *context,
                                    const GLOBAL_PAGE_ID &id,
                                    const liteCacheAllocateOptions &options,
                                    liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace *lps = NULL;
      storageCoreArgs args;
      liteCache *cache = NULL;
      SDB_ASSERT(id.getSpaceType() == SPACE_TYPE_MAIN_DATA, "must be main data");

      if (OSS_UNLIKELY(NULL == context ||
                       !id.isValid() ||
                       tuple.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->getEnv()->sc.getLogicalPageSpace(id.space(), id.getSpaceType(), &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical space of[%s], rc:%d",
                id.toString().c_str(), rc);
         goto error;
      }

      rc = lps->getStorageCoreArgs(id.getFileType(), args);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      cache = getCache(args.pageSize);
      if (OSS_UNLIKELY(NULL == cache))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = cache->allocate(context, id, options, tuple);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   liteCache *liteCacheConsole::getCache(UINT32 pageSize)
   {
      if (DMS_PAGE_SIZE32K == pageSize)
      {
         return &_32KBCache;
      }
      else if (DMS_PAGE_SIZE64K == pageSize)
      {
         return &_64KBCache;
      }
      else
      {
         return NULL;
      }
   }
}//namespace vessel
}//namespace engine