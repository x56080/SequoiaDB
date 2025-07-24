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

   Source File Name = lpageHashTable.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPAGE_HASH_TABLE_H_
#define VESSEL_LPAGE_HASH_TABLE_H_

#include "vessel/pageIdentifier.h"
#include "utilPooledObject.hpp"
#include "vessel/vesselIdDef.h"
#include "ossLatch.hpp"
#include "ossMemPool.hpp"
#include "vessel/lpageDescriptor.h"
#include "vessel/deltaPageList.h"
#include <atomic> ///c++11

namespace engine
{
namespace vessel
{
#pragma pack(4)
   /// WARNING: this is not a common hash table, 
   /// only used to save lpage info.
   class lpageHashTable : public SDBObject
   {
      public:
         lpageHashTable(){}
         ~lpageHashTable();
         lpageHashTable(lpageHashTable &&o);
         lpageHashTable &operator=(lpageHashTable &&o);
         lpageHashTable(const lpageHashTable &) = delete;
         lpageHashTable &operator=(const lpageHashTable &) = delete;

      public:
         static constexpr UINT32 BUCKET_COUNT = 8192;

         /// make keys in node can be read in a cache-line.
         static constexpr UINT32 TABLE_NODE_CAPACITY = 16;

      public:
         /// lpid can not be invalid
         /// v can be invalid, which means clear it.
         INT32 set(PAGE_ID lpid, const lpageDescriptor &v);
         BOOLEAN get(PAGE_ID lpid, lpageDescriptor &v)const;
         UINT64 getBufferSize()const;

         OSS_INLINE UINT32 peek()const
         {
            return _count.load(std::memory_order_relaxed);
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _buckets.empty();
         }

      public:
         /// not thread-safe
         void clear();
         void dump(ossPoolMap<PAGE_ID, lpageDescriptor> &m)const;
         void dump(DELTA_PAGE_LIST &dpl)const;

      private:
         struct _tableNode : public _utilPooledObject
         {
            _tableNode();
            PAGE_ID keys[TABLE_NODE_CAPACITY];
            lpageDescriptor values[TABLE_NODE_CAPACITY];
            _tableNode *next = nullptr;
         };//class _tableNode

      private:
         OSS_INLINE UINT32 getBucketNumber(PAGE_ID lpid)const
         {
            static_assert(BUCKET_COUNT == 8192, "must be power of 2");
            return lpid & (BUCKET_COUNT - 1);
         }

         _tableNode *ensureBucket(UINT32 pos);

         _tableNode *getBucket(UINT32 pos)const;

         _tableNode *seekOrReturnTail(_tableNode *entryNode,
                                      PAGE_ID lpid,
                                      INT32 &pos,
                                      BOOLEAN &found)const;

         INT32 atomicAppendToNodeList(_tableNode *node,
                                      PAGE_ID lpid,
                                      const lpageDescriptor &v,
                                      INT32 pos);

         _tableNode *ensureNext(_tableNode *node);
         
         
      private:
         ossSpinXLatch _mutex;
         UINT32 _nodeCount = 0;
         std::atomic_uint _count = {0};
         ossPoolVector<_tableNode *> _buckets;
   };//class lpageHashTable

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_HASH_TABLE_H_
