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

   Source File Name = freeSpaceMapDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
   static const UINT32 FSM_SPACE_LVL_COUNT = 4;
   static const INT32 FSM_INVALID_SPACE_LVL = -1;
   static const INT32 FSM_MIN_SPACE_LVL = 0;
   static const INT32 FSM_MAX_SPACE_LVL = (FSM_MIN_SPACE_LVL + FSM_SPACE_LVL_COUNT - 1);
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
   const static UINT32 FSM_PAGE_HEAD_SIZE = sizeof(fsmPageHead);

   /// free space map file
   constexpr UINT32 FSM_FILE_PAGE_SIZE = 65536;
   static const UINT32 FSM_FILE_PAGE_COUNT_PER_SEG = 256;
   static const UINT32 FSM_FILE_MAX_SEG_COUNT = 1024;

   static const UINT32 FSM_FILE_PAGE_VERSION = 1;

   static const UINT32 FSM_FILE_PAGE_TYPE_BITMAP = 1;
   static const UINT32 FSM_FILE_PAGE_TYPE_BITMAP_OWNER = 2;

   static const UINT32 FSM_FILE_SMP_PID = 0;


   
   static const UINT32 FSM_BITMAP_BITS_COUNT = (FSM_FILE_PAGE_SIZE - FSM_PAGE_HEAD_SIZE) /
                                               sizeof(UINT64) / FSM_SPACE_LVL_COUNT;

   /// The count of data page can be managed by one bitmap page. 
   static const UINT32 FSM_BITMAP_PAGE_CAPACITY = FSM_BITMAP_BITS_COUNT * 64;

   /// total slot count in pm page
   static const UINT32 FSM_BITMAP_OWNER_PAGE_CAPAITY = (FSM_FILE_PAGE_SIZE - FSM_PAGE_HEAD_SIZE - sizeof(UINT32)) / 
                                                        sizeof(UINT32);

   struct fsmBitmapOwnerPage
   {
      fsmPageHead head;
      UINT32 flags = 0;
      UINT32 pages[FSM_BITMAP_OWNER_PAGE_CAPAITY];
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
   static const UINT32 FSM_CL_ENTRY_SIZE = sizeof(fsmCLEntry);

#pragma pack()

   static const UINT32 FSM_ENTRY_SLOT_COUNT = FSM_FILE_PAGE_SIZE / FSM_CL_ENTRY_SIZE;
   static const UINT32 FSM_ENTRY_PAGE_COUNT = 65536 / FSM_ENTRY_SLOT_COUNT;
   static const UINT32 FSM_FILE_RESERVED_PAGE_CNT = FSM_ENTRY_PAGE_COUNT + 1;

   OSS_INLINE BOOLEAN isValidFsmPageHead(const fsmPageHead &head)
   {
      return FSM_FILE_PAGE_VERSION == head.version &&
             DMS_INVALID_LOGICCLID != head.clLogicalId &&
             (FSM_FILE_PAGE_TYPE_BITMAP == head.type ||
              FSM_FILE_PAGE_TYPE_BITMAP_OWNER == head.type) &&
             FSM_FILE_RESERVED_PAGE_CNT <= head.pre &&
             FSM_FILE_RESERVED_PAGE_CNT <= head.next &&
             0 == head.pad;
   }

   OSS_INLINE BOOLEAN isValidFsmEntry(const fsmCLEntry &head)
   {
      return INVALID_PAGE_ID != head.root &&
             FSM_FILE_RESERVED_PAGE_CNT <= head.root &&
             DMS_INVALID_LOGICCLID != head.logicalID;
   }
;
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FREE_SPACE_MAP_DEF_H_