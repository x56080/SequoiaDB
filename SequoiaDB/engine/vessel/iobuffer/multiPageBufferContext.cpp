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

   Source File Name = multiPageBufferContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/multiPageBufferContext.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   multiPageBufferContext::multiPageBufferContext(UINT32 pageSize,
                                                  blockBasedMemPool *pool):
   _pageSize(pageSize),
   _pool(pool)
   {
      SDB_ASSERT(nullptr != _pool && _pool->isValid(), "can not be invalid");
      SDB_ASSERT(0 < _pageSize && _pageSize <= _pool->getBlockSize(), "can not be invalid");
      SDB_ASSERT(0 == _pool->getBlockSize() % _pageSize, "must be aligned");
   }

   multiPageBufferContext::~multiPageBufferContext()
   {
      
   }

   void multiPageBufferContext::clear()
   {
      _dirtyPids.clear();
      _buffers.clear();
      if (_hasFreeMemBlock())
      {
         _offset = -1;
         _allocator.reset();
      }
   }

   INT32 multiPageBufferContext::makeBufferWritable(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      _blockRef ref;
      PAGE_BUFFER_MAP::const_iterator itr;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (0 < _dirtyPids.count(pid))
      {
         goto done;
      }
      else if (!_hasFreeMemBlock())
      {
         rc = _pool->allocateSharedBlock(_allocator);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new block:%d", rc);
            goto error;
         }
         _offset = 0;
      }

      ref = _blockRef(_allocator, _offset);
      _offset += _pageSize;
      if (_offset == (INT32)_pool->getBlockSize())
      {
         _offset = -1;
         _allocator.reset();
      }

      itr = _buffers.find(pid);
      if (_buffers.cend() != itr)
      {
         const _blockRef &old = itr->second;
         ossMemcpy(ref.getRefBuffer(), old.getRefBuffer(), _pageSize);
      }

      _buffers[pid] = std::move(ref);
      _dirtyPids.insert(pid);
      

   done:
      return rc;
   error:
      goto done;
   }

   strictBuffer multiPageBufferContext::getWritableBuffer(PAGE_ID pid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      strictBuffer buffer;
      if (0 < _dirtyPids.count(pid))
      {
         PAGE_BUFFER_MAP::const_iterator itr = _buffers.find(pid);
         SDB_ASSERT(_buffers.cend() != itr, "impossible");
         buffer.makeWritable(_pageSize, itr->second.getRefBuffer());
      }

      return buffer;
   }

   BOOLEAN multiPageBufferContext::contains(PAGE_ID pid)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      return 0 != _buffers.count(pid);
   }

   strictBuffer multiPageBufferContext::getReadbleBuffer(PAGE_ID pid)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      strictBuffer buffer;
      PAGE_BUFFER_MAP::const_iterator itr = _buffers.find(pid);
      if (_buffers.cend() != itr)
      {
         buffer.reset(_pageSize, itr->second.getRefBuffer());
      }

      return buffer;
   }

   void multiPageBufferContext::copyBuffers(const multiPageBufferContext &o)
   {
      SDB_ASSERT(_pageSize == o._pageSize, "must be same");
      SDB_ASSERT(_pool == o._pool, "must be same");
      clear();

      _buffers = o._buffers;
      return;
   }
} // namespace vessel

} // namespace engine
