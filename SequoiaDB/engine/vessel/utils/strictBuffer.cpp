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

   Source File Name = strictBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/strictBuffer.h"
#include "ossLikely.hpp"
#include "ossUtil.h"

namespace engine
{
namespace vessel
{
   void strictBuffer::setBuffer(CHAR v)
   {
      SDB_ASSERT(isWritable(), "must be writable");
      ossMemset(_wptr, v, _size);
   }

   void strictBuffer::setBuffer(UINT32 offset, UINT32 size, CHAR v)
   {
      SDB_ASSERT(isWritable(), "must be writable");
      ossMemset(_wptr + offset, v, size);
   }

   INT32 strictBuffer::write(UINT32 offset, UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isWritable()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == size || NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isValidAccessing(offset, size))
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      ossMemcpy(_wptr + offset, data, size);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 strictBuffer::read(UINT32 offset, UINT32 size, void *data)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == size || NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isValidAccessing(offset, size))
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      ossMemcpy(data, _rptr + offset, size);
   done:
      return rc;
   error:
      goto done; 
   }

   slice strictBuffer::getSlice()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return slice(_size, _rptr);
   }

   slice strictBuffer::getSlice(UINT32 offset, UINT32 size)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return isValidAccessing(offset, size) ? slice(size, _rptr + offset) : slice();
   }

   strictBuffer strictBuffer::getReadableBuffer()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return strictBuffer(_size, _rptr);
   }

   strictBuffer strictBuffer::getReadableBuffer(UINT32 size, UINT32 offset)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return  isValidAccessing(offset, size) ? strictBuffer(size, _rptr + offset) : strictBuffer();
   }

   strictBuffer strictBuffer::getWritableBuffer(UINT32 size, UINT32 offset)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      strictBuffer buffer;
      if (isValidAccessing(offset, size))
      {
         buffer.makeWritable(size, _wptr + offset);
      }
      return buffer;
   }
} // namespace vessel

} // namespace engine
