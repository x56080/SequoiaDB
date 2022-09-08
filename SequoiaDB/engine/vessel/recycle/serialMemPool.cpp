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

   Source File Name = serialMemPool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/serialMemPool.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 EXCLUSIVE_BUFFER_SIZE = 1024;

   serialMemPool::serialMemPool(const options &o):
   _o(o)
   {
      SDB_ASSERT(64 <= _o.defaultBlockSize, "can not be too small");
      if (nullptr != o.outerMemBlock &&
          MIN_OUTER_BUFFER_SIZE < o.outerMemBlockSize)
      {
         _appendToWorkingList(o.outerMemBlockSize, o.outerMemBlock);
      }
   }

   serialMemPool::~serialMemPool()
   {
      reset();
   }

   void serialMemPool::reset()
   {
      while (nullptr != _exclusiveBlocks)
      {
         memoryBlockListNode *node = _exclusiveBlocks->pre;
         SDB_THREAD_FREE(_exclusiveBlocks);
         _exclusiveBlocks = node;
      }

      while (nullptr != _workingBlocks)
      {
         memoryBlockListNode *node = _workingBlocks->pre;
         if (!_isOuterBlock(_workingBlocks))
         {
            SDB_THREAD_FREE(_workingBlocks);
         }
         _workingBlocks = reinterpret_cast<sharedMemoryBlock *>(node);
      }

      while (nullptr != _freeBlocks)
      {
         memoryBlockListNode *node = _freeBlocks->pre;
         if (!_isOuterBlock(_freeBlocks))
         {
            SDB_THREAD_FREE(_freeBlocks);
         }
         _freeBlocks = reinterpret_cast<sharedMemoryBlock *>(node);
      }

      _o = options();
   }


   void *serialMemPool::allocate(UINT32 size)
   {
      SDB_ASSERT(0 < size, "can not be invalid");
      SDB_ASSERT(size <= memoryItem::MEM_SIZE_MASK, "out of max size");

      CHAR *out = _allocateFromWorkingBlock(size);
      if (nullptr != out)
      {
         goto done;
      }
      else if ((_o.defaultBlockSize / 2) < size)
      {
         out = _allocateFromExclusiveList(size);
      }
      else
      {
         INT32 rc = _allocateNewWorkingBlock();
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to allocate new working block:%d", rc);
            goto done;
         }
         else
         {
            out = _allocateFromWorkingBlock(size);
            SDB_ASSERT(nullptr != out, "can not be invalid");
         }
      }

   done:
      return (void *)out;
   }

   void serialMemPool::deallocate(void *p)
   {
      if (nullptr != p)
      {
         if (_deallocateFromWorkingBlocks((CHAR *)p))
         {
            goto done;
         }
      }
   
   done:
      return;
   }

   serialMemPool::sharedMemoryBlock *serialMemPool::_appendToWorkingList(UINT32 size,
                                                                         void *buffer)
   {
      SDB_ASSERT(MIN_OUTER_BUFFER_SIZE < size, "can not be invalid");
      SDB_ASSERT(nullptr != buffer, "can not be invalid");

      sharedMemoryBlock *block = reinterpret_cast<sharedMemoryBlock *>(buffer);
      block->reset();
      block->set(_workingBlocks, size);
      _workingBlocks = block;
      return block;
   }

   serialMemPool::sharedMemoryBlock *serialMemPool::_appendToFreeList(UINT32 size,
                                                                      void *buffer)
   {
      SDB_ASSERT(MIN_OUTER_BUFFER_SIZE < size, "can not be invalid");
      SDB_ASSERT(nullptr != buffer, "can not be invalid");

      sharedMemoryBlock *block = reinterpret_cast<sharedMemoryBlock *>(buffer);
      block->reset();
      block->set(_freeBlocks, size);
      _freeBlocks = block;
      return block;
   }

   CHAR *serialMemPool::_allocateFromWorkingBlock(UINT32 size)
   {
      SDB_ASSERT(nullptr != _workingBlocks, "can not be invalid");
      CHAR *out = nullptr;
      UINT32 backOffset = 0;
      if (_reserveFromBlock(_workingBlocks, size, backOffset))
      {
         strictBuffer buffer;
         buffer.makeWritable(_workingBlocks->size, _workingBlocks);
         UINT32 frontOffset = SHARED_MB_SIZE + (_workingBlocks->itemNum * MEMORY_ITEM_SIZE);
         memoryItem *item = buffer.getWritableObjPtr<memoryItem>(frontOffset);
         item->offset = backOffset;
         item->size = size;
         out = buffer.getWritablePtrWithoutSize(backOffset);
      }

      return out;
   }

   BOOLEAN serialMemPool::_reserveFromBlock(const sharedMemoryBlock *block,
                                            UINT32 size,
                                            UINT32 &offset)const
   {
      SDB_ASSERT(nullptr != block, "can not be invalid");
      strictBuffer buffer(block->size, block);
      UINT32 itemSize = _getItemMemSize(size);
      UINT32 frontOffset = SHARED_MB_SIZE + (block->itemNum * MEMORY_ITEM_SIZE);
      UINT32 backOffset = 0;
      if (0 == block->itemNum)
      {
         backOffset = block->size;
      }
      else
      {
         UINT32 lastItemOffset = frontOffset - MEMORY_ITEM_SIZE;
         const memoryItem *item = buffer.getReadableObjPtr<memoryItem>(lastItemOffset);
         backOffset = item->offset;
      }

      if ((frontOffset + itemSize) <= backOffset)
      {
         offset = backOffset - size;
         return TRUE;
      }
      else
      {
         return FALSE;
      }
   }

   CHAR *serialMemPool::_allocateFromExclusiveList(UINT32 size)
   {
      UINT32 bufferSize = size + MB_LIST_NODE_SIZE;
      CHAR *buffer = (CHAR *)SDB_THREAD_ALLOC(bufferSize);
      if (nullptr != buffer)
      {
         memoryBlockListNode *node = reinterpret_cast<memoryBlockListNode *>(buffer);
         node->set(_exclusiveBlocks, bufferSize);
         _exclusiveBlocks = node;
         buffer = buffer + MB_LIST_NODE_SIZE;
      }

      return buffer;
   }

   BOOLEAN serialMemPool::_isBufferFromNode(const CHAR *buffer,
                                            const memoryBlockListNode *node)const
   {
      SDB_ASSERT(nullptr != buffer, "can not be invalid");
      SDB_ASSERT(nullptr != node, "can not be invalid");
      const CHAR *startBuffer = reinterpret_cast<const CHAR *>(node);
      return startBuffer <= buffer && buffer < (startBuffer + node->size);
   }

   BOOLEAN serialMemPool::_deallocateFromWorkingBlocks(CHAR *buffer)
   {
      SDB_ASSERT(nullptr != buffer, "can not be invalid");
      BOOLEAN r = FALSE;
      sharedMemoryBlock *next = nullptr;
      sharedMemoryBlock *block = _workingBlocks;
      while (nullptr != block)
      {
         if (_isBufferFromNode(buffer, block))
         {
            _deallocateFromBlock(block, buffer);
            if (0 == block->itemNum)
            {
               _moveToFreeList(block, next);
            }

            r = TRUE;
            break;
         }
      }

      return r;
   }

   void serialMemPool::_deallocateFromBlock(sharedMemoryBlock *node,
                                            CHAR *buffer)
   {
      CHAR *nodeBuffer = reinterpret_cast<CHAR*>(node);
      if (OSS_UNLIKELY(0 < node->itemNum && nodeBuffer <= buffer))
      {
         BOOLEAN found = FALSE;
         UINT32 offset = (UINT32)(buffer - nodeBuffer);
         memoryItem *item = reinterpret_cast<memoryItem*>(
                            nodeBuffer + SHARED_MB_SIZE +
                            ((node->itemNum - 1) * MEMORY_ITEM_SIZE));
         for (UINT32 i = node->itemNum; 0 < i; --i)
         {
            if (offset != item->offset)
            {
               --item;
               continue;
            }
            else if (i == node->itemNum)
            {
               --node->itemNum;
               found = TRUE;
               break;
            }
            else
            {
               UINT32 moveSize = (node->itemNum - i) * MEMORY_ITEM_SIZE;
               ossMemmove(item, item + 1, moveSize);
               --node->itemNum;
               found = TRUE;
               break;
            }
         }

         SDB_ASSERT(found, "buffer to deallocate not found");
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid buffer to deallocate");
      }
      return;
   }

   BOOLEAN serialMemPool::_isOuterBlock(const sharedMemoryBlock *block)const
   {
      SDB_ASSERT(nullptr != block, "can not be invalid");
      const CHAR *buffer = reinterpret_cast<const CHAR *>(block);
      return buffer == _o.outerMemBlock;
   }
} // namespace vessel

} // namespace engine
