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

   Source File Name = dpsJournalPad.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_JOURNAL_PAD_HPP_
#define DPS_JOURNAL_PAD_HPP_

#include "dpsDef.hpp"
#include "dpsLogRecord.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "utilAllocator.hpp"
#include "dpsRequest.hpp"

#include <array>

namespace engine
{
   template<class Allocator>
   class _dpsJournalPad : public SDBObject
   {
      public:
         _dpsJournalPad() = default;
         virtual ~_dpsJournalPad();
         _dpsJournalPad(const _dpsJournalPad &) = delete;
         _dpsJournalPad &operator=(const _dpsJournalPad &) = delete;

      public:
         UINT32 getElementCount()const {return _elementCount;}
         UINT32 getBufferSize()const {return _bufferSize;}
         UINT32 getFreeBufferSize()const {return _bufferSize - _offset;}
         UINT32 getSize()const {return _offset;}
         const CHAR *getBuffer()const {return _buffer;}
         UINT16 getType()const {return _type;}
         void setType(UINT16 type) {_type = type;}
         UINT16 getFlags()const {return _flags;}
         void setFlag(UINT16 flag) {OSS_BIT_SET(_flags, flag);}

         /// appening new element may cause buffer reallocated.
         /// if user get packed request first and append more elements,
         /// the request may hold wild pointer.
         /// here we force user to make pad done first. 
         /// and if it is done, no more appending will be accepted.
         void setDone() {_done = TRUE;}
         dpsPackedRequest done()
         {
            if (!isDone())
            {
               _done = TRUE;
            }
            return getPackedRequest();
         }

         BOOLEAN isDone()const {return _done;}
      
      public:
         /// will kill buffer allocated.
         void fini();
         void reset();
         
         INT32 appendInt32(DPS_TAG tag, INT32 value);
         INT32 appendInt64(DPS_TAG tag, INT64 value);
         INT32 append(DPS_TAG tag, UINT32 size, const void *value);

         /// must be done first
         dpsPackedRequest getPackedRequest()const;

      private:
         UINT32 getElementBufferSize(UINT32 valSize)const
         {
            return sizeof(dpsRecordEle) + valSize;
         }

         BOOLEAN isTagDuplicated(DPS_TAG tag)const;

         INT32 _ensureFreeBuffer(UINT32 size);

      private:/// ensure free buffer first
         dpsRecordEle *getElementPtr()
         {
            SDB_ASSERT(sizeof(dpsRecordEle) <= getFreeBufferSize(), "out of resource");
            return reinterpret_cast<dpsRecordEle *>(_buffer + _offset);
         }

         template<class T>
         T *getObjectPtr()
         {
            SDB_ASSERT(sizeof(T) <= getFreeBufferSize(), "out ouf resource");
            return reinterpret_cast<T *>(_buffer + _offset);
         }

         CHAR *getWritePtr(UINT32 size)
         {
            SDB_ASSERT(0 < size && size <= getFreeBufferSize(), "invalid ptr");
            return _buffer + _offset;
         }

         void _moveOffset(UINT32 size)
         {
            SDB_ASSERT((_offset + size) <= _bufferSize, "out of resource");
            _offset += size;
         }

      private:
         Allocator _allocator;
         BOOLEAN _done = FALSE;
         UINT16 _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _elementCount = 0;
         CHAR *_buffer = nullptr;
         UINT32 _bufferSize = 0;
         UINT32 _offset = 0;
         
   };//class _dpsJournalPad
   typedef class _dpsJournalPad<utilStackAllocator<2048>> dpsStackJournalPad;
   typedef class _dpsJournalPad<utilPoolAllocator> dpsPoolJournalPad;

   template<class Allocator>
   _dpsJournalPad<Allocator>::~_dpsJournalPad()
   {
      if (nullptr != _buffer)
      {
         _allocator.free(_buffer);
      }
   }

   template<class Allocator>
   void _dpsJournalPad<Allocator>::fini()
   {
      _done = FALSE;
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementCount = 0;
      
      if (nullptr != _buffer)
      {
         _allocator.free(_buffer);
         _buffer = nullptr;
      }
      _bufferSize = 0;
      _offset = 0;
   }

   template<class Allocator>
   void _dpsJournalPad<Allocator>::reset()
   {
      _done = FALSE;
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementCount = 0;
      _offset = 0;
   }

   template<class Allocator>
   INT32 _dpsJournalPad<Allocator>::appendInt32(DPS_TAG tag, INT32 value)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(DPS_INVALID_TAG == tag))
      {
         SDB_ASSERT(FALSE, "invalid tag");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementCount))
      {
         SDB_ASSERT(FALSE, "out of max element cout");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }

      SDB_ASSERT(!isDone(), "can not be done");
      
#if defined (_DEBUG)
      SDB_ASSERT(!isTagDuplicated(tag), "duplicated tag");
#endif//_DEBUG
      
      rc = _ensureFreeBuffer(getElementBufferSize(sizeof(INT32)));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
         goto error;
      }

      ++_elementCount;

      getElementPtr()->tag = tag;
      getElementPtr()->len = sizeof(INT32);
      _moveOffset(sizeof(dpsRecordEle));

      *(getObjectPtr<INT32>()) = value;
      _moveOffset(sizeof(INT32));
   done:
      return rc;
   error:
      goto done;
   }

   template<class Allocator>
   INT32 _dpsJournalPad<Allocator>::appendInt64(DPS_TAG tag, INT64 value)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(DPS_INVALID_TAG == tag))
      {
         SDB_ASSERT(FALSE, "invalid tag");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementCount))
      {
         SDB_ASSERT(FALSE, "out of max element cout");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }
      
      SDB_ASSERT(!isDone(), "can not be done");
#if defined (_DEBUG)
      SDB_ASSERT(!isTagDuplicated(tag), "duplicated tag");
#endif//_DEBUG
      
      rc = _ensureFreeBuffer(getElementBufferSize(sizeof(INT64)));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
         goto error;
      }

      ++_elementCount;

      getElementPtr()->tag = tag;
      getElementPtr()->len = sizeof(INT64);
      _moveOffset(sizeof(dpsRecordEle));

      *(getObjectPtr<INT64>()) = value;
      _moveOffset(sizeof(INT64));
   done:
      return rc;
   error:
      goto done;
   }

   template<class Allocator>
   INT32 _dpsJournalPad<Allocator>::append(DPS_TAG tag,
                                           UINT32 size,
                                           const void *value)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(DPS_INVALID_TAG == tag ||
                       0 == size ||
                       nullptr == value))
      {
         SDB_ASSERT(FALSE, "invalid tag");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementCount))
      {
         SDB_ASSERT(FALSE, "out of max element cout");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }

      SDB_ASSERT(!isDone(), "can not be done");
#if defined (_DEBUG)
      SDB_ASSERT(!isTagDuplicated(tag), "duplicated tag");
#endif//_DEBUG
      
      rc = _ensureFreeBuffer(getElementBufferSize(size));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
         goto error;
      }

      ++_elementCount;

      getElementPtr()->tag = tag;
      getElementPtr()->len = size;
      _moveOffset(sizeof(dpsRecordEle));

      ossMemcpy(getWritePtr(size), value, size);
      _moveOffset(size);
   done:
      return rc;
   error:
      goto done;
   }

   template<class Allocator>
   dpsPackedRequest _dpsJournalPad<Allocator>::getPackedRequest()const
   {
      SDB_ASSERT(isDone(), "must be done");
      const CHAR *buffer = 0 < _offset ? _buffer : nullptr;
      return dpsPackedRequest(_type, _flags, _offset, buffer);
   }

   template<class Allocator>
   INT32 _dpsJournalPad<Allocator>::_ensureFreeBuffer(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < size, "can not be zero");
      constexpr UINT32 _MIN_RESERVE_SIZE = 512;
      constexpr UINT32 _DOUBLE_BUF_THRESHOLD = 512 << 10;
      UINT32 bufferSize = 0;
      CHAR *buffer = nullptr;

      if (size <= getFreeBufferSize())
      {
         goto done;
      }

      if (_bufferSize < _DOUBLE_BUF_THRESHOLD)
      {
         bufferSize = OSS_MAX(_MIN_RESERVE_SIZE, _bufferSize << 1);
      }
      else
      {
         bufferSize = _bufferSize + _MIN_RESERVE_SIZE;
      }

      if ((bufferSize - _offset) < size)
      {
         UINT32 delta = size - (bufferSize - _offset);
         /// _MIN_RESERVE_SIZE must be power of 2
         bufferSize += ossAlignX(delta, _MIN_RESERVE_SIZE);
      }

      SDB_ASSERT(ossIsAligned4(bufferSize), "must be aligned");

      if (nullptr == _buffer)
      {
         buffer = (CHAR *)_allocator.malloc(bufferSize);
      }
      else
      {
         buffer = (CHAR *)_allocator.realloc(_buffer, bufferSize);
      }

      if (OSS_UNLIKELY(nullptr == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem for %d bytes.", bufferSize);
         rc = SDB_OOM;
         goto error;
      }

      _buffer = buffer;
      _bufferSize = bufferSize;
   done:
      return rc;
   error:
      goto done;
   }

   template<class Allocator>
   BOOLEAN _dpsJournalPad<Allocator>::isTagDuplicated(DPS_TAG tag)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(DPS_INVALID_TAG != tag, "can not be invalid");
      UINT32 offset = 0;
      for (UINT32 i = 0; i < _elementCount; ++i)
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
} // namespace engine


#endif//DPS_JOURNAL_PAD_HPP_
