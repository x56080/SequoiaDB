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

#include "vessel/fixedBitmap.hpp"
#include "ossMemPool.hpp"
#include "vessel/memoryBlock.h"

///c++11
#include <mutex>
#include <condition_variable>

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
               _size(o._size),
               _buf(o._buf)
               {}
               memBlock &operator=(const memBlock &o)
               {
                  _blockId = o._blockId;
                  _size = o._size;
                  _buf = o._buf;
                  return *this;
               }

            public:
               OSS_INLINE BOOLEAN isValid()const
               {
                  return 0 <= _blockId &&
                         0 < _size &&
                         nullptr != _buf;
               }
               OSS_INLINE UINT32 getSize()const {return _size;}
               OSS_INLINE CHAR *getBuffer(){return _buf;}
               OSS_INLINE INT32 getBlockId()const {return _blockId;}
               OSS_INLINE void reset()
               {
                  _blockId = -1;
                  _size = 0;
                  _buf = nullptr;
               }

            private:
               void set(INT32 blockId, UINT32 size, CHAR *buf)
               {
                  _blockId = blockId;
                  _size = size;
                  _buf = buf;
               }

            private:
               INT32 _blockId = -1;
               UINT32 _size = 0;
               CHAR *_buf = nullptr;
         };

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
         INT32 init(UINT32 blockSize, UINT32 maxChunk);
         void fini();

         INT32 allocate(memBlock &mb);
         INT32 allocate(UINT32 blockNum, memBlock *blocks);

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
         fixedBitmap<_CHUNK_CAPACITY> _allocator;
         _CHUNK_VEC _chunks;
   };//class blockBasedMemPool
} // namespace vessel

} // namespace engine


#endif//VESSEL_BLOCK_BASED_MEM_POOL_H_