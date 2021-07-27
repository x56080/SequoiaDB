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
                                    const GLOBAL_PAGE_ID &gpid,
                                    const liteCacheAllocateOptions &options,
                                    liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      liteCache *cache = NULL;
      UINT32 pageSize = 0;
      SDB_ASSERT(gpid.getSpaceType() == SPACE_TYPE_MAIN_DATA, "must be main data");
      SDB_ASSERT(gpid.getFileType() == FILE_TYPE_DATA_STORAGE, "must be data storage");

      if (OSS_UNLIKELY(NULL == context ||
                       !gpid.isValid() ||
                       tuple.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->getEnv()->dms.getPageSize(gpid.space(), gpid.getSpaceType(),
                                              gpid.getFileType(), pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size of gpid[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      cache = getCache(pageSize);
      if (OSS_UNLIKELY(NULL == cache))
      {
         PD_LOG(PDERROR, "failed to get cache, page size:%d", pageSize);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = cache->allocate(context, gpid, options, tuple);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCacheConsole::allocateToReset(requestContext *context,
                                           const GLOBAL_PAGE_ID &gpid,
                                           liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      liteCache *cache = NULL;
      UINT32 pageSize = 0;
      SDB_ASSERT(gpid.getSpaceType() == SPACE_TYPE_MAIN_DATA, "must be main data");
      SDB_ASSERT(gpid.getFileType() == FILE_TYPE_DATA_STORAGE, "must be data storage");

      if (OSS_UNLIKELY(NULL == context ||
                       !gpid.isValid() ||
                       tuple.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->getEnv()->dms.getPageSize(gpid.space(), gpid.getSpaceType(),
                                              gpid.getFileType(), pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size of gpid[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      cache = getCache(pageSize);
      if (OSS_UNLIKELY(NULL == cache))
      {
         PD_LOG(PDERROR, "failed to get cache, page size:%d", pageSize);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = cache->allocateToReset(context, gpid, tuple);
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