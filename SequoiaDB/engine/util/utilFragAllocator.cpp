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

   Source File Name = utilFragAllocator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilFragAllocator.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
   _utilFragAllocator::_utilFragAllocator(const options &o):
   _o(o)
   {
      SDB_ASSERT(_o.defaultBlockSize <= MAX_BLOCK_SIZE, "out of bound");
   }

   void _utilFragAllocator::reset()
   {
      _rep.reset();
      _o = options();
   }

   void _utilFragAllocator::resetBlocks()
   {
      _rep.reset();
   }

   _utilFragAllocator::repository _utilFragAllocator::reap()
   {
      return std::move(_rep);
   }

   void *_utilFragAllocator::malloc(size_t size)
   {
      SDB_ASSERT(0 < size && size <= MAX_BLOCK_SIZE, "can not be invalid");
      if (_isExclusiveBlock(size))
      {
         return _allocateExclusiveBlock(size);
      }
      else
      {
         return _allocateFromSharedList(size);
      }
   }

   void _utilFragAllocator::free(void *p)
   {
      SDB_ASSERT(nullptr != p, "can not be invalid");
      if (_findAndFree(_rep._sl, p))
      {
         return;
      }
      if (_findAndFree(_rep._el, p))
      {
         return;
      }
      SDB_ASSERT(FALSE, "invalid mem to free");
      return;
   }

   void *_utilFragAllocator::realloc(void *p, size_t size)
   {
      SDB_ASSERT(FALSE, "not supported yet");
      return nullptr;
   }

   BOOLEAN _utilFragAllocator::isMovable()const
   {
      return FALSE;
   }

   BOOLEAN _utilFragAllocator::isMovable(const void *p)const
   {
      return FALSE;
   }

   void *_utilFragAllocator::allocateExclusiveBlock(UINT32 size)
   {
      SDB_ASSERT(0 < size, "can not be invalid");
      return _allocateExclusiveBlock(size);
   }

   _utilFragAllocator::memBlockNode *_utilFragAllocator::_allocateExclusiveNode(size_t blockSize)
   {
      SDB_ASSERT(0 < blockSize, "can not be invalid");
      size_t size = MB_NODE_SIZE + blockSize;
      memBlockNode *node = (memBlockNode *)SDB_THREAD_ALLOC(size);
      if (OSS_LIKELY(nullptr != node))
      {
         node->init();
         node->blockSize = blockSize;
         node->offset = blockSize;
         node->refCount = 1;
         _rep._el.pushBack(node);
      }

      return node;
   }

   CHAR *_utilFragAllocator::_getMemBlock(memBlockNode *node)
   {
      SDB_ASSERT(nullptr != node && 0 < node->blockSize, "can not be invalid");
      CHAR *p = reinterpret_cast<CHAR *>(node);
      return p + MB_NODE_SIZE;
   }

   _utilFragAllocator::memBlockNode *_utilFragAllocator::_allocateSharedNode()
   {
      UINT32 size = _o.defaultBlockSize + MB_NODE_SIZE;
      memBlockNode *node = (memBlockNode *)SDB_THREAD_ALLOC(size);
      if (OSS_LIKELY(nullptr != node))
      {
         node->init();
         node->blockSize = _o.defaultBlockSize;
         _rep._sl.pushBack(node);
      }

      return node;
   }

   void *_utilFragAllocator::_allocateExclusiveBlock(size_t size)
   {
      memBlockNode *node = _allocateExclusiveNode(size);
      if (OSS_LIKELY(nullptr != node))
      {
         return _getMemBlock(node);
      }
      else
      {
         return nullptr;
      }
   }

   void *_utilFragAllocator::_allocateFromSharedList(UINT32 size)
   {
      CHAR *out = nullptr;
      if (_rep._sl.isEmpty())
      {
         memBlockNode *node = _allocateSharedNode();
         if (nullptr == node)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            goto done;
         }
      }

      do
      {
         out = _allocateFromSharedNode(_rep._sl.getBack(), size);
         if (nullptr != out)
         {
            break;
         }
         else
         {
            memBlockNode *node = _allocateSharedNode();
            if (nullptr == node)
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               goto done;
            }
         }
      } while (TRUE);
      

   done:
      return out;
   }

   CHAR *_utilFragAllocator::_allocateFromSharedNode(memBlockNode *node, UINT32 size)
   {
      SDB_ASSERT(nullptr != node, "can not be invalid");
      SDB_ASSERT(0 < size, "can not be invalid");
      CHAR *out = nullptr;

      if (size <= node->getFreeSize())
      {
         vessel::strictBuffer buffer;
         buffer.makeWritable(node->blockSize, _getMemBlock(node));
         out = buffer.getWritablePtrWithoutSize(node->offset);
         node->offset += size;
         ++node->refCount;
      }

      return out;
   }

   BOOLEAN _utilFragAllocator::_findAndFree(MB_NODE_LIST &l, void *p)
   {
      BOOLEAN r = FALSE;
      memBlockNode *node = l.getFront();
      while (nullptr != node)
      {
         if (_isBufferBelongToNode(p, node))
         {
            SDB_ASSERT(0 < node->refCount, "invalid ref count");
            --node->refCount;
            if (0 == node->refCount)
            {
               l.erase(node);
            }
            r = TRUE;
            break;
         }

         node = l.getNext(node);
      }

      return r;
   }

   BOOLEAN _utilFragAllocator::_isBufferBelongToNode(const void *p,
                                                     const memBlockNode *node)const
   {
      const CHAR *start = reinterpret_cast<const CHAR *>(node) + MB_NODE_SIZE;
      const CHAR *end = start + node->blockSize;
      const CHAR *ptr = reinterpret_cast<const CHAR *>(p);
      return start <= ptr && ptr < end;
             
   }
} // namespace engine
