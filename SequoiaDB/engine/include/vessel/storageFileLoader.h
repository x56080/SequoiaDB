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
#include "vessel/vesselFileName.h"

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
         void clear();

         INT32 load(const strSlice &dir,
                    SPACE_ID sid,
                    SPACE_TYPE type,
                    BOOLEAN removeTmpFile);

         /// return null if type not exists
         const FILE_NAME_LIST *getFileList(FILE_TYPE type)const;

         BOOLEAN isEmpty()const
         {
            return _map.empty();
         }
         UINT32 getSize()const
         {
            return _map.size();
         }

      private:
         typedef ossPoolMap<FILE_TYPE, FILE_NAME_LIST> _FILES_MAP;

      
      private:
         _FILES_MAP _map;

   };//class storageFileLoader
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_FILE_LOADER_H_