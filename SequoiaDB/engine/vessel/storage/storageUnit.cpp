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
#include "vessel/storageFileLoader.h"

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
         _unitEntryDir.clear();
         _mds.close();
         _is.close();
         _path = NULL;
      }
      return;
   }

   INT32 storageUnit::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {};
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

      _mds.destroy(context);
      _is.destroy(context);

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
                             const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {};
      strSlice dirSlice;
      UINT32 secretValue = ossRand();
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!vesselFileName::buildDirName(context->getSpaceID(), sizeof(dirName), dirName))
      {
         PD_LOG(PDERROR, "failed to build dir name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      _sid = context->getSpaceID();
      _unitEntryDir.append(dirName);
      dirSlice.reset(_unitEntryDir.c_str(), _unitEntryDir.size());
      _path = &(context->getEnv()->options.path);

      rc = testAllDirsBeforeCreating(*_path, dirSlice);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = createMainDataDir(*_path, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data dir or tmp file:%d", rc);
         goto error;
      }
      rollback = TRUE;

      rc = createStatusFile(*_path, dirSlice, _sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create status file:%d", rc);
         goto error;
      }
   
      rc = createOtherDirs(*_path, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create other dirs:%d", rc);
         goto error;
      }

      rc = createMainDataSpace(context, secretValue, options.dataArgs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }

      rc = createIndexSpace(context, secretValue, options.indexArgs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index space:%d", rc);
         goto error;
      }

      rc = removeStatusFile(*_path, dirSlice, _sid);
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
         INT32 t = rollbackCreating(context, context->getEnv()->options.path, dirSlice);
         if (SDB_OK != t)
         {
            /// storage unit with same sid may not be created any more
            PD_LOG(PDSEVERE, "failed to rollback creating of sid[%d], rc:%d", _sid, t);
         }

         close();
      }
      goto done;
   }

   INT32 storageUnit::open(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {};
      strSlice dirSlice;
      storageFileLoader loader;
      ossPoolString fullDir;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!vesselFileName::buildDirName(context->getSpaceID(), sizeof(dirName), dirName))
      {
         PD_LOG(PDERROR, "failed to build dir name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _sid = context->getSpaceID();
      _unitEntryDir.append(dirName);
      dirSlice.reset(_unitEntryDir.c_str(), _unitEntryDir.size());
      _path = &(context->getEnv()->options.path);

      rc = testAllDirsBeforeOpenning(*_path, dirSlice);
      if (SDB_VESSEL_TEMP_SU == rc)
      {
         PD_LOG(PDINFO, "will remove all files under space id[%d]", _sid);
         INT32 t = rollbackCreating(context, *_path, dirSlice);
         if (SDB_OK != t)
         {
            PD_LOG(PDSEVERE, "failed to remove files under tmp space[%d], rc:%d",
                   _sid, t);
            rc = t;
         }
         goto error;
      }
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test dirs before openning:%d", rc);
         goto error;
      }

      loader.init(_sid);
      buildFullDir(_path, SPACE_TYPE_MAIN_DATA,
                   INVALID_FILE_TYPE, fullDir);
      rc = loader.load(strSlice(fullDir.c_str(), fullDir.size()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load files under[%s], rc:%d",
                fullDir.c_str(), rc);
         goto error;
      }

      rc = _mds.open(context, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data space:%d", rc);
         goto error;
      }

      if (_path->hasExclusiveIndexPath())
      {
         fullDir.clear();
         buildFullDir(_path, SPACE_TYPE_IDX,
                      INVALID_FILE_TYPE, fullDir);
         rc = loader.append(strSlice(fullDir.c_str(), fullDir.size()), SPACE_TYPE_IDX);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append index files to loader:%d", rc);
            goto error;
         }
      }

      rc = _is.open(context, loader);
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

   INT32 storageUnit::rollbackCreating(requestContext *context,
                                       const storagePathOptions &path,
                                       const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");

      PD_LOG(PDINFO, "begin to rollback unit[%d] creating", _sid);

      if (_is.isOpen())
      {
         _is.destroy(context);
      }
      if (_mds.isOpen())
      {
         _mds.destroy(context);
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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};

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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};
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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};
      CHAR fileName[MAX_FILE_NAME_LEN +1] = {};
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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};
      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {};
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

      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};
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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};

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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};
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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};
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
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {};

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
                                          UINT32 secretValue,
                                          const storageCoreArgs &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");
      SDB_ASSERT(!_unitEntryDir.empty(), "can not be empty");
      SDB_ASSERT(args.isValid(), "must be valid");
      createLogicalPageSpaceOptions o;
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

   INT32 storageUnit::createIndexSpace(requestContext *context,
                                       UINT32 secretValue,
                                       const storageCoreArgs &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");
      SDB_ASSERT(!_unitEntryDir.empty(), "can not be empty");
      SDB_ASSERT(args.isValid(), "must be valid");
      createLogicalPageSpaceOptions o;
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

   void storageUnit::buildFullDir(const storagePathOptions *path,
                                   SPACE_TYPE type,
                                   FILE_TYPE ftype,
                                   ossPoolString &dir)const
   {
      SDB_ASSERT(NULL != path, "can not be null");
      SDB_ASSERT(!_unitEntryDir.empty(), "can not be empty");
      SDB_ASSERT(INVALID_SPACE_TYPE != type, "can not be invalid");
      dir.clear();
      const std::string *subPath = NULL;

      if (SPACE_TYPE_MAIN_DATA == type)
      {
         subPath = &(path->dataPath);
      }
      else if (SPACE_TYPE_IDX == type)
      {
         subPath = &(path->autoGetIndexPath());
      }
      else
      {
         SDB_ASSERT(FALSE, "TODO");
      }

      dir.reserve(subPath->size() + _unitEntryDir.size() + 2);
      dir.append(subPath->c_str());
      dir.append(OSS_FILE_SEP);
      dir.append(_unitEntryDir);
      dir.append(OSS_FILE_SEP);
      return;
   }

   INT32 storageUnit::openStorageFile(const vesselFileName &fn,
                                       storageFile *file)const
   {
      INT32 rc = SDB_OK;
      ossPoolString fullDir;

      if (OSS_UNLIKELY(!fn.isValid() ||
                       NULL == file))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_unitEntryDir.empty() || NULL == _path)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      file->close();
      buildFullDir(_path, fn.getSpaceType(), fn.getFileType(), fullDir);

      rc = file->open(strSlice(fullDir.c_str(), fullDir.size()), fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file[%s] under dir[%s], rc:%d",
                fn.getFileName(), fullDir.c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createStorageFile(const vesselFileName &fn,
                                        const createStorageFileOptions &o,
                                        const slice &userDefinedHead,
                                        storageFile *file)const
   {
      INT32 rc = SDB_OK;
      ossPoolString fullDir;

      if (OSS_UNLIKELY(!fn.isValid() ||
                       NULL == file))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_unitEntryDir.empty() || NULL == _path)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      file->close();
      buildFullDir(_path, fn.getSpaceType(), fn.getFileType(), fullDir);
      rc = file->create(strSlice(fullDir.c_str(), fullDir.size()),
                        fn, o, userDefinedHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s] under dir[%s], rc:%d",
                fn.getFileName(), fullDir.c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::getDirPathOfType(SPACE_TYPE type,
                                       ossPoolString &dir)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_unitEntryDir.empty() || NULL == _path)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      dir.clear();
      buildFullDir(_path, type, INVALID_FILE_TYPE, dir);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::destroyStorageFile(const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      ossPoolString fullPath;

      if (OSS_UNLIKELY(!fn.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_unitEntryDir.empty() || NULL == _path)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      buildFullDir(_path, fn.getSpaceType(), fn.getFileType(), fullPath);
      fullPath.append(OSS_FILE_SEP);
      fullPath.append(fn.getFileName());

      rc = ossDelete(fullPath.c_str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file:%s, rc:%d", fullPath.c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine