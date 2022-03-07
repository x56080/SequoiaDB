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

   Source File Name = blockBasedMemPool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Drafts

   Last Changed =

******************************************************************************/

#include "vessel/blockBasedMemPool.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/threadContext.h"
#include <chrono>//c++11

namespace engine
{
namespace vessel
{

   blockBasedMemPool::blockBasedMemPool(){}

   blockBasedMemPool::~blockBasedMemPool()
   {
      fini();
   }

   INT32 blockBasedMemPool::init(UINT32 blockSize, UINT32 maxChunk)
   {
      INT32 rc = SDB_OK;
      fini();
      if (0 == blockSize || 0 == maxChunk)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _blockSize = blockSize;
      _allocator.init(maxChunk);
      _chunks.resize(maxChunk);

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
         cv.wait(guard, [this]{return _allocator.hasNonzeroBit();});
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
      if (!_wl.empty() && _allocator.hasNonzeroBit())
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
            goto error;
         }
         
         mb.set(blockId, _blockSize,
               (CHAR *)(_chunks[chunkId].getBuffer()) + getOffsetInChunk(blockId));
         ++allocated;
      }

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

   void blockBasedMemPool::release(memBlock &mb)
   {
      if (isValid())
      {
         _release(1, &mb);
      }
      return;
   }

   void blockBasedMemPool::release(UINT32 blockNum, memBlock *blocks)
   {
      if (OSS_LIKELY(isValid() && 0 < blockNum && nullptr != blocks))
      {
         _release(blockNum, blocks);
      }

      return;
   }

   void blockBasedMemPool::_release(UINT32 blockNum, memBlock *blocks)
   {
      SDB_ASSERT(0 < blockNum && nullptr != blocks, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");

      std::unique_lock<std::mutex> guard(_mutex);

      for (UINT32 i = 0; i < blockNum; ++i)
      {
         memBlock &block = blocks[i];
         if (block.isValid() &&
             block.getBlockId() < (INT32)getTotalBlockNum())
         {
            BOOLEAN oldValue = FALSE;
            _allocator.set(block.getBlockId(), &oldValue);
            SDB_ASSERT(!oldValue, "nonzero bit to be released");
         }

         block.reset();
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
      if (block.isEmpty())
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

