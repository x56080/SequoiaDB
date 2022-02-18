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
   static const UINT32 RUNTMIE_PAGE_BUFFER_FLAG_WRITING_PREPARED = 0x01;
   runtimePageBuffer::runtimePageBuffer()
   {}

   runtimePageBuffer::~runtimePageBuffer()
   {
      fini();
   }

   void runtimePageBuffer::fini()
   {
      if (_tuple.isValid())
      {
         _tuple.release();
      }
      _gpid.reset();
      _pageSize = 0;
      _flags = 0;
      _buffer = 0;
      _commitedLsn = DPS_INVALID_LSN_OFFSET;
      return;
   }

   INT32 runtimePageBuffer::init(const GLOBAL_PAGE_ID &gpid,
                                 UINT32 pageSize,
                                 const mmapPagePointer &ptr)
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
      _buffer = ptr.get();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 runtimePageBuffer::init(const GLOBAL_PAGE_ID &gpid,
                                 UINT32 pageSize,
                                 liteCacheTuple &tuple)
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
      _tuple = std::move(tuple);
      _buffer = _tuple.getReadableBuffer();
   done:
      return rc;
   error:
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

      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         SDB_ASSERT(!isCacheBuffer(), "can not commit invalid lsn to cache");
         goto done;
      }
      
      if (DPS_INVALID_LSN_OFFSET == _commitedLsn ||
          _commitedLsn < lsn)
      {
         if (!updatePageLsn(_buffer, lsn))
         {
            SDB_ASSERT(FALSE, "impossible");
            goto done;
         }

         if (_tuple.isValid())
         {
            _tuple.commit(lsn);
         }

         _commitedLsn = lsn;
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
      return DPS_INVALID_LSN_OFFSET != _commitedLsn;
   }

   BOOLEAN runtimePageBuffer::isWritingPrepared()const
   {
      return 0 != OSS_BIT_TEST(_flags, RUNTMIE_PAGE_BUFFER_FLAG_WRITING_PREPARED);
   }

   void runtimePageBuffer::setWritingPrepared()
   {
      OSS_BIT_SET(_flags, RUNTMIE_PAGE_BUFFER_FLAG_WRITING_PREPARED);
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
         rc = _tuple.prepareToWrite();
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