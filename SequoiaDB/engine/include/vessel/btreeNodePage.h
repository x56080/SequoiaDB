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

   Source File Name = btreeNodePage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_PAGE_H_
#define VESSEL_BTREE_NODE_PAGE_H_

#include "vessel/pageDef.h"
#include "vessel/indexDef.h"
#include "dms.hpp"
#include "vessel/recordID.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   constexpr UINT32 BTREE_NODE_PAGE_HEAD_VERSION = 1;
#pragma pack(4)
   struct btreeNodePrefixSlot
   {
      btreeNodePrefixSlot(){}
      ~btreeNodePrefixSlot(){}
      btreeNodePrefixSlot(const btreeNodePrefixSlot &) = delete;
      btreeNodePrefixSlot &operator=(const btreeNodePrefixSlot &) = delete;

      OSS_INLINE BOOLEAN isFree()const
      {
         return 0 == prefixOffset;
      }
      OSS_INLINE BOOLEAN isReferenced()const
      {
         return isValidRecordSlotPosition(low) &&
                isValidRecordSlotPosition(high);
      }
      OSS_INLINE UINT32 getOptimizedSize()const
      {
         return (high - low) * prefixSize;
      }

      void reset()
      {
         prefixOffset = 0;
         prefixSize = 0;
         low = 0;
         high = 0;
         return;
      }

      UINT16 prefixOffset = 0;
      UINT16 prefixSize = 0;
      INT16 low = 0;
      INT16 high = 0;

   };//struct btreeNodePrefixSlot
   constexpr UINT32 BTREE_NODE_PREFIX_SLOT_SIZE = sizeof(btreeNodePrefixSlot);


   /// btreeNode flags begin
   constexpr UINT32 BTREE_NODE_FLAG_IS_ROOT = 0x01;
   constexpr UINT32 BTREE_NODE_FLAG_IS_LEAF = 0x02;

   /// tried to generate(or regenerate) prefixes but failed.
   /// reset until next split.
   constexpr UINT32 BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION = 0x04;
   /// btreeNode flags end

   struct btreeNodePageHead
   {
      btreeNodePageHead(){}
      ~btreeNodePageHead(){}
      btreeNodePageHead(const btreeNodePageHead &) = delete;
      btreeNodePageHead &operator=(const btreeNodePageHead &) = delete;

      OSS_INLINE BOOLEAN isValid()const
      {
         return BTREE_NODE_PAGE_HEAD_VERSION == version &&
                INVALID_LOGICAL_INDEX_ID != indexId;
      }

      UINT32 version = 0;
      UINT32 indexId = INVALID_LOGICAL_INDEX_ID;
      UINT32 flags = 0;
      UINT16 totalFreeSpace = 0;
      UINT16 totalSlotCount = 0;
      UINT16 backOffset = 0;
      UINT16 prefixCount = 0;
      UINT16 compressedItemCount = 0;
      UINT16 appendingFactor = 0;
      UINT32 rightChild = INVALID_PAGE_ID;
      INT32 birthNodeLevel = -1;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
      UINT16 transNode = DPS_INVALID_TRANSID_NODEID;
      CHAR pad[22] = {};
   };//struct btreeNodeHead
   constexpr UINT32 BTREE_NODE_PAGE_HEAD_SIZE = sizeof(btreeNodePageHead);
   
#pragma pack()


#pragma pack(4)
   struct btreeItemSlot
   {  
      btreeItemSlot() = default;
      ~btreeItemSlot() = default;
      OSS_INLINE btreeItemSlot(const btreeItemSlot &o):
                 flags(o.flags),
                 ridPos(o.ridPos),
                 ridPage(o.ridPage)
                 {
                    data.value = o.data.value;
                 }
      OSS_INLINE btreeItemSlot &operator=(const btreeItemSlot &o)
      {
         flags = o.flags;
         ridPos = o.ridPos;
         ridPage = o.ridPage;
         data.value = o.data.value;
         return *this;
      }

      static constexpr UINT16 FLAG_IN_USED = 0x01;
      static constexpr UINT16 FLAG_MARKED_DELETED = 0x02;
      static constexpr UINT16 FLAG_KEY_COMPRESSESD = 0x04;
      static constexpr UINT16 FLAG_MAX = 0x2000;

      /// 0x4000, 0x8000 for prefix slot pos.

      OSS_INLINE void reset()
      {
         flags = 0;
         ridPos = 0;
         ridPage = 0;
         data.value = 0;
      }
      OSS_INLINE BOOLEAN isValid()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_IN_USED);
      }

      void initAsNonLeafFormat(const recordID &rid,
                               UINT16 offset,
                               UINT16 size,
                               PAGE_ID leftChild);

      void initAsLeafFormat(const recordID &rid,
                            UINT16 offset,
                            UINT16 size,
                            BOOLEAN compressed,
                            RECORD_SLOT_POS prefixPos = INVALID_RECORD_SLOT_POS);

      OSS_INLINE BOOLEAN isMarkedDeleted()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_MARKED_DELETED);
      }
      OSS_INLINE void markDeleted()
      {
         OSS_BIT_SET(flags, FLAG_MARKED_DELETED);
      }
      OSS_INLINE BOOLEAN hasPrefixSlot()const
      {
         ///WARNING: user should ensure it is in leaf node!
         return isValidRecordSlotPosition(data.lf.prefixSlot);

      }
      OSS_INLINE BOOLEAN isKeyCompressed()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_KEY_COMPRESSESD);
      }
      OSS_INLINE BOOLEAN isKeyPerfectlyCompressed()const
      {
         return isKeyCompressed() && 0 == data.key.size;
      }

      union slotData
      {
         struct
         {
            UINT16 offset;
            UINT16 size;
         }key;

         struct 
         {
            UINT32 reserved;
            UINT32 leftChild;
         } nlf; // non-leaf format

         struct
         {
            UINT32 reserved;
            INT16 prefixSlot;
            UINT16 flags;
         }lf; // leaf format

         UINT64 value = 0;
      };//slotData

      UINT16 flags = 0;
      INT16 ridPos = 0;
      UINT32 ridPage = 0;
      slotData data;
   };//struct btreeItemSlot
#pragma pack()

   constexpr UINT32 BTREE_NODE_SLOT_SIZE = sizeof(btreeItemSlot);

   BOOLEAN initBtreeNodePage(UINT32 pageSize,
                             PAGE_ID pid,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 indexId,
                             BOOLEAN isLeaf,
                             BOOLEAN isRoot,
                             INT32 birthNodeLevel,
                             CHAR *buf);


} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_PAGE_H_
