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

namespace engine
{
namespace vessel
{
   static const UINT32 BTREE_NODE_PAGE_HEAD_VERSION = 1;

   //static const UINT32 BTREE_NODE_FLAG_LEAF = 0x01;
   static const UINT32 BTREE_NODE_FLAG_COMPRESSION_BANNED = 0x02;
   static const UINT32 BTREE_NODE_FLAG_PREFIX_CREATED = 0x04;
#pragma pack(4)
   struct btreeNodePageHead
   {
      btreeNodePageHead(){}
      ~btreeNodePageHead(){}
      btreeNodePageHead(const btreeNodePageHead &) = delete;
      btreeNodePageHead &operator=(const btreeNodePageHead &) = delete;

      BOOLEAN isValid()const;
      
      UINT32 version = 0;
      UINT32 clLogicalID = DMS_INVALID_LOGICCLID;
      UINT32 indexId = INVALID_LOGICAL_INDEX_ID;
      UINT32 flags = 0;
      UINT16 totalFreeSpace = 0;
      UINT16 freeSapceAfterLastSlot = 0;
      UINT16 totalSlotCount = 0;
      UINT16 markedDeleteSlotCount = 0;
      UINT32 rightChild = INVALID_PAGE_ID;
      UINT32 extNode = INVALID_PAGE_ID;
      UINT64 transSN = DPS_INVALID_TRANSID_SN;
      UINT32 splitedTimes = 0;
      INT16 splitFactor = 0;
      UINT16 pad0 = 0;
      UINT64 pad1 = 0;
   };//struct btreeNodeHead
   static const UINT32 BTREE_NODE_PAGE_HEAD_SIZE = sizeof(btreeNodePageHead);
   

   struct btreeNodeSlot
   {  
      OSS_INLINE btreeNodeSlot(){}
      OSS_INLINE ~btreeNodeSlot(){}
      OSS_INLINE btreeNodeSlot(const btreeNodeSlot &o):
                 flags(o.flags),
                 ridSlot(o.ridSlot),
                 ridPage(o.ridPage)
                 {
                    data.value = o.data.value;
                 }
      OSS_INLINE btreeNodeSlot &operator=(const btreeNodeSlot &o)
      {
         flags = o.flags;
         ridSlot = o.ridSlot;
         ridPage = o.ridPage;
         data.value = o.data.value;
         return *this;
      }

      static const UINT16 FLAG_IN_USED = 0x01;
      static const UINT16 FLAG_MARKED_DELETE = 0x02;
      static const UINT16 FLAG_KEY_IN_SLOT = 0x04;
      static const UINT16 FLAG_KEY_IN_EXT_PAGE = 0x08;
      static const UINT16 FLAG_KEY_COMPRESSED = 0x16;
      static const UINT16 FLAG_MAX = 0x8000;


      OSS_INLINE BOOLEAN isValid()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_IN_USED);
      }
      OSS_INLINE BOOLEAN isMarkedDelete()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_MARKED_DELETE);
      }
      OSS_INLINE BOOLEAN isKeySavedInSlot()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_KEY_IN_SLOT);
      }
      OSS_INLINE BOOLEAN isKeyCompressed()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_KEY_COMPRESSED);
      }

      union slotData
      {
         struct
         {
            UINT16 offset;
            UINT16 pad;
            UINT32 leftChild;
         } pointer;//struct pointer
         CHAR keyData[8];
         UINT64 value = 0;
      };//slotData

      UINT16 flags = 0;
      UINT16 ridSlot = 0;
      UINT32 ridPage = 0;
      slotData data;
   };//struct btreeNodeSlot

   static const UINT32 BTREE_NODE_SLOT_SIZE = sizeof(btreeNodeSlot);

   static const UINT32 MAX_BTREE_NODE_PREFIX_COUNT = 4;
   struct btreeNodePrefixSlot
   {
      btreeNodePrefixSlot(){}
      ~btreeNodePrefixSlot(){}
      btreeNodePrefixSlot(const btreeNodePrefixSlot &) = delete;
      btreeNodePrefixSlot &operator=(const btreeNodePrefixSlot &) = delete;

      OSS_INLINE BOOLEAN isValid()const
      {
         return 0 < prefixSize && 0 != prefixOffset;
      }

      UINT16 prefixSize = 0;
      UINT16 prefixOffset = 0;
      UINT16 referencedLow = 0;
      UINT16 referencedHigh = 0;

   };//struct btreeNodePrefixSlot
   static const UINT32 BTREE_NODE_PREFIX_SLOT_SIZE = sizeof(btreeNodePrefixSlot);

   BOOLEAN initBtreeNodePage(UINT32 pageSize,
                             PAGE_ID pid,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 cllid,
                             UINT32 indexId,
                             CHAR *buf);

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_PAGE_H_
