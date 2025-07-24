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

   Source File Name = strictBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         rc = SDB_INVALID_OPERATION;
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
      if (isWritable() && isValidAccessing(offset, size))
      {
         buffer.makeWritable(size, _wptr + offset);
      }
      return buffer;
   }
} // namespace vessel

} // namespace engine
