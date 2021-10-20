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

   Source File Name = slice.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/slice.h"
#include "ossLikely.hpp"
#include "ossUtil.h"

namespace engine
{
namespace vessel
{
   INT32 slice::write(UINT32 offset, UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isWritale()))
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
   
   INT32 slice::read(UINT32 offset, UINT32 size, void *data)const
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

   slice slice::getReadableSlice(UINT32 offset, UINT32 size)const
   {
      slice s;
      if (isValidAccessing(offset, offset))
      {
         s.reset(size, _rptr + offset);
      }
      return s;
   }
   
   slice slice::getWritableSlice(UINT32 offset, UINT32 size)
   {
      slice s;
      if (isWritale() && isValidAccessing(offset, size))
      {
         s.makeWritable(size, _wptr + offset);
      }
      return s;
   }
} // namespace vessel

} // namespace engine
