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

   Source File Name = storageConsole.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageConsole.h"
#include "vessel/requestContext.h"
#include "vessel/storageUnit.h"
#include "vessel/instanceEnv.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   constexpr UINT32 ALLOCATOR_PAGE_CAPACITY_BITWISE = 9;
   constexpr UINT32 ALLOCATOR_PAGE_CAPACITY = ((UINT32)1 << ALLOCATOR_PAGE_CAPACITY_BITWISE);
   storageConsole::storageConsole()
   {}
   
   storageConsole::~storageConsole()
   {}

   INT32 storageConsole::open(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      inMemBitmap::options o;

      if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _storageUnits.init(MAX_SU_COUNT, ALLOCATOR_PAGE_CAPACITY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init su array:%d", rc);
         goto error;
      }

      o.freeBound = 0;
      o.bitmapPageSkipped = 0;
      o.maxBitmapPageCount = MAX_SU_COUNT / ALLOCATOR_PAGE_CAPACITY;

      rc = _allocator.initWithNoLatch(ALLOCATOR_PAGE_CAPACITY, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init allocator:%d", rc);
         goto error;
      }

      rc = loadStorageUnitsOnDisk(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load storage units from disk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void storageConsole::close()
   {
      _allocator.fini();
      _storageUnits.fini();
      _isOpen = FALSE;
      return;
   }


   INT32 storageConsole::occupySpaceId(SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(sid < MAX_SU_COUNT, "can not be invalid");
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      UINT32 minPageCount = (sid >> ALLOCATOR_PAGE_CAPACITY_BITWISE) + 1;
      rc = _allocator.ensureBitmapPageCount(minPageCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure allocator's page count:%d", rc);
         goto error;
      }

      rc = _allocator.occupy(sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy sid[%d], rc:%d", sid, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageConsole::loadStorageUnitsOnDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_storageUnits.isInitialized(), "must be invalid");

      const storagePathOptions &path = context->getEnv()->options.path;
      fs::directory_iterator end_iter;
      fs::path dataDir(path.dataPath);

      if (!fs::exists(dataDir) || !fs::is_directory(dataDir))
      {
         PD_LOG(PDERROR, "invalid data path:%s", path.dataPath.c_str());
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string name = dir_iter->path().filename().string();
         strSlice nameSlice(name.c_str(), name.length());
         SPACE_ID sid = INVALID_SPACE_ID;
         storageUnit *su = NULL;

         if (!fs::is_directory(dir_iter->status()))
         {
            continue;
         }
         if (!vesselFileName::parseDirName(nameSlice, &sid))
         {
            continue;
         }

         rc = occupySpaceId(sid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy space id[%d], rc:%d", sid, rc);
            goto error;
         }

         rc = _storageUnits.ensure(sid, &su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate su obj at[%d], rc:%d",
                   sid, rc);
            goto error;
         }

         rc = su->open(context, sid);
         if (SDB_OK != rc)
         {

         }
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine