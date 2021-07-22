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

   Source File Name = collectionSpaceGlobalPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_SPACE_GLOBAL_PAGE_H_
#define VESSEL_COLLECTION_SPACE_GLOBAL_PAGE_H_

#include "vessel/pageDef.h"
#include "dms.hpp"
#include "utilUniqueID.hpp"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   static const UINT32 CMR_VERSION_1 = 1;

   enum CMR_STATUS
   {
      CMR_STATUS_INVALID = 0,
      CMR_STATUS_ONLINE = 1,
      CMR_STATUS_REMOVING = 2,
      CMR_STATUS_REMOVED_BUT_SNAPSHOT = 3,
   };

   enum CMR_TYPE
   {
      CMR_TYPE_INVALID = 0,
      CMR_TYPE_NORMAL = 1,
   };

#pragma pack(4)
   struct csMetaRecord
   {
      UINT32 version = 0;
      UINT16 status = 0;
      UINT16 type = 0;
      UINT32 flags = 0;
      UINT32 uniqueID = UTIL_INVALID_CS_UNIQUE_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      CHAR name[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {0};

      csMetaRecord &operator=(const csMetaRecord &o)
      {
         version = o.version;
         status = o.status;
         type = o.type;
         flags = o.flags;
         uniqueID = o.uniqueID;
         logicalID = o.logicalID;
         ossMemcpy(name, o.name, sizeof(name));
         return *this;
      }
   
      OSS_INLINE csMetaRecord(){}

      OSS_INLINE ~csMetaRecord(){}

      OSS_INLINE BOOLEAN isOnline()const
      {
         return CMR_STATUS_ONLINE == status;
      }

      BOOLEAN isValid()const;

      OSS_INLINE void reset()
      {
         version = 0;
         status = 0;
         type = 0;
         flags = 0;
         uniqueID = UTIL_INVALID_CS_UNIQUE_ID;
         logicalID = DMS_INVALID_LOGICCSID;
         ossMemset(name, 0, sizeof(name));
      }
   };//struct csMetaRecord
   const UINT32 CS_META_RECORD_LEN = sizeof(csMetaRecord);

#pragma pack()


   BOOLEAN initGmp(UINT32 pageSize,
                   PAGE_ID pid,
                   PAGE_ID lpid,
                   PAGE_SNAPSHOT_VERION psv,
                   void *buf);
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_SPACE_GLOBAL_PAGE_H_
