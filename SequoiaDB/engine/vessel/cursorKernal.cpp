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

      if (OSS_UNLIKELY(NULL == db))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(isOpen()))
      {
         SDB_ASSERT(FALSE, "do not reopen");
         close();
      }

      /// Set open first, ensure close can be executed.
      OSS_BIT_SET(_flags, CURSOR_FLAG_IS_OPEN);
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

      _db = db;
      _filter = filter;

      rc = _open();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void cursorKernal::close()
   {
      SDB_ASSERT(0 == _usage, "must be zero");
      if (isOpen())
      {
         _close();
         _options = cursorOptions();
         _usage = 0;
         _totalPushed = 0;
         _flags = 0;

         _bufSize = 0;
         if (NULL != _buf)
         {
            SDB_THREAD_FREE(_buf);
            _buf = NULL;
         }
         _w = 0;
         _r = 0;
         _db = NULL;
         _filter = NULL;
      }
   
      return;
   }

   INT32 cursorKernal::getNext(ISession *session, slice &content)
   {
      INT32 rc = SDB_OK;
      UINT32 size = 0;
      const CHAR *buf = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!hasMoreDataToFetch())
      {
         if (0 != OSS_BIT_TEST(_flags, CURSOR_FLAG_NO_MORE_PUSHING) ||
             _options.limit == _totalPushed)
         {
            rc = SDB_VESSEL_EOC;
            goto error;
         }

         _r = 0;
         _w = 0;
         rc = _db->pushMoreToCursor(session, this);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (0 != OSS_BIT_TEST(_flags, CURSOR_FLAG_NO_MORE_PUSHING))
         {
            rc = SDB_VESSEL_EOC;
            goto error;
         }

         if (OSS_UNLIKELY(!hasMoreDataToFetch()))
         {
            PD_LOG(PDERROR, "pushed nothing but flag not set");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      
      SDB_ASSERT((_r + sizeof(UINT32)) < _w, "impossible");
      size = *((const UINT32 *)(_buf + _r));
      _r += sizeof(UINT32);

      SDB_ASSERT((_r + size) <= _w, "impossible");
      content.reset(size, _buf + _r);
      _r += size;
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
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == len || NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_options.limit == _totalPushed)
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      rc = allocateSpaceForPushing(len);
      if (SDB_OK != rc)
      {
         goto error;
      }

      *((UINT32 *)(_buf + _w)) = len;
      _w += sizeof(UINT32);
      ossMemcpy(_buf + _w, data, len);
      _w += len;
      ++_totalPushed;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::push(const slice &content)
   {
      return push(content.len(), content.data());
   }

   INT32 cursorKernal::pushFragments(std::initializer_list<std::pair<UINT32, const CHAR *>> il)
   {
      INT32 rc = SDB_OK;
      UINT32 len = 0;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_options.limit == _totalPushed)
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (0 == i->first || NULL == i->second)
         {
            SDB_ASSERT(FALSE, "invalid fragment");
            rc = SDB_INVALIDARG;
            goto error;
         }
         len += i->first;
      }

      rc = allocateSpaceForPushing(len);
      if (SDB_OK != rc)
      {
         goto error;
      }

      *((UINT32 *)(_buf + _w)) = len;
      _w += sizeof(UINT32);
      for (auto i = il.begin(); i != il.end(); ++i)
      {
         ossMemcpy(_buf + _w, i->second, i->first);
         _w += i->first;
      }
      ++_totalPushed;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::allocateSpaceForPushing(UINT32 dataLen)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(_w <= _bufSize, "impossible");

      UINT32 freeBufSize = _bufSize - _w;
      UINT32 needSize = getRealBufSizeOfSlice(dataLen);
      UINT32 extendingSize = 0;
      
      if (needSize <= freeBufSize)
      {
         goto done;
      }

      extendingSize = needSize - freeBufSize;
      if (_options.maxBufSize < (_bufSize + extendingSize))
      {
         /// max buf size is too small
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }
      else if (0 < _w)
      {
         /// extend buffer only when first pushing of current loop
         /// _w zeroed before every pushing more
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

   void cursorKernal::pushEnd()
   {
      if (OSS_LIKELY(isOpen()))
      {
         OSS_BIT_SET(_flags, CURSOR_FLAG_NO_MORE_PUSHING);
      }
      return;
   }

   BOOLEAN cursorKernal::hasSpaceToPush(UINT32 size)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isOpen(), "can not be closed");
      UINT32 sliceSize = getRealBufSizeOfSlice(size);
      SDB_ASSERT(sliceSize <= 16777216, "can not be over size");
      UINT32 freeSize = _bufSize - _w;

      if (OSS_UNLIKELY(!isOpen()))
      {
         goto done;
      }
      else if (_options.limit == _totalPushed)
      {
         goto done;
      }
      else if (sliceSize <= freeSize)
      {
         r = TRUE;
         goto done;
      }
      else if ((0 == _w) && (sliceSize <= _options.maxBufSize))
      {
         r = TRUE;
         goto done;
      }
   done:
      return r;
   }

   INT32 cursorKernal::extendBuf(UINT32 deltaSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < deltaSize, "impossible");
      SDB_ASSERT(_bufSize + deltaSize <= _options.maxBufSize, "impossible");
      SDB_ASSERT(0 == _w, "must be first pushing");
      CHAR *tmp = (CHAR *)SDB_THREAD_ALLOC(deltaSize + _bufSize);
      if (OSS_UNLIKELY(NULL == tmp))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (NULL != _buf)
      {
         SDB_THREAD_FREE(_buf);
         _buf = NULL;
      }

      _buf = tmp;
      _bufSize += deltaSize;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine