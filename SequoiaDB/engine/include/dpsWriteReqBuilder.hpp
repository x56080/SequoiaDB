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

#include "dpsWriteRequest.hpp"
#include "dpsTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
   class dpsWriteReqBuilder : public SDBObject
   {
      public:
         dpsWriteReqBuilder() = default;
         ~dpsWriteReqBuilder();
         dpsWriteReqBuilder(const dpsWriteReqBuilder &) = delete;
         dpsWriteReqBuilder &operator=(const dpsWriteReqBuilder &) = delete;

      public:
         struct options : public SDBObject
         {
            UINT32 initBufferSize = 4096;
            UINT32 minBufferGrowthSize = 128;
            UINT32 doubleBufThreshold = 512 << 10;
         };

      public:
         void reset();
         void refresh();
         OSS_INLINE void setType(DPS_LOG_TYPE type) {_type = type;}
         OSS_INLINE DPS_LOG_TYPE getType()const {return _type;}
         OSS_INLINE void setFlag(UINT32 flag) {OSS_BIT_SET(_flags, flag);}
         OSS_INLINE UINT16 getFlags()const {return _flags;}
         OSS_INLINE void setOptions(const options &o) {_o = o;}
         OSS_INLINE BOOLEAN isDone()const {return _done;}
         OSS_INLINE void setDone() {_done = TRUE;}

         /// return owned request and reset builder.
         dpsWriteRequest reap();

         dpsWriteRequest done();

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
            return appendRawData(tag, sizeof(T), &v);
         }

         INT32 append(DPS_TAG tag, UINT32 size, const void *data);

      private:
         INT32 _ensureFreeBuffer(UINT32 size);
         BOOLEAN _isTagDuplicated(DPS_TAG tag)const;
         OSS_INLINE UINT32 getElementBufferSize(UINT32 valSize)const
         {
            return sizeof(dpsRecordEle) + valSize;
         }

         OSS_INLINE dpsRecordEle *getElementPtr()
         {
            SDB_ASSERT(sizeof(dpsRecordEle) <= getFreeBufferSize(), "out of resource");
            return reinterpret_cast<dpsRecordEle *>(_buffer + _bufferOffset);
         }

         template<class T>
         T *getObjectPtr()
         {
            SDB_ASSERT(sizeof(T) <= getFreeBufferSize(), "out ouf resource");
            return reinterpret_cast<T *>(_buffer + _bufferOffset);
         }

         OSS_INLINE CHAR *getWritePtr(UINT32 size)
         {
            SDB_ASSERT(0 < size && size <= getFreeBufferSize(), "invalid ptr");
            return _buffer + _bufferOffset;
         }

         OSS_INLINE void _moveOffset(UINT32 size)
         {
            SDB_ASSERT((_bufferOffset + size) <= _bufferSize, "out of resource");
            _bufferOffset += size;
         }

         OSS_INLINE UINT32 getFreeBufferSize()const
         {
            return _bufferSize - _bufferOffset;
         }
      private:
         BOOLEAN _done = FALSE;
         options _o;
         DPS_LOG_TYPE _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _elementNum = 0;
         UINT32 _bufferSize = 0;
         UINT32 _bufferOffset = 0;
         CHAR *_buffer = nullptr;

   };//class dpsWriteReqBuilder

   template<typename T>
   INT32 dpsWriteReqBuilder::appendNumeric(DPS_TAG tag, T v)
   {
      INT32 rc = SDB_OK;
      static_assert(std::numeric_limits<T>::is_specialized, "must be numeric");
      SDB_ASSERT(!isDone(), "can not be done");

      if (OSS_UNLIKELY(DPS_INVALID_TAG != tag))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementNum))
      {
         SDB_ASSERT(FALSE, "out of max element cout");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }
      else
      {
      #if defined (_DEBUG)
         SDB_ASSERT(!_isTagDuplicated(tag), "duplicated tag");
      #endif//_DEBUG
         UINT32 size = sizeof(T);
         rc = _ensureFreeBuffer(getElementBufferSize(size));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
            goto error;
         }

         getElementPtr()->tag = tag;
         getElementPtr()->len = size;
         _moveOffset(sizeof(dpsRecordEle));

         *reinterpret_cast<T*>(getWritePtr(size)) = v;
         _moveOffset(size);
         ++_elementNum;
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace engine


#endif//DPS_WRITE_REQ_BUILDER_HPP_