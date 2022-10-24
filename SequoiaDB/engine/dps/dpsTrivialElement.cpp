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

   Source File Name = dpsTrivialElement.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsTrivialElement.hpp"
#include "pdTrace.hpp"
#include "dpsWriteReqBuilder.hpp"
#include "dpsLogRecord.hpp"

namespace engine
{
   constexpr UINT32 DEFAULT_BUFFER_SIZE = 128;

   _dpsTrivialElement::_dpsTrivialElement(utilFragAllocator *allocator):
   _allocator(allocator)
   {
      SDB_ASSERT(nullptr != _allocator, "can not be invalid");
   }

   _dpsTrivialElement::_dpsTrivialElement(_dpsTrivialElement &&o):
   _allocator(o._allocator),
   _buffer(o._buffer),
   _bufferSize(o._bufferSize),
   _size(o._size),
   _last(o._last)
   {
      o._reset();
   }

   _dpsTrivialElement &_dpsTrivialElement::operator=(_dpsTrivialElement &&o)
   {
      reset();
      _allocator = o._allocator;
      _buffer = o._buffer;
      _bufferSize = o._bufferSize;
      _size = o._size;
      _last = o._last;
      o._reset();
      return *this;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSTSELEMENT_RESET, "_dpsTrivialElement::reset" )
   void _dpsTrivialElement::reset()
   {
      PD_TRACE_ENTRY(SDB__DPSTSELEMENT_RESET);
      if (nullptr != _buffer)
      {
         _allocator->free(_buffer);
      }
      _reset();

      PD_TRACE_EXIT(SDB__DPSTSELEMENT_RESET);
      return;
   }

   void _dpsTrivialElement::_reset()
   {
      _allocator = nullptr;
      _buffer = nullptr;
      _bufferSize = 0;
      _size = 0;
      _last = nullptr;
      return;
   }

   void _dpsTrivialElement::done()
   {
      SDB_ASSERT(nullptr != _last, "nothing to be done");
      if (!_last->isEnding())
      {
         _last->setEnding();
      }

      return;
   }

   BOOLEAN _dpsTrivialElement::isDone()const
   {
      return nullptr != _last && _last->isEnding();
   }

   // _dpsTrivialString _dpsTrivialElement::getTsString()const
   // {
   //    SDB_ASSERT(nullptr != _last && _last->isEnding(), "can not be invalid");
   //    return _dpsTrivialString(_buffer, _size);
   // }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSTSELEMENT_APPEND, "_dpsTrivialElement::append" )
   INT32 _dpsTrivialElement::append(DPS_TS_FIELD_TAG tag,
                                    UINT32 size,
                                    const void *data)
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(SDB__DPSTSELEMENT_APPEND);
      SDB_ASSERT(nullptr != _allocator, "can not be invalid");
      SDB_ASSERT(!(tag & DPS_TS_FIELD_ENDING_FLAG), "invalid tag");
      SDB_ASSERT(nullptr == _last || !_last->isEnding(), "no more appending");

      UINT32 headSize = dpsGetTsFieldHeadSize(size);
      UINT32 bufferSize = size + headSize;
      CHAR *buffer = nullptr;

      if (OSS_UNLIKELY(0 == size || nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      buffer = _ensureBuffer(bufferSize);
      if (OSS_UNLIKELY(nullptr == buffer))
      {
         rc = SDB_OOM;
         goto error;
      }

      dpsInitTsFieldHead(tag, size, bufferSize, buffer);
      ossMemcpy(buffer + headSize, data, size);
      _size += bufferSize;
      _last = reinterpret_cast<dpsTsFieldHeader*>(buffer);
      
   done:
      PD_TRACE_EXITRC(SDB__DPSTSELEMENT_APPEND, rc) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSTSELEMENT__ENSUREBUFFER, "_dpsTrivialElement::_ensureBuffer" )
   CHAR *_dpsTrivialElement::_ensureBuffer(UINT32 size)
   {
      SDB_ASSERT(nullptr != _allocator, "can not be invalid");
      SDB_ASSERT(0 < size, "can not be invalid");
      PD_TRACE_ENTRY(SDB__DPSTSELEMENT__ENSUREBUFFER);
      CHAR *buf = nullptr;

      if (size <= _getFreeBufSize())
      {
         buf = _buffer + _getFreeBufOffset();
      }
      else
      {
         UINT32 current = _getFreeBufOffset();
         UINT32 newBufSize = 0 == _bufferSize ? DEFAULT_BUFFER_SIZE : _bufferSize << 1;

         if (newBufSize < (current + size))
         {
            newBufSize = (current + size + DEFAULT_BUFFER_SIZE);
         }

         CHAR *tmp = (CHAR*)_allocator->malloc(newBufSize);
         if (OSS_UNLIKELY(nullptr == tmp))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            goto done;
         }

         if (0 < _size)
         {
            ossMemcpy(tmp + sizeof(dpsRecordEle),
                      _buffer + sizeof(dpsRecordEle),
                      _size);
         }

         if (nullptr != _buffer)
         {
            _allocator->free(_buffer);
            _buffer = nullptr;
         }

         _buffer = tmp;
         _bufferSize = newBufSize;

         buf = _buffer + current;
      }

   done:
      PD_TRACE_EXIT(SDB__DPSTSELEMENT__ENSUREBUFFER);
      return buf;
   }

   UINT32 _dpsTrivialElement::_getFreeBufOffset()const
   {
      return _size + sizeof(dpsRecordEle);
   }

} // namespace engine
