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
   static const UINT32 BTREE_NODE_PAGE_HEAD_VERSION = 1;
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
         return low != INVALID_RECORD_SLOT_ID;
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
      UINT16 low = 0;
      UINT16 high = 0;

   };//struct btreeNodePrefixSlot
   static const UINT32 BTREE_NODE_PREFIX_SLOT_SIZE = sizeof(btreeNodePrefixSlot);
   static const UINT32 BTREE_NODE_MAX_PREFIX_COUNT = 128;


   /// btreeNode flags begin
   static const UINT32 BTREE_NODE_FLAG_IS_LEAF = 0x01;

   /// tried to generate(or regenerate) prefixes but failed.
   /// reset until next split.
   static const UINT32 BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION = 0x02;
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
                DMS_INVALID_LOGICCLID != clLogicalID &&
                INVALID_LOGICAL_INDEX_ID != indexId;
      }

      UINT32 version = 0;
      UINT32 clLogicalID = DMS_INVALID_LOGICCLID;
      UINT32 indexId = INVALID_LOGICAL_INDEX_ID;
      UINT32 flags = 0;
      UINT16 totalFreeSpace = 0;
      UINT16 freeSapceAfterLastSlot = 0;
      UINT16 totalSlotCount = 0;
      UINT16 externalKeySize = 0;
      UINT32 rightChild = INVALID_PAGE_ID;
      UINT32 splitedTimes = 0;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
      UINT16 transNode = DPS_INVALID_TRANSID_NODEID;
      UINT16 prefixCount = 0;
      UINT16 compressedItemCount = 0;
      UINT16 appendingFactor = 0;
      CHAR pad[24] = {};
   };//struct btreeNodeHead
   static const UINT32 BTREE_NODE_PAGE_HEAD_SIZE = sizeof(btreeNodePageHead);
   
#pragma pack()


#pragma pack(4)
   struct btreeItemSlot
   {  
      OSS_INLINE btreeItemSlot(){}
      OSS_INLINE ~btreeItemSlot(){}
      OSS_INLINE btreeItemSlot(const btreeItemSlot &o):
                 flags(o.flags),
                 ridSlot(o.ridSlot),
                 ridPage(o.ridPage)
                 {
                    data.value = o.data.value;
                 }
      OSS_INLINE btreeItemSlot &operator=(const btreeItemSlot &o)
      {
         flags = o.flags;
         ridSlot = o.ridSlot;
         ridPage = o.ridPage;
         data.value = o.data.value;
         return *this;
      }

      static const UINT16 FLAG_IN_USED = 0x01;
      static const UINT16 FLAG_MARKED_DELETED = 0x02;
      static const UINT16 FLAG_KEY_IN_EXTERNAL_PAGE = 0x04;
      static const UINT16 FLAG_KEY_COMPRESSESD = 0x08;
      static const UINT16 FLAG_MAX = 0x2000;

      /// 0x4000, 0x8000 for prefix slot pos.

      OSS_INLINE void reset()
      {
         flags = 0;
         ridSlot = 0;
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
                            RECORD_SLOT_ID prefixPos = INVALID_RECORD_SLOT_ID);

      void initWhenKeyInExtPage(const recordID &rid,
                                PAGE_ID leftChild,
                                PAGE_ID extp);

      OSS_INLINE BOOLEAN isMarkedDeleted()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_MARKED_DELETED);
      }
      OSS_INLINE BOOLEAN isKeyInExtPage()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_KEY_IN_EXTERNAL_PAGE);
      }
      OSS_INLINE BOOLEAN isKeyCompressed()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_KEY_COMPRESSESD);
      }
      OSS_INLINE BOOLEAN isKeyPerfectlyCompressed()const
      {
         return isKeyCompressed() && 0 == data.key.size;
      }
      OSS_INLINE PAGE_ID getExternalPage()const
      {
         return data.ekf.extp;
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
            UINT16 prefixSlot;
            UINT16 flags;
         }lf; // leaf format

         struct
         {
            UINT32 extp;
            UINT32 leftChild;
         }ekf;/// external key format

         UINT64 value = 0;
      };//slotData

      UINT16 flags = 0;
      UINT16 ridSlot = 0;
      UINT32 ridPage = 0;
      slotData data;
   };//struct btreeItemSlot
#pragma pack()

   static const UINT32 BTREE_NODE_SLOT_SIZE = sizeof(btreeItemSlot);

   BOOLEAN initBtreeNodePage(UINT32 pageSize,
                             PAGE_ID pid,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 cllid,
                             UINT32 indexId,
                             BOOLEAN isLeaf,
                             CHAR *buf);


} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_PAGE_H_
