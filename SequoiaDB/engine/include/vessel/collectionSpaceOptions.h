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

   Source File Name = collectionSpaceOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_SPACE_OPTIONS_H_
#define VESSEL_COLLECTION_SPACE_OPTIONS_H_

#include "dms.hpp"
#include "vessel/storageFileDef.h"
#include "../bson/bson.hpp"

namespace engine
{
namespace vessel
{
   class createCSOptions : public SDBObject
   {
      public:
         createCSOptions(){}
         ~createCSOptions(){}
         createCSOptions(const createCSOptions &) = delete;
         createCSOptions &operator=(const createCSOptions &o);

         BOOLEAN isValid()const;
         bson::BSONObj toBson()const;
         BOOLEAN loadFromBson(const bson::BSONObj &obj);

      public:
         UINT32 dataPageSize = DMS_PAGE_SIZE32K;
         UINT32 dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;
         UINT32 idxPageSize = DMS_PAGE_SIZE32K;
         UINT32 idxSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;
         UINT32 lobPageSize = DMS_PAGE_SIZE256K;
         UINT32 lobSegSize = STORAGE_FILE_SEGMENT_SIZE_128MB;
      
   };/// end of class createCSOptions
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_SPACE_OPTIONS_H_