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

   Source File Name = runtimePageBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/runtimePageBuffer.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   static const UINT32 RUNTIME_PAGE_BUFFER_RT_FLAG_WRITING_PREPARED = 0x01;
   static const UINT32 RUNTIME_PAGE_BUFFER_RT_FLAG_COMMITTED = 0x02;
   static const UINT32 RUNTIME_PAGE_BUFFER_RT_FLAG_ABORTED = 0x04;

   //////////////runtimePageBuffer::options
   UINT32 runtimePageBuffer::options::toFlags()const
   {
      return 0;
   }

   //////////////runtimePageBuffer::options end

   runtimePageBuffer::runtimePageBuffer()
   {}

   runtimePageBuffer::~runtimePageBuffer()
   {
      fini();
   }

   void runtimePageBuffer::fini()
   {
      if (isWritingPrepared())
      {
         SDB_ASSERT(isCommitted() || isAborted(), "commit/abort missed");
      }

      if (_tuple.isValid())
      {
         _tuple.release();
      }
      _gpid.reset();
      _pageSize = 0;
      _flags = 0;
      _runtimeFlags = 0;
      _buffer = 0;
      return;
   }

   INT32 runtimePageBuffer::init(const GLOBAL_PAGE_ID &gpid,
                                 UINT32 pageSize,
                                 const mmapPagePointer &ptr,
                                 const options &o)
   {
      INT32 rc = SDB_OK;
      fini();
      if (OSS_UNLIKELY(!gpid.isValid() ||
                       !isValidPageSize(pageSize) ||
                       !ptr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   
      _gpid = gpid;
      _pageSize = pageSize;
      _flags = o.toFlags();
      _buffer = ptr.get();
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 runtimePageBuffer::init(const GLOBAL_PAGE_ID &gpid,
                                 UINT32 pageSize,
                                 const liteCacheTuple &tuple,
                                 const options &o)
   {
      INT32 rc = SDB_OK;

      fini();
      if (OSS_UNLIKELY(!gpid.isValid() ||
                       !isValidPageSize(pageSize)||
                       !tuple.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   
      _gpid = gpid;
      _pageSize = pageSize;
      _flags = o.toFlags();
      _tuple = tuple;
      _buffer = _tuple.getReadableBuffer();
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void runtimePageBuffer::commit(DPS_LSN_OFFSET lsn)
   {
      if (OSS_UNLIKELY(!isValid() ||
                       !isWritingPrepared()))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }
      else if (OSS_UNLIKELY(isCacheBuffer() &&
                            DPS_INVALID_LSN_OFFSET == lsn))
      {
         SDB_ASSERT(FALSE, "can not commit invalid lsn to cache");
         goto done;
      }

      if (!updatePageLsn(_buffer, lsn))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }
      
      if (_tuple.isValid())
      {
         _tuple.commit(lsn);
      }

      setCommitted();
   done:
      return;
   }

   void runtimePageBuffer::abort()
   {
      if (isValid() && isWritingPrepared())
      {
         setAborted();
      }
      return;
   }

   INT32 runtimePageBuffer::getReadablePtrOfBodyWithRc(UINT32 offset,
                                                       UINT32 size,
                                                       ossValuePtr &ptr)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidPtrOfPageBody(offset, size)))
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      ptr = _buffer + PAGE_HEAD_SIZE + offset;
   done:
      return rc;
   error:
      goto done;
   }

   const void *runtimePageBuffer::getReadablePtrOfBody(UINT32 offset, UINT32 size)const
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      rc = getReadablePtrOfBodyWithRc(offset, size, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return (const void *)ptr;
   error:
      ptr = 0;
      goto done;
   }

   INT32 runtimePageBuffer::getWritablePtrOfBodyWithRc(UINT32 offset,
                                                       UINT32 size,
                                                       ossValuePtr &ptr)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isWritingPrepared())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (isWritingFinished())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidPtrOfPageBody(offset, size)))
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      ptr = _buffer + PAGE_HEAD_SIZE + offset;
   done:
      return rc;
   error:
      goto done;
   }

   void *runtimePageBuffer::getWritablePtrOfBody(UINT32 offset, UINT32 size)const
   {
      ossValuePtr ptr = 0;
      INT32 rc = getWritablePtrOfBodyWithRc(offset, size, ptr);
      if (SDB_OK != rc)
      {
         ptr = 0;
      }
   done:
      return (void *)ptr;
   }

   BOOLEAN runtimePageBuffer::isValidPtrOfPageBody(UINT32 offset, UINT32 size)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 pageBodySize = getPageBodySize(_pageSize);
      SDB_ASSERT(0 < pageBodySize, "can not be invalid");
      return (offset + size) <= pageBodySize;
   }

   void runtimePageBuffer::setCommitted()
   {
      OSS_BIT_CLEAR(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_ABORTED);
      OSS_BIT_SET(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_COMMITTED);
   }

   BOOLEAN runtimePageBuffer::isCommitted()const
   {
      return 0 != OSS_BIT_TEST(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_COMMITTED);
   }

   BOOLEAN runtimePageBuffer::isWritingPrepared()const
   {
      return 0 != OSS_BIT_TEST(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_WRITING_PREPARED);
   }

   void runtimePageBuffer::setWritingPrepared()
   {
      OSS_BIT_SET(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_WRITING_PREPARED);
   }

   BOOLEAN runtimePageBuffer::isAborted()const
   {
      return 0 != OSS_BIT_TEST(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_ABORTED);
   }

   void runtimePageBuffer::setAborted()
   {
      OSS_BIT_CLEAR(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_COMMITTED);
      OSS_BIT_SET(_runtimeFlags, RUNTIME_PAGE_BUFFER_RT_FLAG_ABORTED);
   }

   BOOLEAN runtimePageBuffer::isWritingFinished()const
   {
      return isCommitted() || isAborted();
   }

   BOOLEAN runtimePageBuffer::hasRuntimeFlags()const
   {
      return 0 != _runtimeFlags;
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
         OSS_BIT_CLEAR(_runtimeFlags, (RUNTIME_PAGE_BUFFER_RT_FLAG_COMMITTED|
                                       RUNTIME_PAGE_BUFFER_RT_FLAG_ABORTED));
         goto done;
      }
      else if (isCacheBuffer())
      {
         rc = _tuple.prepareToWrite(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get tuple writing prepared:%d", rc);
            goto error;
         }
         _buffer = _tuple.getWritableBuffer();
      }

      setWritingPrepared();
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine