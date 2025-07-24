/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = storageUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
   struct manifestBsonProperties : public SDBObject
   {
      static constexpr CHAR *SPACE_ID = "SpaceId";
      static constexpr CHAR *LOGICAL_ID = "LogicalId";
      static constexpr CHAR *UNIQUE_ID = "UniqueId";
      static constexpr CHAR *FLAGS = "Flags";
      static constexpr CHAR *SECRET_VALUE = "SecretValue";
      static constexpr CHAR *PAGE_SIZE = "PageSize";
      static constexpr CHAR *PAGE_COUNT = "PageCount";
      static constexpr CHAR *SEG_COUNT = "SegmentCount";

      static constexpr CHAR *DATA_ARGS = "DataArgs";
      static constexpr CHAR *IDX_ARGS = "IndexArgs";
      static constexpr CHAR *LOB_ARGS = "LobArgs";
   };


   INT32 renameFileShadowSuffix(const strSlice &dir,
                                BOOLEAN replaceNewFile,
                                const storageFileName &oldFileName,
                                const storageFileName &newFileName)
   {
      INT32 rc = SDB_OK;
      storageFileName newFn;
      CHAR src[OSS_MAX_PATHSIZE + 1] = {0};
      CHAR dst[OSS_MAX_PATHSIZE + 1] = {0};

      if (OSS_UNLIKELY(dir.empty() ||
                       !oldFileName.isValid() ||
                       !newFileName.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(oldFileName.getSpaceType() != newFileName.getSpaceType() ||
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
                                STORAGE_FILE_NAME_LIST &fl)
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
         storageFileName fn;

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

         if (fn.getSpaceType() != spaceType)
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

   BOOLEAN buildSpaceDirName(SPACE_ID sid, UINT32 bufLen, CHAR *buf)
   {
      BOOLEAN r = FALSE;
      if (INVALID_SPACE_ID == sid ||
          MAX_SPACE_ID < sid)
      {
         goto done;
      }
      else if (bufLen < (MAX_SPACE_DIR_LEN + 1) ||
               nullptr == buf)
      {
         goto done;
      }

      ossMemset(buf, 0, MAX_SPACE_DIR_LEN + 1);
      ossSnprintf(buf, MAX_SPACE_DIR_LEN + 1, "%s%05d",
                  DIR_NAME_PREFIX, sid);
      
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN parseSpaceDirName(const strSlice &dirName, SPACE_ID *sid)
   {
      BOOLEAN r = FALSE;
      UINT32 digit = 0;
      /// _cs_<space id>
      if (dirName.strLen() <= DIR_NAME_PREFIX_LEN)
      {
         goto done;
      }
      else if (0 != ossStrncmp(DIR_NAME_PREFIX, dirName.str(), DIR_NAME_PREFIX_LEN))
      {
         goto done;
      }
      else if (!utilStrIsDigit(dirName.str() + DIR_NAME_PREFIX_LEN))
      {
         goto done;
      }

      digit = ossAtoi(dirName.str() + DIR_NAME_PREFIX_LEN);
      if (MAX_SPACE_ID < digit)
      {
         goto done;
      }

      r = TRUE;
      if (NULL != sid)
      {
         *sid = (SPACE_ID)digit;
      }
   done:
      return r;
   }

   bson::BSONObj buildSuManifestObj(const storageUnitManifest &manifest)
   {
      SDB_ASSERT(manifest.isValid(), "can not be invalid");
      bson::BSONObjBuilder builder;
      builder.append(manifestBsonProperties::SPACE_ID, manifest.id.getSpaceId());
      builder.append(manifestBsonProperties::LOGICAL_ID, manifest.id.getLid());
      builder.append(manifestBsonProperties::UNIQUE_ID, manifest.id.getUniqueId());
      builder.append(manifestBsonProperties::FLAGS, manifest.flags);
      builder.append(manifestBsonProperties::SECRET_VALUE, manifest.secretValue);

      bson::BSONObjBuilder data(builder.subobjStart(manifestBsonProperties::DATA_ARGS));
      data.append(manifestBsonProperties::PAGE_SIZE, manifest.dataArgs.pageSize);
      data.append(manifestBsonProperties::PAGE_COUNT, manifest.dataArgs.maxPageCountPerSeg);
      data.append(manifestBsonProperties::SEG_COUNT, manifest.dataArgs.maxSegmentCountPerFile);
      data.doneFast();

      bson::BSONObjBuilder index(builder.subobjStart(manifestBsonProperties::IDX_ARGS));
      index.append(manifestBsonProperties::PAGE_SIZE, manifest.idxArgs.pageSize);
      index.append(manifestBsonProperties::PAGE_COUNT, manifest.idxArgs.maxPageCountPerSeg);
      index.append(manifestBsonProperties::SEG_COUNT, manifest.idxArgs.maxSegmentCountPerFile);
      index.doneFast();

      bson::BSONObjBuilder lob(builder.subobjStart(manifestBsonProperties::LOB_ARGS));
      lob.append(manifestBsonProperties::PAGE_SIZE, manifest.lobArgs.pageSize);
      lob.append(manifestBsonProperties::PAGE_COUNT, manifest.lobArgs.maxPageCountPerSeg);
      lob.append(manifestBsonProperties::SEG_COUNT, manifest.lobArgs.maxSegmentCountPerFile);
      lob.doneFast();

      return builder.obj();
   }

   BOOLEAN parseSuManifestObj(const bson::BSONObj &obj,
                              storageUnitManifest &manifest)
   {
      BOOLEAN r = FALSE;
      SPACE_ID sid = INVALID_SPACE_ID;
      UINT32 lid = DMS_INVALID_LOGICCLID;
      utilCSUniqueID uniqueId = UTIL_UNIQUEID_NULL;
      bson::BSONElement e;
      manifest.reset();

      PD_LOG(PDDEBUG, "parsing manifest bson:%s",
             obj.toPoolString(FALSE, TRUE, TRUE).c_str());

      e = obj.getField(manifestBsonProperties::SPACE_ID);
      if (!e.isNumber() ||
          (INT32)MAX_SPACE_ID < e.numberInt() ||
          e.numberInt() < 0)
      {
         goto done;
      }
      sid = e.numberInt();

      e = obj.getField(manifestBsonProperties::LOGICAL_ID);
      if (!e.isNumber())
      {
         goto done;
      }
      lid = e.numberInt();

      e = obj.getField(manifestBsonProperties::UNIQUE_ID);
      if (!e.isNumber())
      {
         goto done;
      }
      uniqueId = e.numberInt();

      manifest.id = collectionSpaceId(lid, uniqueId, sid);

      e = obj.getField(manifestBsonProperties::FLAGS);
      if (!e.isNumber())
      {
         goto done;
      }
      manifest.flags = e.numberInt();

      e = obj.getField(manifestBsonProperties::SECRET_VALUE);
      if (!e.isNumber())
      {
         goto done;
      }
      manifest.secretValue = e.numberInt();

      e = obj.getField(manifestBsonProperties::DATA_ARGS);
      if (!e.isABSONObj())
      {
         goto done;
      }
      else
      {
         bson::BSONObj args = e.embeddedObject();
         e = args.getField(manifestBsonProperties::PAGE_SIZE);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.dataArgs.pageSize = e.numberInt();

         e = args.getField(manifestBsonProperties::PAGE_COUNT);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.dataArgs.maxPageCountPerSeg = e.numberInt();

         e = args.getField(manifestBsonProperties::SEG_COUNT);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.dataArgs.maxSegmentCountPerFile = e.numberInt();
      }

      e = obj.getField(manifestBsonProperties::IDX_ARGS);
      if (!e.isABSONObj())
      {
         goto done;
      }
      else
      {
         bson::BSONObj args = e.embeddedObject();
         e = args.getField(manifestBsonProperties::PAGE_SIZE);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.idxArgs.pageSize = e.numberInt();

         e = args.getField(manifestBsonProperties::PAGE_COUNT);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.idxArgs.maxPageCountPerSeg = e.numberInt();

         e = args.getField(manifestBsonProperties::SEG_COUNT);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.idxArgs.maxSegmentCountPerFile = e.numberInt();
      }

      e = obj.getField(manifestBsonProperties::LOB_ARGS);
      if (!e.isABSONObj())
      {
         goto done;
      }
      else
      {
         bson::BSONObj args = e.embeddedObject();
         e = args.getField(manifestBsonProperties::PAGE_SIZE);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.lobArgs.pageSize = e.numberInt();

         e = args.getField(manifestBsonProperties::PAGE_COUNT);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.lobArgs.maxPageCountPerSeg = e.numberInt();

         e = args.getField(manifestBsonProperties::SEG_COUNT);
         if (!e.isNumber())
         {
            goto done;
         }
         manifest.lobArgs.maxSegmentCountPerFile = e.numberInt();
      }

      if (!manifest.isValid())
      {
         goto done;
      }

      r = TRUE;

   done:
      if (!r)
      {
         manifest.reset();
      }
      return r;
   }
}//namespace vessel
}//namespace engine