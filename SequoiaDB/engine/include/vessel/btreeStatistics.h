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

   Source File Name = btreeStatistics.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_STATISTICS_H_
#define VESSEL_BTREE_STATISTICS_H_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

namespace engine
{
namespace vessel
{
   struct btreeStatistics
   {
      void reset()
      {
         nonleafNodeNum = 0;
         leafNodeNum = 0;
         compressedNodeNum = 0;
         totalEntryNum = 0;
         compressedEntryNum = 0;
         origTotalEntrySize = 0;
         realTotalEntrySize = 0;
         totalPrefixNum = 0;

         totalEntryInserted = 0;
         totalEntryRemoved = 0;
         nodesAllocated = 0;
         nodesDestroyed = 0;
         newRootCreatedNum = 0;
         childNodesRefilled = 0;
         nodesCompressedTimes = 0;
         return;
      }

      OSS_INLINE UINT32 getTotalNodeNum() const
      {
         return nonleafNodeNum + leafNodeNum;
      }

      void insertCompressedIndex(UINT32 optimizedSize, UINT32 remainSize);
      void insertUncompressedIndex(UINT32 size);
      void removeCompressedIndex(UINT32 optimizedSize, UINT32 remainSize);
      void removeUncompressedIndex(UINT32 size);
      void removeMarkedDeletedIndexes(UINT32 num);
      void releaseMarkedDeletedIndexSpace(UINT32 size);
      void recompress(UINT32 newCompressedEntryNum,
                      UINT64 newRealTotalEntrySize,
                      UINT32 newPrefixNum,
                      UINT32 oldCompressedEntryNum,
                      UINT64 oldRealTotalEntrySize,
                      UINT32 oldPrefixNum);
      void addLeafNode(BOOLEAN isCompressed);
      void removeLeafNode(BOOLEAN isCompressed);
      void addNonLeafNode();
      void removeNonLeafNode();
      void allocateNode();
      void destroyNode();
      void createNewRoot();
      void refillChildNode();
      void truncateTree();
      void truncate(UINT32 newTotalEntryNum,
                    UINT32 newCompressedEntryNum,
                    UINT32 oldTotalEntryNum,
                    UINT32 oldCompressedEntryNum,
                    UINT64 origTotalEntrySizeDiff,
                    UINT64 realTotalEntrySizeDiff);
      void compact(UINT64 realTotalEntrySizeDiff,
                   UINT32 newPrefixNum,
                   UINT32 oldPrefixNum);
      void split(UINT32 newTotalEntryNum,
                 UINT32 newCompressedEntryNum,
                 UINT64 newOrigTotalEntrySize,
                 UINT64 newRealTotalEntrySize,
                 UINT32 prefixNum);

      bson::BSONObj toBSON() const;
      
      static constexpr const CHAR* BTREE_STATISTICE_FIELDNAME_NON_LEAF_NODE_NUM = "NonleafNodeNum";
      static constexpr const CHAR* BTREE_STATISTICE_FIELDNAME_LEAF_NODE_NUM = "LeafNodeNum";
      static constexpr const CHAR* BTREE_STATISTICE_FIELDNAME_COMPRESSED_NODE_NUM = "CompressedNodeNum";
      static constexpr const CHAR* BTREE_STATISTICE_FIELDNAME_TOTAL_ENTRY_NUM = "TotalEntryNum";
      static constexpr const CHAR* BTREE_STATISTICE_FIELDNAME_COMPRESSED_ENTRY_NUM = "CompressedEntryNum";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_ORIG_TOTAL_ENTRY_SIZE = "OrigTotalEntrySize";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_REAL_TOTAL_ENTRY_SIZE = "RealTotalEntrySize";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_TOTAL_PREFIX_NUM = "TotalPrefixNum";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_ENTRY_COMPRESSION_RATIO = "EntryCompressionRatio";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_TOTAL_ENTRY_INSERTED = "TotalEntryInserted ";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_TOTAL_ENTRY_REMOVED = "TotalEntryRemoved";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_NODES_ALLOCATED = "NodesAllocated";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_NODES_DESTROYED = "NodesDestroyed";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_NEW_ROOT_CREATED_NUM = "NewRootCreatedNum";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_CHILD_NODES_REFILLED = "ChildNodesRefilled";
      static constexpr const CHAR* BTREE_STATISTICS_FIELDNAME_NODES_COMPRESSED_TIMES = "NodesCompressedTimes";

      /// real time
      UINT32 nonleafNodeNum = 0;
      UINT32 leafNodeNum = 0;
      UINT32 compressedNodeNum = 0;
      UINT64 totalEntryNum = 0;
      UINT64 compressedEntryNum = 0;
      UINT64 origTotalEntrySize = 0;
      UINT64 realTotalEntrySize = 0;
      UINT64 totalPrefixNum = 0;

      /// history
      UINT64 totalEntryInserted = 0;
      UINT64 totalEntryRemoved = 0;
      UINT64 nodesAllocated = 0;
      UINT64 nodesDestroyed = 0;
      UINT32 newRootCreatedNum = 0;
      UINT64 childNodesRefilled = 0;
      UINT64 nodesCompressedTimes = 0;
   }; // struct btreeStatistics
} // namespace vessel

} // namespace engine

#endif // VESSEL_BTREE_STATISTICS_H_
