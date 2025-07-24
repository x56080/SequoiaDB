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

   Source File Name = btreeContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_CONTEXT_H_
#define VESSEL_BTREE_CONTEXT_H_

#include "vessel/btreeNodeBase.h"
#include "vessel/btreeStatistics.h"

namespace engine
{
namespace vessel
{
   class btreeContext : public SDBObject
   {
      public:
         btreeContext() = default;
         virtual ~btreeContext() = default;
         btreeContext(const btreeContext &) = delete;
         btreeContext &operator=(const btreeContext &) = delete;

      public:
         virtual INT32 allocateNewNode(UINT32 depth,
                                       const btreeNodePageHead &header,
                                       BTREE_NODE_UPTR &node) = 0;

         virtual void destroyNode(BTREE_NODE_UPTR &node) = 0;

         virtual void statsInsertCompressedIndex(UINT32 optimizedSize, UINT32 remainSize){}
         virtual void statsInsertUncompressedIndex(UINT32 size){}
         virtual void statsRemoveCompressedIndex(UINT32 optimizedSize, UINT32 remainSize){}
         virtual void statsRemoveUncompressedIndex(UINT32 size){}
         virtual void statsRemoveMarkedDeletedIndexes(UINT32 num){}
         virtual void statsReleaseMarkedDeletedIndexSpace(UINT32 size){}
         virtual void statsRecompress(UINT32 newCompressedEntryNum,
                                      UINT64 newRealTotalEntrySize,
                                      UINT32 newPrefixNum,
                                      UINT32 oldCompressedEntryNum,
                                      UINT64 oldRealTotalEntrySize,
                                      UINT32 oldPrefixNum){}
         virtual void statsAddLeafNode(BOOLEAN isCompressed){}
         virtual void statsRemoveLeafNode(BOOLEAN isCompressed){}
         virtual void statsAddNonLeafNode(){}
         virtual void statsRemoveNonLeafNode(){}
         virtual void statsAllocateNode(){}
         virtual void statsDestroyNode(){}
         virtual void statsCreateNewRoot(){}
         virtual void statsRefillChildNode(){}
         virtual void statsTruncateTree(){}
         virtual void statsTruncate(UINT32 newTotalEntryNum,
                                    UINT32 newCompressedEntryNum,
                                    UINT32 oldTotalEntryNum,
                                    UINT32 oldCompressedEntryNum,
                                    UINT64 origTotalEntrySizeDiff,
                                    UINT64 realTotalEntrySizeDiff){}
         virtual void statsCompact(UINT64 realTotalEntrySizeDiff,
                                   UINT32 newPrefixNum,
                                   UINT32 oldPrefixNum){}
         virtual void statsSplit(UINT32 newTotalEntryNum,
                                 UINT32 newCompressedEntryNum,
                                 UINT64 newOrigTotalEntrySize,
                                 UINT64 newRealTotalEntrySize,
                                 UINT32 prefixNum){}
   };//class btreeContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_CONTEXT_H_