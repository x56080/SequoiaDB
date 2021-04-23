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

namespace engine
{
namespace vessel
{
   const UINT32 INALID_CMR_VERSION = 0;
   const UINT32 CMR_VERSION_1 = 1;

   const static UINT32 CMR_STATUS_CREATING = 0;
   const static UINT32 CMR_STATUS_ONLINE = 1;
   const static UINT32 CMR_STATUS_SNAPSHOT = 2;

   const static UINT64 CSGP_UPDATE_MASK_STATUS = 0x01;
   const static UINT64 CSGP_UPDATE_MASK_FLAGS = 0x02;
   const static UINT64 CSGP_UPDATE_MASK_MAX_CLLID = 0x04;
   const static UINT64 CSGP_UPDATE_MASK_NAME = 0x08;

#pragma pack(4)
   struct csMetaRecord
   {
      UINT32 version;
      UINT32 status;
      UINT32 flags;
      UINT32 uniqueID;
      UINT32 maxCLLogicalID;
      CHAR name[DMS_COLLECTION_SPACE_NAME_SZ + 1];

      csMetaRecord &operator=(const csMetaRecord &o)
      {
         version = o.version;
         status = o.status;
         flags = o.flags;
         uniqueID = o.uniqueID;
         maxCLLogicalID = o.maxCLLogicalID;
         ossMemcpy(name, o.name, sizeof(name));
         return *this;
      }
   
      OSS_INLINE csMetaRecord():
      version(INALID_CMR_VERSION),
      status(0),
      flags(0),
      uniqueID(UTIL_INVALID_CS_UNIQUE_ID),
      maxCLLogicalID(DMS_INVALID_LOGICCLID)
      {
         ossMemset(name, 0, sizeof(name));
      }

      OSS_INLINE ~csMetaRecord(){}

      OSS_INLINE BOOLEAN isOnline()const
      {
         return CMR_STATUS_ONLINE == status;
      }

      OSS_INLINE void reset()
      {
         version = INALID_CMR_VERSION;
         status = 0;
         flags = 0;
         uniqueID = UTIL_INVALID_CS_UNIQUE_ID;
         maxCLLogicalID = DMS_INVALID_LOGICCLID;
         ossMemset(name, 0, sizeof(name));
      }
   };//struct csMetaRecord
   const UINT32 CS_META_RECORD_LEN = sizeof(csMetaRecord);

#pragma pack()

   BOOLEAN metaRecordIsValid(const csMetaRecord &record);
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_SPACE_GLOBAL_PAGE_H_
