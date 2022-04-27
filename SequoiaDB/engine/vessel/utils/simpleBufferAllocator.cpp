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

   Source File Name = simpleBufferAllocator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/simpleBufferAllocator.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   simpleBufferAllocator::simpleBufferAllocator(CHAR *buffer,
                                                UINT32 bufferSize):
   _buffer(buffer),
   _bufferSize(bufferSize)
   {
      SDB_ASSERT(!(0 < bufferSize && NULL == buffer), "can not be invalid");
   }

   simpleBufferAllocator::~simpleBufferAllocator()
   {
      SDB_ASSERT(!hasUnfreeBuffer(), "memory leak");
      clearBufferAllocated();
   }

   CHAR *simpleBufferAllocator::allocate(UINT32 size)
   {
      CHAR *out = NULL;
      SDB_ASSERT(0 < size, "can not be zero");
      UINT32 staticBufSize = getAvailableStaticBufferSize();

      if (staticBufSize < size)
      {
         out = (CHAR *)SDB_THREAD_ALLOC(size);
         if (OSS_UNLIKELY(NULL != out))
         {
            _dynamic.push_back(_bufferAllocated(out, size));
         }
      }
      else
      {
         out = _buffer + (_bufferSize - staticBufSize);
         _static.push_back(_bufferAllocated(out, size));
      }

      return out;
   }

   void simpleBufferAllocator::release(CHAR *buffer)
   {
      SDB_ASSERT(NULL != buffer, "can not be null");
      BOOLEAN found = FALSE;
      ossPoolVector<_bufferAllocated> *pool = NULL;
      if (_buffer <= buffer && buffer < (_buffer + _bufferSize))
      {
         pool = &_static;
      }
      else
      {
         pool = &_dynamic;
      }

      for (ossPoolVector<_bufferAllocated>::iterator itr = pool->begin();
           itr != pool->end(); ++itr)
      {
         if (itr->buffer == buffer)
         {
            found = TRUE;
            if (pool == &_dynamic)
            {
               SDB_THREAD_FREE(itr->buffer);
            }
            itr->reset();
            pool->erase(itr);
            break;
         }
      }

      SDB_ASSERT(found, "invalid buffer to free");
      return;
   }

   void simpleBufferAllocator::clearBufferAllocated()
   {
      _static.clear();
      for (ossPoolVector<_bufferAllocated>::const_iterator itr = _dynamic.begin();
           itr != _dynamic.end(); ++itr)
      {
         if (itr->isValid())
         {
            SDB_THREAD_FREE(itr->buffer);
         }
      }
      _dynamic.clear();
   }
   
   BOOLEAN simpleBufferAllocator::hasUnfreeBuffer()const
   {
      return !_static.empty() || !_dynamic.empty();
   }

   UINT32 simpleBufferAllocator::getAvailableStaticBufferSize()const
   {
      if (_static.empty())
      {
         return _bufferSize;
      }
      else
      {
         const _bufferAllocated &ba = _static.back();
         UINT32 offset = (UINT32)(ba.buffer - _buffer) + ba.size;
         SDB_ASSERT(offset <= _bufferSize, "impossible");
         return _bufferSize - offset;
      }
   }
} // namespace vessel

} // namespace engine
