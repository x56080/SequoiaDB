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

   Source File Name = btreeEntryPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_ENTRY_PAGE_H_
#define VESSEL_BTREE_ENTRY_PAGE_H_

#include "vessel/indexDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "vessel/slice.h"
#include "vessel/btreeStatistics.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 BTREE_ENTRY_PAGE_VERSION = 1;


#pragma pack(4)
   struct btreeEntryPageHead
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return BTREE_ENTRY_PAGE_VERSION == version &&
                INVALID_LOGICAL_INDEX_ID != logicalIndexId;
      }

      UINT32 version = 0;
      UINT32 logicalIndexId = INVALID_LOGICAL_INDEX_ID;
      UINT32 btreeRoot = INVALID_PAGE_ID;
      UINT32 transferTick = 0;
      /// btree statistics
      UINT32 nonleafNodeNum = 0;
      UINT32 leafNodeNum = 0;
      UINT32 compressedNodeNum = 0;
      UINT64 totalEntryNum = 0;
      UINT64 compressedEntryNum = 0;
      UINT64 origTotalEntrySize = 0;
      UINT64 realTotalEntrySize = 0;
      UINT64 totalPrefixNum = 0;
      UINT64 totalEntryInserted = 0;
      UINT64 totalEntryRemoved = 0;
      UINT64 nodesAllocated = 0;
      UINT64 nodesDestroyed = 0;
      UINT32 newRootCreatedNum = 0;
      UINT64 childNodesRefilled = 0;
      UINT64 nodesCompressedTimes = 0;
   };//struct btreeEntryPageHead

   constexpr UINT32 BTREE_ENTRY_PAGE_HEAD_SIZE = sizeof(btreeEntryPageHead);

#pragma pack()
   BOOLEAN initBtreeEntryPage(UINT32 pageSize,
                              PAGE_ID pid,
                              PAGE_ID lpid,
                              PAGE_SNAPSHOT_VERION psv,
                              UINT32 logicalIndexId,
                              CHAR *buf);
}//namespace vessel
}//namespace engine

#endif//VESSEL_BTREE_ENTRY_PAGE_H_