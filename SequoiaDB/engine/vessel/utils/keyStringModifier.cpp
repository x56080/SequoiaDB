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

   Source File Name = keyStringModifier.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#include "vessel/keyStringModifier.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/strictBuffer.h"
#include "vessel/keyStringCoder.h"

namespace engine
{
namespace vessel
{
   keyStringModifier::keyStringModifier(const keyString &src):
   _src(src)
   {
      SDB_ASSERT(_src.isValid(), "can not be invalid");
   }

   keyStringModifier::~keyStringModifier()
   {
      if (nullptr != _buffer)
      {
         _allocator.free(_buffer);
      }
   }

   void keyStringModifier::reset()
   {
      if (nullptr != _buffer)
      {
         _allocator.free(_buffer);
         _bufferSize = 0;
      }
   }

   INT32 keyStringModifier::incRid(BOOLEAN force)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      recordID rid;
      UINT32 bufferOffset = 0;

      if (OSS_UNLIKELY(!_src.isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_src.getKeyTailSize() < keyStringCoder::RID_ENCODING_SIZE)
      {
         PD_LOG(PDERROR, "has no rid coded");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rid = _src.getRid();
      if (!force && !rid.isValid())
      {
         PD_LOG(PDERROR, "can not modify invalid rid unforced");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (rid.isMaxRid())
      {
         PD_LOG(PDERROR, "already been max rid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      rid.setPos(rid.getPos() + 1);

      rc = _ensureBuffer(_src.getRawDataSize());
      if (SDB_OK != rc)
      {
         goto error;
      }

      bufferOffset = _src.getKeySizeExceptTail();
      buffer.makeWritable(_bufferSize, _buffer);
      buffer.write(0, _src.getRawDataSize(), _src.getRawData().data());
      keyStringCoder().encodeRid(rid, buffer.getWritablePtr(bufferOffset, sizeof(recordID)));
   done:
      return rc;
   error:
      goto done;
   }

   INT32 keyStringModifier::decRid(BOOLEAN force)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      recordID rid;
      UINT32 bufferOffset = 0;

      if (OSS_UNLIKELY(!_src.isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_src.getKeyTailSize() < keyStringCoder::RID_ENCODING_SIZE)
      {
         PD_LOG(PDERROR, "has no rid coded");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rid = _src.getRid();
      if (!force && !rid.isValid())
      {
         PD_LOG(PDERROR, "can not modify invalid rid unforced");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (rid.isMinRid())
      {
         PD_LOG(PDERROR, "already been min rid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      rid.setPos(rid.getPos() - 1);

      rc = _ensureBuffer(_src.getRawDataSize());
      if (SDB_OK != rc)
      {
         goto error;
      }

      bufferOffset = _src.getKeySizeExceptTail();
      buffer.makeWritable(_bufferSize, _buffer);
      buffer.write(0, _src.getRawDataSize(), _src.getRawData().data());
      keyStringCoder().encodeRid(rid, buffer.getWritablePtr(bufferOffset, sizeof(recordID)));
   done:
      return rc;
   error:
      goto done;
   }

   keyString keyStringModifier::getShallowKeyString()const
   {
      SDB_ASSERT(0 < _bufferSize, "can not be invalid");
      return keyString(slice(_bufferSize, _buffer));
   }

   INT32 keyStringModifier::_ensureBuffer(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < size, "can not be invalid");
      if (size <= _bufferSize)
      {
         goto done;
      }
      else
      {
         CHAR *buffer = (CHAR *)_allocator.realloc(_buffer, size);
         if (OSS_UNLIKELY(nullptr == buffer))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         _buffer = buffer;
         _bufferSize = size;
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
