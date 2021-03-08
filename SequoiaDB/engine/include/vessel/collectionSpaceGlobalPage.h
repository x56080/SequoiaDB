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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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

#include "vessel/extentDef.h"

namespace engine
{
namespace vessel
{
   const UINT32 INALID_CMR_VERSION = 0;
   const UINT32 CMR_VERSION_1 = 1;
   const UINT32 CMR_STATUS_ONLINE = 0x01;
   const UINT32 CMR_FLAG_IS_SYS = 0x01;

   const UINT32 CS_META_RECORD_ON_DISK_LEN = 1024;
   const PAGE_ID CS_GLOBAL_META_PAGE_ID = 1;

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
      uniqueID(UTIL_INVLIAD_CS_UNIQUE_ID),
      maxCLLogicalID(DMS_INVALID_LOGICCLID)
      {
         ossMemset(name, 0, sizeof(name));
      }

      OSS_INLINE void setOnline()
      {
         status = CMR_STATUS_ONLINE;
      }

      OSS_INLINE BOOLEAN isOnline()const
      {
         return CMR_STATUS_ONLINE == status;
      }

      OSS_INLINE void reset()
      {
         version = INALID_CMR_VERSION;
         status = 0;
         flags = 0;
         uniqueID = UTIL_INVLIAD_CS_UNIQUE_ID;
         maxCLLogicalID = DMS_INVALID_LOGICCLID;
         ossMemset(name, 0, sizeof(name));
      }
   };//struct csMetaRecord
   const UINT32 CS_META_RECORD_LEN = sizeof(csMetaRecord);

   struct csMetaRecordOnDisk
   {
      csMetaRecordOnDisk()
      {
         ossMemset(pad, 0, sizeof(pad));
      }
      csMetaRecord record;
      CHAR pad[CS_META_RECORD_ON_DISK_LEN-CS_META_RECORD_LEN];
   }; //struct csMetaRecordOnDisk

   BOOLEAN metaRecordIsValid(const csMetaRecord &record);
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_SPACE_GLOBAL_PAGE_H_