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

   Source File Name = simpleBufferAllocator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         
      private:
         CHAR *_buffer = NULL;
         UINT32 _bufferSize = 0;
         ossPoolVector<_bufferAllocated> _static;
         ossPoolVector<_bufferAllocated> _dynamic;
   };//class simpleBufferAllocator
} // namespace vessel

} // namespace engine


#endif//VESSEL_SIMPLE_BUFFER_ALLOCATOR_H_
