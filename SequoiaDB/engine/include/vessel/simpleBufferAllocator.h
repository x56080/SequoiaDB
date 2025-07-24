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

   Source File Name = simpleBufferAllocator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SIMPLE_BUFFER_ALLOCATOR_H_
#define VESSEL_SIMPLE_BUFFER_ALLOCATOR_H_

#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class simpleBufferAllocator : public SDBObject
   {
      public:
         simpleBufferAllocator() = delete;
         simpleBufferAllocator(CHAR *buffer,
                               UINT32 bufferSize);
         ~simpleBufferAllocator();
         simpleBufferAllocator(const simpleBufferAllocator &) = delete;
         simpleBufferAllocator &operator=(const simpleBufferAllocator &) = delete;

      private:
         struct _bufferAllocated : public SDBObject
         {
            _bufferAllocated(){}
            explicit _bufferAllocated(CHAR *buf, UINT32 s):
            buffer(buf),
            size(s){}
            ~_bufferAllocated(){}
            _bufferAllocated(const _bufferAllocated &o):
            buffer(o.buffer),
            size(o.size){}
            _bufferAllocated &operator=(const _bufferAllocated &o)
            {
               buffer = o.buffer;
               size = o.size;
               return *this;
            }

            OSS_INLINE BOOLEAN isValid()const
            {
               return NULL != buffer;
            }
            OSS_INLINE void reset()
            {
               buffer = NULL;
               size = 0;
               return;
            }

            CHAR *buffer = NULL;
            UINT32 size = 0;
         };

      public:
         CHAR *allocate(UINT32 size);
         void release(CHAR *buffer);
         void clearBufferAllocated();
         BOOLEAN hasUnfreeBuffer()const;

      private:
         UINT32 getAvailableStaticBufferSize()const;
         BOOLEAN reserveArray(UINT32 size, ossPoolVector<_bufferAllocated> &vec);
         
      private:
         CHAR *_buffer = NULL;
         UINT32 _bufferSize = 0;
         ossPoolVector<_bufferAllocated> _static;
         ossPoolVector<_bufferAllocated> _dynamic;
   };//class simpleBufferAllocator
} // namespace vessel

} // namespace engine


#endif//VESSEL_SIMPLE_BUFFER_ALLOCATOR_H_
