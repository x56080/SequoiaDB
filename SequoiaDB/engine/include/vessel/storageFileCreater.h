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

   Source File Name = storageFileCreater.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_CREATER_H_
#define VESSEL_STORAGE_FILE_CREATER_H_

#include "vessel/vesselFileName.h"
#include "vessel/storageFileDef.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class storageFile;

   class storageFileCreater : public SDBObject
   {
      public:
         storageFileCreater();
         ~storageFileCreater();
         storageFileCreater(const storageFileCreater &o):
         _sid(o._sid),
         _type(o._type),
         _secretValue(o._secretValue),
         _dir(o._dir){}

         storageFileCreater &operator=(const storageFileCreater &o)
         {
            _sid = o._sid;
            _type = o._type;
            _secretValue = o._secretValue;
            _dir = o._dir;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _sid;
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }
         OSS_INLINE SPACE_TYPE getSpaceType()const
         {
            return _type;
         }
         OSS_INLINE UINT32 getSecretValue()const
         {
            return _secretValue;
         }
         OSS_INLINE const std::string &getDir()const
         {
            return _dir;
         }
         OSS_INLINE strSlice getDirSlice()const
         {
            return strSlice(_dir.c_str(), _dir.size());
         }

      public:
         INT32 init(SPACE_ID sid,
                    SPACE_TYPE type,
                    UINT32 secretValue,
                    const strSlice &dir);

         void fini();

         INT32 createNewFile(FILE_TYPE fileType,
                             UINT64 sequence,
                             const storageCoreArgs &args,
                             BOOLEAN tmpMode,
                             BOOLEAN replace,
                             const slice &userDefinedHead,
                             storageFile *file)const;

         INT32 createTmpFile(FILE_TYPE fileType,
                             UINT64 sequence,
                             const storageCoreArgs &args,
                             storageFile *file,
                             const slice &userDefinedHead = slice())const;

         INT32 createFormalFile(FILE_TYPE fileType,
                                UINT64 sequence,
                                const storageCoreArgs &args,
                                storageFile *file,
                                const slice &userDefinedHead = slice())const;

      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         UINT32 _secretValue = 0;
         std::string _dir;
   };//class storageFileCreater
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_FILE_CREATER_H_