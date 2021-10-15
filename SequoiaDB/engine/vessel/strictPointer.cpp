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

   Source File Name = strictPointer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/strictPointer.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   void strictPointer::setReadable(UINT32 size, const CHAR *ptr)
   {
      reset();
      if (0 < size && NULL != ptr)
      {
         _size = size;
         _readablePtr = ptr;
      }
      return;
   }

   void strictPointer::setWritable(UINT32 size, CHAR *ptr)
   {
      reset();
      if (0 < size && NULL != ptr)
      {
         _size = size;
         _readablePtr = ptr;
         _writablePtr = ptr;
      }
      return;
   }

   const CHAR *strictPointer::getReadablePtr(UINT32 offset, UINT32 size)const
   {
      SDB_ASSERT(isReadable(), "can not be invalid");
      if (isReadable() && isValidAccessing(offset, size))
      {
         return _readablePtr + offset;
      }
      else
      {
         return NULL;
      }
   }

   const CHAR *strictPointer::getReadablePtrWithoutSize(UINT32 offset)const
   {
      SDB_ASSERT(isReadable(), "can not be invalid");
      if (isReadable() && isValidAccessing(offset, 0))
      {
         return _readablePtr + offset;
      }
      else
      {
         return NULL;
      }
   }

   CHAR *strictPointer::getWritablePtr(UINT32 offset, UINT32 size)
   {
      SDB_ASSERT(isWritable(), "can not be invalid");
      if (isWritable() && isValidAccessing(offset, size))
      {
         return _writablePtr + offset;
      }
      else
      {
         return NULL;
      }
   }

   CHAR *strictPointer::getWritablePtrWithoutSize(UINT32 offset)
   {
      SDB_ASSERT(isWritable(), "can not be invalid");
      if (isWritable() && isValidAccessing(offset, 0))
      {
         return _writablePtr + offset;
      }
      else
      {
         return NULL;
      }
   }

   INT32 strictPointer::write(UINT32 offset, UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;
      CHAR *wptr = NULL;

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

      wptr = getWritablePtr(offset, size);
      if (NULL == wptr)
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      ossMemcpy(wptr, data, size);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 strictPointer::read(UINT32 offset, UINT32 size, void *data)const
   {
      INT32 rc = SDB_OK;
      const CHAR *rptr = NULL;

      if (OSS_UNLIKELY(!isReadable()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == size || NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rptr = getReadablePtr(offset, size);
      if (NULL == rptr)
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      ossMemcpy(data, rptr, size);
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
