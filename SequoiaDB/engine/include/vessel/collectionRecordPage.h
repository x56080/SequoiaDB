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
#include "vessel/compressionDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(2)

   const UINT16 COLLECTION_RECORD_VERSION = 1;
   const UINT16 COLLECTION_RECORD_INVALID_VERSION = 0;
   
   struct collectionRecord
   {
      collectionRecord():
      version(COLLECTION_RECORD_INVALID_VERSION),
      type(INVALID_COLLECTION_TYPE),
      logicalCSID(DMS_INVALID_LOGICCSID),
      logicalCLID(DMS_INVALID_LOGICCLID),
      mbID(INVALID_CL_MB_ID),
      flags(0),
      nextPageSequence(0),
      firstLvl0Page(INVALID_PAGE_ID),
      maxStripingGroup(0),
      compressionType(CL_COMPRESSION_TYPE_NONE),
      compressionAlgrithm(CL_COMPRESSION_ALGRITHM_LZW),
      compressionDic(INVALID_PAGE_ID),
      nonUniqueIndexCount(0),
      uniqueIndexCount(0),
      nextIndexID(0)
      {
         ossMemset(name, 0, sizeof(name));
         ossMemset(indexSlots, 0xff, sizeof(indexSlots));
      }

      void reset()
      {
         version = COLLECTION_RECORD_INVALID_VERSION;
         type = INVALID_COLLECTION_TYPE;
         logicalCSID = DMS_INVALID_LOGICCSID;
         logicalCLID = DMS_INVALID_LOGICCLID;
         mbID = INVALID_CL_MB_ID;
         flags = 0;
         nextPageSequence = 0;
         firstLvl0Page = INVALID_PAGE_ID;
         maxStripingGroup = 0;
         compressionType = CL_COMPRESSION_TYPE_NONE;
         compressionAlgrithm = CL_COMPRESSION_ALGRITHM_LZW;
         compressionDic = INVALID_PAGE_ID;
         nonUniqueIndexCount = 0;
         uniqueIndexCount = 0;
         nextIndexID = 0;
         ossMemset(name, 0, sizeof(name));
         ossMemset(indexSlots, 0xff, sizeof(indexSlots));
      }

      UINT16 version;
      UINT16 type;
      UINT32 logicalCSID;
      UINT32 logicalCLID;
      CL_MB_ID mbID;
      CHAR name[DMS_COLLECTION_NAME_SZ + 1];
      UINT32 flags;
      UINT32 nextPageSequence;
      PAGE_ID firstLvl0Page;
      UINT32 maxStripingGroup;

      /// compression begin
      UINT8 compressionType;
      UINT8 compressionAlgrithm;
      PAGE_ID compressionDic;
      /// compression end

      /// index begin
      UINT8 nonUniqueIndexCount;
      UINT8 uniqueIndexCount;
      UINT32 nextIndexID;
      PAGE_ID indexSlots[DMS_COLLECTION_MAX_INDEX];
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
#pragma pack()

   struct collectionRecordPageHead
   {
      UINT32 version;
      UINT32 minMBID;
      UINT32 capacity;
      UINT64 bitmap;
      UINT64 pad;
   };//struct collectionPageHead
   const UINT32 COLLECTION_RECORD_PAGE_HEAD_LEN = sizeof(collectionRecordPageHead);

   const UINT32 COLLECTION_RECORD_PAGE_INVALID_VERSION = 0;
   const UINT32 COLLECTION_RECORD_PAGE_VERSION_1 = 1;
   INT32 getCapacityOfCLRecordPage(UINT32 pageSize, UINT32 &capacity);

}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_RECORD_H_