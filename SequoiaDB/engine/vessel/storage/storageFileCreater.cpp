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

   Source File Name = storageFileCreater.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageFileCreater.h"
#include "ossLikely.hpp"
#include "vessel/storageFile.h"

namespace engine
{
namespace vessel
{
   INT32 storageFileCreater::init(SPACE_ID sid,
                                  SPACE_TYPE type,
                                  UINT32 secretValue,
                                  const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       INVALID_SPACE_TYPE == type ||
                       dir.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _sid = sid;
      _type = type;
      _secretValue = secretValue;
      _dir = dir.str();
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void storageFileCreater::fini()
   {
      _sid = INVALID_SPACE_ID;
      _type = INVALID_SPACE_TYPE;
      _secretValue = 0;
      _dir.clear();
      return;
   }

   INT32 storageFileCreater::createNewFile(FILE_TYPE fileType,
                                           UINT64 sequence,
                                           const storageCoreArgs &args,
                                           BOOLEAN tmpMode,
                                           BOOLEAN replace,
                                           const slice &userDefinedHead,
                                           storageFile *file)const
   {
      INT32 rc = SDB_OK;
      createStorageFileOptions o;
      vesselFileName fn;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_FILE_TYPE == fileType ||
                            !args.isValid() ||
                            NULL == file ||
                            file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!fn.build(_sid, fileType, _type, sequence)))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.dir.reset(_dir.c_str(), _dir.size());
      o.secretValue = _secretValue;
      o.args = args;
      o.replaceWhenCreate = replace;
      o.createAsTmpFile = tmpMode;

      rc = file->create(fn, o, userDefinedHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileCreater::createTmpFile(FILE_TYPE fileType,
                                           UINT64 sequence,
                                           const storageCoreArgs &args,
                                           storageFile *file,
                                           const slice &userDefinedHead)const
   {
      return createNewFile(fileType, sequence, args, TRUE, TRUE, userDefinedHead, file);
   }

   INT32 storageFileCreater::createFormalFile(FILE_TYPE fileType,
                                              UINT64 sequence,
                                              const storageCoreArgs &args,
                                              storageFile *file,
                                              const slice &userDefinedHead)const
   {
      return createNewFile(fileType, sequence, args, FALSE, FALSE, userDefinedHead, file);
   }

}//namespace vessel
}//namespace engine
