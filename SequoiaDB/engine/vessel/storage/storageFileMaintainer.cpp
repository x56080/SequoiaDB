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

   Source File Name = storageFileMaintainer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageFileMaintainer.h"
#include "pdTrace.hpp"
#include "ossMemPool.hpp"
#include "vessel/storageUtils.h"

namespace engine
{
namespace vessel
{
   storageFileMaintainer::storageFileMaintainer(const storagePathOptions *path,
                                                SPACE_ID sid):
   _path(path),
   _sid(sid)
   {
      SDB_ASSERT(nullptr != _path, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");
      BOOLEAN r = buildSpaceDirName(_sid, sizeof(_subDir), _subDir);
      SDB_ASSERT(r, "must be ok");
   }

   INT32 storageFileMaintainer::init(const storagePathOptions *path,
                                     SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      reset();
      if (OSS_UNLIKELY(nullptr == path ||
                       INVALID_SPACE_ID == sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _path = path;
      _sid = sid;
      if (OSS_UNLIKELY(!buildSpaceDirName(sid, sizeof(_subDir), _subDir)))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void storageFileMaintainer::reset()
   {
      _path = nullptr;
      _sid = INVALID_SPACE_ID;
      ossMemset(_subDir, 0, sizeof(_subDir));
      return;
   }

   INT32 storageFileMaintainer::testBeforeOpenning()const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      ossPoolString fullPath;
      ossPoolSet<std::string> uniquePath;

      uniquePath.insert(_path->dataPath);
      uniquePath.insert(_path->autoGetIndexPath());
      uniquePath.insert(_path->autoGetLobmPath());
      uniquePath.insert(_path->autoGetLobdPath());

      for (auto &p : uniquePath)
      {
         fullPath = _build(p.c_str(), _subDir);
         if (OSS_UNLIKELY(fullPath.empty()))
         {
            PD_LOG(PDERROR, "failed to build full path");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossAccess(fullPath.c_str());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to test path:%s, rc:%d", fullPath.c_str(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileMaintainer::createSpaceDirs()const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      ossPoolSet<std::string> uniquePath;
      ossPoolString fullPath;
      ossPoolVector<ossPoolString> created;

      uniquePath.insert(_path->dataPath);
      uniquePath.insert(_path->autoGetIndexPath());
      uniquePath.insert(_path->autoGetLobmPath());
      uniquePath.insert(_path->autoGetLobdPath());

      for (auto const &p : uniquePath)
      {
         fullPath = _build(p.c_str(), _subDir);
         if (OSS_UNLIKELY(fullPath.empty()))
         {
            PD_LOG(PDERROR, "failed to build full path");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossAccess(fullPath.c_str());
         if (SDB_OK == rc)
         {
            PD_LOG(PDERROR, "path[%s] already exists", fullPath.c_str());
            rc = SDB_FE;
            goto error;
         }
         else if (SDB_FNE == rc)
         {
            rc = SDB_OK;
            continue;
         }
         else
         {
            PD_LOG(PDERROR, "failed to test path:%s, rc:%d", fullPath.c_str(), rc);
            goto error;
         }
      }

      for (auto const &p : uniquePath)
      {
         fullPath = _build(p.c_str(), _subDir);
         if (OSS_UNLIKELY(fullPath.empty()))
         {
            PD_LOG(PDERROR, "failed to build full path");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossMkdir(fullPath.c_str());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make dir[%s], rc:%d", fullPath.c_str(), rc);
            goto error;
         }

         created.push_back(std::move(fullPath));
      }

   done:
      return rc;
   error:
      for (UINT32 i = 0; i < created.size(); ++i)
      {
         INT32 tmpRc = ossDelete(created.at(i).c_str());
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDERROR, "failed to rollback dir[%s], rc:%d", created.at(i).c_str(), tmpRc);
         }
      }
      goto done;
   }

   INT32 storageFileMaintainer::removeSpaceDirs()const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      ossPoolString fullPath;
      ossPoolSet<std::string> uniquePath;

      uniquePath.insert(_path->dataPath);
      uniquePath.insert(_path->autoGetIndexPath());
      uniquePath.insert(_path->autoGetLobmPath());
      uniquePath.insert(_path->autoGetLobdPath());

      for (auto const &p : uniquePath)
      {
         fullPath = _build(p.c_str(), _subDir);
         rc = ossDelete(fullPath.c_str());
         if (SDB_FNE == rc)
         {
            PD_LOG(PDWARNING, "dir[%s] does not exist", fullPath.c_str());
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove dir[%s], rc:%d", fullPath.c_str(), rc);
            goto error;
         }
         else
         {
            PD_LOG(PDINFO, "dir[%s] removed", fullPath.c_str());
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileMaintainer::createStorageFile(const storageFileName &fn,
                                                  const createStorageFileOptions &o,
                                                  storageFile &file)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(!file.isOpen(), "can not be open");

      ossPoolString fullPath;

      if (OSS_UNLIKELY(!fn.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (FILE_TYPE_LOBM != fn.getFileType())
      {
         if (!buildFullDir(fn.getSpaceType(), fullPath))
         {
            PD_LOG(PDERROR, "failed to build full path of space[%d]", fn.getSpaceType());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      else
      {
         fullPath = _build(_path->autoGetLobmPath().c_str(), _subDir);
      }

      rc = file.create(strSlice(fullPath.c_str(), fullPath.size()),
                        fn, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s] under dir[%s], rc:%d",
                fn.getFileName(), fullPath.c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileMaintainer::openStorageFile(const storageFileName &fn,
                                                UINT32 flags,
                                                storageFile &file,
                                                invalidFileReason *reason)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(!file.isOpen(), "can not be open");

      ossPoolString fullPath;
      strSlice pathSlice;
      invalidFileReason invalidReason = invalidFileReason::NONE;

      if (nullptr != reason)
      {
         *reason = invalidFileReason::NONE;
      }

      if (OSS_UNLIKELY(!fn.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (FILE_TYPE_LOBM != fn.getFileType())
      {
         if (!buildFullDir(fn.getSpaceType(), fullPath))
         {
            PD_LOG(PDERROR, "failed to build full path of space[%d]", fn.getSpaceType());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      else
      {
         fullPath = _build(_path->autoGetLobmPath().c_str(), _subDir);
      }

      pathSlice.reset(fullPath.c_str(), fullPath.size());
      rc = file.open(pathSlice, fn, flags, invalidReason);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s] under dir[%s], rc:%d",
                fn.getFileName(), fullPath.c_str(), rc);
         if (nullptr != reason)
         {
            *reason = invalidReason;
         }
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileMaintainer::removeStorageFile(const storageFileName &fn)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      ossPoolString fullPath;

      if (OSS_UNLIKELY(!fn.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fullPath = buildFullPath(fn);
      if (fullPath.empty())
      {
         PD_LOG(PDERROR, "failed to build full path of [%s]", fn.getFileName());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

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

   BOOLEAN storageFileMaintainer::buildFullDir(SPACE_TYPE type,
                                               ossPoolString &path)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isValid(), "must be valid");
      const std::string *p = nullptr;
      path.clear();

      switch (type)
      {
      case SPACE_TYPE_MAIN_DATA:
         p = &(_path->dataPath);
         break;
      case SPACE_TYPE_IDX:
         p = &(_path->autoGetIndexPath());
         break;
      case SPACE_TYPE_LOB:
         p = &(_path->autoGetLobdPath());
         break;
      default:
         break;
      }

      if (nullptr == p || p->empty())
      {
         PD_LOG(PDERROR, "failed to get path of type[%d]", type);
         goto done;
      }

      path.reserve(p->size() + MAX_SPACE_DIR_LEN + 4);
      path.append(p->c_str());
      path.append(OSS_FILE_SEP);
      path.append(_subDir);
      r = TRUE;

   done:
      return r;
   }

   ossPoolString storageFileMaintainer::buildFullPath(const storageFileName &fn)const
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(fn.isValid(), "can not be invalid");
      ossPoolString path;
      if (buildFullDir(fn.getSpaceType(), path))
      {
         path.append(OSS_FILE_SEP);
         path.append(fn.getFileName());
      }
      return std::move(path);
   }

   ossPoolString storageFileMaintainer::buildFullPath(SPACE_TYPE type,
                                                      const CHAR *fileName)const
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(INVALID_SPACE_TYPE != type, "can not be invalid");
      SDB_ASSERT(nullptr != fileName && 0 < ossStrlen(fileName), "can not be invalid");
      ossPoolString path;
      if (buildFullDir(type, path))
      {
         path.append(OSS_FILE_SEP);
         path.append(fileName);
      }
      return std::move(path);
   }

   INT32 storageFileMaintainer::load(storageFileLoader &loader)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      ossPoolString fullDir;
      ossPoolSet<ossPoolString> uniquePath;
      loader.clear();

      fullDir = _build(_path->dataPath.c_str(), _subDir);
      rc = loader.load(strSlice(fullDir.c_str(), fullDir.size()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load files under path[%s], rc:%d",
                fullDir.c_str(), rc);
         goto error;
      }
      uniquePath.insert(std::move(fullDir));

      fullDir = _build(_path->autoGetIndexPath().c_str(), _subDir);
      if (0 == uniquePath.count(fullDir))
      {
         rc = loader.append(strSlice(fullDir.c_str(), fullDir.size()), SPACE_TYPE_IDX);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to load files under path[%s], rc:%d",
                  fullDir.c_str(), rc);
            goto error;
         }
         uniquePath.insert(std::move(fullDir));
      }

      fullDir = _build(_path->autoGetLobmPath().c_str(), _subDir);
      if (0 == uniquePath.count(fullDir))
      {
         rc = loader.append(strSlice(fullDir.c_str(), fullDir.size()), SPACE_TYPE_LOB);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to load files under path[%s], rc:%d",
                  fullDir.c_str(), rc);
            goto error;
         }
         uniquePath.insert(std::move(fullDir));
      }

      fullDir = _build(_path->autoGetLobdPath().c_str(), _subDir);
      if (0 == uniquePath.count(fullDir))
      {
         rc = loader.append(strSlice(fullDir.c_str(), fullDir.size()), SPACE_TYPE_LOB);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to load files under path[%s], rc:%d",
                  fullDir.c_str(), rc);
            goto error;
         }
         uniquePath.insert(std::move(fullDir));
      }

   done:
      return rc;
   error:
      loader.clear();
      goto done;
   }

   ossPoolString storageFileMaintainer::_build(const CHAR *l, const CHAR *r)const
   {
      SDB_ASSERT(nullptr != l && nullptr != r, "can not be null");
      UINT32 size = 0;
      ossPoolString str;
      str.append(l);
      SDB_ASSERT(size < str.size(), "l str is empty");
      str.append(OSS_FILE_SEP);
      size = str.size();
      str.append(r);
      SDB_ASSERT(size < str.size(), "l str is empty");
      return std::move(str);
   }
} // namespace vessel

} // namespace engine

