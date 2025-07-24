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

   Source File Name = storageFileLoader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

         INT32 append(const strSlice &dir, SPACE_TYPE filter=INVALID_SPACE_TYPE);

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