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
#include "dpsLogRecord.hpp"
#include "dpsTrace.hpp"

namespace engine
{
   _dpsTrivialElement::~_dpsTrivialElement()
   {
      reset();
   }

   _dpsTrivialElement::_dpsTrivialElement(utilBufferBuilder *buf, DPS_TAG tag):
   _buf(buf),
   _tag(tag)
   {
      SDB_ASSERT(nullptr != _buf, "can not be invalid");
      _originalBufSize = _buf->getSize();
      SDB_ASSERT(DPS_INVALID_TAG != _tag, "can not be invalid");
   }

   _dpsTrivialElement::_dpsTrivialElement(_dpsTrivialElement &&o):
   _buf(o._buf),
   _tag(o._tag),
   _originalBufSize(o._originalBufSize),
   _size(o._size),
   _lastFieldOffset(o._lastFieldOffset)
   {
      o._reset();
   }

   _dpsTrivialElement &_dpsTrivialElement::operator=(_dpsTrivialElement &&o)
   {
      reset();
      _buf = o._buf;
      _tag = o._tag;
      _originalBufSize = o._originalBufSize;
      _size = o._size;
      _lastFieldOffset = o._lastFieldOffset;
      o._reset();
      return *this;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSTSELEMENT_RESET, "_dpsTrivialElement::reset" )
   void _dpsTrivialElement::reset()
   {
      PD_TRACE_ENTRY(SDB__DPSTSELEMENT_RESET);
      if (nullptr != _buf && _originalBufSize != _buf->getSize())
      {
         /// rollback buf builder's size
         _buf->shrink(_originalBufSize);
      }

      _reset();

      PD_TRACE_EXIT(SDB__DPSTSELEMENT_RESET);
      return;
   }

   void _dpsTrivialElement::_reset()
   {
      _buf = nullptr;
      _tag = DPS_INVALID_TAG;
      _originalBufSize = 0;
      _size = 0;
      _lastFieldOffset = 0;
      return;
   }

   void _dpsTrivialElement::done()
   {
      if (0 < _size)
      {
         SDB_ASSERT(_originalBufSize < _buf->getSize(), "impossible");
         utilUniqueBuffer &buf = _buf->getBuf();
         reinterpret_cast<dpsRecordEle*>(buf.getByOffset(_originalBufSize))->len = _size;

         /// reset _buf to avoid rollback.
         _buf = nullptr;
      }
      
      reset();

      return;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSTSELEMENT_APPEND, "_dpsTrivialElement::append" )
   INT32 _dpsTrivialElement::append(DPS_TS_FIELD_TAG tag,
                                    UINT32 size,
                                    const void *data)
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(SDB__DPSTSELEMENT_APPEND);
      SDB_ASSERT(nullptr != _buf, "can not be invalid");
      SDB_ASSERT(dpsIsValidTsTag(tag), "invalid tag");
      SDB_ASSERT(size < DPS_TS_SIZE_BOUND, "invalid size");

      if (OSS_UNLIKELY(0 == size || DPS_TS_SIZE_BOUND <= size || nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!_isBufInited())
      {
         rc = _initBuf();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init buf:%d", rc);
            goto error;
         }
      }

      /// not else
      {
         _resetLastFieldEndingFlag();
         UINT32 offset = _buf->getSize();

         dpsTsFieldHeader header;
         header.setTag(tag);
         header.size = size;
         rc = _buf->appendObj(header);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append field header:%d", rc);
            goto error;
         }

         rc = _buf->append(size, data);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append value:%d", rc);
            goto error;
         }

         _size += (DPS_TS_FIELD_HEAD_SIZE + size);
         _lastFieldOffset = offset;
      }

      
   done:
      PD_TRACE_EXITRC(SDB__DPSTSELEMENT_APPEND, rc) ;
      return rc;
   error:
      reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSTSELEMENT__IINITBUF, "_dpsTrivialElement::_initBuf" )
   INT32 _dpsTrivialElement::_initBuf()
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(SDB__DPSTSELEMENT__IINITBUF);
      SDB_ASSERT(nullptr != _buf, "can not be invalid");
      if (_buf->getSize() == _originalBufSize)
      {
         dpsRecordEle e(_tag, 0);
         rc = _buf->appendObj(e);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append element header:%d", rc);
            goto error;
         }
      }
   done:
      PD_TRACE_EXITRC(SDB__DPSTSELEMENT__IINITBUF, rc);
      return rc;
   error:
      goto done;
   }

   void _dpsTrivialElement::_resetLastFieldEndingFlag()
   {
      if (0 < _lastFieldOffset)
      {
         utilUniqueBuffer &buf = _buf->getBuf();
         reinterpret_cast<dpsTsFieldHeader*>(buf.getByOffset(_lastFieldOffset))->setMore();
      }
      return;
   }

} // namespace engine
