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

   Source File Name = dpsWriteReqBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsWriteReqBuilder.hpp"

namespace engine
{
   dpsWriteReqBuilder::~dpsWriteReqBuilder()
   {
      if (nullptr != _buffer)
      {
         SDB_THREAD_FREE(_buffer);
      }
   }

   void dpsWriteReqBuilder::reset()
   {
      _done = FALSE;
      _o = options();
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementNum = 0;
      _bufferSize = 0;
      _bufferOffset = 0;
      if (nullptr != _buffer)
      {
         SDB_THREAD_FREE(_buffer);
         _buffer = nullptr;
      }

      return;
   }

   void dpsWriteReqBuilder::refresh()
   {
      _done = FALSE;
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementNum = 0;
      _bufferOffset = 0;
      return;
   }

   dpsWriteRequest dpsWriteReqBuilder::reap()
   {
      dpsWriteRequest req;

      req._type = _type;
      req._flags = _flags;
      req._elementNum = _elementNum;
      req._bufferSize = _bufferOffset;
      req._buffer = _buffer;
      req._bufferOwned = _buffer;

      _bufferSize = 0;
      _buffer = nullptr;
      refresh();
      return std::move(req);
   }

   dpsWriteRequest dpsWriteReqBuilder::done()
   {
      dpsWriteRequest req;

      _done = TRUE;

      req._type = _type;
      req._flags = _flags;
      req._elementNum = _elementNum;
      req._bufferSize = _bufferOffset;
      req._buffer = _buffer;

      return std::move(req);
   }

   INT32 dpsWriteReqBuilder::append(DPS_TAG tag, UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isDone(), "can not be done");

      if (OSS_UNLIKELY(DPS_INVALID_TAG == tag ||
                       0 == size ||
                       nullptr == data))
      {
         SDB_ASSERT(FALSE, "invalid tag");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementNum))
      {
         SDB_ASSERT(FALSE, "out of max element cout");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }
      
#if defined (_DEBUG)
      SDB_ASSERT(!_isTagDuplicated(tag), "duplicated tag");
#endif//_DEBUG
      
      rc = _ensureFreeBuffer(getElementBufferSize(size));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
         goto error;
      }

      getElementPtr()->tag = tag;
      getElementPtr()->len = size;
      _moveOffset(sizeof(dpsRecordEle));

      ossMemcpy(getWritePtr(size), data, size);
      _moveOffset(size);
      ++_elementNum;
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN dpsWriteReqBuilder::_isTagDuplicated(DPS_TAG tag)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(DPS_INVALID_TAG != tag, "can not be invalid");
      UINT32 offset = 0;
      for (UINT32 i = 0; i < _elementNum; ++i)
      {
         const dpsRecordEle *ele = (const dpsRecordEle *)(_buffer + offset);
         if (tag == ele->tag)
         {
            r = TRUE;
            break;
         }
         offset += getElementBufferSize(ele->len);
      }
      return r;
   }

   INT32 dpsWriteReqBuilder::_ensureFreeBuffer(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < size, "can not be invalid");
      UINT32 bufferSize = 0;
      CHAR *buffer = nullptr;
      
      if (size <= getFreeBufferSize())
      {
         goto done;
      }

      if (0 == _bufferSize)
      {
         bufferSize = _o.initBufferSize;
      }
      else if (_bufferSize < _o.doubleBufThreshold)
      {
         bufferSize = OSS_MAX((_o.minBufferGrowthSize + _bufferSize),
                               (_bufferSize << 1));
      }
      else
      {
         bufferSize = _bufferSize + _o.minBufferGrowthSize;
      }

      if (bufferSize < (_bufferOffset + size))
      {
         /// still not enough
         UINT32 delta = size - (bufferSize - _bufferOffset);
         bufferSize += ossAlignX(delta, _o.minBufferGrowthSize);
      }

      buffer = (CHAR *)SDB_THREAD_ALLOC(bufferSize);
      if (OSS_UNLIKELY(nullptr == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem for %d bytes.", bufferSize);
         rc = SDB_OOM;
         goto error;
      }

      if (0 < _bufferOffset)
      {
         ossMemcpy(buffer, _buffer, _bufferOffset);
      }

      if (nullptr != _buffer)
      {
         SDB_THREAD_FREE(_buffer);
         _buffer = nullptr;
      }

      _buffer = buffer;
      _bufferSize = bufferSize;
   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine
