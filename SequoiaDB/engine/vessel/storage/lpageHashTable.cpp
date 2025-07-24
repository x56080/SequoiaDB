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

   Source File Name = lpageHashTable.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lpageHashTable.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   lpageHashTable::_tableNode::_tableNode()
   {
      ossMemset(keys, 0xFF, sizeof(keys));
   }


/////////////////lpageHashTable
   lpageHashTable::~lpageHashTable()
   {
      clear();
   }

   lpageHashTable::lpageHashTable(lpageHashTable &&o)
   {
      if (!o._buckets.empty())
      {
         UINT32 count = o._count.exchange(0);
         _count.store(count, std::memory_order_relaxed);
         _buckets = std::move(o._buckets);
         o._buckets.clear();
         _nodeCount = o._nodeCount;
         o._nodeCount = 0;
      }
   }

   lpageHashTable &lpageHashTable::operator=(lpageHashTable &&o)
   {
      clear();
      if (!o._buckets.empty())
      {
         UINT32 count = o._count.exchange(0);
         _count.store(count, std::memory_order_relaxed);
         _buckets = std::move(o._buckets);
         _nodeCount = o._nodeCount;
         o._nodeCount = 0;
      }
      return *this;
   }

   void lpageHashTable::clear()
   {
      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         _tableNode *node = _buckets[i];
         while (nullptr != node)
         {
            _tableNode *next = node->next;
            SDB_OSS_DEL node;
            node = next;
         }
      }
      _buckets.clear();
      _count.store(0, std::memory_order_relaxed);
      _nodeCount = 0;
      return;
   }

   UINT64 lpageHashTable::getBufferSize()const
   {
      return (UINT64)_nodeCount * sizeof(_tableNode) +
             _buckets.size() * sizeof(_tableNode *);
   }

   void lpageHashTable::dump(ossPoolMap<PAGE_ID, lpageDescriptor> &m)const
   {
      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         _tableNode *node = _buckets[i];
         while (nullptr != node)
         {
            for (INT32 i = 0; i < (INT32)TABLE_NODE_CAPACITY; ++i)
            {
               PAGE_ID lpid = node->keys[i];
               if (INVALID_PAGE_ID == lpid)
               {
                  SDB_ASSERT(nullptr == node->next, "impossible");
                  break;
               }
               else
               {
                  m[lpid] = node->values[i];
               }
            }

            node = node->next;
         }
      }

      return;
   }

   void lpageHashTable::dump(DELTA_PAGE_LIST &dpl)const
   {
      dpl.reserve(_count.load(std::memory_order_relaxed));
      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         _tableNode *node = _buckets[i];
         while (nullptr != node)
         {
            for (INT32 i = 0; i < (INT32)TABLE_NODE_CAPACITY; ++i)
            {
               PAGE_ID lpid = node->keys[i];
               if (INVALID_PAGE_ID == lpid)
               {
                  SDB_ASSERT(nullptr == node->next, "impossible");
                  break;
               }
               else
               {
                  dpl.push_back(std::make_tuple(lpid, node->values[i].pid,
                                                node->values[i].psv));
               }
            }

            node = node->next;
         }
      }
   }

   INT32 lpageHashTable::set(PAGE_ID lpid, const lpageDescriptor &v)
   {
      INT32 rc = SDB_OK;
      _tableNode *entryNode = nullptr;
      _tableNode *candidate = nullptr;
      BOOLEAN found = FALSE;
      INT32 pos = -1;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      entryNode = ensureBucket(getBucketNumber(lpid));
      if (OSS_UNLIKELY(nullptr == entryNode))
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to init bucket[%d]:%d", getBucketNumber(lpid), rc);
         goto error;
      }

      candidate = seekOrReturnTail(entryNode, lpid, pos, found);
      if (OSS_UNLIKELY(nullptr == candidate))
      {
         PD_LOG(PDERROR, "unexpected error happened");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (found)
      {
         SDB_ASSERT(0 <= pos && pos < (INT32)TABLE_NODE_CAPACITY, "impossible");
         candidate->values[pos] = v;
      }
      else
      {
         rc = atomicAppendToNodeList(candidate, lpid, v, pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert lpid[%d] into table:%d", rc);
            goto error;
         }

         _count.fetch_add(1, std::memory_order_relaxed);
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lpageHashTable::get(PAGE_ID lpid, lpageDescriptor &v)const
   {
      BOOLEAN r = FALSE;
      _tableNode *entry = nullptr;
      _tableNode *candidate = nullptr;
      INT32 pos = -1;
      BOOLEAN found = FALSE;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      v.reset();

      entry = getBucket(getBucketNumber(lpid));
      if (nullptr == entry)
      {
         goto done;
      }

      candidate = seekOrReturnTail(entry, lpid, pos, found);
      SDB_ASSERT(nullptr != candidate, "impossible");
      if (found)
      {
         SDB_ASSERT(0 <= pos && pos < (INT32)TABLE_NODE_CAPACITY, "impossible");
         v = candidate->values[pos];
         r = TRUE;
      }
   done:
      return r;
   }

   INT32 lpageHashTable::atomicAppendToNodeList(_tableNode *node,
                                                PAGE_ID lpid,
                                                const lpageDescriptor &v,
                                                INT32 pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != node, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(pos < (INT32)TABLE_NODE_CAPACITY, "out of bound");

      INT32 possiblePos = pos < 0 ? (INT32)TABLE_NODE_CAPACITY : pos;
      _tableNode *target = node;

      do
      {
         if (nullptr == target->next)
         {
            for (INT32 i = possiblePos; i < (INT32)TABLE_NODE_CAPACITY; ++i)
            {
               if (INVALID_PAGE_ID != target->keys[i])
               {
                  continue;
               }
               else
               {
                  std::atomic_uint *tmp = (std::atomic_uint *)(target->keys + i);
                  PAGE_ID expected = INVALID_PAGE_ID;
                  if (tmp->compare_exchange_strong(expected, lpid, std::memory_order_relaxed))
                  {
                     target->values[i] = v;
                     goto done;
                  }
                  else
                  {
                     continue;
                  }
               }
            }
         }

         /// no free slot in current node.
         possiblePos = 0;
         target = ensureNext(target);
         if (OSS_UNLIKELY(nullptr == target))
         {
            rc = SDB_OOM;
            goto error;
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   lpageHashTable::_tableNode *lpageHashTable::seekOrReturnTail(_tableNode *entryNode,
                                                                PAGE_ID lpid,
                                                                INT32 &pos,
                                                                BOOLEAN &found)const
   {
      SDB_ASSERT(nullptr != entryNode, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      
      _tableNode *node = entryNode;
      found = FALSE;

      do
      {
         pos = -1;
         for (INT32 i = 0; i < (INT32)TABLE_NODE_CAPACITY; ++i)
         {
            if (INVALID_PAGE_ID == node->keys[i])
            {
               pos = i;
               goto done;
            }
            else if (lpid == node->keys[i])
            {
               pos = i;
               found = TRUE;
               goto done;
            }
         }

         if (nullptr == node->next)
         {
            goto done;
         }

         node = node->next;
      } while (TRUE);
      
   done:
      return node;
   }

   lpageHashTable::_tableNode *lpageHashTable::ensureBucket(UINT32 pos)
   {
      SDB_ASSERT(pos < BUCKET_COUNT, "out of bound");
      _tableNode *node = nullptr;
      if (!_buckets.empty() && nullptr != _buckets[pos])
      {
         node = _buckets[pos];
         goto done;
      }
      else
      {
         ossXLatchGuard guard(&_mutex);
         if (_buckets.empty())
         {
            _buckets.resize(BUCKET_COUNT, nullptr);
         }

         if (nullptr == _buckets[pos])
         {
            node = SDB_OSS_NEW _tableNode();
            if (OSS_UNLIKELY(nullptr == node))
            {
               goto done;
            }
            _buckets[pos] = node;
            ++_nodeCount;
         }
         else
         {
            node = _buckets[pos];
         }
      }

   done:
      return node;
   }

   lpageHashTable::_tableNode *lpageHashTable::getBucket(UINT32 pos)const
   {
      SDB_ASSERT(pos < BUCKET_COUNT, "out of bound");
      _tableNode *node = nullptr;
      if (!_buckets.empty() && nullptr != _buckets[pos])
      {
         node = _buckets[pos];
      }
      return node;
   }

   lpageHashTable::_tableNode *lpageHashTable::ensureNext(_tableNode *node)
   {
      SDB_ASSERT(nullptr != node, "can not be null");
      _tableNode *next = nullptr;
      if (nullptr != node->next)
      {
         next = node->next;
         goto done;
      }
      else
      {
         ossXLatchGuard guard(&_mutex);
         if (nullptr == node->next)
         {
            next = SDB_OSS_NEW _tableNode();
            if (OSS_LIKELY(nullptr != next))
            {
               node->next = next;
               ++_nodeCount;
            }
         }
         else
         {
            next = node->next;
         }
      }

   done:
      return next;
   }
} // namespace vessel

} // namespace engine

