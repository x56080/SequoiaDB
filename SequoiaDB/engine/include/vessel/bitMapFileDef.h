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

   Source File Name = bitMapFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BIT_MAP_FILE_DEF_H_
#define VESSEL_BIT_MAP_FILE_DEF_H_

#include "vessel/extentDef.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   const static UINT32 INVALID_BIT_MAP_SMP_VERSION = 0;
   const static UINT32 BIT_MAP_SMP_VERSION = 1;
   struct bitMapFileSMPHead
   {
      UINT32 version;
      PAGE_ID pid;
      UINT32 flags;
      UINT32 capacity;
      UINT32 free;
      INT32 possibleFree;
      UINT64 lsn;
      UINT64 pad;

   };//struct bitMapFileSMEHead

   UINT32 getBitMapFileSMPCapacity()
   {
      return ((DMS_PAGE_SIZE4K - sizeof(bitMapFileSMPHead) - sizeof(UINT64)) & 0xfffffffc) * 8;
   }

   const static UINT16 INVALID_BIT_MAP_VERSION = 0;
   const static UINT16 BIT_MAP_VERSION = 1;

   struct bitMapPageHead
   {
      OSS_INLINE bitMapPageHead():
      version(INVALID_BIT_MAP_VERSION),
      mbid(INVALID_CL_MB_ID),
      cllid(DMS_INVALID_LOGICCLID),
      lastExtent(INVALID_PAGE_ID),
      w(0),
      count(0),
      lvl0Free(0),
      lvl1Free(0),
      lvl2Free(0),
      lvl3Free(0),
      pad(0)
      {}

      OSS_INLINE ~bitMapPageHead()
      {}

      OSS_INLINE BOOLEAN crashed(UINT32 w)const
      {
         return BIT_MAP_VERSION != version ||
                this->w != w; 
      }

      OSS_INLINE BOOLEAN valid()const
      {
         return BIT_MAP_VERSION == version &&
                INVALID_CL_MB_ID != mbid &&
                DMS_INVALID_LOGICCLID != cllid &&
                INVALID_PAGE_ID != lastExtent;
      }

      OSS_INLINE BOOLEAN isSame(CL_MB_ID mbid, UINT32 logicalID)const
      {
         return this->mbid == mbid &&
                this->cllid == logicalID;
      }

      UINT16 version;
      UINT16 mbid;
      UINT32 cllid;
      UINT32 lastExtent;
      UINT32 w;
      UINT16 count;
      UINT16 lvl0Free;
      UINT16 lvl1Free;
      UINT16 lvl2Free;
      UINT16 lvl3Free;
      UINT16 pad;

   };//struct bitMapPageHead

   const UINT32 BIT_MAP_LPID_SLOT_COUNT = 60;
   const UINT32 BIT_MAP_BITS_SLOT_COUNT = 15;
   

   static const UINT32 BIT_MAP_BIT_LVL0 = 0;
   static const UINT32 BIT_MAP_BIT_LVL1 = 1;
   static const UINT32 BIT_MAP_BIT_LVL2 = 2;
   static const UINT32 BIT_MAP_BIT_LVL3 = 3;
   static const UINT32 BIT_MAP_MAX_LEVEL = BIT_MAP_BIT_LVL3;

   UINT32 *getLvl0Bits(ossValuePtr ptr)
   {
      return ptr + sizeof(bitMapPageHead) +
             sizeof(UINT32) * BIT_MAP_LPID_SLOT_COUNT;
   }
   UINT32 *getLvl1Bits(ossValuePtr ptr)
   {
      return ptr + sizeof(bitMapPageHead) +
             sizeof(UINT32) * BIT_MAP_LPID_SLOT_COUNT +
             sizeof(UINT32) * BIT_MAP_BITS_SLOT_COUNT;
   }
   UINT32 *getLvl2Bits(ossValuePtr ptr)
   {
      return ptr + sizeof(bitMapPageHead) +
             sizeof(UINT32) * BIT_MAP_LPID_SLOT_COUNT +
             sizeof(UINT32) * BIT_MAP_BITS_SLOT_COUNT * 2;
   }
   UINT32 *getLvl3Bits(ossValuePtr ptr)
   {
      return ptr + sizeof(bitMapPageHead) +
             sizeof(UINT32) * BIT_MAP_LPID_SLOT_COUNT +
             sizeof(UINT32) * BIT_MAP_BITS_SLOT_COUNT * 3;
   }

/*
   struct bitMapPage
   {
      bitMapPageHead head;
      UINT32 lpids[BIT_MAP_LPID_SLOT_COUNT];
      UINT32 bits0[BIT_MAP_LPID_SLOT_COUNT * 8 / 32];
      UINT32 bits1[BIT_MAP_LPID_SLOT_COUNT * 8 / 32];
      UINT32 bits2[BIT_MAP_LPID_SLOT_COUNT * 8 / 32];
      UINT32 bits3[BIT_MAP_LPID_SLOT_COUNT * 8 / 32];
      UINT32 w;
   };//struct bitMapPage
   */

   const static UINT32 BIT_MAP_PAGE_SIZE = 512;
}//namespace vessel
}//namespace engine

#endif//VESSEL_BIT_MAP_FILE_DEF_H_