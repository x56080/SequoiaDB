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

namespace engine
{
namespace vessel
{
   simpleBufferAllocator::simpleBufferAllocator(CHAR *buffer,
                                                UINT32 bufferSize):
   _buffer(buffer),
   _bufferSize(bufferSize)
   {
      SDB_ASSERT(NULL != _buffer, "can not be null");
   }

   simpleBufferAllocator::~simpleBufferAllocator()
   {
      _allocated.clear();
   }

   CHAR *simpleBufferAllocator::allocate(UINT32 size)
   {
      CHAR *out = NULL;
      SDB_ASSERT(0 < size, "can not be zero");
      UINT32 offset = getAvailableOffset();
      if ((size + offset) <= _bufferSize)
      {
         _allocated.push_back(_bufferAllocated(offset, size));
         out = _buffer + offset;
      }

      return out;
   }

   void simpleBufferAllocator::release(CHAR *buffer)
   {
      SDB_ASSERT(NULL != buffer, "can not be null");
      if (contains(buffer))
      {
         for (ossPoolVector<_bufferAllocated>::reverse_iterator itr = _allocated.rbegin();
              itr != _allocated.rend(); ++itr)
         {
            if (buffer == (itr->offset + _buffer))
            {
               itr->reset();
               break;
            }
         }

         popReleasedBuffers();
      }
      else
      {
         SDB_ASSERT(FALSE, "buffer not managed by allocator");
      }

      return;
   }

   BOOLEAN simpleBufferAllocator::contains(CHAR *buffer)const
   {
      SDB_ASSERT(NULL != buffer, "can not be null");
      return _buffer <= buffer && buffer < (_buffer + _bufferSize);
   }

   UINT32 simpleBufferAllocator::getAvailableOffset()const
   {
      UINT32 offset = 0;
      if (!_allocated.empty())
      {
         const _bufferAllocated &o = _allocated.back();
         SDB_ASSERT(o.isValid(), "impossible");
         offset = (UINT32)(o.offset + o.size);
      }
      return offset;
   }

   void simpleBufferAllocator::popReleasedBuffers()
   {
      while (!_allocated.empty())
      {
         const _bufferAllocated &o = _allocated.back();
         if (o.isValid())
         {
            break;
         }

         _allocated.pop_back();
      }
      return;
   }

   void simpleBufferAllocator::clearBufferAllocated()
   {
      _allocated.clear();
   }
   
   BOOLEAN simpleBufferAllocator::isTotallyFree()const
   {
      return _allocated.empty();
   }
} // namespace vessel

} // namespace engine
