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

   Source File Name = collectionRecordPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_RECORD_PAGE_H_
#define VESSEL_COLLECTION_RECORD_PAGE_H_

#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "vessel/pageDef.h"
#include "utilCompression.hpp"
#include "vessel/indexDef.h"

namespace engine
{
namespace vessel
{
   static constexpr PAGE_ID COLLECTION_RECORD_PAGE_MIN_LPID = 1;

   const static UINT16 COLLECTION_RECORD_VERSION = 1;
   const static UINT16 COLLECTION_RECORD_INVALID_VERSION = 0;
   const static UINT32 COLLECTION_ROUTE_PAGE_SLOT_COUNT = 4;
   const static UINT32 COLLECTION_ROOT_LVL0 = 0;
   const static UINT32 COLLECTION_FIRST_ROOT_LVL1 = 1;
   const static UINT32 COLLECTION_SECOND_ROOT_LVL1 = 2;
   const static UINT32 COLLECTION_ROOT_LVL2 = 3;
   const static UINT32 COLLECTION_MAX_ROUTE_ROOT = COLLECTION_ROOT_LVL2;
   const static UINT32 COLLECTION_MIN_ROUTE_ROOT = COLLECTION_ROOT_LVL0;

   const static UINT64 COLLECTION_UPDATE_MASK_ROUTE_PAGES = 0x01ull;

   enum COLLECTION_TYPE
   {
      COLLECTION_TYPE_NORMAL = 0,
      COLLECTION_TYPE_INVALID = 65535,
   };//enum COLLECTION_TYPE
   
#pragma pack(4)
   struct collectionRecord
   {
      collectionRecord()
      {
      }

      OSS_INLINE collectionRecord &operator=(const collectionRecord &o)
      {
         ossMemcpy(this, &o, sizeof(collectionRecord));
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return COLLECTION_RECORD_VERSION == version &&
                COLLECTION_TYPE_INVALID != type &&
                DMS_INVALID_LOGICCLID != logicalCLID &&
                INVALID_CL_MB_ID != mbID;
      }

      OSS_INLINE BOOLEAN isStripingMode()const
      {
         return INVALID_STRIPING_ID != minStriping;
      }

      void reset()
      {
         version = COLLECTION_RECORD_INVALID_VERSION;
         type = COLLECTION_TYPE_INVALID;
         innerID = UTIL_INVALID_CL_INNER_ID;
         logicalCLID = DMS_INVALID_LOGICCLID;
         mbID = INVALID_CL_MB_ID;
         pad = 0;
         flags = 0;
         freeSizeReserved = 0;
         minStriping = INVALID_STRIPING_ID;
         maxStriping = INVALID_STRIPING_ID;
         compressionType = UTIL_COMPRESSOR_INVALID;
         compressionDic = INVALID_PAGE_ID;
         uniqueIndexes = 0;
         nonUniqueIndexes = 0;
         nextIndexId = 0;
         ossMemset(name, 0, sizeof(name));
         ossMemset(routePages, 0xFF, sizeof(routePages));
      }

      UINT16 version = 0;
      UINT16 type = COLLECTION_TYPE_INVALID;
      UINT32 innerID = UTIL_INVALID_CL_INNER_ID;
      UINT32 logicalCLID = DMS_INVALID_LOGICCLID;
      UINT16 mbID = INVALID_CL_MB_ID;
      UINT8 compressionType = UTIL_COMPRESSOR_INVALID;
      UINT8 pad = 0;
      UINT32 flags = 0;
      UINT32 freeSizeReserved = 0;
      UINT16 minStriping = INVALID_STRIPING_ID;
      UINT16 maxStriping = INVALID_STRIPING_ID;

      CHAR name[DMS_COLLECTION_NAME_SZ + 1] = {0};

      UINT32 routePages[COLLECTION_ROUTE_PAGE_SLOT_COUNT] = {0xFF};

      UINT32 compressionDic = INVALID_PAGE_ID;

      /// index begin
      UINT64 uniqueIndexes = 0;
      UINT64 nonUniqueIndexes = 0;
      UINT32 nextIndexId = 0;
      
      /// index end
   };//class collectionRecord
   const UINT32 COLLECTION_RECORD_LEN = sizeof(collectionRecord);

   struct collectionRecordOnDisk
   {
      collectionRecordOnDisk()
      {
         ossMemset(pad, 0, sizeof(pad));
      }
      collectionRecord record;
      CHAR pad[1024 - COLLECTION_RECORD_LEN];
   };//class collectionRecordOnDisk
   const UINT32 COLLECTION_DISK_RECORD_LEN = sizeof(collectionRecordOnDisk);

   INT32 getCapacityOfCLRecordPage(UINT32 pageSize, UINT32 &capacity);

   BOOLEAN initCollectionRecordPage(UINT32 pageSize,
                                    PAGE_ID pid,
                                    PAGE_ID lpid,
                                    PAGE_SNAPSHOT_VERION psv,
                                    void *buf);

   BOOLEAN getCollectionRecordIfValid(const void *ptr,
                                      UINT32 i,
                                      collectionRecord &record);

   PAGE_ID getCrpLpidOfCollection(UINT32 pageSize, CL_MB_ID mbID);
#pragma pack()

}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_RECORD_H_