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

   Source File Name = fixedSizeDataPad.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/fixedSizeDataPad.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   fixedSizeDataPad::fixedSizeDataPad(fixedSizeDataPad &&o)
   {
      if (NULL != o._buffer)
      {
         _bufferSize = o._bufferSize;
         _buffer = o._buffer;
         _count = o._count;
         _backOffset = o._backOffset;
         o.fini();
      }
   }

   fixedSizeDataPad &fixedSizeDataPad::operator=(fixedSizeDataPad &&o)
   {
      fini();
      if (NULL != o._buffer)
      {
         _bufferSize = o._bufferSize;
         _buffer = o._buffer;
         _count = o._count;
         _backOffset = o._backOffset;
         o.fini();
      }
      return *this;
   }

   void fixedSizeDataPad::init(UINT32 bufferSize, CHAR *buffer)
   {
      SDB_ASSERT(0 < bufferSize && NULL != buffer, "can not be invalid");
      _bufferSize = bufferSize;
      _buffer = buffer;
      _count = 0;
      _backOffset = bufferSize;
   }

   void fixedSizeDataPad::clear()
   {
      _count = 0;
      _backOffset = _bufferSize;
   }

   void fixedSizeDataPad::fini()
   {
      _bufferSize = 0;
      _buffer = NULL;
      _count = 0;
      _backOffset = 0;
   }

   INT32 fixedSizeDataPad::push(const slice &row)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      _tag *tag = NULL;

      if (OSS_UNLIKELY(NULL == _buffer))
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

      tag = buffer.getWritableObjPtr<_tag>(getFrontOffset());
      SDB_ASSERT(NULL != tag, "impossible");
      _backOffset -= row.getSize();
      tag->offset = _backOffset;
      tag->size = row.getSize();
      ++_count;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 fixedSizeDataPad::pushRowFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      _tag *tag = NULL;
      UINT32 size = 0;
      UINT32 w = 0;

      if (OSS_UNLIKELY(NULL == _buffer))
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

      tag = buffer.getWritableObjPtr<_tag>(getFrontOffset());
      SDB_ASSERT(NULL != tag, "impossible");
      _backOffset -= size;
      tag->offset = _backOffset;
      tag->size = size;
      ++_count;

   done:
      return rc;
   error:
      goto done;
   }

   slice fixedSizeDataPad::getRow(UINT32 pos)const
   {
      slice s;
      if (OSS_LIKELY(pos < _count))
      {
         const _tag *tag = strictBuffer(_bufferSize, _buffer).
                           getReadableObjPtr<_tag>(pos * sizeof(_tag));
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
      return getTag(pos)->size;
   }

   BOOLEAN fixedSizeDataPad::isFreeToPush(UINT32 size)const
   {
      return getSavingSize(size) <= getFreeSize();
   }

   INT32 fixedSizeDataPad::overwrite(const fixedSizeDataPad &pad)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      UINT32 dataSize = 0;
      INT64 correctOffset = 0;

      if (OSS_UNLIKELY(NULL == _buffer))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == pad._buffer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_bufferSize < pad.getUnfreeSize())
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }
      
      clear();
      if (0 == pad.getCount())
      {
         goto done;
      }

      dataSize = pad._bufferSize - pad._backOffset;
      correctOffset = (INT64)_bufferSize - (INT64)pad._bufferSize;

      buffer.makeWritable(_bufferSize, _buffer);
      buffer.write(_bufferSize - dataSize, dataSize,
                   pad._buffer + pad._backOffset);
      for (UINT32 i = 0; i < pad._count; ++i)
      {
         _tag *tag = buffer.getWritableObjPtr<_tag>(i * sizeof(_tag));
         const _tag *src = pad.getTag(i);
         tag->size = src->size;
         tag->offset = (UINT32)((INT64)src->offset + correctOffset); 
      }

      _count = pad._count;
      _backOffset = _bufferSize - dataSize;
   done:
      return rc;
   error:
      goto done;
   }

   const fixedSizeDataPad::_tag *fixedSizeDataPad::getTag(UINT32 pos)const
   {
      SDB_ASSERT(pos < _count, "out of bound");
      return strictBuffer(_bufferSize, _buffer).
                           getReadableObjPtr<_tag>(pos * sizeof(_tag));
   }

   UINT32 fixedSizeDataPad::getSavingSize(UINT32 size)
   {
      return sizeof(_tag) + size;
   }
} // namespace vessel

} // namespace engine
