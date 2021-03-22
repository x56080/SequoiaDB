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
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   const UINT16 INVALID_RDP_VERSION = 0;
   const UINT16 RDP_VERSION_1 = 1;
   const UINT16 RDP_VERSION = RDP_VERSION_1;

#pragma pack(4)
   struct recordDataPageHead
   {
      OSS_INLINE recordDataPageHead():
      version(INVALID_RDP_VERSION),
      compressionDicSlot(INVALID_RECORD_SLOT_ID),
      clLogcalID(DMS_INVALID_LOGICCLID),
      sequenceID(0),
      totalSlotCount(0),
      freeSlotCount(0),
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

      OSS_INLINE recordDataPageHead &operator=(const recordDataPageHead &o)
      {
         version = o.version;
         compressionDicSlot = o.compressionDicSlot;
         clLogcalID = o.clLogcalID;
         sequenceID = o.sequenceID;
         totalSlotCount = o.totalSlotCount;
         freeSlotCount = o.freeSlotCount;
         totalFreeSpace = o.totalFreeSpace;
         freeSpaceAfterLastSlot = o.freeSpaceAfterLastSlot;
         flags = o.flags;
         minStriping = o.minStriping;
         maxStriping = o.maxStriping;
         transSN = o.transSN;
         pad0 = o.pad0;
         pad1 = o.pad1;
         return *this;
      }

      UINT16 version;
      UINT16 compressionDicSlot;
      UINT32 clLogcalID;
      UINT32 sequenceID;
      UINT16 totalSlotCount;
      UINT16 freeSlotCount;
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
      _flags(0),
      _pad(0),
      _offset(0){}

      OSS_INLINE ~recordSlot()
      {}

      OSS_INLINE recordSlot(const recordSlot &o):
      _flags(o._flags),
      _pad(o._pad),
      _offset(o._offset)
      {}

      OSS_INLINE recordSlot &operator=(const recordSlot &o)
      {
         _flags = o._flags;
         _pad = o._pad;
         _offset = o._offset;
         return *this;
      }

      OSS_INLINE void setType(UINT8 type)
      {
         _flags &= (0xF & type);
         return;
      }

      OSS_INLINE void setOffset(UINT16 offset)
      {
         _offset = offset;
      }

      OSS_INLINE UINT32 getOffset()const
      {
         return _offset;
      }
      OSS_INLINE BOOLEAN isFree()const
      {
         return 0 == _flags && 0 == _pad && 0 == _offset;
      }
      OSS_INLINE BOOLEAN skipScanning()const
      {
         return isFree() || (_flags & RDP_RSLOT_FLAG_SKIP_SCANNING);
      }

      OSS_INLINE void setSkipScanning()
      {
         OSS_BIT_SET(_flags, RDP_RSLOT_FLAG_SKIP_SCANNING);
      }
   
      private:
      /// lower 4bits: record head type
      /// upper 4bits: flags
      UINT8 _flags;
      UINT8 _pad;
      UINT16 _offset;
   };//struct recordSlot
   const static UINT32 RDP_RSLOT_SIZE = sizeof(recordSlot);

   const UINT8 RDP_RECORD_FLAG_DEPENDENT = 0x01;
   const UINT8 RDP_RECORD_FLAG_TOMBSTONE = 0x02;
   const UINT8 RDP_RECORD_FLAG_OVERFLOW = 0x04;

   struct recordHead
   {
      public:
      OSS_INLINE recordHead():
      size(0),
      type(RDP_R_HEAD_TYPE_INVALID),
      flags(0),
      compressionType(UTIL_COMPRESSOR_INVALID),
      pad(0),
      transNode(DPS_INVALID_TRANSID_NODEID),
      transSN(DPS_INVALID_TRANSID_SN)
      {}

      OSS_INLINE ~recordHead()
      {}

      OSS_INLINE UINT16 getSize()const
      {
         return size;
      }
      OSS_INLINE BOOLEAN isDependent()const
      {
         return OSS_BIT_TEST(flags, RDP_RECORD_FLAG_DEPENDENT);
      }
      OSS_INLINE BOOLEAN isTombstone()const
      {
         return OSS_BIT_TEST(flags, RDP_RECORD_FLAG_TOMBSTONE);
      }
      OSS_INLINE BOOLEAN isOverflow()const
      {
         return OSS_BIT_TEST(flags, RDP_RECORD_FLAG_OVERFLOW);
      }
      OSS_INLINE void setTypeAndFormat(UINT8 ht, UINT8 format)
      {
         type &= (ht & 0xF);
         type &= (format << 4);
      }
      OSS_INLINE UINT8 getType()const
      {
         return type & 0xF;
      }
      OSS_INLINE UINT8 getFormat()const
      {
         return type >> 4;
      }

      UINT16 size;
      /// lower 4bits: record head type
      /// upper 4bits: record format
      private:
      UINT8 type;
      public:
      UINT8 flags;
      UINT8 compressionType;
      UINT8 pad;
      UINT16 transNode;
      UINT64 transSN;
   };//struct recordHead
   const static UINT32 RDP_RECORD_HEAD_LEN = sizeof(recordHead);


   struct bigRecordHead
   {
      recordHead normalHead;
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
      UINT16 size;
      UINT8 type;
      UINT8 flags;
   };//struct lz4DicRecordHead
   const static UINT32 RDP_DIC_RECORD_HEAD_LEN = sizeof(compressionDicRecordHead);
   
#pragma pack()

   OSS_INLINE UINT32 getMaxFreeSizeOfRdp(UINT32 pageSize)
   {
      SDB_ASSERT(32768 == pageSize || 65536 == pageSize, "impossible");
      const static UINT32 len = PAGE_HEAD_LEN + PAGE_TAIL_LEN + RECORD_PAGE_HEAD_LEN;
      return pageSize - len;
   }

   OSS_INLINE UINT32 getMaxSizeOfRecordInRdp(UINT32 recordSize)
   {
      return RDP_RECORD_HEAD_LEN + RDP_RSLOT_SIZE + ossAlign4(recordSize);
   }

   OSS_INLINE UINT32 getMinSizeOfRecordInRdp()
   {
      return RDP_RECORD_HEAD_LEN + RDP_RSLOT_SIZE;
   }
   
   /// in fact, we may not allocate new slot when insert record.
   /// but ignore it here.
   OSS_INLINE BOOLEAN isBigRecordInRdp(UINT32 pageSize, UINT32 recordSize)
   {
      SDB_ASSERT(32768 == pageSize || 65536 == pageSize, "impossible");
      return getMaxSizeOfRecordInRdp(recordSize) > getMaxFreeSizeOfRdp(pageSize);
   }
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_DATA_PAGE_H_