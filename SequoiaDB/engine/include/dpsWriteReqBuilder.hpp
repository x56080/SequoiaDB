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

   Source File Name = dpsWriteReqBuilder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_WRITE_REQ_BUILDER_HPP_
#define DPS_WRITE_REQ_BUILDER_HPP_

#include "dpsRequest.hpp"
#include "dpsTrace.hpp"
#include "ossLikely.hpp"
#include "utilFragAllocator.hpp"

#include <array>

namespace engine
{
   class dpsWriteReqBuilder : public SDBObject
   {
      public:
         struct options : public SDBObject
         {
            UINT32 defaultBufferBlockSize = 8192;
         };

      public:
         dpsWriteReqBuilder();
         dpsWriteReqBuilder(const options &o);
         ~dpsWriteReqBuilder() = default;
         dpsWriteReqBuilder(const dpsWriteReqBuilder &) = delete;
         dpsWriteReqBuilder &operator=(const dpsWriteReqBuilder &) = delete;

      public:
         void reset();
         OSS_INLINE void setType(DPS_LOG_TYPE type) {_type = type;}
         OSS_INLINE DPS_LOG_TYPE getType()const {return _type;}
         OSS_INLINE void setFlag(UINT32 flag) {OSS_BIT_SET(_flags, flag);}
         OSS_INLINE UINT16 getFlags()const {return _flags;}

         /// return owned request and reset builder.
         dpsWriteRequest reap();
      public:
         ///WARNING: the data size is according to the exact type of T.
         template<typename T>
         INT32 appendNumeric(DPS_TAG tag, T v);

         INT32 appendInt16(DPS_TAG tag, INT16 v)
         {
            return appendNumeric(tag, v);
         }

         INT32 appendInt32(DPS_TAG tag, INT32 v)
         {
            return appendNumeric(tag, v);
         }

         INT32 appendInt64(DPS_TAG tag, INT64 v)
         {
            return appendNumeric(tag, v);
         }

         template<typename T>
         INT32 appendObj(DPS_TAG tag, const T &v)
         {
            return append(tag, sizeof(T), &v);
         }

         INT32 append(DPS_TAG tag, UINT32 size, const void *data);

      private:
         INT32 _ensureMetaBlock();
         BOOLEAN _isTagDuplicated(DPS_TAG tag)const;
         OSS_INLINE UINT32 getElementBufferSize(UINT32 valSize)const
         {
            return sizeof(dpsRecordEle) + valSize;
         }

      private:
         DPS_LOG_TYPE _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _totalDataSize = 0;
         UINT32 _elementNum = 0;
         utilSlice *_elements = nullptr;
         utilFragAllocator _allocator;
   };//class dpsWriteReqBuilder

   template<typename T>
   INT32 dpsWriteReqBuilder::appendNumeric(DPS_TAG tag, T v)
   {
      INT32 rc = SDB_OK;
      static_assert(std::numeric_limits<T>::is_specialized, "must be numeric");

      if (OSS_UNLIKELY(DPS_INVALID_TAG == tag))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementNum))
      {
         SDB_ASSERT(FALSE, "out of max element count");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }
      else
      {
      #if defined (_DEBUG)
         SDB_ASSERT(!_isTagDuplicated(tag), "duplicated tag");
      #endif//_DEBUG
         CHAR *buffer = nullptr;
         constexpr UINT32 size = sizeof(T);
         UINT32 bufferSize = getElementBufferSize(size);
         rc = _ensureMetaBlock();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure element meta block:%d", rc);
            goto error;
         }

         buffer = (CHAR *)_allocator.malloc(bufferSize);
         if (OSS_UNLIKELY(nullptr == buffer))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         reinterpret_cast<dpsRecordEle *>(buffer)->tag = tag;
         reinterpret_cast<dpsRecordEle *>(buffer)->len = size;
         *reinterpret_cast<T *>(buffer + sizeof(dpsRecordEle)) = v;
         _elements[_elementNum].reset(bufferSize, buffer);
         ++_elementNum;
         _totalDataSize += bufferSize;
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace engine


#endif//DPS_WRITE_REQ_BUILDER_HPP_