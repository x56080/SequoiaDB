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

   Source File Name = memoryBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/memoryBlock.h"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   memoryBlock::memoryBlock(memoryBlock &&o)
   {
      _buffer = o._buffer;
      _capacity = o._capacity;
      _size = o._size;
      o._buffer = NULL;
      o._capacity = 0;
      o._size = 0;
   }

   memoryBlock &memoryBlock::operator=(memoryBlock &&o)
   {
      release();
      _buffer = o._buffer;
      _capacity = o._capacity;
      _size = o._size;
      o._buffer = NULL;
      o._capacity = 0;
      o._size = 0;
      return *this;
   }

   memoryBlock::~memoryBlock()
   {
      release();
   }

   void memoryBlock::release()
   {
      if (NULL != _buffer)
      {
         SDB_THREAD_FREE(_buffer);
         _buffer = NULL;
      }
      _size = 0;
      _capacity = 0;
      return;
   }

   INT32 memoryBlock::reserve(UINT32 size)
   {
      INT32 rc = SDB_OK;
      void *buffer = NULL;
      UINT32 capacity = size + _size;

      if (capacity <= _capacity)
      {
         goto done;
      }

      buffer = SDB_THREAD_ALLOC(capacity);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (0 != _size)
      {
         SDB_ASSERT(NULL != _buffer, "impossible");
         ossMemcpy(buffer, _buffer, _size);
      }

      if (NULL != _buffer)
      {
         SDB_THREAD_FREE(_buffer);
         _buffer = NULL;
      }

      _buffer = (CHAR *)buffer;
      _capacity = capacity;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 memoryBlock::resize(UINT32 size, CHAR v)
   {
      INT32 rc = SDB_OK;

      if (size <= _size)
      {
         _size = size;
         goto done;
      }

      rc = reserve(size - _size);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemset((CHAR *)_buffer + _size, v, size - _size);
      _size = size;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 memoryBlock::copy(UINT32 size, const void *buffer)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == size ||
                       NULL == buffer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      resize(0);
      rc = reserve(size);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemcpy(_buffer, buffer, size);
      _size = size;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 memoryBlock::append(UINT32 size, const void *buffer)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == size ||
                       NULL == buffer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = reserve(size);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemcpy((CHAR *)_buffer + _size, buffer, size);
      _size += size;
   done:
      return rc;
   error:
      goto done;
   }

   void memoryBlock::swap(memoryBlock &mb)
   {
      CHAR *buffer = _buffer;
      UINT32 size = _size;
      UINT32 capacity = _capacity;

      _buffer = mb._buffer;
      _size = mb._size;
      _capacity = mb._capacity;

      mb._buffer = buffer;
      mb._size = size;
      mb._capacity = capacity;
      return;
   }
}//namespace vessel
}//namespace engine