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

   Source File Name = blockBasedMemPool.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BLOCK_BASED_MEM_POOL_H_
#define VESSEL_BLOCK_BASED_MEM_POOL_H_


#include "vessel/unitedBitmap.hpp"
#include "ossMemPool.hpp"
#include "vessel/memoryBlock.h"

///c++11
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace engine
{
namespace vessel
{
   class blockBasedMemPool : public SDBObject
   {
      public:
         blockBasedMemPool();
         ~blockBasedMemPool();
         blockBasedMemPool(const blockBasedMemPool &) = delete;
         blockBasedMemPool &operator=(const blockBasedMemPool &) = delete;

      public:
         class memBlock : public SDBObject
         {
            friend class blockBasedMemPool;
            public:
               memBlock(){}
               ~memBlock(){}
               memBlock(const memBlock &o):
               _blockId(o._blockId),
               _buf(o._buf)
               {}
               memBlock &operator=(const memBlock &o)
               {
                  _blockId = o._blockId;
                  _buf = o._buf;
                  return *this;
               }

            public:
               OSS_INLINE BOOLEAN isValid()const
               {
                  return 0 <= _blockId &&
                         nullptr != _buf;
               }

               OSS_INLINE CHAR *getBuffer()const {return _buf;}
               OSS_INLINE INT32 getBlockId()const {return _blockId;}
               OSS_INLINE void reset()
               {
                  _blockId = -1;
                  _buf = nullptr;
               }

            private:
               void set(INT32 blockId, CHAR *buf)
               {
                  _blockId = blockId;
                  _buf = buf;
               }

            private:
               INT32 _blockId = -1;
               CHAR *_buf = nullptr;
         };//class memBlock


         class sharedMemBlock : public SDBObject
         {
            friend class blockBasedMemPool;
            public:
               sharedMemBlock(){}
               explicit sharedMemBlock(blockBasedMemPool *pool,
                                       const memBlock &b);
               ~sharedMemBlock();
               sharedMemBlock(sharedMemBlock &&);
               sharedMemBlock &operator=(sharedMemBlock &&);
               sharedMemBlock(const sharedMemBlock &) = delete;
               sharedMemBlock &operator=(const sharedMemBlock &) = delete;

            public:
               OSS_INLINE BOOLEAN isValid()const
               {
                  return nullptr != _pool && _block.isValid();
               }
               OSS_INLINE const memBlock &getBlock()const
               {
                  return _block;
               }
               OSS_INLINE UINT32 getBlockSize()const
               {
                  return nullptr == _pool ? 0 : _pool->getBlockSize();
               }

            private:
               void clear();

            private:
               blockBasedMemPool *_pool = nullptr;
               memBlock _block;
         };//class sharedMemBlock

         typedef std::shared_ptr<sharedMemBlock> sharedMemBlockPtr;

      private:
         static constexpr UINT32 _CHUNK_CAPACITY = 512;
         typedef ossPoolVector<memoryBlock> _CHUNK_VEC;

         OSS_INLINE INT32 getChunkId(INT32 blockId)const
         {
            return blockId / _CHUNK_CAPACITY;
         }
         OSS_INLINE UINT32 getChunkMemSize()const
         {
            return _blockSize * _CHUNK_CAPACITY;
         }
         OSS_INLINE UINT32 getOffsetInChunk(INT32 blockId)const
         {
            return _blockSize * (blockId % _CHUNK_CAPACITY);
         }

      public:
         OSS_INLINE BOOLEAN isValid()const {return 0 < _blockSize;}
         OSS_INLINE UINT32 getTotalBlockNum()const
         {
            return _chunks.size() * _CHUNK_CAPACITY;
         }
         OSS_INLINE UINT32 getBlockSize()const {return _blockSize;}
         OSS_INLINE UINT64 getMaxMemCapacity()const
         {
            return static_cast<UINT64>(_blockSize) * getTotalBlockNum();
         }
         OSS_INLINE UINT32 getBlockAllocated()const
         {
            return _blockAllocated.load(std::memory_order_relaxed);
         }
         OSS_INLINE UINT64 getTotalSizeAllocated()const
         {
            return _blockSize * _blockAllocated.load(std::memory_order_relaxed);
         }
         
         INT32 init(UINT32 maxChunk, UINT32 blockSize=65536);
         void fini();

         INT32 allocate(memBlock &mb);
         INT32 allocate(UINT32 blockNum, memBlock *blocks);

         INT32 allocateSharedBlock(sharedMemBlockPtr &out);

         void release(memBlock &mb);
         void release(UINT32 blockNum, memBlock *blocks);

      private:
         INT32 ensureMemChunk(INT32 chunkId);

         INT32 _allocate(UINT32 blockNum, memBlock *blocks);

         void _release(UINT32 blockNum, memBlock *blocks);

      private:
         INT32 _tryToAllocate(UINT32 blockNum,
                              memBlock *blocks,
                              UINT32 &allocated);

      private:
         typedef class ossPoolList<std::condition_variable *> _WAITING_LIST;

      private:
         std::mutex _mutex;
         _WAITING_LIST _wl;

         UINT32 _blockSize = 0;
         unitedBitmap<_CHUNK_CAPACITY> _allocator;
         _CHUNK_VEC _chunks;
         std::atomic_uint _blockAllocated = {0};
   };//class blockBasedMemPool
} // namespace vessel

} // namespace engine


#endif//VESSEL_BLOCK_BASED_MEM_POOL_H_