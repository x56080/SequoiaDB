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
         return INVALID_STRIPING_ID != minStriping &&
                INVALID_STRIPING_ID != maxStriping &&
                minStriping <= maxStriping;
      }

      UINT32 version = INVALID_RDP_VERSION;
      UINT32 flags = 0;
      UINT32 clLogcalID = DMS_INVALID_LOGICCLID;
      UINT32 pageSeq = INVALID_CL_PAGE_SEQ;
      UINT16 totalSlotCount = 0;
      UINT16 recordCount = 0;
      UINT16 totalFreeSpace = 0;
      UINT16 backOffset = 0;
      UINT16 firstFreeSlot = INVALID_RECORD_SLOT_ID;
      UINT16 dicSlot = INVALID_RECORD_SLOT_ID;
      UINT16 minStriping = INVALID_STRIPING_ID;
      UINT16 maxStriping = INVALID_STRIPING_ID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
      CHAR pad[32] = {};
   };//struct recordDataPageHead
   constexpr UINT32 RECORD_PAGE_HEAD_SIZE = sizeof(recordDataPageHead);

   constexpr UINT16 RDP_SLOT_FLAG_IN_USED = 0x01;
   constexpr UINT16 RDP_SLOT_FLAG_INVISIBLE = 0x02;
   constexpr UINT16 RDP_SLOT_FLAG_OVERFLOW = 0x04;
   constexpr UINT16 RDP_SLOT_FLAG_TOMBSTONE = 0x08;
   constexpr UINT16 RDP_SLOT_FLAG_BIG_RECORD = 0x10;

   constexpr UINT8 RDP_RECORD_HEAD_TYPE_INVALID = 0;
   constexpr UINT8 RDP_RECORD_HEAD_TYPE_NORMAL = 1;
   constexpr UINT8 RDP_RECORD_HEAD_TYPE_DIC = 2;

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
      OSS_INLINE BOOLEAN isInvisible()const
      {
         return 0 != OSS_BIT_TEST(flags, RDP_SLOT_FLAG_INVISIBLE);
      }
      OSS_INLINE void setInvisible()
      {
         OSS_BIT_SET(flags, RDP_SLOT_FLAG_INVISIBLE);
      }
      OSS_INLINE BOOLEAN isOverflow()const
      {
         return 0 != OSS_BIT_TEST(flags, RDP_SLOT_FLAG_OVERFLOW);
      }
      OSS_INLINE void setOverflow()
      {
         OSS_BIT_SET(flags, RDP_SLOT_FLAG_OVERFLOW);
      }
      OSS_INLINE BOOLEAN isTombstone()const
      {
         return 0 != OSS_BIT_TEST(flags, RDP_SLOT_FLAG_TOMBSTONE);
      }
      OSS_INLINE void setTombstone()
      {
         OSS_BIT_SET(flags, RDP_SLOT_FLAG_TOMBSTONE);
      }
      OSS_INLINE void setBigRecord()
      {
         OSS_BIT_SET(flags, RDP_SLOT_FLAG_BIG_RECORD);
      }
      OSS_INLINE BOOLEAN isBigRecord()const
      {
         return 0 != OSS_BIT_TEST(flags, RDP_SLOT_FLAG_BIG_RECORD);
      }
   
      public:
         UINT16 flags = 0;
         UINT8 type = RDP_RECORD_HEAD_TYPE_INVALID;
         UINT8 reserved = 0;
         UINT16 offset = 0;
         UINT16 size = 0;
   };//struct recordSlot
   constexpr UINT32 RDP_RSLOT_SIZE = sizeof(recordSlot);

   struct recordHead
   {
      public:
         OSS_INLINE recordHead()
         {
            format.v.v0 = 0;
            format.v.v1 = 0;
         }

         OSS_INLINE ~recordHead()
         {}

         OSS_INLINE recordHead(const recordHead &o)
         {
            format.v.v0 = o.format.v.v0;
            format.v.v1 = o.format.v.v1;
         }
         

         OSS_INLINE recordHead &operator=(const recordHead &o)
         {
            format.v.v0 = o.format.v.v0;
            format.v.v1 = o.format.v.v1;
            return *this;
         }

         void reset()
         {
            format.v.v0 = 0;
            format.v.v1 = 0;
            return;
         }
      public:
         union FORMAT
         {
            struct
            {
               UINT8 flags;
               UINT8 compressionType;
               UINT16 transNode;
               UINT64 transSN;

               OSS_INLINE BOOLEAN isCompressed()const
               {
                  return UTIL_COMPRESSOR_INVALID != compressionType;
               }
               OSS_INLINE DPS_TRANS_ID getTransID()const
               {
                  return DPS_TRANS_ID(transSN, transNode);
               }
            }normal;

            struct
            {
               UINT32 lpid;
               UINT16 slot;
               UINT16 flags;
               UINT32 pad;

               recordID getRid()const
               {
                  return recordID(lpid, slot);
               }
            }overflow;

            struct
            {
               UINT32 v0;
               UINT64 v1;
            }v;
         };

         FORMAT format; 
   };//struct recordHead
   constexpr UINT32 RDP_RECORD_HEAD_SIZE = sizeof(recordHead);


   struct compressionDicHead
   {
      UINT16 size = 0;
      UINT8 type = UTIL_COMPRESSOR_INVALID;
      UINT8 flags = 0;
   };//struct compressionDicRecordHead
   const static UINT32 RDP_DIC_HEAD_LEN = sizeof(compressionDicHead);
   
#pragma pack()

   /// in fact, we may not allocate new slot when insert record.
   /// but ignore it here.
   OSS_INLINE UINT32 getMinSizeOfRecordInRdp()
   {
      return RDP_RECORD_HEAD_SIZE + RDP_RSLOT_SIZE;
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
      return recordSize + RDP_RECORD_HEAD_SIZE + RDP_RSLOT_SIZE;
   }

   OSS_INLINE BOOLEAN isBigRecord(UINT32 pageSize, UINT32 originalRecordSize)
   {
      UINT32 size = estimateNormalRecordSavingSize(originalRecordSize);
      /// 128 is meaningless magic number.
      return getMaxFreeSizeOfRdp(pageSize) < (size + 128);
   }
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_DATA_PAGE_H_