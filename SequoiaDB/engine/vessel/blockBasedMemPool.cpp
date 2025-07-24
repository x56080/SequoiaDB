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

   Source File Name = blockBasedMemPool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Drafts

   Last Changed =

*******************************************************************************/
#include "vessel/blockBasedMemPool.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/threadContext.h"
#include "utilSharedPtrMaker.hpp"
#include <chrono>//c++11

namespace engine
{
namespace vessel
{
   const UINT32 _MAX_BLOCK_SIZE = 65536;

/////////////////////////////blockBasedMemPool::memBlockGroup
   blockBasedMemPool::sharedMemBlock::sharedMemBlock(sharedMemBlock &&o):
   _pool(o._pool),
   _block(std::move(o._block))
   {
      o._pool = nullptr;
   }

   blockBasedMemPool::sharedMemBlock::sharedMemBlock(blockBasedMemPool *pool,
                                                     const memBlock &b):
   _pool(pool),
   _block(b)
   {
      SDB_ASSERT(isValid(), "must be valid");
   }

   blockBasedMemPool::sharedMemBlock &
   blockBasedMemPool::sharedMemBlock::operator=(blockBasedMemPool::sharedMemBlock &&o)
   {
      clear();
      _pool = o._pool;
      o._pool = nullptr;
      _block = std::move(o._block);
      return *this;
   }

   blockBasedMemPool::sharedMemBlock::~sharedMemBlock()
   {
      clear();
   }

   void blockBasedMemPool::sharedMemBlock::clear()
   {
      if (isValid())
      {
         _pool->release(1, &_block);
         _pool = nullptr;
         _block.reset();
      }
   }

/////////////////////////////blockBasedMemPool::memBlockGroup end

   blockBasedMemPool::blockBasedMemPool(){}

   blockBasedMemPool::~blockBasedMemPool()
   {
      fini();
   }

   INT32 blockBasedMemPool::init(UINT64 maxMemSize, UINT32 blockSize)
   {
      INT32 rc = SDB_OK;
      UINT32 chunkNum = 0;
      UINT64 chunkSize = 0;

      fini();
      if (0 == blockSize ||
          !ossIsPowerOf2(blockSize) ||
          _MAX_BLOCK_SIZE < blockSize ||
          0 == maxMemSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      chunkSize = blockSize * _CHUNK_CAPACITY;
      maxMemSize = ossRoundUpToMultipleX(maxMemSize, chunkSize);
      chunkNum = maxMemSize / chunkSize;

      rc = _allocator.extendUnitNum(chunkNum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init bitmap allocator:%d", rc);
         goto error;
      }

      _blockSize = blockSize;

      try
      {
         _chunks.resize(chunkNum);
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to resize chunk vec:%d", e.what());
         rc = SDB_OOM;
         goto error;
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void blockBasedMemPool::fini()
   {
      SDB_ASSERT(_wl.empty(), "must be empty");
      _blockSize = 0;
      _allocator.fini();
      _chunks.clear();
      _blockAllocated.store(0, std::memory_order_relaxed);
      return;
   }

   INT32 blockBasedMemPool::allocate(memBlock &mb)
   {
      INT32 rc = SDB_OK;

      mb.reset();
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _allocate(1, &mb);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 blockBasedMemPool::allocate(UINT32 blockNum, memBlock *blocks)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == blockNum ||
                            _CHUNK_CAPACITY < blockNum ||
                            nullptr == blocks))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _allocate(blockNum, blocks);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 blockBasedMemPool::allocateSharedBlock(sharedMemBlockPtr &out)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      INT32 rc = SDB_OK;
      memBlock block;
      out.reset();

      rc = _allocate(1, &block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate mem blocks:%d", rc);
         goto error;
      }

      out = makeSharedPtrFromPool<sharedMemBlock>(this, block);
      if (!out)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      if (block.isValid())
      {
         _release(1, &block);
      }
      goto done;
   }

   INT32 blockBasedMemPool::_allocate(UINT32 blockNum, memBlock *blocks)
   {
      INT32 rc = SDB_OK;
      UINT32 allocated = 0;
      std::condition_variable cv;
      std::unique_lock<std::mutex> guard(_mutex);
      _WAITING_LIST::iterator pos = _wl.end();

      if (_wl.empty())
      {
         UINT32 n = 0;
         rc = _tryToAllocate(blockNum, blocks, n);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate buffer:%d", rc);
            goto error;
         }

         if (n == blockNum)
         {
            goto done;
         }

         allocated = n;
      }

      pos = _wl.insert(_wl.end(), &cv);
      do
      {
         cv.wait(guard, [this]{return !_allocator.none();});
         UINT32 n = 0;
         rc = _tryToAllocate(blockNum - allocated, blocks + allocated, n);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate buffer:%d", rc);
            goto error;
         }

         allocated += n;
      } while (allocated < blockNum);

