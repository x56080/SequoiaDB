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
      
   }

   keyStringModifier::~keyStringModifier()
   {
      if (nullptr != _buffer)
      {
         _allocator.free(_buffer);
      }
   }

   void keyStringModifier::init(const keyString &ks)
   {
      reset();
      _src = ks;
   }

   void keyStringModifier::reset()
   {
      _src.reset();
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
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (rid.isMaxRid())
      {
         PD_LOG(PDERROR, "already been max rid");
         rc = SDB_INVALID_OPERATION;
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
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (rid.isMinRid())
      {
         PD_LOG(PDERROR, "already been min rid");
         rc = SDB_INVALID_OPERATION;
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
