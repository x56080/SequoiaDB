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
#include "utilFragAllocator.hpp"
#include "ossLikely.hpp"

namespace engine
{
   class _dpsTrivialElement : public SDBObject
   {
      friend class dpsWriteReqBuilder;
      public:
         _dpsTrivialElement() = default;
         _dpsTrivialElement(const _dpsTrivialElement &) = delete;
         _dpsTrivialElement &operator=(const _dpsTrivialElement &) = delete;
         _dpsTrivialElement(_dpsTrivialElement &&);
         _dpsTrivialElement &operator=(_dpsTrivialElement &&);

      private:
         explicit _dpsTrivialElement(utilFragAllocator *allocator);

      public:
         void reset();

         void done();

         BOOLEAN isDone()const;

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

         CHAR *_ensureBuffer(UINT32 size);

      private:
         OSS_INLINE UINT32 _getFreeBufSize()const
         {
            return _bufferSize - _size;
         }

         UINT32 _getFreeBufOffset()const;
         
      private:
         utilFragAllocator *_allocator = nullptr;
         CHAR *_buffer = nullptr;
         UINT32 _bufferSize = 0;
         UINT32 _size = 0;
         dpsTsFieldHeader *_last = nullptr;
   };//class _utilTrivialString
   using dpsTrivialElement = _dpsTrivialElement;

   template<typename T>
   INT32 _dpsTrivialElement::appendNumeric(DPS_TS_FIELD_TAG tag, T value)
   {
      INT32 rc = SDB_OK;
      static_assert(std::numeric_limits<T>::is_specialized, "must be numeric");
      SDB_ASSERT(nullptr != _allocator, "can not be invalid");
      SDB_ASSERT(!(tag & DPS_TS_FIELD_ENDING_FLAG), "invalid tag");
      SDB_ASSERT(nullptr == _last || !_last->isEnding(), "no more appending");

      UINT32 size = DPS_TS_FIELD_HEAD_SIZE + sizeof(T);
      CHAR *buffer = _ensureBuffer(size);
      if (OSS_UNLIKELY(nullptr == buffer))
      {
         rc = SDB_OOM;
         goto error;
      }
      else
      {
         dpsInitTsFieldHead(tag, sizeof(T), size, buffer);
         *reinterpret_cast<T*>(buffer + DPS_TS_FIELD_HEAD_SIZE) = value;
         _size += size;
         _last = reinterpret_cast<dpsTsFieldHeader *>(buffer);
      }

   done:
      return rc;
   error:
      goto done;
   }

   template<typename T>
   INT32 _dpsTrivialElement::appendObj(DPS_TS_FIELD_TAG tag,
                                       const T &value)
   {
      static_assert(std::is_standard_layout<T>::value, "must be standard layout");
      return append(tag, sizeof(T), &value);
   }
} // namespace engine


#endif//DPS_TRIVIAL_ELEMENT_HPP__
