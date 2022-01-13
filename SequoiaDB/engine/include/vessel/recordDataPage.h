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
#include "vessel/recordID.h"
#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "dmsStripingId.hpp"
#include "vessel/pageDef.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 INVALID_RDP_VERSION = 0;
   constexpr UINT32 RDP_VERSION_1 = 1;
   constexpr UINT32 RDP_VERSION = RDP_VERSION_1;

#pragma pack(4)
   struct recordDataPageHead
   {
      recordDataPageHead(){}

      ~recordDataPageHead(){}

      recordDataPageHead &operator=(const recordDataPageHead &o)
      {
         ossMemcpy(this, &o, sizeof(recordDataPageHead));
         return *this;
      }

      OSS_INLINE BOOLEAN hasValidStripingRange()const
      {
         return dmsStripingRange(minStriping, maxStriping).isValid();
      }

      UINT32 version = INVALID_RDP_VERSION;
      UINT32 flags = 0;
      UINT32 clLogcalID = DMS_INVALID_LOGICCLID;
      UINT32 pageSeq = INVALID_CL_PAGE_SEQ;
      UINT16 totalSlotCount = 0;
      UINT16 totalFreeSpace = 0;
      UINT16 backOffset = 0;
      INT16 firstFreeSlot = INVALID_RECORD_SLOT_POS;
      INT32 minStriping = DMS_INVALID_STRIPING_ID;
      INT32 maxStriping = DMS_INVALID_STRIPING_ID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
      UINT16 outerRecordCount = 0;
      CHAR pad[22] = {};
   };//struct recordDataPageHead
   constexpr UINT32 RECORD_PAGE_HEAD_SIZE = sizeof(recordDataPageHead);

   constexpr UINT16 RDP_SLOT_FLAG_IN_USED = 0x01;
   constexpr UINT16 RDP_SLOT_FLAG_INVISIBLE = 0x02;

   constexpr UINT8 RDP_RECORD_HEAD_TYPE_INVALID = 0;
   constexpr UINT8 RDP_RECORD_HEAD_TYPE_NORMAL = 1;
   constexpr UINT8 RDP_RECORD_HEAD_OVERFLOW = 2;
   constexpr UINT8 RDP_RECORD_HEAD_TOMBSTONE = 3;
   constexpr UINT8 RDP_RECORD_HEAD_BIG_RECORD_ENTRY = 4;
   constexpr UINT8 RDP_RECORD_HEAD_BIG_RECORD_BODY = 5;

   struct recordSlot
   {
      OSS_INLINE recordSlot(){}
      OSS_INLINE ~recordSlot(){}

      OSS_INLINE recordSlot(const recordSlot &o):
      flags(o.flags),
      type(o.type),
      reserved(o.reserved),
      offset(o.offset),
      size(o.size)
      {}

      OSS_INLINE recordSlot &operator=(const recordSlot &o)
      {
         flags = o.flags;
         type = o.type;
         reserved = o.reserved;
         offset = o.offset;
         size = o.size;
         return *this;
      }

      OSS_INLINE void reset()
      {
         *((UINT64 *)this) = 0;
         return;
      }

      OSS_INLINE void init(UINT8 type,
                           UINT8 reserved,
                           UINT16 offset,
                           UINT16 size)
      {
         reset();
         OSS_BIT_SET(flags, RDP_SLOT_FLAG_IN_USED);
         this->type = type;
         this->reserved = reserved;
         this->offset = offset;
         this->size = size;
         return;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return 0 != OSS_BIT_TEST(flags, RDP_SLOT_FLAG_IN_USED) &&
                RDP_RECORD_HEAD_TYPE_INVALID != type &&
                0 != offset &&
                0 != size;
      }
      OSS_INLINE BOOLEAN isValidAndVisible()const
      {
         return isValid() && !isInvisible();
      }
      OSS_INLINE BOOLEAN isInvisible()const
      {
         return 0 != OSS_BIT_TEST(flags, RDP_SLOT_FLAG_INVISIBLE);
      }
      OSS_INLINE void setInvisible()
      {
         OSS_BIT_SET(flags, RDP_SLOT_FLAG_INVISIBLE);
      }
      OSS_INLINE BOOLEAN isNormalRecord()const
      {
         return type == RDP_RECORD_HEAD_TYPE_NORMAL;
      }
      OSS_INLINE BOOLEAN isTombstoneRecord()const
      {
         return type == RDP_RECORD_HEAD_TOMBSTONE;
      }
      OSS_INLINE BOOLEAN isOverflowedRecord()const
      {
         return type == RDP_RECORD_HEAD_OVERFLOW;
      }

      OSS_INLINE UINT32 getMaxSpaceSize()const
      {
         return (UINT32)size + (UINT32)reserved;
      }
   
      static constexpr UINT32 getMaxReservedSize()
      {
         return 128;
      }
   
      public:
         UINT16 flags = 0;
         UINT8 type = RDP_RECORD_HEAD_TYPE_INVALID;
         UINT8 reserved = 0;
         UINT16 offset = 0;
         UINT16 size = 0;
   };//struct recordSlot
   constexpr UINT32 RDP_RSLOT_SIZE = sizeof(recordSlot);
   static_assert(sizeof(UINT64) == RDP_RSLOT_SIZE, "invalid size");

   struct normalRecordHead
   {
      normalRecordHead(){}
      ~normalRecordHead(){}
      normalRecordHead(const normalRecordHead &o):
      flags(o.flags),
      compressionType(o.compressionType),
      transNode(o.transNode),
      transSN(o.transSN){}
      normalRecordHead &operator=(const normalRecordHead &o)
      {
         flags = o.flags;
         compressionType = o.compressionType;
         transNode = o.transNode;
         transSN = o.transSN;
         return *this;
      }

      OSS_INLINE void setTransID(const DPS_TRANS_ID &transID)
      {
         transNode = transID.getNodeID();
         transSN = transID.getSN();
         return;
      }
      OSS_INLINE BOOLEAN isCompressed()const
      {
         return UTIL_COMPRESSOR_INVALID != compressionType;
      }

      UINT8 flags = 0;
      UINT8 compressionType = UTIL_COMPRESSOR_INVALID;
      UINT16 transNode = DPS_INVALID_TRANSID_NODEID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
   };//struct normalRecordHead
   constexpr UINT32 NORMAL_RECORD_HEAD_SIZE = sizeof(normalRecordHead);
   static_assert(ossIsAligned4(NORMAL_RECORD_HEAD_SIZE), "invalid size");

   struct overflowedRecord
   {
      overflowedRecord(){}
      ~overflowedRecord(){}

      overflowedRecord(const overflowedRecord &o):
      flags(o.flags),
      pos(o.pos),
      lpid(o.lpid),
      pad(o.pad){}

      overflowedRecord &operator=(const overflowedRecord &o)
      {
         flags = o.flags;
         pos = o.pos;
         lpid = o.lpid;
         pad = o.pad;
         return *this;
      }

      private:
      static constexpr UINT16 FLAG_BIG_RECORD = 0x01;

      public:
      OSS_INLINE void setAsBigRecord()
      {
         OSS_BIT_SET(flags, overflowedRecord::FLAG_BIG_RECORD);
      }
      OSS_INLINE BOOLEAN isBigRecord()const
      {
         return 0 != OSS_BIT_TEST(flags, overflowedRecord::FLAG_BIG_RECORD);
      }
      OSS_INLINE recordID getOverflowAddr()const
      {
         return recordID(lpid, pos);
      }

      UINT16 flags = 0;
      INT16 pos = INVALID_RECORD_SLOT_POS;
      UINT32 lpid = INVALID_PAGE_ID;
      UINT32 pad = 0;
   };//struct overflowedRecord
   constexpr UINT32 OVERFLOWED_RECORD_SIZE = sizeof(overflowedRecord);
   static_assert(OVERFLOWED_RECORD_SIZE == NORMAL_RECORD_HEAD_SIZE, "invalid size");

   struct tombstoneRecord
   {
      tombstoneRecord(){}
      ~tombstoneRecord(){}
      tombstoneRecord(const tombstoneRecord &o):
      flags(o.flags),
      transNode(o.transNode),
      transSN(o.transSN){}

      tombstoneRecord &operator=(const tombstoneRecord &o)
      {
         flags = o.flags;
         transNode = o.transNode;
         transSN = o.transSN;
         return *this;
      }

      OSS_INLINE void setTransID(const DPS_TRANS_ID &transID)
      {
         transNode = transID.getNodeID();
         transSN = transID.getSN();
         return;
      }

      UINT16 flags = 0;
      UINT16 transNode = DPS_INVALID_TRANSID_NODEID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
   };//struct tombstoneRecord
   constexpr UINT32 TOMBSTONE_RECORD_SIZE = sizeof(tombstoneRecord);
   static_assert(TOMBSTONE_RECORD_SIZE == NORMAL_RECORD_HEAD_SIZE, "invalid size");
   
   
