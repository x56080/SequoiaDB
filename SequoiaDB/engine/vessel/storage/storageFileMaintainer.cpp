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

   Source File Name = storageFileMaintainer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

   INT32 storageFileMaintainer::testBeforeCreating()const
   {
      INT32 rc = SDB_OK;
      ossPoolString fullPath;

      if (!buildFullDir(SPACE_TYPE_MAIN_DATA, fullPath))
      {
         PD_LOG(PDERROR, "failed to build full path of data");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = ossAccess(fullPath.c_str());
      if (SDB_OK == rc)
      {
         rc = SDB_FE;
         PD_LOG(PDERROR, "dir already exists:%s", fullPath.c_str());
         goto error;
      }
      else if (SDB_FNE == rc)
      {
         rc = SDB_OK;
      }
      else
      {
         PD_LOG(PDERROR, "failed to access path:%s, rc:%d", fullPath.c_str(), rc);
         goto error;
      }

      if (_path->hasExclusiveIndexPath())
      {
         if (!buildFullDir(SPACE_TYPE_IDX, fullPath))
         {
            PD_LOG(PDERROR, "failed to build full path of data");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossAccess(fullPath.c_str());
         if (SDB_OK == rc)
         {
            rc = SDB_FE;
            PD_LOG(PDERROR, "dir already exists:%s", fullPath.c_str());
            goto error;
         }
         else if (SDB_FNE == rc)
         {
            rc = SDB_OK;
         }
         else
         {
            PD_LOG(PDERROR, "failed to access path:%s, rc:%d", fullPath.c_str(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileMaintainer::createSpaceDir()const
   {
      INT32 rc = SDB_OK;
      ossPoolVector<ossPoolString> created;
      
      rc = testBeforeCreating();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test dirs before creating:%d", rc);
         goto error;
      }

      {
         ossPoolString fullPath;
         if (!buildFullDir(SPACE_TYPE_MAIN_DATA, fullPath))
         {
            PD_LOG(PDERROR, "failed to build full path of data");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossMkdir(fullPath.c_str());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to mk dir[%s], rc:%d", fullPath.c_str(), rc);
            goto error;
         }

         created.push_back(std::move(fullPath));
      }

      if (_path->hasExclusiveIndexPath())
      {
         ossPoolString fullPath;
         if (!buildFullDir(SPACE_TYPE_IDX, fullPath))
         {
            PD_LOG(PDERROR, "failed to build full path of data");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossMkdir(fullPath.c_str());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to mk dir[%s], rc:%d", fullPath.c_str(), rc);
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

   INT32 storageFileMaintainer::removeSpaceDir()const
   {
      INT32 rc = SDB_OK;
      ossPoolString fullPath;

      if (_path->hasExclusiveIndexPath())
      {
         if (!buildFullDir(SPACE_TYPE_IDX, fullPath))
         {
            PD_LOG(PDERROR, "failed to build full path of data");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossDelete(fullPath.c_str());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to mk dir[%s], rc:%d", fullPath.c_str(), rc);
            goto error;
         }
      }

      {
         fullPath.clear();
         if (!buildFullDir(SPACE_TYPE_MAIN_DATA, fullPath))
         {
            PD_LOG(PDERROR, "failed to build full path of data");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = ossDelete(fullPath.c_str());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to mk dir[%s], rc:%d", fullPath.c_str(), rc);
            goto error;
         }
      }

      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileMaintainer::createStorageFile(const storageFileName &fn,
                                                const createStorageFileOptions &o,
                                                const slice &userDefinedHead,
                                                storageFile &file)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!file.isOpen(), "can not be open");

      ossPoolString fullPath;

      if (OSS_UNLIKELY(!fn.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!buildFullDir(fn.getSpaceType(), fullPath))
      {
         PD_LOG(PDERROR, "failed to build full path of space[%d]", fn.getSpaceType());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = file.create(strSlice(fullPath.c_str(), fullPath.size()),
                        fn, o, userDefinedHead);
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
                                              storageFile &file)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!file.isOpen(), "can not be open");

      ossPoolString fullPath;
      strSlice pathSlice;

      if (OSS_UNLIKELY(!fn.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!buildFullDir(fn.getSpaceType(), fullPath))
      {
         PD_LOG(PDERROR, "failed to build full path of space[%d]", fn.getSpaceType());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      pathSlice.reset(fullPath.c_str(), fullPath.size());
      rc = file.open(pathSlice, fn);
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

   INT32 storageFileMaintainer::removeStorageFile(const storageFileName &fn)const
   {
      INT32 rc = SDB_OK;
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
      SDB_ASSERT(fn.isValid(), "can not be invalid");
      ossPoolString path;
      if (buildFullDir(fn.getSpaceType(), path))
      {
         path.append(OSS_FILE_SEP);
         path.append(fn.getFileName());
      }
      return std::move(path);
   }
} // namespace vessel

} // namespace engine

