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

   Source File Name = fixedSizeDataPad.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/fixedSizeDataPad.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   INT32 fixedSizeDataPad::init(UINT32 bufferSize,
                                CHAR *buffer,
                                BOOLEAN resetBuffer)
   {
      INT32 rc = SDB_OK;
      fini();

      if (OSS_UNLIKELY(bufferSize < getMinBufferSize() ||
                       nullptr == buffer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _bufferSize = bufferSize;
      _buffer = buffer;
      if (resetBuffer)
      {
         this->resetBuffer();
      }
      else if (0 == _getCount())
      {
         _backOffset = _bufferSize;
      }
      else
      {
         const _tag *tag = _getTag(_getCount() - 1);
         _backOffset = tag->offset;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void fixedSizeDataPad::resetBuffer()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      _resetCounter();
      _backOffset = _bufferSize;
      return;
   }

   void fixedSizeDataPad::fini()
   {
      _bufferSize = 0;
      _buffer = nullptr;
      _backOffset = 0;
   }

   INT32 fixedSizeDataPad::push(const slice &row)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      _tag *tag = nullptr;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!row.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isFreeToPush(row.getSize()))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      buffer.makeWritable(_bufferSize, _buffer);
      rc = buffer.write(_backOffset - row.getSize(), row.getSize(), row.data());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "unexpected error:%d", rc);
         goto error;
      }

      tag = buffer.getWritableObjPtr<_tag>(_getFrontOffset());
      SDB_ASSERT(nullptr != tag, "impossible");
      _backOffset -= row.getSize();
      tag->offset = _backOffset;
      tag->size = row.getSize();
      _incCount();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 fixedSizeDataPad::pushRowFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      _tag *tag = nullptr;
      UINT32 size = 0;
      UINT32 w = 0;

      if (OSS_UNLIKELY(nullptr == _buffer))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (!i->isValid())
         {
            SDB_ASSERT(FALSE, "invalid fragment");
            rc = SDB_INVALIDARG;
            goto error;
         }
         size += i->getSize();
      }

      if (!isFreeToPush(size))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      buffer.makeWritable(_bufferSize, _buffer);
      for (auto i = il.begin(); i != il.end(); ++i)
      {
         rc = buffer.write(_backOffset - size + w, i->getSize(), i->data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "unexpected error:%d", rc);
            goto error;
         }
         w += i->getSize();
      }

      tag = buffer.getWritableObjPtr<_tag>(_getFrontOffset());
      SDB_ASSERT(nullptr != tag, "impossible");
      _backOffset -= size;
      tag->offset = _backOffset;
      tag->size = size;
      _incCount();

   done:
      return rc;
   error:
      goto done;
   }

   slice fixedSizeDataPad::getRow(UINT32 pos)const
   {
      slice s;
      if (OSS_LIKELY(pos < getRowCount()))
      {
         const _tag *tag = _getTag(pos);
         s.reset(tag->size, _buffer + tag->offset);
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
      }
      return s;
   }

   UINT32 fixedSizeDataPad::getRowSize(UINT32 pos)const
   {
      return _getTag(pos)->size;
   }

   BOOLEAN fixedSizeDataPad::isFreeToPush(UINT32 size)const
   {
      return getSavingSize(size) <= getFreeSize();
   }

   INT32 fixedSizeDataPad::overwrite(const fixedSizeDataPad &pad)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!pad.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_bufferSize < pad.getUnfreeSize())
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }
      
      resetBuffer();
      if (0 == pad._getCount())
      {
         goto done;
      }
      else
      {
         INT64 deltaBufferSize =  (INT64)_bufferSize - (INT64)pad._bufferSize;
         strictBuffer buffer;
         buffer.makeWritable(_bufferSize, _buffer);
         UINT32 dataSize = pad._bufferSize - pad._backOffset;
         buffer.write(_bufferSize - dataSize, dataSize,
                      pad._buffer + pad._backOffset);
         for (UINT32 i = 0; i < pad._getCount(); ++i)
         {
            _tag *tag = buffer.getWritableObjPtr<_tag>(_getTagOffset(i));
            const _tag *src = pad._getTag(i);
            tag->size = src->size;
            tag->offset = (UINT32)((INT64)src->offset + deltaBufferSize); 
         }

         *_getCounter() = pad._getCount();
         _backOffset = _bufferSize - dataSize;
      }
   done:
      return rc;
   error:
      goto done;
   }

   const fixedSizeDataPad::_tag *fixedSizeDataPad::_getTag(UINT32 pos)const
   {
      SDB_ASSERT(pos < _getCount(), "out of bound");
      static_assert(sizeof(_tag) == 8, "must be 8");
      return strictBuffer(_bufferSize, _buffer).
                           getReadableObjPtr<_tag>(_getTagOffset(pos));
   }

   UINT32 fixedSizeDataPad::getSavingSize(UINT32 size)
   {
      return sizeof(_tag) + size;
   }

   UINT32 fixedSizeDataPad::getRowCountFast(const CHAR *buf)
   {
      SDB_ASSERT(nullptr != buf, "can not be invalid");
      const UINT32 *counter = reinterpret_cast<const UINT32 *>(buf);
      return *counter;
   }
} // namespace vessel

} // namespace engine