#pragma pack()

   /// in fact, we may not allocate new slot when insert record.
   /// but ignore it here.
   OSS_INLINE constexpr UINT32 getMinSizeOfRecordInRdp()
   {
      return NORMAL_RECORD_HEAD_SIZE + RDP_RSLOT_SIZE;
   }

   BOOLEAN initRecordDataPage(UINT32 pageSize,
                              PAGE_ID pid,      
                              PAGE_ID lpid,
                              PAGE_SNAPSHOT_VERION psv,
                              UINT32 logicalID,
                              UINT32 pageSeq,
                              void *buf);

   OSS_INLINE UINT32 getMaxFreeSizeOfRdp(UINT32 pageSize)
   {
      return getPageBodySize(pageSize) - RECORD_PAGE_HEAD_SIZE;
   }

   OSS_INLINE UINT32 estimateNormalRecordSavingSize(UINT32 recordSize)
   {
      UINT32 size = recordSize + NORMAL_RECORD_HEAD_SIZE + RDP_RSLOT_SIZE;
      return ossAlign4(size);
   }

   OSS_INLINE BOOLEAN isBigRecord(UINT32 pageSize, UINT32 originalRecordSize)
   {
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      UINT32 size = estimateNormalRecordSavingSize(originalRecordSize);
      static constexpr UINT32 _MIN_FREE_SIZE = 1024;
      return pageSize < (size + _MIN_FREE_SIZE);
   }
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_DATA_PAGE_H_