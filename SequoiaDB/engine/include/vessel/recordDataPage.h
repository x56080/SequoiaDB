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
   const UINT16 INVALID_RDP_VERSION = 0;
   const UINT16 RDP_VERSION_1 = 1;
   const UINT16 RDP_VERSION = RDP_VERSION_1;

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

      UINT16 version = INVALID_RDP_VERSION;
      UINT16 flags = 0;
      UINT32 clLogcalID = DMS_INVALID_LOGICCLID;
      UINT32 pageSeq = INVALID_CL_PAGE_SEQ;
      UINT16 totalSlotCount = 0;
      UINT16 recordCount = 0;
      UINT16 totalFreeSpace = 0;
      UINT16 freeSpaceAfterLastSlot = 0;
      UINT16 firstFreeSlot = INVALID_RECORD_SLOT_ID;
      UINT16 dicSlot = INVALID_RECORD_SLOT_ID;
      UINT16 minStriping = INVALID_STRIPING_ID;
      UINT16 maxStriping = INVALID_STRIPING_ID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
      UINT64 pad = 0;
   };//struct recordDataPageHead
   const static UINT32 RECORD_PAGE_HEAD_LEN = sizeof(recordDataPageHead);


   /// value range: [0x0, 0xFF]
   const static UINT8 RDP_SLOT_TYPE_INVALID = 0x0;
   const static UINT8 RDP_SLOT_TYPE_NORMAL = 0x01;
   const static UINT8 RDP_SLOT_TYPE_BIG_RECORD_HEAD = 0x02;
   const static UINT8 RDP_SLOT_TYPE_BIG_RECORD_BODY = 0x03;
   const static UINT8 RDP_SLOT_TYPE_COMPRESSION_DIC = 0x04;

   /// value range: [0x0, 0xFF]
   const static UINT8 RDP_SLOT_FLAG_INVISIBLE = 0x01;

   struct recordSlot
   {
      OSS_INLINE recordSlot(){}
      OSS_INLINE ~recordSlot(){}

      OSS_INLINE recordSlot(const recordSlot &o):
      _type(o._type),
      _flags(o._flags),
      _offset(o._offset)
      {}

      OSS_INLINE recordSlot &operator=(const recordSlot &o)
      {
         _type = o._type;
         _flags = o._flags;
         _offset = o._offset;
         return *this;
      }

      OSS_INLINE BOOLEAN operator==(const recordSlot &o)const
      {
         return _type == o._type &&
                _flags == o._flags &&
                _offset == o._flags;
      }

      OSS_INLINE void setType(UINT8 type)
      {
         _type = type;
         return;
      }
      OSS_INLINE UINT8 getType()const
      {
         return _type;
      }

      OSS_INLINE void setOffset(UINT16 offset)
      {
         _offset = offset;
      }

      OSS_INLINE UINT32 getOffset()const
      {
         return _offset;
      }
      OSS_INLINE BOOLEAN isValid()const
      {
         return RDP_SLOT_TYPE_INVALID != getType() &&
                0 != _offset;
      }
      OSS_INLINE BOOLEAN isInvisible()const
      {
         return 0 != OSS_BIT_TEST(_flags, RDP_SLOT_FLAG_INVISIBLE);
      }
      OSS_INLINE BOOLEAN isValidAndVisible()const
      {
         return isValid() && !isInvisible();
      }

      OSS_INLINE BOOLEAN isNormalRecordHead()const
      {
         return isValid() && RDP_SLOT_TYPE_NORMAL == _type;
      }
      OSS_INLINE BOOLEAN isBigRecordHead()const
      {
         return isValid() && RDP_SLOT_TYPE_BIG_RECORD_HEAD == _type;
      }

      OSS_INLINE void setInvisible()
      {
         OSS_BIT_SET(_flags, RDP_SLOT_FLAG_INVISIBLE);
      }
   
      private:
      UINT8 _type = RDP_SLOT_TYPE_INVALID;
      UINT8 _flags = 0;
      UINT16 _offset = 0;
   };//struct recordSlot
   constexpr UINT32 RDP_RSLOT_SIZE = sizeof(recordSlot);

   static const UINT16 RDP_RECORD_FLAG_DEPENDENT = 0x01;
   static const UINT16 RDP_RECORD_FLAG_TOMBSTONE = 0x02;
   static const UINT16 RDP_RECORD_FLAG_OVERFLOW = 0x04;

   struct recordHead
   {
      public:
         OSS_INLINE recordHead()
         {}

         OSS_INLINE ~recordHead()
         {}

         OSS_INLINE recordHead(const recordHead &o):
         _size(o._size),
         _flags(o._flags),
         _compressionType(o._compressionType),
         _pad(o._pad),
         _transNode(o._transNode),
         _transSN(o._transSN){}

         OSS_INLINE recordHead &operator=(const recordHead &o)
         {
            _size = o._size;
            _flags = o._flags;
            _compressionType = o._compressionType;
            _pad = o._pad;
            _transNode = o._transNode;
            _transSN = o._transSN;
            return *this;
         }

         OSS_INLINE UINT16 getSize()const
         {
            return _size;
         }
         OSS_INLINE void setSize(UINT16 size)
         {
            _size = size;
         }
         OSS_INLINE void setDependent()
         {
            OSS_BIT_SET(_flags, RDP_RECORD_FLAG_DEPENDENT);
         }
         OSS_INLINE BOOLEAN isDependent()const
         {
            return 0 != OSS_BIT_TEST(_flags, RDP_RECORD_FLAG_DEPENDENT);
         }
         OSS_INLINE void setTombstone()
         {
            OSS_BIT_SET(_flags, RDP_RECORD_FLAG_TOMBSTONE);
         }
         OSS_INLINE BOOLEAN isTombstone()const
         {
            return 0 != OSS_BIT_TEST(_flags, RDP_RECORD_FLAG_TOMBSTONE);
         }
         OSS_INLINE void setOverflow()
         {
            OSS_BIT_SET(_flags, RDP_RECORD_FLAG_OVERFLOW);
         }
         OSS_INLINE BOOLEAN isOverflow()const
         {
            return 0 != OSS_BIT_TEST(_flags, RDP_RECORD_FLAG_OVERFLOW);
         }
         OSS_INLINE void setCompressionType(UTIL_COMPRESSOR_TYPE type)
         {
            _compressionType = type;
         }
         OSS_INLINE void setTransInfo(UINT16 transNode, UINT64 transSN)
         {
            _transNode = transNode;
            _transSN = transSN;
         }
         OSS_INLINE UINT16 getTransNode()const
         {
            return _transNode;
         }
         OSS_INLINE UINT64 getTransSN()const
         {
            return _transSN;
         }
         OSS_INLINE BOOLEAN isCompressed()const
         {
            return UTIL_COMPRESSOR_INVALID != _compressionType;
         }
         OSS_INLINE DPS_TRANS_ID getTransID()const
         {
            return DPS_TRANS_ID(_transSN, _transNode);
         }

      private:
         UINT16 _size = 0;
         UINT16 _flags = 0;
         UINT8 _compressionType = UTIL_COMPRESSOR_INVALID;
         UINT8 _pad = 0;
         UINT16 _transNode = DPS_INVALID_TRANSID_NODEID;
         UINT64 _transSN = DPS_INVALID_TRANSID_SN;
   };//struct recordHead
   const static UINT32 RDP_RECORD_HEAD_LEN = sizeof(recordHead);


   struct bigRecordHead
   {
      recordHead normalHead;
      UINT32 originalLen = 0;
      UINT32 compressedLen = 0;
      UINT32 nextPid = INVALID_PAGE_ID;
      UINT16 nextSlot = INVALID_RECORD_SLOT_ID;
      UINT16 totalSlice = 0;
   };
   const static UINT32 RDP_BIG_RECORD_HEAD_HEAD_LEN = sizeof(bigRecordHead);

   struct bigRecordBodyHead
   {
      recordHead normalHead;
      UINT32 nextPid = INVALID_PAGE_ID;
      UINT16 nextSlot = INVALID_RECORD_SLOT_ID;
      UINT16 sliceNo = 0;
   };//struct bigRecordBodyHead
   const static UINT32 RDP_BIT_RECORD_BODY_HEAD_LEN = sizeof(bigRecordBodyHead);

   struct compressionDicRecordHead
   {
      UINT16 size = 0;
      UINT8 type = UTIL_COMPRESSOR_INVALID;
      UINT8 flags = 0;
   };//struct compressionDicRecordHead
   const static UINT32 RDP_DIC_RECORD_HEAD_LEN = sizeof(compressionDicRecordHead);
   
