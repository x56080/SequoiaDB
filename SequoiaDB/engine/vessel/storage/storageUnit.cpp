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

   Source File Name = storageUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageUnit.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "utilStr.hpp"
#include "vessel/mainDataSpace.h"
#include "vessel/storageFileLoader.h"
#include "vessel/storageUtils.h"
#include "vessel/storageFileMaintainer.h"
#include "vessel/controlFile.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   storageUnit::storageUnit():
   _mds(&_manifest),
   _is(&_manifest),
   _los(&_manifest)
   {

   }

   storageUnit::~storageUnit()
   {
      close();
   }

   void storageUnit::close()
   {
      if (isOpen())
      {
         _mds.close();
         _is.close();
         _los.close();

         _manifest.reset();
      }
      return;
   }

   INT32 storageUnit::destroy()
   {
      INT32 rc = SDB_OK;
      storageFileMaintainer sfm;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      if (!isOpen())
      {
         goto done;
      }

      sfm.init(&po, _manifest.id.getSpaceId());

      _mds.destroy();
      _is.destroy();
      _los.destroy();

      sfm.removeSpaceDirs();
      _manifest.reset();
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 storageUnit::create(const collectionSpaceId &id,
                             const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      BOOLEAN dirCreated = FALSE;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be invalid");
      const storagePathOptions &po = tc->getEnv()->options.path;
      storageFileMaintainer sfm;
      ossPoolString manifestPath;

      if (OSS_UNLIKELY(!id.isValid() ||
                       !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      sfm.init(&po, id.getSpaceId());

      manifestPath = sfm.buildFullPath(SPACE_TYPE_MAIN_DATA,
                                       MANIFEST_FILE_NAME);
      if (OSS_UNLIKELY(manifestPath.empty()))
      {
         PD_LOG(PDERROR, "failed to build manifest path");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _manifest.id = id;
      _manifest.flags = 0;
      _manifest.secretValue = ossRand();
      _manifest.dataArgs = options.dataArgs;
      _manifest.idxArgs = options.indexArgs;
      _manifest.lobArgs = options.lobArgs;

      rc = sfm.createSpaceDirs();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create space[%d] dir, rc:%d", id.getSpaceId(), rc);
         goto error;
      }
      dirCreated = TRUE;

      rc = createManifestFile(manifestPath.c_str(), _manifest);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create manifest file:%d", rc);
         goto error;
      }

      rc = createMainDataSpace();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }

      rc = createIndexSpace();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index space:%d", rc);
         goto error;
      }

      /// delay to create lob space
   done:
      return rc;
   error:
      if (_is.isOpen())
      {
         _is.destroy();
      }
      if (_mds.isOpen())
      {
         _mds.destroy();
      }
      if (dirCreated)
      {
         ensureManifestFileRemoved(manifestPath.c_str());
         INT32 tmprc = sfm.removeSpaceDirs();
         if (SDB_OK != tmprc)
         {
            PD_LOG(PDSEVERE, "failed to rollback space dirs:%d", tmprc);
         }
      }
      
      _manifest.reset();
      goto done;
   }

   INT32 storageUnit::open(SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      storageFileLoader loader;
      storageFileMaintainer sfm;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path; 
      ossPoolString manifestPath;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      sfm.init(&po, sid);

      rc = sfm.testBeforeOpenning();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test space[%d] dir, rc:%d", sid, rc);
         goto error;
      }

      manifestPath = sfm.buildFullPath(SPACE_TYPE_MAIN_DATA, MANIFEST_FILE_NAME);
      if (OSS_UNLIKELY(manifestPath.empty()))
      {
         PD_LOG(PDERROR, "failed to build manifest path");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = loadManifestFile(manifestPath.c_str(), _manifest);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open mainifest file:%d", rc);
         goto error;
      }

      if (_manifest.id.getSpaceId() != sid)
      {
         PD_LOG(PDERROR, "unexpected sid[%d] saved in manifest when open cs[%d]",
                _manifest.id.getSpaceId(), sid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = sfm.load(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load files under space[%d]:%d", sid, rc);
         goto error;
      }

      rc = _mds.open(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data space:%d", rc);
         goto error;
      }

      rc = _is.open(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index space:%d", rc);
         goto error;
      }

      rc = _los.open(&loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open lob space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 storageUnit::createMainDataSpace()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_manifest.isValid(), "can not be invalid");

      rc = _mds.create();
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

   INT32 storageUnit::createIndexSpace()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_manifest.isValid(), "can not be invalid");

      rc = _is.create();
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

      if (SPACE_TYPE_MAIN_DATA == spaceType &&
          FILE_TYPE_DATA_STORAGE == fileType)
      {
         rc = _mds.getFileCluster()->getPageMmapPtr(pid, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (SPACE_TYPE_IDX == spaceType &&
               FILE_TYPE_DATA_STORAGE == fileType)
      {
         rc = _is.getFileCluster()->getPageMmapPtr(pid, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         PD_LOG(PDERROR, "invalid ptr accessing[%d, %d]", spaceType, fileType);
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 storageUnit::getStoragePageSize(SPACE_TYPE spaceType)const
   {
      UINT32 psz = 0;
      if (SPACE_TYPE_MAIN_DATA == spaceType)
      {
         psz = _manifest.dataArgs.pageSize;
      }
      else if (SPACE_TYPE_IDX == spaceType)
      {
         psz = _manifest.idxArgs.pageSize;
      }
      else if (SPACE_TYPE_LOB == spaceType)
      {
         psz = _manifest.lobArgs.pageSize;
      }
      else
      {
         SDB_ASSERT(FALSE, "impossible");
      }
      
      return psz;
   }

   INT32 storageUnit::createManifestFile(const CHAR *fullPath,
                                         const storageUnitManifest &manifest)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(manifest.isValid(), "can not be invalid");
      strSlice pathSlice(fullPath);
      SDB_ASSERT(!pathSlice.empty(), "can not be empty");

      bson::BSONObj manifestObj = buildSuManifestObj(manifest);
      if (manifestObj.isEmpty())
      {
         PD_LOG(PDERROR, "failed to build manifest obj");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = controlFile::create(pathSlice,
                               manifestObj.objdata(),
                               manifestObj.objsize());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create manifest file:%s, rc:%d",
                fullPath, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::loadManifestFile(const CHAR *fullPath,
                                       storageUnitManifest &manifest)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      CHAR *buffer = nullptr;
      strSlice path(fullPath);
      SDB_ASSERT(!path.empty(), "can not be invalid");
      controlFile file;
      UINT32 contentLen = 0;
      manifest.reset();
      invalidFileReason reason = invalidFileReason::NONE;

      rc = file.openToRead(path, reason);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open manifest file[%s], rc:%d",
                fullPath, rc);
         goto error;
      }

      contentLen = file.getContentLen();
      buffer = tc->allocateBuffer(contentLen);
      if (nullptr == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = file.read(buffer, contentLen);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read manifest file:%d", rc);
         goto error;
      }

      try
      {
         bson::BSONObj obj(buffer);
         if (!parseSuManifestObj(obj, manifest))
         {
            PD_LOG(PDERROR, "failed to parse manifest obj");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "unexpected error:%s", e.what());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
   done:
      if (nullptr != buffer)
      {
         tc->releaseBuffer(buffer);
      }
      file.close();
      return rc;
   error:

      goto done;
   }

   void storageUnit::ensureManifestFileRemoved(const CHAR *fullPath)
   {
      SDB_ASSERT(nullptr != fullPath && fullPath[0] != '\0', "can not be invalid");
      INT32 rc = ossDelete(fullPath);
      if (SDB_OK != rc && SDB_FNE != rc)
      {
         PD_LOG(PDSEVERE, "failed to remvoe manifest file:%s, rc:%d",
                fullPath, rc);
      }
      return;
   }
}//namespace vessel
}//namespace engine