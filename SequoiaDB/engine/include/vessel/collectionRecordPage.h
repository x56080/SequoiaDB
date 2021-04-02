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

#ifndef VESSEL_COLLECTION_RECORD_PAGE_H_
#define VESSEL_COLLECTION_RECORD_PAGE_H_

#include "vessel/vesselDef.h"
#include "vessel/collectionDef.h"
#include "dms.hpp"
#include "vessel/extentDef.h"
#include "utilCompression.hpp"

namespace engine
{
namespace vessel
{
   const static UINT16 COLLECTION_RECORD_VERSION = 1;
   const static UINT16 COLLECTION_RECORD_INVALID_VERSION = 0;
   const static UINT32 COLLECTION_ROUTE_PAGE_SLOT_COUNT = 4;
   const static UINT32 COLLECTION_ROOT_LVL0 = 0;
   const static UINT32 COLLECTION_FIRST_ROOT_LVL1 = 1;
   const static UINT32 COLLECTION_SECOND_ROOT_LVL1 = 2;
   const static UINT32 COLLECTION_ROOT_LVL2 = 3;
   const static UINT32 COLLECTION_MAX_ROUTE_ROOT = COLLECTION_ROOT_LVL2;
   const static UINT32 COLLECTION_MIN_ROUTE_ROOT = COLLECTION_ROOT_LVL0;

   const static UINT64 COLLECTION_UPDATE_MASK_COMPRESSTYPE = 0x01;
   const static UINT64 COLLECTION_UPDATE_MASK_FLAGS = 0x02;
   const static UINT64 COLLECTION_UPDATE_MASK_NAME = 0x04;
   const static UINT64 COLLECTION_UPDATE_MASK_ROUTE_PAGES = 0x08;
   const static UINT64 COLLECTION_UPDATE_MASK_COMPRESSION_DIC = 0x010;
   const static UINT64 COLLECTION_UPDATE_MASK_INDEX = 0x20;
   
#pragma pack(4)
   struct collectionRecord
   {
      collectionRecord():
      version(COLLECTION_RECORD_INVALID_VERSION),
      type(INVALID_COLLECTION_TYPE),
      innerID(UTIL_INVALID_CL_INNER_ID),
      logicalCLID(DMS_INVALID_LOGICCLID),
      mbID(INVALID_CL_MB_ID),
      maxSGCount(0),
      compressionType(UTIL_COMPRESSOR_INVALID),
      flags(0),
      compressionDic(INVALID_PAGE_ID),
      nonUniqueIndexCount(0),
      uniqueIndexCount(0),
      indexPad(0),
      nextIndexID(0),
      indexSlots(0)
      {
         ossMemset(name, 0, sizeof(name));
         ossMemset(routePages, 0xFF, sizeof(routePages));
      }

      OSS_INLINE collectionRecord &operator=(const collectionRecord &o)
      {
         version = o.version;
         type = o.type;
         innerID = o.innerID;
         logicalCLID = o.logicalCLID;
         mbID = o.mbID;
         maxSGCount = o.maxSGCount;
         flags = o.flags;
         compressionType = o.compressionType;
         compressionDic = o.compressionDic;
         nonUniqueIndexCount = o.nonUniqueIndexCount;
         uniqueIndexCount = o.uniqueIndexCount;
         indexPad = o.indexPad;
         nextIndexID = o.nextIndexID;
         indexSlots = o.indexSlots;
         ossMemcpy(name, o.name, sizeof(name));
         ossMemcpy(routePages, o.routePages, sizeof(routePages));
         return *this;
      }

      void reset()
      {
         version = COLLECTION_RECORD_INVALID_VERSION;
         type = INVALID_COLLECTION_TYPE;
         innerID = UTIL_INVALID_CL_INNER_ID;
         logicalCLID = DMS_INVALID_LOGICCLID;
         mbID = INVALID_CL_MB_ID;
         maxSGCount = 0;
         flags = 0;
         compressionType = UTIL_COMPRESSOR_INVALID;
         compressionDic = INVALID_PAGE_ID;
         nonUniqueIndexCount = 0;
         uniqueIndexCount = 0;
         indexPad = 0;
         nextIndexID = 0;
         ossMemset(name, 0, sizeof(name));
         ossMemset(routePages, 0xFF, sizeof(routePages));
         indexSlots = 0;
      }

      UINT16 version;
      UINT16 type;
      UINT32 innerID;
      UINT32 logicalCLID;
      UINT16 mbID;
      UINT8 maxSGCount;
      UINT8 compressionType;
      UINT32 flags;

      CHAR name[DMS_COLLECTION_NAME_SZ + 1];

      UINT32 routePages[COLLECTION_ROUTE_PAGE_SLOT_COUNT];

      UINT32 compressionDic;

      /// index begin
      UINT8 nonUniqueIndexCount;
      UINT8 uniqueIndexCount;
      UINT16 indexPad;
      UINT32 nextIndexID;
      UINT64 indexSlots;
      
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

   struct collectionRecordPageHead
   {
      UINT16 version;
      UINT16 flags;
      CHAR pad[12];
   };//struct collectionPageHead
   const UINT32 COLLECTION_RECORD_PAGE_HEAD_LEN = sizeof(collectionRecordPageHead);

   const UINT16 COLLECTION_RECORD_PAGE_INVALID_VERSION = 0;
   const UINT16 COLLECTION_RECORD_PAGE_VERSION_1 = 1;
   INT32 getCapacityOfCLRecordPage(UINT32 pageSize, UINT32 &capacity);
#pragma pack()

}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_RECORD_H_