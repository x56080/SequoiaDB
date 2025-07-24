/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = clMetaBlockPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CL_META_BLOCK_PAGE_H_
#define VESSEL_CL_META_BLOCK_PAGE_H_

#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "vessel/pageDef.h"
#include "utilCompression.hpp"
#include "vessel/indexDef.h"
#include "dmsStripingId.hpp"
#include "vessel/objectBaseDef.h"

namespace engine
{
namespace vessel
{
   constexpr PAGE_ID CL_META_BLOCK_PAGE_MIN_LPID = 1;

   constexpr UINT32 CL_META_BLOCK_VERSION = 1;
   constexpr UINT32 CL_META_BLOCK_INVALID_VERSION = 0;
   constexpr UINT32 COLLECTION_ROUTE_PAGE_SLOT_COUNT = 4;
   constexpr UINT32 COLLECTION_ROOT_LVL0 = 0;
   constexpr UINT32 COLLECTION_FIRST_ROOT_LVL1 = 1;
   constexpr UINT32 COLLECTION_SECOND_ROOT_LVL1 = 2;
   constexpr UINT32 COLLECTION_ROOT_LVL2 = 3;
   constexpr UINT32 COLLECTION_MAX_ROUTE_ROOT = COLLECTION_ROOT_LVL2;
   constexpr UINT32 COLLECTION_MIN_ROUTE_ROOT = COLLECTION_ROOT_LVL0;

   constexpr UINT64 COLLECTION_UPDATE_MASK_ROUTE_PAGES = 0x01ull;


   
#pragma pack(4)
   struct clMetaBlock
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return CL_META_BLOCK_VERSION == version &&
                CL_TYPE_INVALID != type &&
                DMS_INVALID_LOGICCLID != logicalCLID &&
                INVALID_CL_MB_ID != mbID;
      }

      OSS_INLINE BOOLEAN isStripingMode()const
      {
         return DMS_INVALID_STRIPING_ID != minStriping;
      }

      void reset()
      {
         version = CL_META_BLOCK_INVALID_VERSION;
         type = CL_TYPE_INVALID;
         mbID = INVALID_CL_MB_ID;
         innerID = UTIL_UNIQUEID_NULL;
         logicalCLID = DMS_INVALID_LOGICCLID;
         flags = 0;
         minStriping = DMS_INVALID_STRIPING_ID;
         maxStriping = DMS_INVALID_STRIPING_ID;
         ossMemset(name, 0, sizeof(name));
         ossMemset(routePages, 0xFF, sizeof(routePages));
         compressionType = UTIL_COMPRESSOR_INVALID;
         minFreePercent = 0;
         pad = 0;
      }

      UINT32 version = 0;
      UINT16 type = CL_TYPE_INVALID;
      UINT16 mbID = INVALID_CL_MB_ID;
      UINT32 innerID = UTIL_UNIQUEID_NULL;
      UINT32 logicalCLID = DMS_INVALID_LOGICCLID;
      UINT32 flags = 0;
      UINT32 routePages[COLLECTION_ROUTE_PAGE_SLOT_COUNT] =
      {INVALID_PAGE_ID,INVALID_PAGE_ID, INVALID_PAGE_ID,INVALID_PAGE_ID};
      INT32 minStriping = DMS_INVALID_STRIPING_ID;
      INT32 maxStriping = DMS_INVALID_STRIPING_ID;
      CHAR name[DMS_COLLECTION_NAME_SZ + 1] = {};
      UINT8 compressionType = UTIL_COMPRESSOR_INVALID;
      UINT8 minFreePercent = 0;
      UINT16 pad = 0;
   };//class clMetaBlock
   constexpr UINT32 CL_META_BLOCK_LEN = sizeof(clMetaBlock);

   struct clMetaBlockOnDisk
   {
      clMetaBlockOnDisk()
      {
         ossMemset(pad, 0, sizeof(pad));
      }
      clMetaBlock block;
      CHAR pad[1024 - CL_META_BLOCK_LEN];
   };//class clMetaBlockOnDisk
   constexpr UINT32 CL_DISK_META_BLOCK_LEN = sizeof(clMetaBlockOnDisk);
   static_assert(1024 == CL_DISK_META_BLOCK_LEN, "invalid size");

   INT32 getCapacityOfCLMetaBlockPage(UINT32 pageSize, UINT32 &capacity);

   BOOLEAN initCLMetaBlockPage(UINT32 pageSize,
                               PAGE_ID pid,
                               PAGE_ID lpid,
                               PAGE_SNAPSHOT_VERION psv,
                               void *buf);

   BOOLEAN getCLMetaBlockIfValid(const void *ptr,
                                 UINT32 i,
                                 clMetaBlock &block);

   PAGE_ID getMbpLpidOfCollection(UINT32 pageSize, CL_MB_ID mbID);
#pragma pack()

}//namespace vessel
}//namespace engine

#endif//VESSEL_CL_META_BLOCK_PAGE_H_