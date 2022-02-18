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
   constexpr UINT32 POOL_NO_32KB = 0;
   constexpr UINT32 POOL_NO_64KB = 1;

   liteCacheConsole::liteCacheConsole()
   {}

   liteCacheConsole::~liteCacheConsole()
   {
      fini();
   }

   void liteCacheConsole::fini()
   {
      _32KBCache.fini();
      //_64KBCache.fini();
   }

   INT32 liteCacheConsole::init(const liteCacheOptions &o)
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

   liteCache *liteCacheConsole::getCache(UINT32 pageSize)
   {
      if (DMS_PAGE_SIZE32K == pageSize)
      {
         return &_32KBCache;
      }
      else
      {
         return nullptr;
      }
   }

   liteCache *liteCacheConsole::getCacheByPoolNo(UINT32 poolNo)
   {
      liteCache *pool = nullptr;
      if (POOL_NO_32KB == poolNo)
      {
         pool = &_32KBCache;
      }
      return pool;
   }
}//namespace vessel
}//namespace engine