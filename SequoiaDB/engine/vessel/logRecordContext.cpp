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

   Source File Name = logRecordContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logRecordContext.h"
#include "utilMemListPool.hpp"
#include "ossLikely.hpp"
#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   logRecordContext::~logRecordContext()
   {
      if (NULL != _fullDumpBuffer)
      {
         SDB_THREAD_FREE(_fullDumpBuffer);
      }
   }

   void logRecordContext::open(UINT16 type)
   {
      close();
      _head._type = type;
      return;
   }

   void logRecordContext::prepush(UINT32 len)
   {
      SDB_ASSERT(0 < len, "can not be invalid");
      SDB_ASSERT(len <= DPS_MAX_TAGV_LEN, "can not be invalid");
      SDB_ASSERT(!prepared(), "can not be prepared");
      _originalLen += len;
      _originalLen += 4;/// 1byte for tag and 3bytes for len.
      return;
   }

   void logRecordContext::close()
   {
      _head.clear();
      _originalLen = 0;
      _dpsBufLen = 0;
      if (NULL != _fullDumpBuffer)
      {
         SDB_THREAD_FREE(_fullDumpBuffer);
         _fullDumpBuffer = NULL;
      }
      _fullDumpBufferSize = 0;
      _fullDumpDataSize = 0;
      return;
   }

   INT32 logRecordContext::fullDumpPage(UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == size ||
                       NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _fullDumpDataSize = 0;
      rc = ensureBuffer(size);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemcpy(_fullDumpBuffer, data, size);
      _fullDumpDataSize = size;
      prepush(_fullDumpDataSize);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logRecordContext::ensureBuffer(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < size, "can not be zero");
      if (size <= _fullDumpBufferSize)
      {
         goto done;
      }
      else if (NULL != _fullDumpBuffer)
      {
         SDB_THREAD_FREE(_fullDumpBuffer);
         _fullDumpBuffer = NULL;
      }

      _fullDumpBufferSize = 0;
      _fullDumpBuffer = (CHAR *)SDB_THREAD_ALLOC(size);
      if (NULL == _fullDumpBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      _fullDumpBufferSize = size;
   done:
      return rc;
   error:
      goto done;
   }

   void logRecordContext::setOplistHead()
   {
      SDB_ASSERT(!prepared(), "can not be prepared");
      return;
   }

   void logRecordContext::setOplistTail()
   {
      SDB_ASSERT(!prepared(), "can not be prepared");
   }

   void logRecordContext::setOplist(DPS_LSN_OFFSET lsn)
   {
      SDB_ASSERT(!prepared(), "can not be prepared");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      return;
   }

   void logRecordContext::setResetPage()
   {
      
   }

   BOOLEAN logRecordContext::isResetPage()const
   {
      
   }

   void logRecordContext::prepushDone()
   {
      SDB_ASSERT(!prepared(), "can not be prepared");
      
      _head._length = ossAlign4((UINT32)sizeof(dpsLogRecordHeader) + _originalLen);
      return;
   }

   void logRecordContext::setDDL()
   {
      SDB_ASSERT(!prepared(), "can not be prepared");

      return;
   }
}//namespace vessel
}//namespace engine