#pragma pack()

   /// in fact, we may not allocate new slot when insert record.
   /// but ignore it here.
   OSS_INLINE UINT32 getMinSizeOfRecordInRdp()
   {
      return RDP_RECORD_HEAD_LEN + RDP_RSLOT_SIZE;
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
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      return pageSize - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE - RECORD_PAGE_HEAD_LEN;
   }

   OSS_INLINE UINT32 getAlignedSizeOfNormalRecordAndHead(UINT32 recordSize)
   {
      return RDP_RECORD_HEAD_LEN + ossAlign4(recordSize);
   }

   OSS_INLINE UINT32 getMaxSizeOfRecordInRdp(UINT32 recordSize)
   {
      return getAlignedSizeOfNormalRecordAndHead(recordSize) + RDP_RSLOT_SIZE;
   }

   OSS_INLINE BOOLEAN isBigRecord(UINT32 pageSize, UINT32 originalRecordSize)
   {
      SDB_ASSERT(32768 == pageSize || 65536 == pageSize, "impossible");
      UINT32 maxSize = getMaxSizeOfRecordInRdp(originalRecordSize);
      /// 512 is meaningless magic number.
      return (maxSize + RECORD_PAGE_HEAD_LEN + 512) > getPageBodySize(pageSize);
   }
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_DATA_PAGE_H_