      _wl.erase(pos);
      if (!_wl.empty() && !_allocator.none())
      {
         _wl.front()->notify_one();
      }
      
   done:
      return rc;
   error:
      SDB_ASSERT(guard.owns_lock(), "must own lock");
      if (pos != _wl.end())
      {
         _wl.erase(pos);
      }
      for (UINT32 i = 0; i < allocated; ++i)
      {
         _allocator.set(blocks[i].getBlockId());
         blocks[i].reset();
      }
      if (0 < allocated)
      {
         _blockAllocated.fetch_sub(allocated,
                                   std::memory_order_relaxed);
      }
      goto done;
   }

   INT32 blockBasedMemPool::_tryToAllocate(UINT32 blockNum,
                                           memBlock *blocks,
                                           UINT32 &allocated)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < blockNum && nullptr != blocks, "can not be invalid");
      allocated = 0;

      for (UINT32 i = 0; i < blockNum; ++i)
      {
         INT32 blockId = -1;
         INT32 chunkId = -1;
         memBlock &mb = blocks[i];

         blockId = _allocator.pop();
         if (blockId < 0)
         {
            goto done;
         }

         chunkId = getChunkId(blockId);
         rc = ensureMemChunk(chunkId);
         if (SDB_OK != rc)
         {
            _allocator.set(blockId);
            goto error;
         }
         
         mb.set(blockId,
               (CHAR *)(_chunks[chunkId].getBuffer()) + getOffsetInChunk(blockId));
         ++allocated;
      }

      SDB_ASSERT(0 < allocated, "impossible");
      _blockAllocated.fetch_add(allocated,
                                std::memory_order_relaxed);
   done:
      return rc;
   error:
      for (UINT32 i = 0; i < allocated; ++i)
      {
         _allocator.set(blocks[i].getBlockId());
         blocks[i].reset();
      }
      allocated = 0;
      goto done;
   }

   void blockBasedMemPool::release(const memBlock &mb)
   {
      if (isValid())
      {
         _release(1, &mb);
      }
      return;
   }

   void blockBasedMemPool::release(UINT32 blockNum, const memBlock *blocks)
   {
      if (OSS_LIKELY(isValid() && 0 < blockNum && nullptr != blocks))
      {
         _release(blockNum, blocks);
      }

      return;
   }

   void blockBasedMemPool::_release(UINT32 blockNum, const memBlock *blocks)
   {
      SDB_ASSERT(0 < blockNum && nullptr != blocks, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");

      UINT32 released = 0;
      std::unique_lock<std::mutex> guard(_mutex);

      for (UINT32 i = 0; i < blockNum; ++i)
      {
         const memBlock &block = blocks[i];
         if (block.isValid() &&
             block.getBlockId() < (INT32)getTotalBlockNum())
         {
            BOOLEAN oldValue = FALSE;
            _allocator.set(block.getBlockId(), &oldValue);
            if (OSS_LIKELY(!oldValue))
            {
               ++released;
            }
            else
            {
               SDB_ASSERT(FALSE, "nonzero bit to be released");
            }
         }
      }

      if (0 < released)
      {
         _blockAllocated.fetch_sub(released, std::memory_order_relaxed);
      }

      if (!_wl.empty())
      {
         _wl.front()->notify_one();
      }

      return;
   }

   INT32 blockBasedMemPool::ensureMemChunk(INT32 chunkId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 <= chunkId && chunkId < (INT32)_chunks.size(),
                "can not be invalid");

      memoryBlock &block = _chunks[chunkId];
      if (0 == block.getCapacity())
      {
         rc = block.reserve(getChunkMemSize());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve memory size:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel

} // namespace engine

