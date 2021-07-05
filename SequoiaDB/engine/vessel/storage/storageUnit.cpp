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

   Source File Name = storageUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageUnit.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "utilStr.hpp"
#include "vessel/idMapFile.h"
#include "vessel/mainDataSpace.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   storageUnit::storageUnit(){}

   storageUnit::~storageUnit()
   {
      close();
   }

   void storageUnit::close()
   {
      _mds.close();
      _sid = INVALID_SPACE_ID;
      return;
   }

   void storageUnit::destroy(requestContext *context)
   {
      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {0};
      strSlice dirSlice;

      if (!isOpen())
      {
         goto done;
      }

      if (OSS_UNLIKELY(!vesselFileName::buildDirName(_sid, sizeof(dirName), dirName)))
      {
         PD_LOG(PDERROR, "failed to build dir name");
         goto done;
      }

      dirSlice.reset(dirName);

      _mds.destroy();

      removeAllDirs(context->getEnv()->options.path, dirSlice);
   done:
      return;
   }
   

   INT32 storageUnit::create(requestContext *context,
                             const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(EXCLUSIVE == context->getSpaceIDLockedMode(), "impossible");
      SDB_ASSERT(!isOpen(), "do not reinit");

      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {0};
      strSlice dirSlice;
      const storagePathOptions *path = NULL;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(NULL == context))
      {
         PD_LOG(PDERROR, "invalid ptr");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!vesselFileName::buildDirName(options.sid, sizeof(dirName), dirName))
      {
         PD_LOG(PDERROR, "failed to build dir name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      dirSlice.reset(dirName);
      path = &(context->getEnv()->options.path);

      rc = testAllDirsBeforeCreating(*path, dirSlice);
      if (SDB_OK != rc)
      {
         goto error;
      }
   
      rc = createAllDirs(*path, dirSlice);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _sid = options.sid;

      rc = createMainDataSpace(context, *path, dirSlice, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      destroy(context);
      goto done;
   }

   INT32 storageUnit::open(requestContext *context,
                           SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {0};
      strSlice dirSlice;
      const storagePathOptions *path = NULL;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_SPACE_ID == sid))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!vesselFileName::buildDirName(sid, sizeof(dirName), dirName))
      {
         PD_LOG(PDERROR, "failed to build dir name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      dirSlice.reset(dirName);
      path = &(context->getEnv()->options.path);
      _sid = sid;

      rc = testAllDirsBeforeOpenning(*path, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test dirs before openning:%d", rc);
         goto error;
      }

      rc = openMainDataSpace(context, path->dataPath, dirSlice, sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 storageUnit::testAllDirsBeforeCreating(const storagePathOptions &path,
                                                const strSlice &dirName)const
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

      if (path.dataPath.empty() || dirName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
         goto error;
      }

      rc = ossAccess(fullPath);
      if (SDB_OK == rc)
      {
         rc = SDB_FE;
         PD_LOG(PDERROR, "dir already exists:%s", fullPath);
         goto error;
      }
      else if (SDB_FNE == rc)
      {
         rc = SDB_OK;
      }
      else
      {
         goto error;
      }
      
      if (path.autoGetIndexPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetIndexPath().c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossAccess(fullPath);
         if (SDB_OK == rc)
         {
            rc = SDB_FE;
            PD_LOG(PDERROR, "dir already exists:%s", fullPath);
            goto error;
         }
         else if (SDB_FNE == rc)
         {
            rc = SDB_OK;
         }
         else
         {
            goto error;
         }
      }

      if (path.autoGetLobPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetLobPath().c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossAccess(fullPath);
         if (SDB_OK == rc)
         {
            rc = SDB_FE;
            PD_LOG(PDERROR, "dir already exists:%s", fullPath);
            goto error;
         }
         else if (SDB_FNE == rc)
         {
            rc = SDB_OK;
         }
         else
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::testAllDirsBeforeOpenning(const storagePathOptions &path,
                                                const strSlice &dirName)const
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

      if (path.dataPath.empty() || dirName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
         goto error;
      }

      rc = ossAccess(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to access path:%s", fullPath);
         goto error;
      }
      
      if (path.autoGetIndexPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetIndexPath().c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossAccess(fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to access path:%s", fullPath);
            goto error;
         }
      }

      if (path.autoGetLobPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetLobPath().c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossAccess(fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to access path:%s", fullPath);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createAllDirs(const storagePathOptions &path,
                                    const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      BOOLEAN rollbackDataDir = FALSE;
      BOOLEAN rollbackIndexDir = FALSE;

      if (dirName.empty() || path.dataPath.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
         goto error;
      }

      rc = ossMkdir(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create dir:%s, %d", fullPath, rc);
         goto error;
      }

      rollbackDataDir = TRUE;

      if (path.autoGetIndexPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetIndexPath().c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossMkdir(fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create dir:%s, %d", fullPath, rc);
            goto error;
         }
         rollbackIndexDir = TRUE;
      }

      if (path.autoGetLobPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetLobPath().c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossMkdir(fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create dir:%s, %d", fullPath, rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      if (rollbackIndexDir)
      {
         if (SDB_OK == utilBuildFullPath(path.autoGetIndexPath().c_str(), dirName.str(),
                                         OSS_MAX_PATHSIZE, fullPath))
         {
            if (SDB_OK != ossDelete(fullPath))
            {
               PD_LOG(PDSEVERE, "failed to rollback index dir:%s", fullPath);
            }
         }
      }

      if (rollbackDataDir)
      {
         if(SDB_OK == utilBuildFullPath(path.dataPath.c_str(), dirName.str(),
                                        OSS_MAX_PATHSIZE, fullPath))
         {
            if (SDB_OK != ossDelete(fullPath))
            {
               PD_LOG(PDSEVERE, "failed to rollback data dir:%s", fullPath);
            }
         }
      }
      goto done;
   }

   INT32 storageUnit::removeAllDirs(const storagePathOptions &path,
                                    const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

      rc = utilBuildFullPath(path.dataPath.c_str(), dir.str(), 
                             OSS_MAX_PATHSIZE + 1, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%d", rc);
         goto error;
      }

      rc = ossDelete(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove path:%s, rc:%d", fullPath, rc);
         goto error;
      }

      if (path.autoGetIndexPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetIndexPath().c_str(), dir.str(), 
                             OSS_MAX_PATHSIZE + 1, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path:%d", rc);
            goto error;
         }

         rc = ossDelete(fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove path:%s, rc:%d", fullPath, rc);
            goto error;
         }
      }

      if (path.autoGetLobPath() != path.dataPath)
      {
         rc = utilBuildFullPath(path.autoGetLobPath().c_str(), dir.str(), 
                             OSS_MAX_PATHSIZE + 1, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path:%d", rc);
            goto error;
         }

         rc = ossDelete(fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove path:%s, rc:%d", fullPath, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createMainDataSpace(requestContext *context,
                                          const storagePathOptions &path,
                                          const strSlice &subDir,
                                          const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!subDir.empty(), "can not be empty");
      SDB_ASSERT(options.isValid(), "must be valid");
      CHAR *dirBuffer = NULL;
      UINT32 dirBufferSize = 0;
      createLogicalPageSpaceOptions o;

      dirBufferSize = path.dataPath.size() + subDir.strLen() + 8;
      dirBuffer = context->allocateBuffer(dirBufferSize);
      if (NULL == dirBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(),
                             subDir.str(),
                             dirBufferSize,
                             dirBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full dir path:%d", rc);
         goto error;
      }

      o.sid = options.sid;
      o.logicalID = options.logicalID;
      o.dir.reset(dirBuffer);
      o.dataArgs = options.dataArgs;

      rc = _mds.create(context, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }
      
      
   done:
      if (NULL != dirBuffer)
      {
         context->releaseBuffer(dirBuffer, dirBufferSize);
      }
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::openMainDataSpace(requestContext *context,
                                        const std::string &dir,
                                        const strSlice &subDir,
                                        SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(!subDir.empty(), "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      CHAR dirPath[OSS_MAX_PATHSIZE + 1] = {0};

      rc = utilBuildFullPath(dir.c_str(), subDir.str(), OSS_MAX_PATHSIZE + 1, dirPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build path:%d", rc);
         goto error;
      }

      rc = _mds.open(context, sid, dirPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data path:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine