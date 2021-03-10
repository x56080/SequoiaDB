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

   Source File Name = recordDataPage.h

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

#ifndef VESSEL_RECORD_DATA_PAGE_H_
#define VESSEL_RECORD_DATA_PAGE_H_

#include "ossTypes.hpp"
#include "utilCompression.hpp"
#include "vessel/vesselDef.h"
#include "vessel/vesselDef.h"

namespace engine
{
namespace vessel
{
   const UINT16 INVALID_RDP_VERSION = 0;
   const UINT16 RDP_VERSION_1 = 1;
   const UINT16 RDP_VERSION = RDP_VERSION_1;

   const UINT8 RDP_COMPRESSION_FLAGS_LZ4 = 0x01;
   const UINT8 RDP_INVALID_COMPRESSION_SLOT = 255;

#pragma pack(4)
   struct recordDataPageHead
   {
      OSS_INLINE recordDataPageHead():
      version(INVALID_RDP_VERSION),
      totalSlotCount(0),
      freeSlotCount(0),
      compressionFlags(0),
      compressionDicSlot(RDP_INVALID_COMPRESSION_SLOT),
      clLogcalID(DMS_INVALID_LOGICCLID),
      sequenceID(UINT32(-1)),
      totalFreeSpace(0),
      freeSpaceAfterLastSlot(0),
      flags(0),
      minStriping(INVALID_STRIPING_ID),
      maxStriping(INVALID_STRIPING_ID),
      transSN(0),
      pad0(0),
      pad1(0)
      {}

      OSS_INLINE ~recordDataPageHead(){}

      UINT16 version;
      UINT16 totalSlotCount;
      UINT16 freeSlotCount;
      UINT8  compressionFlags;
      UINT8  compressionDicSlot;
      UINT32 clLogcalID;
      UINT32 sequenceID;
      UINT32 totalFreeSpace;
      UINT32 freeSpaceAfterLastSlot;
      UINT32 flags;
      UINT16 minStriping;
      UINT16 maxStriping;
      UINT64 transSN;
      UINT32 pad0;
      UINT32 pad1;
   };//struct recordDataPageHead
   const static UINT32 RECORD_PAGE_HEAD_LEN = sizeof(recordDataPageHead);


   /// value range: [0x0, 0xF]
   const static UINT8 RDP_R_HEAD_TYPE_INVALID = 0x0;
   const static UINT8 RDP_R_HEAD_TYPE_NORMAL = 0x01;
   const static UINT8 RDP_R_HEAD_TYPE_BIG_RECORD_HEAD = 0x02;
   const static UINT8 RDP_R_HEAD_TYPE_BIG_RECORD_BODY = 0x03;
   const static UINT8 RDP_R_HEAD_TYPE_COMPRESSION_DIC = 0x04;

   const static UINT8 RDP_RSLOT_FLAG_SKIP_SCANNING = 0x10;

   struct recordSlot
   {
      OSS_INLINE recordSlot():
      flags(0),
      pad(0),
      offset(0){}

      OSS_INLINE ~recordSlot()
      {}

      OSS_INLINE recordSlot(const recordSlot &o):
      flags(o.flags),
      pad(o.pad),
      offset(o.offset)
      {}

      OSS_INLINE recordSlot &operator=(const recordSlot &o)
      {
         flags = o.flags;
         pad = o.pad;
         offset = o.offset;
         return *this;
      }

      OSS_INLINE UINT32 getOffset()const
      {
         return offset
      }
      OSS_INLINE BOOLEAN isFree()const
      {
         return 0 == flags && 0 == pad && 0 == offset;
      }
      OSS_INLINE BOOLEAN skipScanning()const
      {
         return isFree() || (flags & RDP_RSLOT_FLAG_SKIP_SCANNING);
      }
   
      /// lower 4bits: record head type
      /// upper 4bits: flags
      UINT8 flags;
      UINT8 pad;
      UINT16 offset;
   };//struct recordSlot
   const static UINT32 RDP_RSLOT_SIZE = sizeof(recordSlot);

   const UINT8 RDP_RECORD_FLAG_DEPENDENT = 0x01;
   const UINT8 RDP_RECORD_FLAG_TOMBSTONE = 0x02;
   const UINT8 RDP_RECORD_FLAG_OVERFLOW = 0x04;

   struct recordHead
   {
      OSS_INLINE recordHead():
      type(RDP_R_HEAD_TYPE_INVALID),
      pad(0),
      flags(0),
      compressionType(UTIL_COMPRESSOR_INVALID),
      transNode(DPS_INVALID_TRANSID_NODEID),
      transSN(DPS_INVALID_TRANSID_SN)
      {}

      OSS_INLINE ~recordHead()
      {}

      OSS_INLINE UINT8 geType()const
      {
         return UINT8(lenAndType >> 24);
      }
      OSS_INLINE UINT32 getLength()const
      {
         return lenAndType & 0x1FFFFFF;
      }
      OSS_INLINE BOOLEAN isDependtent()const
      {
         return OSS_BIT_TEST(flags, RDP_RECORD_FLAG_DEPENDENT);
      }
      OSS_INLINE BOOLEAN isTombstone()const
      {
         return OSS_BIT_TEST(flags, RDP_RECORD_FLAG_TOMBSTONE);
      }

      /// lower 24bit: len
      /// upper 8bit: type
      UINT8 type;
      UINT8 pad;
      UINT16 size;
      UINT8 flags;
      UINT8 compressionType;
      UINT16 transNode;
      UINT64 transSN;
   };//struct recordHead
   const static UINT32 RDP_RECORD_HEAD_LEN = sizeof(recordHead);

/*
   struct normalRecordHead
   {
      recordHead commonHead;
      /// commonHead.pad is transNode
      UINT64 transSN; 
   };//struct normalRecordHead
   const static UINT32 RDP_NORMAL_RECORD_HEAD_LEN = sizeof(normalRecordHead);

   struct overflowedRecordHead
   {
      recordHead commonHead;
      /// commonHead.pad is slot
      UINT32 pid;
   };//struct overflowedRecordHead
   const static UINT32 RDP_OVERFLOWED_RECORD_HEAD_LEN = sizeof(overflowedRecordHead);

   struct bigRecordHead
   {
      normalRecordHead normalHead;
      UINT32 originalLen;
      UINT32 compressedLen;
      UINT32 nextPid;
      UINT16 nextSlot;
      UINT16 totalSlice;
   };
   const static UINT32 RDP_BIG_RECORD_HEAD_HEAD_LEN = sizeof(bigRecordHead);

   struct bigRecordBodyHead
   {
      recordHead normalHead;
      UINT32 nextPid;
      UINT16 nextSlot;
      UINT16 sliceNo;
   };//struct bigRecordBodyHead
   const static UINT32 RDP_BIT_RECORD_BODY_HEAD_LEN = sizeof(bigRecordBodyHead);

   struct compressionDicRecordHead
   {
      recordHead commonHead;
   };//struct lz4DicRecordHead
   const static UINT32 RDP_DIC_RECORD_HEAD_LEN = sizeof(compressionDicRecordHead);
   */
#pragma pack()

   OSS_INLINE BOOLEAN isBigRecord(UINT32 pageSize, UINT32 recordSize)
   {
      return (ossAlign4(recordSize) +
              PAGE_HEAD_LEN +
              PAGE_TAIL_LEN +
              RDP_RECORD_HEAD_LEN +
              RDP_RSLOT_SIZE) > pageSize;

   }

}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_DATA_PAGE_H_