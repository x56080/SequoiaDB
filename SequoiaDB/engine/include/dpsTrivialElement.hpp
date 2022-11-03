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

   Source File Name = dpsTrivialElement.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_TRIVIAL_ELEMENT_HPP__
#define DPS_TRIVIAL_ELEMENT_HPP__

#include "dpsTrivialString.hpp"
#include "utilBufferBuilder.hpp"
#include "ossLikely.hpp"
#include "dpsDef.hpp"

namespace engine
{
   class _dpsTrivialElement : public SDBObject
   {
      friend class dpsWriteReqBuilder;
      public:
         _dpsTrivialElement() = default;
         ~_dpsTrivialElement();
         _dpsTrivialElement(const _dpsTrivialElement &) = delete;
         _dpsTrivialElement &operator=(const _dpsTrivialElement &) = delete;
         _dpsTrivialElement(_dpsTrivialElement &&);
         _dpsTrivialElement &operator=(_dpsTrivialElement &&);

      private:
         explicit _dpsTrivialElement(utilBufferBuilder *buf, DPS_TAG tag);

      public:
         void reset();

         void done();

         INT32 append(DPS_TS_FIELD_TAG tag,
                      UINT32 size,
                      const void *data);

         template<typename T>
         INT32 appendNumeric(DPS_TS_FIELD_TAG tag,
                             T value);

         template<typename T>
         INT32 appendObj(DPS_TS_FIELD_TAG tag,
                         const T &value);

      private:
         void _reset();

         INT32 _initBuf();

         void _resetLastFieldEndingFlag();

         OSS_INLINE BOOLEAN _isBufInited() const
         {
            return _originalBufSize < _buf->getSize();
         }
         
      private:
         utilBufferBuilder *_buf = nullptr;
         DPS_TAG _tag = DPS_INVALID_TAG;
         UINT32 _originalBufSize = 0;
         UINT32 _size = 0;
         UINT32 _lastFieldOffset = 0;
   };//class _utilTrivialString
   using dpsTrivialElement = _dpsTrivialElement;

   template<typename T>
   INT32 _dpsTrivialElement::appendNumeric(DPS_TS_FIELD_TAG tag, T value)
   {
      INT32 rc = SDB_OK;
      static_assert(std::numeric_limits<T>::is_specialized, "must be numeric");
      SDB_ASSERT(nullptr != _buf, "can not be invalid");
      SDB_ASSERT(dpsIsValidTsTag(tag), "invalid tag");

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
         header.size = sizeof(T);
         rc = _buf->appendObj(header);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append field header:%d", rc);
            goto error;
         }

         rc = _buf->appendNumeric(value);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append value:%d", rc);
            goto error;
         }

         _size += (DPS_TS_FIELD_HEAD_SIZE + sizeof(T));
         _lastFieldOffset = offset;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template<typename T>
   INT32 _dpsTrivialElement::appendObj(DPS_TS_FIELD_TAG tag,
                                       const T &value)
   {
      INT32 rc = SDB_OK;
      static_assert(std::is_standard_layout<T>::value, "must be standard layout");
      SDB_ASSERT(nullptr != _buf, "can not be invalid");
      SDB_ASSERT(dpsIsValidTsTag(tag), "invalid tag");
      SDB_ASSERT(sizeof(value) < DPS_TS_SIZE_BOUND, "invalid size");
      
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
         header.size = sizeof(T);
         rc = _buf->appendObj(header);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append field header:%d", rc);
            goto error;
         }

         rc = _buf->appendObj(value);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append value:%d", rc);
            goto error;
         }

         _size += (DPS_TS_FIELD_HEAD_SIZE + sizeof(T));
         _lastFieldOffset = offset;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }
} // namespace engine


#endif//DPS_TRIVIAL_ELEMENT_HPP__
