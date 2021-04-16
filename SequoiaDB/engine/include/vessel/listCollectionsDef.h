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

   Source File Name = listCollectionsDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LIST_COLLECTIONS_DEF_H_
#define VESSEL_LIST_COLLECTIONS_DEF_H_

#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
   class listCollectionsRecord : public SDBObject
   {
      public:
         listCollectionsRecord():
         version(0),
         csUniqueID(UTIL_INVALID_CS_UNIQUE_ID),
         clInnerID(UTIL_INVALID_CL_INNER_ID),
         clLogicalID(DMS_INVALID_LOGICCLID),
         spaceID(INVALID_SPACE_ID),
         mbID(INVALID_CL_MB_ID),
         maxSGCount(0)
         {
            ossMemset(name, 0, sizeof(name));
         }

         ~listCollectionsRecord()
         {}

      public:
         UINT32 version;
         CHAR name[DMS_COLLECTION_NAME_SZ+1];
         utilCSUniqueID csUniqueID;
         utilCLInnerID clInnerID;
         UINT32 clLogicalID;
         SPACE_ID spaceID;
         CL_MB_ID mbID;
         UINT16 maxSGCount;
   };//class listCollectionsRecord
}
}

#endif//VESSEL_LIST_COLLECTION_SPACE_DEF_H_
