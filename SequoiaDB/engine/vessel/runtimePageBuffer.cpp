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

   Source File Name = runtimePageBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/runtimePageBuffer.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 FLAG_WRITING_PREPARED = 0x01;

   runtimePageBuffer::~runtimePageBuffer()
   {
      fini();
   }

   runtimePageBuffer::runtimePageBuffer(runtimePageBuffer &&o)
   {
      if (o.isValid())
      {
         _gpid = o._gpid;
         _pageSize = o._pageSize;
         _flags = o._flags;
         _iob = std::move(o._iob);
         _buffer = o._buffer;
         _committedLsn = o._committedLsn;

         o.fini();
      }
   }

   runtimePageBuffer &runtimePageBuffer::operator=(runtimePageBuffer &&o)
   {
      fini();
      if (o.isValid())
      {
         _gpid = o._gpid;
         _pageSize = o._pageSize;
         _flags = o._flags;
         _iob = std::move(o._iob);
         _buffer = o._buffer;
         _committedLsn = o._committedLsn;
         o.fini();
      }
      return *this;
   }

   void runtimePageBuffer::fini()
   {
      if (_iob.isValid())
      {
         _iob.reset();
      }
      _gpid.reset();
      _pageSize = 0;
      _flags = 0;
      _buffer = 0;
      _committedLsn = DPS_INVALID_LSN_OFFSET;
      return;
   }

   void runtimePageBuffer::initWithMmap(const GLOBAL_PAGE_ID &gpid,
                                        UINT32 pageSize,
                                        const mmapPagePointer &ptr)
   {
      fini();
      SDB_ASSERT(gpid.isValid(), "can not be invalid");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(ptr.isValid(), "can not be invalid");
      
      _gpid = gpid;
      _pageSize = pageSize;
      _buffer = ptr.get();

      return;
   }

   void runtimePageBuffer::initWithBuffer(const GLOBAL_PAGE_ID &gpid,
                                          UINT32 pageSize,
                                          liteIOBuffer &iob)
   {
      fini();
      SDB_ASSERT(gpid.isValid(), "can not be invalid");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(iob.isValid(), "can not be invalid");
      SDB_ASSERT(iob.getBufferSize() == pageSize, "must be same");
      
      _gpid = gpid;
      _pageSize = pageSize;
      _iob = std::move(iob);
      _buffer = (ossValuePtr)_iob.getBufferPtr();
   }

   void runtimePageBuffer::commit(DPS_LSN_OFFSET lsn)
   {
      if (OSS_UNLIKELY(!isValid() ||
                       !isWritingPrepared()))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         SDB_ASSERT(!isCacheBuffer(), "can not commit invalid lsn to cache");
         goto done;
      }
      
      if (DPS_INVALID_LSN_OFFSET == _committedLsn ||
          _committedLsn < lsn)
      {
         if (!updatePageLsn(_buffer, lsn))
         {
            SDB_ASSERT(FALSE, "impossible");
            goto done;
         }

         if (_iob.isValid())
         {
            _iob.commit(lsn);
         }

         _committedLsn = lsn;
      }
      else
      {
         SDB_ASSERT(FALSE, "lsn to commit must be greater");
      }
   done:
      return;
   }

   BOOLEAN runtimePageBuffer::isCommitted()const
   {
      return DPS_INVALID_LSN_OFFSET != _committedLsn;
   }

   BOOLEAN runtimePageBuffer::isWritingPrepared()const
   {
      return 0 != OSS_BIT_TEST(_flags, FLAG_WRITING_PREPARED);
   }

   void runtimePageBuffer::setWritingPrepared()
   {
      OSS_BIT_SET(_flags, FLAG_WRITING_PREPARED);
   }

   INT32 runtimePageBuffer::prepareToWrite(requestContext *context)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isWritingPrepared())
      {
         goto done;
      }
      else if (isCacheBuffer())
      {
         rc = _iob.makeWritable();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get buffer writable:%d", rc);
            goto error;
         }
         _buffer = (ossValuePtr)_iob.getBufferPtr();
      }

      setWritingPrepared();
   done:
      return rc;
   error:
      goto done;
   }

   const pageHead *runtimePageBuffer::getPageHead()const
   {
      return isValid() ? (const pageHead *)_buffer : NULL;
   }

   strictBuffer runtimePageBuffer::getReadableBuffer()const
   {
      if (isValid())
      {
         return strictBuffer(_pageSize, (const void *)_buffer);
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         return strictBuffer();
      }
   }
   
   strictBuffer runtimePageBuffer::getWritableBuffer()
   {
      strictBuffer buffer;
      if (isWritingPrepared())
      {
         buffer.makeWritable(_pageSize, (void *)_buffer);
      }
      else
      {
         SDB_ASSERT(FALSE, "must be prepared");
      }
      return buffer;
   }

   strictBuffer runtimePageBuffer::getReadableBodyBuffer()const
   {
      if (isValid())
      {
         UINT32 pageBodySize = getPageBodySize(_pageSize);
         return getReadableBuffer().getReadableBuffer(pageBodySize, PAGE_HEAD_SIZE);
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         return strictBuffer();
      }
   }
   
   strictBuffer runtimePageBuffer::getWritableBodyBuffer()
   {
      strictBuffer buffer;
      if (isWritingPrepared())
      {
         UINT32 pageBodySize = getPageBodySize(_pageSize);
         buffer.makeWritable(pageBodySize, (CHAR *)_buffer + PAGE_HEAD_SIZE);
      }
      else
      {
         SDB_ASSERT(FALSE, "must be prepared");
      }
      return buffer;
   }
}//namespace vessel
}//namespace engine