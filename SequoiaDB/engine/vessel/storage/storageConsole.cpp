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
   storageConsole::storageConsole()
   {}
   
   storageConsole::~storageConsole()
   {}

   INT32 storageConsole::open(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _storageUnits.init(MAX_SPACE_COUNT, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init su slots:%d", rc);
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
      _isOpen = FALSE;
      for (UINT32 i = 0; i < _storageUnits.size(); ++i)
      {
         storageUnit *su = _storageUnits.getObject(i);
         if (NULL != su)
         {
            su->close();
         }
      }
      _storageUnits.fini();
      _pool.clear();
      return;
   }

   INT32 storageConsole::allocateFreeSpaceID(SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      ossScopedLock guard(&_poolLatch);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (_pool.empty())
      {
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }

      sid = _pool.front();
      _pool.pop_front();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageConsole::releaseSpaceID(SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                            MAX_SPACE_COUNT <= sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL != _storageUnits.getObject(sid))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      {
      ossScopedLock guard(&_poolLatch);
      _pool.push_back(sid);
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

         rc = _storageUnits.allocateNewObj(sid, &su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate su obj at[%d], rc:%d",
                   sid, rc);
            goto error;
         }

         rc = su->open()
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine