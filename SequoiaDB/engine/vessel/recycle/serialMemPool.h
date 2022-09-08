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

   Source File Name = serialMemPool.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SERIAL_MEM_POOL_H_
#define VESSEL_SERIAL_MEM_POOL_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"

namespace engine
{
namespace vessel
{

#pragma pack(4)

   class serialMemPool : public SDBObject
   {
      public:
         struct options
         {
            options() = default;
            explicit options(UINT32 blokSize,
                             CHAR *outerBuffer,
                             UINT32 outerBufferSize):
            defaultBlockSize(blokSize),
            outerMemBlock(outerBuffer),
            outerMemBlockSize(outerBufferSize)
            {

            }

            UINT32 defaultBlockSize = 4096;
            CHAR *outerMemBlock = nullptr;
            UINT32 outerMemBlockSize = 0;
         };//struct options

      public:
         serialMemPool() = default;
         explicit serialMemPool(const options &);
         ~serialMemPool();
         serialMemPool(const serialMemPool &) = delete;
         serialMemPool &operator=(const serialMemPool &) = delete;

      public:
         void *allocate(UINT32 size);
         void deallocate(void *p);
         void reset();

      private:
         
         struct memoryItem
         {
            memoryItem() = default;

            OSS_INLINE void reset()
            {
               size = 0;
               offset = 0;
            }

            UINT32 size = 0;
            UINT32 offset = 0;
         };
         static constexpr UINT32 MEMORY_ITEM_SIZE = sizeof(memoryItem);

         struct memoryBlockListNode
         {
            OSS_INLINE void reset()
            {
               pre = nullptr;
               size = 0;
            }
            OSS_INLINE void set(memoryBlockListNode *pre,
                                UINT32 size)
            {
               this->pre = pre;
               this->size = size;
            }

            memoryBlockListNode *pre = nullptr;
            UINT32 size = 0;
         };
         static constexpr UINT32 MB_LIST_NODE_SIZE = sizeof(memoryBlockListNode);

         struct sharedMemoryBlock : public memoryBlockListNode
         {  
            ~sharedMemoryBlock() = delete;

            OSS_INLINE void reset()
            {
               itemNum = 0;
               memoryBlockListNode::reset();
            }

            UINT32 itemNum = 0;
         };//class sharedMemoryBlock
         static constexpr UINT32 SHARED_MB_SIZE = sizeof(sharedMemoryBlock);
         static_assert(16 == SHARED_MB_SIZE, "must be 16"); 


         static constexpr UINT32 MIN_OUTER_BUFFER_SIZE = SHARED_MB_SIZE + MEMORY_ITEM_SIZE;

      private:
         OSS_INLINE UINT32 _getItemMemSize(UINT32 size)const
         {
            return MEMORY_ITEM_SIZE + size;
         }

         sharedMemoryBlock *_appendToWorkingList(UINT32 size, void *buffer);
         sharedMemoryBlock *_appendToFreeList(UINT32 size, void *buffer);
         INT32 _allocateNewWorkingBlock();
         CHAR *_allocateFromWorkingBlock(UINT32 size);
         BOOLEAN _reserveFromBlock(const sharedMemoryBlock *block,
                                   UINT32 size,
                                   UINT32 &offset)const;

         CHAR *_allocateFromExclusiveList(UINT32 size);

         BOOLEAN _isBufferFromNode(const CHAR *buffer,
                                   const memoryBlockListNode *node)const;

         BOOLEAN _deallocateFromWorkingBlocks(CHAR *buffer);
         void _deallocateFromBlock(sharedMemoryBlock *node,
                                   CHAR *buffer);

         BOOLEAN _isOuterBlock(const sharedMemoryBlock *block)const;
         void _moveToFreeList(sharedMemoryBlock *current,
                              sharedMemoryBlock *next);

      private:
         options _o;
         memoryBlockListNode *_exclusiveBlocks = nullptr;
         sharedMemoryBlock *_workingBlocks = nullptr;
         sharedMemoryBlock *_freeBlocks = nullptr;
   };//class serialMemPool

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_SERIAL_MEM_POOL_H_