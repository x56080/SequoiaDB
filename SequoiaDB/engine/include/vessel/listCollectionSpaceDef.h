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

   Source File Name = listCollectionSpaceDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LIST_COLLECTION_SPACE_DEF_H_
#define VESSEL_LIST_COLLECTION_SPACE_DEF_H_

#include "vessel/vesselIdDef.h"
#include "utilUniqueID.hpp"

namespace engine
{
namespace vessel
{
   class listCollectionSpaceRecord : public SDBObject
   {
      public:
         listCollectionSpaceRecord():
         version(0),
         uniqueID(UTIL_INVALID_CS_UNIQUE_ID),
         sid(INVALID_SPACE_ID),
         status(0),
         flags(0),
         dataPageSize(0),
         dataPageCountPerSeg(0),
         idxPageSize(0),
         idxPageCountPerSeg(0)
         {
            ossMemset(name, 0, sizeof(name));
         }

         ~listCollectionSpaceRecord()
         {}

      public:
         UINT32 version;
         CHAR name[DMS_COLLECTION_SPACE_NAME_SZ+1];
         utilCSUniqueID uniqueID;
         SPACE_ID sid;
         UINT32 status;
         UINT32 flags;
         UINT32 dataPageSize;
         UINT32 dataPageCountPerSeg;
         UINT32 idxPageSize;
         UINT32 idxPageCountPerSeg;
   };//class listCollectionSpaceRecord
}
}

#endif//VESSEL_LIST_COLLECTION_SPACE_DEF_H_
