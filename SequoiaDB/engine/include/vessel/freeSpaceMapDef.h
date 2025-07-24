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

   Source File Name = freeSpaceMapDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_VESSEL_FREE_SPACE_MAP_DEF_H_
#define SDB_VESSEL_FREE_SPACE_MAP_DEF_H_

#include "vessel/pageDef.h"
#include "pdTrace.hpp"
#include "vessel/recordDataPage.h"
#include "vessel/forwardList.hpp"
#include "ossSpinLatch.hpp"

namespace engine
{
namespace vessel
{
   /// free space map
   /// size range of lvl0:  (0, x]
   /// size range of lvl1: (x, 2x]
   /// ...
   /// size range of lvl max: (n * x, page size]
   constexpr UINT32 FSM_SPACE_LVL_COUNT = 4;
   constexpr INT32 FSM_INVALID_SPACE_LVL = -1;
   constexpr INT32 FSM_MIN_SPACE_LVL = 0;
   constexpr INT32 FSM_MAX_SPACE_LVL = (FSM_MIN_SPACE_LVL + FSM_SPACE_LVL_COUNT - 1);
   constexpr INT32 FSM_SPACE_LVL_2 = 2;
   OSS_INLINE BOOLEAN isValidFsmLvL(INT32 lvl)
   {
      return FSM_MIN_SPACE_LVL <= lvl &&
             lvl <= FSM_MAX_SPACE_LVL;
   }

   /// return space lvl, not rc code.
   INT32 getFsmSpaceLvl(UINT32 pageSize, UINT32 size);

   /// Downgrade lvl if delta size lower than factor.
   INT32 getAdjustedFsmSpaceLvl(UINT32 pageSize,
                                UINT32 size,
                                UINT32 factor = 512);

#pragma pack(4)

   struct fsmPageHead
   {
      UINT16 version = 0;
      UINT16 type = 0;
      UINT32 clLogicalId = DMS_INVALID_LOGICCLID;
      UINT32 flags = 0;
      UINT32 pre = INVALID_PAGE_ID;
      UINT32 next = INVALID_PAGE_ID;
      UINT64 pad = 0;
   };//struct fsmPageHead
   constexpr UINT32 FSM_PAGE_HEAD_SIZE = sizeof(fsmPageHead);
   
   /// free space map file
   constexpr UINT32 FSM_FILE_PAGE_SIZE = 65536;
   constexpr UINT32 FSM_FILE_PAGE_COUNT_PER_SEG = 64;
   constexpr UINT32 FSM_FILE_MAX_SEG_COUNT = 4096;
   
   constexpr UINT32 FSM_FILE_PAGE_VERSION = 1;

   constexpr UINT32 FSM_FILE_PAGE_TYPE_BITMAP = 1;
   constexpr UINT32 FSM_FILE_PAGE_TYPE_BITMAP_OWNER = 2;


   
   constexpr UINT32 FSM_BITMAP_BITS_COUNT = (FSM_FILE_PAGE_SIZE - FSM_PAGE_HEAD_SIZE) /
                                               sizeof(UINT64) / FSM_SPACE_LVL_COUNT;

   /// The count of data page can be managed by one bitmap page. 
   constexpr UINT32 FSM_BITMAP_PAGE_CAPACITY = FSM_BITMAP_BITS_COUNT * 64;

   /// total slot count in pm page
   constexpr UINT32 FSM_BITMAP_OWNER_PAGE_CAPACITY = (FSM_FILE_PAGE_SIZE - FSM_PAGE_HEAD_SIZE - sizeof(UINT32)) / 
                                                        sizeof(UINT32);

   struct fsmBitmapOwnerPage
   {
      fsmPageHead head;
      UINT32 flags = 0;
      UINT32 pages[FSM_BITMAP_OWNER_PAGE_CAPACITY];
   };//struct fsmPMapPage
   constexpr UINT32 FSM_BITMAP_OWNER_PAGE_SIZE = sizeof(fsmBitmapOwnerPage);
   static_assert(FSM_BITMAP_OWNER_PAGE_SIZE == FSM_FILE_PAGE_SIZE, "invalid page size");


   struct fsmBitmapPage
   {
      fsmPageHead head;
      UINT32 flags = 0;
      UINT64 lvlBitmaps[FSM_SPACE_LVL_COUNT][FSM_BITMAP_BITS_COUNT];
   };//struct fsmBitMapPage
   constexpr UINT32 FSM_BITMAP_PAGE_SIZE = sizeof(fsmBitmapPage);
   static_assert(FSM_BITMAP_PAGE_SIZE == FSM_FILE_PAGE_SIZE, "invalid page size");

   struct fsmCLEntry
   {
      OSS_INLINE fsmCLEntry(){}

      OSS_INLINE ~fsmCLEntry()
      {}

      OSS_INLINE fsmCLEntry &operator=(const fsmCLEntry &o)
      {
         root = o.root;
         logicalID = o.logicalID;
         return *this;
      }

      UINT32 root = INVALID_PAGE_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCLID;
   };//struct fsmCLEntry
   constexpr UINT32 FSM_CL_ENTRY_SIZE = sizeof(fsmCLEntry);

   //reserved area
   constexpr UINT32 FSM_FILE_SME_CAPACITY = FSM_FILE_PAGE_COUNT_PER_SEG * FSM_FILE_MAX_SEG_COUNT;  //256K
   constexpr UINT32 FSM_FILE_SME_USED_SIZE = FSM_FILE_SME_CAPACITY >> 3;  //32KB
   constexpr UINT32 FSM_FILE_SME_ALIGNED_SIZE = FSM_FILE_PAGE_SIZE * ((FSM_FILE_SME_USED_SIZE-1) / FSM_FILE_PAGE_SIZE + 1);  //64KB, reserved area size must be aligned to page size
   constexpr UINT32 FSM_FILE_ENTRY_ARRAY_SIZE = 65536 * FSM_CL_ENTRY_SIZE;   //512KB
   constexpr UINT32 FSM_FILE_ENTRY_BLOCK_SIZE = FSM_FILE_PAGE_SIZE;
   constexpr UINT32 FSM_FILE_ENTRY_BLOCK_CAPACITY = FSM_FILE_ENTRY_BLOCK_SIZE / FSM_CL_ENTRY_SIZE;
   constexpr UINT32 FSM_FILE_RESERVED_AREA_SIZE = FSM_FILE_SME_ALIGNED_SIZE + FSM_FILE_ENTRY_ARRAY_SIZE;   //576KB

#pragma pack()
   OSS_INLINE BOOLEAN isValidFsmPageHead(const fsmPageHead &head)
   {
      return FSM_FILE_PAGE_VERSION == head.version &&
             DMS_INVALID_LOGICCLID != head.clLogicalId &&
             (FSM_FILE_PAGE_TYPE_BITMAP == head.type ||
              FSM_FILE_PAGE_TYPE_BITMAP_OWNER == head.type) &&
             0 == head.pad;
   }

   OSS_INLINE BOOLEAN isValidFsmEntry(const fsmCLEntry &head)
   {
      return INVALID_PAGE_ID != head.root &&
             DMS_INVALID_LOGICCLID != head.logicalID;
   }
;
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FREE_SPACE_MAP_DEF_H_