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

   Source File Name = storageUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageUtils.h"
#include "vessel/vesselFileDef.h"
#include "pdTrace.hpp"
#include "utilStr.hpp"
#include "ossLikely.hpp"
#include "vessel/storageFile.h"
#include "vessel/idMapPage.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{

   INT32 renameFileShadowSuffix(const strSlice &dir,
                                BOOLEAN replaceNewFile,
                                const vesselFileName &oldFileName,
                                const vesselFileName &newFileName)
   {
      INT32 rc = SDB_OK;
      vesselFileName newFn;
      CHAR src[OSS_MAX_PATHSIZE + 1] = {0};
      CHAR dst[OSS_MAX_PATHSIZE + 1] = {0};

      if (OSS_UNLIKELY(dir.empty() ||
                       !oldFileName.isValid() ||
                       !newFileName.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(oldFileName.getSpaceID() != newFileName.getSpaceID() ||
                            oldFileName.getSpaceType() != newFileName.getSpaceType() ||
                            oldFileName.getFileType() != newFileName.getFileType() ||
                            oldFileName.getSequence() != newFileName.getSequence()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(oldFileName.getShadowSuffix() == newFileName.getShadowSuffix()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = utilBuildFullPath(dir.str(), oldFileName.getFileName(),
                             OSS_MAX_PATHSIZE + 1, src);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build src full path:%d", rc);
         goto error;
      }

      rc = utilBuildFullPath(dir.str(), newFileName.getFileName(),
                             OSS_MAX_PATHSIZE + 1, dst);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build dst full path:%d", rc);
         goto error;
      }

      if (replaceNewFile && SDB_OK == ossAccess(dst))
      {
         PD_LOG(PDINFO, "remove dst file before rename:%s",dst);
         rc = ossDelete(dst);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove file:%s, rc:%d", dst, rc);
            goto error;
         }
      }

      rc = ossRenamePath(src, dst);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename file from %s to %s",
                src, dst);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

/*
   INT32 renameToFormalAndReopen(const strSlice &dir,
                                 BOOLEAN replaceNewFile,
                                 UINT32 shadowSuffix,
                                 storageFile *file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_FILE_SHADOW_SUFFIX != shadowSuffix, "can not be invalid");
      vesselFileName oldFn;
      vesselFileName newFn;
      SPACE_ID sid = INVALID_SPACE_ID;
      SPACE_TYPE spaceType = INVALID_SPACE_TYPE;
      FILE_TYPE fileType = INVALID_FILE_TYPE;
      UINT64 sequence = 0;

      if (OSS_UNLIKELY(dir.empty() ||
                       NULL == file ||
                       !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = file->getCommonHeadInMem().spaceID;
      spaceType = file->getCommonHeadInMem().spaceType;
      fileType = file->getCommonHeadInMem().fileType;
      sequence = file->getCommonHeadInMem().sequence;

      if (!oldFn.build(sid, fileType, spaceType, sequence, shadowSuffix))
      {
         PD_LOG(PDERROR, "failed to build old file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!newFn.build(sid, fileType, spaceType, sequence))
      {
         PD_LOG(PDERROR, "failed to build new file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// To clear full path saved in ossMmapFile, we alwasy reopen file.
      file->close();

      rc = renameFileShadowSuffix(dir, replaceNewFile, oldFn, newFn);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = file->open(dir, newFn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reopen new file[%s]:%d", newFn.getFileName(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
   */

   INT32 createFileListUnderDir(const strSlice &dir,
                                SPACE_ID sid,
                                SPACE_TYPE spaceType,
                                UINT32 fileTypesSize,
                                const FILE_TYPE *fileTypes,
                                BOOLEAN removeTmpFile,
                                ossPoolList<vesselFileName> &fl)
   {
      INT32 rc = SDB_OK;

      fs::directory_iterator end_iter ;
      fs::path dataDir(dir.str());

      if (OSS_UNLIKELY(dir.empty() ||
                       INVALID_SPACE_ID == sid ||
                       INVALID_SPACE_TYPE == spaceType ||
                       0 == fileTypesSize ||
                       NULL == fileTypes))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!fs::exists(dataDir) ||
          !fs::is_directory(dataDir))
      {
         PD_LOG(PDERROR, "invalid dir path:%s", dir.str());
         rc = SDB_FNE;
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string fileName = dir_iter->path().filename().string();
         strSlice fileNameSlice(fileName.c_str(), fileName.size());
         vesselFileName fn;

         if (!fs::is_regular_file(dir_iter->status()))
         {
            PD_LOG(PDDEBUG, "not regular file", fileName.c_str());
            continue;
         }

         if (!fn.extract(fileNameSlice, TRUE))
         {
            PD_LOG(PDDEBUG, "invalid file name:%s", fileName.c_str());
            continue;
         }

         if (fn.getSpaceID() != sid ||
             fn.getSpaceType() != spaceType)
         {
            PD_LOG(PDDEBUG, "not target file, file name:%s", fileName.c_str());
            continue;
         }

         if (removeTmpFile &&
             fn.hasShadowSuffix() &&
             FILE_SHADOW_SUFFIX_TMP == fn.getShadowSuffix())
         {
            PD_LOG(PDINFO, "remove tmp vessel file:%s",
                   dir_iter->path().string().c_str());
            fs::remove(dir_iter->path());
            continue;
         }

         for (UINT32 i = 0; i < fileTypesSize; ++i)
         {
            if (fn.getFileType() == fileTypes[i])
            {
               fl.push_back(fn);
               break;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine