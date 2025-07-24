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

   Source File Name = dpsWriteReqBuilder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_WRITE_REQ_BUILDER_HPP_
#define DPS_WRITE_REQ_BUILDER_HPP_

#include "dpsRequest.hpp"
#include "dpsTrace.hpp"
#include "ossLikely.hpp"
#include "dpsTrivialElement.hpp"
#include "utilBufferBuilder.hpp"
#include "dpsRecordElements.hpp"

#include <limits>

namespace engine
{
   class dpsWriteReqBuilder : public SDBObject
   {
      public:
         dpsWriteReqBuilder();
         dpsWriteReqBuilder(UINT32 defaultBufSize);
         ~dpsWriteReqBuilder() = default;
         dpsWriteReqBuilder(const dpsWriteReqBuilder &) = delete;
         dpsWriteReqBuilder &operator=(const dpsWriteReqBuilder &) = delete;

      public:
         void reset();
         OSS_INLINE void setType(DPS_LOG_TYPE type) {_type = type;}
         OSS_INLINE DPS_LOG_TYPE getType()const {return _type;}
         OSS_INLINE void setFlag(UINT16 flag) {OSS_BIT_SET(_flags, flag);}
         OSS_INLINE void overwriteFlags(UINT16 flags) {_flags = flags;}
         OSS_INLINE UINT16 getFlags()const {return _flags;}

         /// return owned request and reset builder.
         dpsWriteRequest reap();

         dpsRecordElements peekElements() const ;

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

         template<class T>
         INT32 appendObj(DPS_TAG tag, const T &v);

         INT32 append(DPS_TAG tag, UINT32 size, const void *data);

      public:
         ///WARNING: do not append any other element when building trivial element!
         dpsTrivialElement startToBuildTsElement(DPS_TAG tag);

      private:
         BOOLEAN _isTagDuplicated(DPS_TAG tag)const;
         OSS_INLINE UINT32 _getElementBufferSize(UINT32 valSize)const
         {
            /// always reserve one more byte to save ending tag.
            return sizeof(dpsRecordEle) + valSize + sizeof(DPS_TAG);
         }

         void _appendEndTag();

      private:
         DPS_LOG_TYPE _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _elementNum = 0;
         utilBufferBuilder _buf;
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

         dpsRecordEle e(tag, sizeof(T));

         rc = _buf.reserve(_getElementBufferSize(sizeof(T)));
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to reserve buffer size:%d", rc);
            goto error;
         }

         rc = _buf.appendObj(e);
         SDB_ASSERT(SDB_OK == rc, "can not be failed");
         rc = _buf.appendNumeric(v);
         SDB_ASSERT(SDB_OK == rc, "can not be failed");
         ++_elementNum;
      }
   done:
      return rc;
   error:
      goto done;
   }

   template<class T>
   INT32 dpsWriteReqBuilder::appendObj(DPS_TAG tag, const T &v)
   {
      INT32 rc = SDB_OK;
      static_assert(std::is_standard_layout<T>::value, "must be standard layout");

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

         dpsRecordEle e(tag, sizeof(T));

         rc = _buf.reserve(_getElementBufferSize(sizeof(T)));
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to reserve buffer size:%d", rc);
            goto error;
         }

         rc = _buf.appendObj(e);
         SDB_ASSERT(SDB_OK == rc, "can not be failed");
         rc = _buf.appendObj(v);
         SDB_ASSERT(SDB_OK == rc, "can not be failed");
         ++_elementNum;
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace engine


#endif//DPS_WRITE_REQ_BUILDER_HPP_