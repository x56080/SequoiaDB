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

   Source File Name = multiPageBufferContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_MULTI_PAGE_BUFFER_CONTEXT_H_
#define VESSEL_MULTI_PAGE_BUFFER_CONTEXT_H_

#include "vessel/blockBasedMemPool.h"
#include "ossMemPool.hpp"
#include "vessel/pageIdentifier.h"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   class multiPageBufferContext : public SDBObject
   {
      public:
         multiPageBufferContext(UINT32 pageSize, blockBasedMemPool *pool);
         ~multiPageBufferContext();

         multiPageBufferContext(const multiPageBufferContext &) = delete;
         multiPageBufferContext &operator=(const multiPageBufferContext &) = delete;

      private:
         typedef typename blockBasedMemPool::sharedMemBlockPtr _sharedMemBlockPtr;
         struct _blockRef : public SDBObject
         {
            _blockRef(){}
            explicit _blockRef(_sharedMemBlockPtr &ptr, UINT32 off):
            block(ptr),
            offset(off){}
            
            ~_blockRef(){}
            _blockRef(const _blockRef &o):
            block(o.block),
            offset(o.offset){}
            _blockRef &operator=(const _blockRef &o)
            {
               block = o.block;
               offset = o.offset;
               return *this;
            }
            _blockRef(_blockRef &&o):
            block(std::move(o.block)),
            offset(o.offset)
            {
               o.offset = 0;
            }

            _blockRef &operator=(_blockRef &&o)
            {
               block = std::move(o.block);
               offset = o.offset;
               o.offset = 0;
               return *this;
            }

            BOOLEAN isValid()const
            {
               return block && offset < block->getBlockSize();
            }

            CHAR *getRefBuffer()const
            {
               return isValid() ?
                      ((CHAR *)(block->getBlock().getBuffer()) + offset) : nullptr;
            }

            void reset()
            {
               block.reset();
               offset = 0;
            }

            _sharedMemBlockPtr block;
            UINT32 offset = 0;
         };//struct _blockRef

         typedef ossPoolMap<PAGE_ID, _blockRef> PAGE_BUFFER_MAP;
         typedef ossPoolSet<PAGE_ID> _DIRTY_PID_SET;

      public:
         const ossPoolSet<PAGE_ID> &getDirtyPids()const {return _dirtyPids;}

         OSS_INLINE UINT32 getPageSize()const {return _pageSize;}
         
         OSS_INLINE UINT32 getDirtyBufferSize()const
         {
            return _pageSize * _dirtyPids.size();
         }
         OSS_INLINE UINT32 getDirtyBufferCount()const
         {
            return _dirtyPids.size();
         }
         OSS_INLINE BOOLEAN hasDirtyBuffer()const
         {
            return 0 < _dirtyPids.size();
         }
         OSS_INLINE UINT32 getBufferSize()const
         {
            return (_pageSize * _buffers.size()) +
                   (_hasFreeMemBlock() ? (_pool->getBlockSize() - _offset) : 0);

         }

         void clear();

         BOOLEAN contains(PAGE_ID pid)const;

         strictBuffer getReadbleBuffer(PAGE_ID pid)const;

         INT32 makeBufferWritable(PAGE_ID pid);

         /// make it writable first.
         strictBuffer getWritableBuffer(PAGE_ID pid);

         void copyBuffers(const multiPageBufferContext &o); 

      private:
         CHAR *allocatePageBuffer();


      private:
         BOOLEAN _hasFreeMemBlock()const
         {
            return 0 <= _offset;
         }

      private:
         UINT32 _pageSize = 0;
         blockBasedMemPool *_pool = nullptr;

         ///WARNING: do not modify sub vars without chunk x lock.
         ///         do not release sub vars without recycling flag.
         _DIRTY_PID_SET _dirtyPids;
         PAGE_BUFFER_MAP _buffers;
         INT32 _offset = -1;
         _sharedMemBlockPtr _allocator;
   };//class multiPageBufferContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_MULTI_PAGE_BUFFER_CONTEXT_H_