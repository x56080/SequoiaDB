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

   Source File Name = storageFileLoader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_LOADER_H_
#define VESSEL_STORAGE_FILE_LOADER_H_

#include "ossMemPool.hpp"
#include "vessel/vesselFileDef.h"
#include "vessel/vesselIdDef.h"
#include "vessel/storageFileName.h"

namespace engine
{
namespace vessel
{
   class storageFileLoader : public SDBObject
   {
      public:
         storageFileLoader();
         ~storageFileLoader();
         storageFileLoader(const storageFileLoader &) = delete;
         storageFileLoader &operator=(const storageFileLoader &) = delete;

      public:
         typedef ossPoolMap<FILE_TYPE, STORAGE_FILE_NAME_LIST> FILES_WITH_SPACE_TYPE;

      private:
         typedef ossPoolMap<SPACE_TYPE, FILES_WITH_SPACE_TYPE*> _ALL_FILE_MAP;

      public:
         void clear();

         INT32 load(const strSlice &dir);

         INT32 append(const strSlice &dir, SPACE_TYPE type);

         /// return null if type not exists
         const STORAGE_FILE_NAME_LIST *getFileList(SPACE_TYPE stype,
                                                   FILE_TYPE ftype)const;

         BOOLEAN isEmpty()const
         {
            return _all.empty();
         }

         void autoRemoveTmpFiles(BOOLEAN v)
         {
            _removeTmpFile = v;
         }

      private:
         /// load all types when specifiedType is not valid
         INT32 _load(const strSlice &dir, SPACE_TYPE specifiedType);
      
      private:
         BOOLEAN _removeTmpFile = TRUE;
         _ALL_FILE_MAP _all;

   };//class storageFileLoader
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_FILE_LOADER_H_