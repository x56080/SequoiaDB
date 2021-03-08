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

   Source File Name = cursorKernal.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/cursorKernal.h"
#include "ossMem.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/vesselImpl.h"
#include "vessel/ISession.h"

namespace engine
{
namespace vessel
{
   UINT32 CURSOR_FLAG_IS_OPEN = 0x01;
   UINT32 CURSOR_FLAG_NO_MORE_PUSHING = 0x02;

   cursorKernal::~cursorKernal()
   {
      SDB_ASSERT(0 == _usage, "must be zero");
      close();
   }

   BOOLEAN cursorKernal::isOpen()const
   {
      return OSS_BIT_TEST(_flags, CURSOR_FLAG_IS_OPEN);
   }

   INT32 cursorKernal::open(vesselImpl *db,
                            IQueryFilter *filter,
                            const cursorOptions *options)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL == db)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (NULL != options)
      {
         _options = *options;
      }

      if (0 < _options.initBufSize)
      {
         rc = extendBuf(_options.initBufSize);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = _open();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open:%d", rc);
         goto error;
      }

      _db = db;
      OSS_BIT_SET(_flags, CURSOR_FLAG_IS_OPEN);
      _filter = filter;
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 cursorKernal::close()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _close();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed close cursor:%d", rc);
      }

      _options = cursorOptions();
      _usage = 0;
      _flags = 0;
      _totalSliceInBuf = 0;
      _nextSlice = NULL;
      _bufSize = 0;
      _usedBufSize = 0;
      SAFE_OSS_FREE(_buf);
      _db = NULL;
      _filter = NULL;
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::getNext(ISession *session, slice &content)
   {
      INT32 rc = SDB_OK;
      UINT32 len = 0;
      const CHAR *buf = NULL;
      UINT32 pushLoopCount = 0;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      while (NULL == _nextSlice)
      {
         if (OSS_BIT_TEST(_flags, CURSOR_FLAG_NO_MORE_PUSHING))
         {
            rc = SDB_VESSEL_END_OF_CURSOR;
            goto error;
         }
         else if (OSS_LIKELY(0 == pushLoopCount))
         {
            ++pushLoopCount;
            rc = _db->pushMoreToCursor(session, this);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
         else
         {
            SDB_ASSERT(FALSE, "impossible");
            PD_LOG(PDERROR, "pushed nothing but flag not set");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         
      }
      
      len = *((const UINT32 *)_nextSlice);
      buf = _nextSlice + sizeof(UINT32);
      content.reset(len, buf);
      if (++_fetchedSliceInBuf == _totalSliceInBuf)
      {
         _totalSliceInBuf = 0;
         _fetchedSliceInBuf = 0;
         _nextSlice = NULL;
      }
      else
      {
         _nextSlice = _nextSlice + sizeof(UINT32) + len;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::push(UINT32 len, const CHAR *data)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == len || NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = allocateSpaceForPushing(len);
      if (SDB_OK != rc)
      {
         goto error;
      }

      *((UINT32 *)(_buf + _usedBufSize)) = len;
      _usedBufSize += sizeof(UINT32);
      ossMemcpy(_buf + _usedBufSize, data, len);
      _usedBufSize += len;
      ++_totalSliceInBuf;
      if (NULL == _nextSlice)
      {
         _nextSlice = _buf;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::push(const slice &content)
   {
      return push(content.len(), content.data());
   }

   INT32 cursorKernal::allocateSpaceForPushing(UINT32 dataLen)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      UINT32 freeBufSize = getFreeBufSize();
      UINT32 needSize = getRealBufSizeOfSlice(dataLen);
      UINT32 extendingSize = 0;
      
      if (needSize <= freeBufSize)
      {
         goto done;
      }

      extendingSize = needSize - freeBufSize;
      if (_options.maxBufSize < (_bufSize + extendingSize))
      {
         if (0 < _totalSliceInBuf)
         {
            rc = SDB_VESSEL_CURSOR_NO_SPACE;
            goto error;
         }
         else
         {
            /// max buf size is too small
            rc = SDB_VESSEL_OUT_OF_RESOURCE;
            goto error;
         }
      }
      else if (0 < _totalSliceInBuf)
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      rc = extendBuf(extendingSize);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::pushEnd()
   {
      INT32 rc = SDB_OK;
      if (OSS_LIKELY(isOpen()))
      {
         OSS_BIT_SET(_flags, CURSOR_FLAG_NO_MORE_PUSHING);
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN cursorKernal::hasNoSpaceToPush(UINT32 size)const
   {
      BOOLEAN r = TRUE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         goto done;
      }
      else if (size <= getFreeBufSize())
      {
         r = FALSE;
         goto done;
      }
      else if (0 < _totalSliceInBuf)
      {
         goto done;
      }
      else
      {
         UINT32 extendingSize = size - getFreeBufSize();
         if (_options.maxBufSize >= (_bufSize + extendingSize))
         {
            r = FALSE;
            goto done; 
         }
      }
   done:
      return r;
   }

   INT32 cursorKernal::extendBuf(UINT32 deltaSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < deltaSize, "impossible");
      SDB_ASSERT(_bufSize + deltaSize <= _options.maxBufSize, "impossible");
      CHAR *old = _buf;
      _buf = (CHAR *)SDB_OSS_MALLOC(deltaSize + _bufSize);
      if (OSS_UNLIKELY(NULL == _buf))
      {
         _buf = old;
         rc = SDB_OOM;
         goto error;
      }

      if (NULL != old)
      {
         ossMemcpy(_buf, old, _bufSize);
         SDB_OSS_FREE(old);
      }

      _bufSize += deltaSize;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine