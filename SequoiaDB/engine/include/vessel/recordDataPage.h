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

   Source File Name = recordDataPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
   constexpr UINT16 INVALID_RDP_VERSION = 0;
   constexpr UINT16 RDP_VERSION_1 = 1;
   constexpr UINT16 RDP_VERSION = RDP_VERSION_1;

#pragma pack(4)
   struct recordDataPageHead
   {
      OSS_INLINE BOOLEAN hasValidStripingRange()const
      {
         return dmsStripingRange(minStriping, maxStriping).isValid();
      }

      UINT16 version = INVALID_RDP_VERSION;
      UINT16 flags = 0;
      UINT32 clLogcalID = DMS_INVALID_LOGICCLID;
      UINT32 pageSeq = 0;
      UINT16 totalFreeSpace = 0;
      UINT16 backOffset = 0;
      UINT16 totalSlotCount = 0;
      UINT16 freeSlotCount = 0;
      /// visible and not tomestone but may be overflowed
      UINT16 totalRecordCount = 0;

      UINT16 dicSize = 0;
      UINT16 dicOffset = 0;
      UINT8 compressor = UTIL_COMPRESSOR_INVALID;
      UINT8 _pad = 0;
      
      INT32 minStriping = DMS_INVALID_STRIPING_ID;
      INT32 maxStriping = DMS_INVALID_STRIPING_ID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;

      UINT64 _reserved0 = 0;
      UINT64 _reserved1 = 0;
   };//struct recordDataPageHead
   constexpr UINT32 RECORD_PAGE_HEAD_SIZE = sizeof(recordDataPageHead);

   constexpr UINT8 RDP_RECORD_HEAD_TYPE_INVALID = 0;
   constexpr UINT8 RDP_RECORD_HEAD_TYPE_NORMAL = 1;
   constexpr UINT8 RDP_RECORD_HEAD_OVERFLOW = 2;
   //constexpr UINT8 RDP_RECORD_HEAD_TOMBSTONE = 3;
   constexpr UINT8 RDP_RECORD_HEAD_BIG_RECORD_ENTRY = 4;
   constexpr UINT8 RDP_RECORD_HEAD_BIG_RECORD_BODY = 5;

   struct recordSlot
   {
      enum FLAG : UINT16
      {
         IN_USED = 0x01,
         INVISIBLE = 0x02,
         TOMBSTONE = 0x04,
      };

      OSS_INLINE void reset()
      {
         static_assert(sizeof(UINT64) == sizeof(recordSlot), "invalid size");
         *((UINT64 *)this) = 0;
         return;
      }

      OSS_INLINE void init(UINT8 type,
                           UINT8 reservedSpaceSize,
                           UINT16 offset,
                           UINT16 size)
      {
         this->flags = IN_USED;
         this->type = type;
         this->reservedSpaceSize = reservedSpaceSize;
         this->offset = offset;
         this->size = size;
         return;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return 0 != OSS_BIT_TEST(flags, IN_USED) &&
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
         return 0 != OSS_BIT_TEST(flags, INVISIBLE);
      }
      OSS_INLINE void setInvisible()
      {
         OSS_BIT_SET(flags, INVISIBLE);
      }
      OSS_INLINE BOOLEAN isNormalRecord()const
      {
         return type == RDP_RECORD_HEAD_TYPE_NORMAL;
      }
      OSS_INLINE void setTombstone()
      {
         OSS_BIT_SET(flags, TOMBSTONE);
      }
      OSS_INLINE BOOLEAN isTombstone()const
      {
         return 0 != OSS_BIT_TEST(flags, TOMBSTONE);
      }
      OSS_INLINE BOOLEAN isOverflowedRecord()const
      {
         return type == RDP_RECORD_HEAD_OVERFLOW;
      }
      OSS_INLINE BOOLEAN isBigRecordEntry()const
      {
         return type == RDP_RECORD_HEAD_BIG_RECORD_ENTRY;
      }
      OSS_INLINE BOOLEAN isBigRecordBody()const
      {
         return type == RDP_RECORD_HEAD_BIG_RECORD_BODY;
      }

      OSS_INLINE UINT32 getMaxSpaceSize()const
      {
         return (UINT32)size + (UINT32)reservedSpaceSize;
      }
   
      static constexpr UINT32 getMaxReservedSize()
      {
         return 128;
      }
   
      UINT16 flags = 0;
      UINT8 type = RDP_RECORD_HEAD_TYPE_INVALID;
      UINT8 reservedSpaceSize = 0;
      UINT16 offset = 0;
      UINT16 size = 0;
   };//struct recordSlot
   constexpr UINT32 RDP_RSLOT_SIZE = sizeof(recordSlot);

   struct normalRecordHead
   {
      enum FLAG : UINT16
      {
         COMPRESSED = 0x01,
      };

      OSS_INLINE void setTransID(const DPS_TRANS_ID &transID)
      {
         transNode = transID.getNodeID();
         transSN = transID.getSN();
         return;
      }
      OSS_INLINE void setCompressed()
      {
         OSS_BIT_SET(flags, COMPRESSED);
      }
      OSS_INLINE BOOLEAN isCompressed()const
      {
         return 0 != OSS_BIT_TEST(flags, COMPRESSED);
      }

      UINT16 flags = 0;
      UINT16 transNode = DPS_INVALID_TRANSID_NODEID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
   };//struct normalRecordHead
   constexpr UINT32 NORMAL_RECORD_HEAD_SIZE = sizeof(normalRecordHead);
   static_assert(ossIsAligned4(NORMAL_RECORD_HEAD_SIZE), "invalid size");

   struct overflowedRecord
   {
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

/*
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
*/

   struct bigRecordEntrySlice
   {
      bigRecordEntrySlice(){}
      ~bigRecordEntrySlice(){}

      OSS_INLINE void setRid(const recordID &rid)
      {
         nextPage = rid.getPid();
         nextPos = rid.getPos();
      }
      OSS_INLINE void setTransID(const DPS_TRANS_ID &transID)
      {
         transNode = transID.getNodeID();
         transSN = transID.getSN();
      }
      OSS_INLINE BOOLEAN isCompressed()const
      {
         return UTIL_COMPRESSOR_INVALID != compressionType;
      }

      public:
         UINT32 nextPage = INVALID_PAGE_ID;
         INT16  nextPos = INVALID_RECORD_SLOT_POS;
         UINT16 transNode = DPS_INVALID_TRANSID_NODEID;
         UINT64 transSN = DPS_INVALID_TRANSID_SN;
         UINT32 sliceCount = 0;
         // actual storage length
         UINT32 totalRecordSize = 0;
         // If compressiontype is valid, 
         // originalRecordSize means uncompressed record length
         UINT32 originalRecordSize = 0;
         UINT16 flags = 0;
         UINT8  compressionType = UTIL_COMPRESSOR_INVALID;
         UINT8  pad = 0;
   };
   constexpr UINT32 BIG_RECORD_ENTRY_SIZE = sizeof(bigRecordEntrySlice);
   static_assert(BIG_RECORD_ENTRY_SIZE >= NORMAL_RECORD_HEAD_SIZE, "can not be lower");

   struct bigRecordBodySlice
   {
      bigRecordBodySlice(){}
      ~bigRecordBodySlice(){}
      bigRecordBodySlice(const bigRecordBodySlice &bs):
      flags(bs.flags),
      nextPos(bs.nextPos),
      nextPage(bs.nextPage){}

      bigRecordBodySlice &operator=(const bigRecordBodySlice &bs)
      {
         flags = bs.flags;
         nextPos = bs.nextPos;
         nextPage = bs.nextPage;
         return *this;
      }

      OSS_INLINE void setRid(const recordID &rid)
      {
         nextPos = rid.getPos();
         nextPage = rid.getPid();
      }

      public:
         UINT16 flags = 0;
         INT16  nextPos = INVALID_RECORD_SLOT_POS;
         UINT32 nextPage = INVALID_PAGE_ID;
         UINT32 pad = 0;
   };
   constexpr UINT32 BIG_RECORD_BODY_SLICE_SIZE = sizeof(bigRecordBodySlice);
   static_assert(BIG_RECORD_BODY_SLICE_SIZE >= NORMAL_RECORD_HEAD_SIZE, "can not be lower");
   
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