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

   Source File Name = btreeStatistics.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/31/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/btreeStatistics.h"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   void btreeStatistics::insertCompressedIndex(UINT32 optimizedSize, UINT32 remainSize)
   {
      ++totalEntryNum;
      ++compressedEntryNum;
      origTotalEntrySize += optimizedSize + remainSize;
      realTotalEntrySize += remainSize;
      ++totalEntryInserted;
   }

   void btreeStatistics::insertUncompressedIndex(UINT32 size)
   {
      ++totalEntryNum;
      origTotalEntrySize += size;
      realTotalEntrySize += size;
      ++totalEntryInserted;
   }

   void btreeStatistics::removeCompressedIndex(UINT32 optimizedSize, UINT32 remainSize)
   {
      --totalEntryNum;
      origTotalEntrySize -= optimizedSize + remainSize;
      realTotalEntrySize -= remainSize;
      ++totalEntryRemoved;
   }

   void btreeStatistics::removeUncompressedIndex(UINT32 size)
   {
      --totalEntryNum;
      origTotalEntrySize -= size;
      realTotalEntrySize -= size;
      ++totalEntryRemoved;
   }
   
   void btreeStatistics::removeMarkedDeletedIndexes(UINT32 num)
   {
      --totalEntryNum;
      ++totalEntryRemoved;
   }

   void btreeStatistics::releaseMarkedDeletedIndexSpace(UINT32 size)
   {
      origTotalEntrySize -= size;
      realTotalEntrySize -= size;
   }

   void btreeStatistics::recompress(UINT32 newCompressedEntryNum,
                                    UINT64 newRealTotalEntrySize,
                                    UINT32 newPrefixNum,
                                    UINT32 oldCompressedEntryNum,
                                    UINT64 oldRealTotalEntrySize,
                                    UINT32 oldPrefixNum)
   {
      SDB_ASSERT(compressedEntryNum >= oldCompressedEntryNum,
                 "must be equal or greater");
      compressedEntryNum -= oldCompressedEntryNum;
      compressedEntryNum += newCompressedEntryNum;

      SDB_ASSERT(realTotalEntrySize >= oldRealTotalEntrySize, "must be equal or greater");
      realTotalEntrySize -= oldRealTotalEntrySize;
      realTotalEntrySize += newRealTotalEntrySize;
      
      SDB_ASSERT(totalPrefixNum >= oldPrefixNum, "must be equal or greater");
      totalPrefixNum -= oldPrefixNum;
      totalPrefixNum += newPrefixNum;
      
      if (0 == oldPrefixNum)
      {
         ++compressedNodeNum;
      }
      ++nodesCompressedTimes;
   }

   void btreeStatistics::addLeafNode(BOOLEAN isCompressed)
   {
      if(isCompressed)
      {
         ++compressedNodeNum;
      }
      ++leafNodeNum;
   }

   void btreeStatistics::removeLeafNode(BOOLEAN isCompressed)
   {
      if(isCompressed)
      {
         --compressedNodeNum;
      }
      --leafNodeNum;
   }

   void btreeStatistics::addNonLeafNode()
   {
      ++nonleafNodeNum;
   }

   void btreeStatistics::removeNonLeafNode()
   {
      --nonleafNodeNum;
   }

   void btreeStatistics::allocateNode()
   {
      ++nodesAllocated;
   }

   void btreeStatistics::destroyNode()
   {
      ++nodesDestroyed;
   }

   void btreeStatistics::createNewRoot()
   {
      ++newRootCreatedNum;
   }

   void btreeStatistics::refillChildNode()
   {
      addLeafNode(FALSE);
      ++childNodesRefilled;
   }

   void btreeStatistics::truncateTree()
   {
      totalEntryRemoved += totalEntryNum;

      nonleafNodeNum = 0;
      leafNodeNum = 0;
      compressedNodeNum = 0;
      totalEntryNum = 0;
      compressedEntryNum = 0;
      origTotalEntrySize = 0;
      realTotalEntrySize = 0;
      totalPrefixNum = 0;
   }

   void btreeStatistics::truncate(UINT32 newTotalEntryNum,
                                  UINT32 newCompressedEntryNum,
                                  UINT32 oldTotalEntryNum,
                                  UINT32 oldCompressedEntryNum,
                                  UINT64 origTotalEntrySizeDiff,
                                  UINT64 realTotalEntrySizeDiff)
   {
      SDB_ASSERT(newTotalEntryNum <= oldTotalEntryNum,
                 "must be equal or greater");
      SDB_ASSERT(totalEntryNum >= oldTotalEntryNum, "must be equal or greater");
      totalEntryNum -= oldTotalEntryNum;
      totalEntryNum += newTotalEntryNum;
      SDB_ASSERT(compressedEntryNum >= oldCompressedEntryNum,
                 "must be equal or greater");
      compressedEntryNum -= oldCompressedEntryNum;
      compressedEntryNum += newCompressedEntryNum;
      SDB_ASSERT(origTotalEntrySize >= origTotalEntrySizeDiff,
                 "must be equal or greater");
      origTotalEntrySize -= origTotalEntrySizeDiff;
      SDB_ASSERT(realTotalEntrySize >= realTotalEntrySizeDiff,
                 "must be equal or greater");
      realTotalEntrySize -= realTotalEntrySizeDiff;
   }

   void btreeStatistics::compact(UINT64 realTotalEntrySizeDiff,
                                 UINT32 newPrefixNum,
                                 UINT32 oldPrefixNum)
   {
      SDB_ASSERT(realTotalEntrySize >= realTotalEntrySizeDiff,
                 "must be equal or greater");
      realTotalEntrySize -= realTotalEntrySizeDiff;
      SDB_ASSERT(totalPrefixNum >= oldPrefixNum, "must be equal or greater");
      totalPrefixNum -= oldPrefixNum;
      totalPrefixNum += newPrefixNum;
   }

   void btreeStatistics::split(UINT32 newTotalEntryNum,
                               UINT32 newCompressedEntryNum,
                               UINT64 newOrigTotalEntrySize,
                               UINT64 newRealTotalEntrySize,
                               UINT32 prefixNum)
   {
      totalEntryNum += newTotalEntryNum;
      compressedEntryNum += newCompressedEntryNum;
      origTotalEntrySize += newOrigTotalEntrySize;
      realTotalEntrySize += newRealTotalEntrySize;
      totalPrefixNum += prefixNum;
   }

   bson::BSONObj btreeStatistics::toBSON() const
   {
      bson::BSONObjBuilder builder;
      builder.append(BTREE_STATISTICE_FIELDNAME_NON_LEAF_NODE_NUM, nonleafNodeNum);
      builder.append(BTREE_STATISTICE_FIELDNAME_LEAF_NODE_NUM , leafNodeNum);
      builder.append(BTREE_STATISTICE_FIELDNAME_COMPRESSED_NODE_NUM , compressedNodeNum);
      builder.appendIntOrLL(BTREE_STATISTICE_FIELDNAME_TOTAL_ENTRY_NUM , totalEntryNum);
      builder.appendIntOrLL(BTREE_STATISTICE_FIELDNAME_COMPRESSED_ENTRY_NUM , compressedEntryNum);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_ORIG_TOTAL_ENTRY_SIZE , origTotalEntrySize);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_REAL_TOTAL_ENTRY_SIZE , realTotalEntrySize);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_TOTAL_PREFIX_NUM , totalPrefixNum);
      FLOAT64 entryCompressionRatio =
          FLOAT64(origTotalEntrySize - realTotalEntrySize) /
          FLOAT64(origTotalEntrySize);
      builder.append(BTREE_STATISTICS_FIELDNAME_ENTRY_COMPRESSION_RATIO , entryCompressionRatio);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_TOTAL_ENTRY_INSERTED , totalEntryInserted);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_TOTAL_ENTRY_REMOVED , totalEntryRemoved);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_NODES_ALLOCATED , nodesAllocated);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_NODES_DESTROYED , nodesDestroyed);
      builder.append(BTREE_STATISTICS_FIELDNAME_NEW_ROOT_CREATED_NUM , newRootCreatedNum);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_CHILD_NODES_REFILLED , childNodesRefilled);
      builder.appendIntOrLL(BTREE_STATISTICS_FIELDNAME_NODES_COMPRESSED_TIMES , nodesCompressedTimes);
      return builder.obj();
   }
}
} // namespace engine