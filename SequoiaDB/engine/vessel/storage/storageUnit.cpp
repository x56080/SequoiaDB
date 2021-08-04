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
      if (isOpen())
      {
         _sid = INVALID_SPACE_ID;
         _mds.close();
         _is.close();
      }
      return;
   }

   INT32 storageUnit::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {0};
      strSlice dirSlice;
      SPACE_ID sid = _sid;

      if (NULL == context)
      {
         SDB_ASSERT(FALSE, "can not be null");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         goto done;
      }

      _sid = INVALID_SPACE_ID;

      if (OSS_UNLIKELY(!vesselFileName::buildDirName(sid, sizeof(dirName), dirName)))
      {
         PD_LOG(PDERROR, "failed to build dir name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      dirSlice.reset(dirName);

      rc = createStatusFile(context->getEnv()->options.path,
                            dirSlice, sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "[sid:%d]failed to create status file before removing:%d",
                sid, rc);
         goto error;
      }

      _mds.destroy();
      _is.destroy();

      rc = ensureOtherDirRemoved(context->getEnv()->options.path, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "[sid:%d]failed to remove other dirs:%d", sid, rc);
         goto error;
      }

      rc = ensureMainDataDirRemoved(context->getEnv()->options.path, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "[sid:%d]failed to remove main data dir:%d", sid, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 storageUnit::create(requestContext *context,
                             SPACE_ID sid,
                             const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {0};
      strSlice dirSlice;
      const storagePathOptions *path = NULL;
      UINT32 secretValue = ossRand();
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _sid = sid;
      if (!vesselFileName::buildDirName(sid, sizeof(dirName), dirName))
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

      rc = createMainDataDir(*path, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data dir or tmp file:%d", rc);
         goto error;
      }
      rollback = TRUE;

      rc = createStatusFile(*path, dirSlice, sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create status file:%d", rc);
         goto error;
      }
   
      rc = createOtherDirs(*path, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create other dirs:%d", rc);
         goto error;
      }

      rc = createMainDataSpace(context, sid, dirSlice,
                               secretValue, options.dataArgs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }

      rc = createIndexSpace(context, sid, dirSlice,
                            secretValue, options.indexArgs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index space:%d", rc);
         goto error;
      }

      rc = removeStatusFile(*path, dirSlice, sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove status file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         INT32 t = rollbackCreating(*path, dirSlice);
         if (SDB_OK != t)
         {
            /// storage unit with same sid may not be created any more
            PD_LOG(PDSEVERE, "failed to rollback creating of sid[%d], rc:%d", sid, t);
         }
      }
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
         rc = SDB_INVALIDARG;
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
      if (SDB_VESSEL_TEMP_SU == rc)
      {
         PD_LOG(PDINFO, "will remove all files under space id[%d]", sid);
         INT32 t = rollbackCreating(*path, dirSlice);
         if (SDB_OK != t)
         {
            PD_LOG(PDSEVERE, "failed to remove files under tmp space[%d], rc:%d",
                   sid, t);
            rc = t;
         }
         goto error;
      }
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test dirs before openning:%d", rc);
         goto error;
      }

      rc = openMainDataSpace(context, dirSlice, sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data space:%d", rc);
         goto error;
      }

      rc = openIndexSpace(context, dirSlice, sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 storageUnit::rollbackCreating(const storagePathOptions &path,
                                       const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");

      _sid = INVALID_SPACE_ID;

      if (_is.isOpen())
      {
         _is.destroy();
      }
      if (_mds.isOpen())
      {
         _mds.destroy();
      }

      rc = ensureOtherDirRemoved(path, dir);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove other dirs:%d", rc);
         goto error;
      }

      rc = ensureMainDataDirRemoved(path, dir);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove main data dir:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createMainDataDir(const storagePathOptions &path,
                                        const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!path.dataPath.empty(), "can not be empty");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

      rc = utilBuildFullPath(path.dataPath.c_str(), dir.str(),
                             OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dir.str(), rc);
         goto error;
      }

      rc = ossMkdir(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mk dir:%s, rc:%d", fullPath, rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::ensureMainDataDirRemoved(const storagePathOptions &path,
                                               const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      rc = utilBuildFullPath(path.dataPath.c_str(), dir.str(),
                             OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%s, %d", dir.str(), rc);
         goto error;
      }

      rc = ossDelete(fullPath);
      if (SDB_OK == rc)
      {
         /// do nothing.
      }
      else if (SDB_FNE == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file:%s, rc:%d", fullPath, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createStatusFile(const storagePathOptions &path,
                                       const strSlice &dir,
                                       SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");

      OSSFILE file;
      BOOLEAN rollbackFile = FALSE;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      CHAR fileName[MAX_FILE_NAME_LEN +1] = {0};
      strSlice suffix(SIMPLE_FILE_SUFFIX_TMPSU);
      if (!vesselFileName::buildSimpleName(sid, suffix,
                                           MAX_FILE_NAME_LEN + 1, fileName))
      {
         PD_LOG(PDERROR, "failed to build tmp file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(),
                             dir.str(),
                             OSS_MAX_PATHSIZE +1,
                             fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path[%s],[%s], rc:%d",
                path.dataPath.c_str(), dir.str(), rc);
         goto error;
      }

      rc = utilCatPath(fullPath, OSS_MAX_PATHSIZE + 1, fileName);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cat path:%d", rc);
         goto error;
      }

      rc = ossOpen(fullPath, OSS_CREATEONLY, OSS_RU|OSS_WU|OSS_RG, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create status file:%s, rc:%d",
                fullPath, rc);
         goto error;
      }
      rollbackFile = TRUE;

      rc = ossFdatasync(&file);
      if (SDB_OK != rc)
      {
         ossClose(file);
         PD_LOG(PDERROR, "failed to fsync file:%s, rc:%d", fullPath, rc);
         goto error;
      }

      ossClose(file);
   done:
      return rc;
   error:
      if (rollbackFile)
      {
         ossDelete(fullPath);
      }
      goto done;
   }

   INT32 storageUnit::testStatusFile(const strSlice &fullDir,
                                     SPACE_ID sid,
                                     BOOLEAN &exists)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullDir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {0};
      exists = FALSE;
      strSlice suffix(SIMPLE_FILE_SUFFIX_TMPSU);

      if (!vesselFileName::buildSimpleName(sid, suffix,
                                           MAX_FILE_NAME_LEN + 1, fileName))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = utilBuildFullPath(fullDir.str(), fileName, OSS_MAX_PATHSIZE + 1, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%d", rc);
         goto error;
      }

      rc = ossAccess(fullPath);
      if (SDB_OK == rc)
      {
         exists = TRUE;
      }
      else if (SDB_FNE == rc)
      {
         rc = SDB_OK;
         exists = FALSE;
      }
      else
      {
         PD_LOG(PDERROR, "failed to access file:%s, rc:%d", fullPath, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::removeStatusFile(const storagePathOptions &path,
                                       const strSlice &dir,
                                       SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!path.dataPath.empty(), "can not be empty");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");

      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      strSlice suffix(SIMPLE_FILE_SUFFIX_TMPSU);

      if (!vesselFileName::buildSimpleName(sid, suffix,
                                           MAX_FILE_NAME_LEN + 1, fileName))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(), dir.str(),
                             OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%s, %d", dir.str(), rc);
         goto error;
      }

      rc = utilCatPath(fullPath, OSS_MAX_PATHSIZE + 1, fileName);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cat path:%d", rc);
         goto error;
      }

      rc = ossDelete(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file:%s, rc:%d", fullPath, rc);
         goto error;
      }
   done:
      return rc;
   error:
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
      SDB_ASSERT(!dirName.empty(), "can not be empty");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      BOOLEAN statusFileExists = FALSE;

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName.str(),
                             OSS_MAX_PATHSIZE, fullPath);
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

      rc = testStatusFile(strSlice(fullPath), _sid, statusFileExists);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test status file:%d", rc);
         goto error;
      }
      else if (statusFileExists)
      {
         PD_LOG(PDERROR, "temp status file found under[%s]", fullPath);
         rc = SDB_VESSEL_TEMP_SU;
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

   INT32 storageUnit::createOtherDirs(const storagePathOptions &path,
                                      const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dirName.empty(), "can not be empty");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      BOOLEAN rollbackIndexDir = FALSE;

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
               PD_LOG(PDSEVERE, "failed to rollback index dir:%s, rc:%d", fullPath, rc);
               ossPanic();
            }
         }
      }
      goto done;
   }

   INT32 storageUnit::ensureOtherDirRemoved(const storagePathOptions &path,
                                            const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

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
         if (SDB_FNE == rc)
         {  
            rc = SDB_OK;
         }
         else if (SDB_OK != rc)
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
         if (SDB_FNE == rc)
         {  
            rc = SDB_OK;
         }
         else if (SDB_OK != rc)
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
                                          SPACE_ID sid,
                                          const strSlice &dir,
                                          UINT32 secretValue,
                                          const storageCoreArgs &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(args.isValid(), "must be valid");

      const storagePathOptions &path = context->getEnv()->options.path;
      createLogicalPageSpaceOptions o;
      CHAR dirPath[OSS_MAX_PATHSIZE + 1] = {0};

      rc = utilBuildFullPath(path.dataPath.c_str(),
                             dir.str(),
                             OSS_MAX_PATHSIZE + 1,
                             dirPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full dir path:%d", rc);
         goto error;
      }

      o.sid = sid;
      o.dir.reset(dirPath);
      o.dataArgs = args;
      o.secretValue = secretValue;

      rc = _mds.create(context, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::openMainDataSpace(requestContext *context,
                                        const strSlice &dir,
                                        SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      CHAR dirPath[OSS_MAX_PATHSIZE + 1] = {0};
      const storagePathOptions &path = context->getEnv()->options.path;

      rc = utilBuildFullPath(path.dataPath.c_str(), dir.str(),
                             OSS_MAX_PATHSIZE + 1, dirPath);
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

   INT32 storageUnit::createIndexSpace(requestContext *context,
                                       SPACE_ID sid,
                                       const strSlice &dir,
                                       UINT32 secretValue,
                                       const storageCoreArgs &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(args.isValid(), "must be valid");

      const storagePathOptions &path = context->getEnv()->options.path;
      createLogicalPageSpaceOptions o;
      CHAR dirPath[OSS_MAX_PATHSIZE + 1] = {0};

      rc = utilBuildFullPath(path.autoGetIndexPath().c_str(),
                             dir.str(),
                             OSS_MAX_PATHSIZE + 1,
                             dirPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full dir path:%d", rc);
         goto error;
      }

      o.sid = sid;
      o.dir.reset(dirPath);
      o.dataArgs = args;
      o.secretValue = secretValue;

      rc = _is.create(context, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::openIndexSpace(requestContext *context,
                                     const strSlice &dir,
                                     SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      CHAR dirPath[OSS_MAX_PATHSIZE + 1] = {0};
      const storagePathOptions &path = context->getEnv()->options.path;

      rc = utilBuildFullPath(path.autoGetIndexPath().c_str(), dir.str(),
                             OSS_MAX_PATHSIZE + 1, dirPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build path:%d", rc);
         goto error;
      }

      rc = _is.open(context, sid, dirPath);
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

   INT32 storageUnit::getMmapPagePointer(SPACE_TYPE spaceType,
                                          FILE_TYPE fileType,
                                          PAGE_ID pid,
                                          mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_SPACE_TYPE == spaceType ||
               INVALID_FILE_TYPE == fileType ||
               INVALID_PAGE_ID == pid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (SPACE_TYPE_MAIN_DATA == spaceType)
      {
         rc = _mds.getPagePtr(fileType, pid, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (SPACE_TYPE_IDX == spaceType)
      {
         SDB_ASSERT(FALSE, "todo");
      }
      else if (SPACE_TYPE_LOB == spaceType)
      {
         SDB_ASSERT(FALSE, "todo");
      }
      else
      {
         PD_LOG(PDERROR, "invalid space type[%d]", spaceType);
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine