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
#include "../bson/util/builder.h"

namespace engine
{
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
         UINT32 getFreeBufferSize()const {return _bufferSize - _size;}
         UINT32 getSize()const {return _size;}
         const CHAR *getBuffer()const {return _buffer;}
         UIN16 getType()const {return _type;}
         void setType(UINT16 type) {_type = type;}
         UIN16 getFlags()const {return _flags;}
         void setFlag(UINT16 flag) {OSS_BIT_SET(_flags, flag);}
      
      public:
         /// will kill buffer allocated.
         void fini();
         void reset();
         

      public:
         INT32 appendInt32(DPS_TAG tag, INT32 value);
         INT32 appendInt64(DPS_TAG tag, INT64 value);
         INT32 append(DPS_TAG tag, UINT32 size, const void *value);


      private:
         UINT32 getElementSize(UINT32 size)const
         {
            return sizeof(dpsRecordEle) + size;
         }

         BOOLEAN isTagDuplicated(DPS_TAG tag)const;

         INT32 _reserveBuffer(UINT32 size);

      private:/// ensure free buffer first
         template<class T>
         T *getWritePtr()
         {
            SDB_ASSERT(sizeof(T) <= getFreeBufferSize(), "out ouf resource");
            return reinterpret_cast<T *>(_buffer + _offset);
         }

         CHAR *getWritePtr(UINT32 size)
         {
            SDB_ASSERT(0 < size && size <= getFreeBufferSize(), "invalid ptr");
            return _buffer + _offset;
         }
      private:
         Allocator _allocator;
         UINT16 _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _elementCount = 0;
         CHAR *_buffer = nullptr;
         UINT32 _bufferSize = 0;
         UINT32 _offset = 0;
         
   };//class _dpsJournalPad
   typedef class _dpsJournalPad<bson::StackAllocator> dpsStackJournalPad;

   template<class Allocator>
   _dpsJournalPad<Allocator>::~_dpsJournalPad()
   {
      if (nullptr != _buffer)
      {
         _allocator.Free(_buffer);
      }
   }

   template<class Allocator>
   void _dpsJournalPad<Allocator>::fini()
   {
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementCount = 0;
      
      if (nullptr != _buffer)
      {
         _allocator.Free(_buffer);
         _buffer = nullptr;
      }
      _bufferSize = 0;
      _offset = 0;
   }

   template<class Allocator>
   void _dpsJournalPad<Allocator>::reset()
   {
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementCount = 0;
      _woffset = 0;
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
      
      SDB_ASSERT(_elementCount < DPS_MERGE_BLOCK_MAX_DATA, "over max count");
      SDB_ASSERT(!isTagDuplicated(tag), "duplicated tag");
      
      rc = _ensureBufferToWrite(sizeof(value));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
         goto error;
      }

      getElementPtr()->tag = tag;
      getElementPtr()->len = sizeof(value);
      *((INT32 *)getElementDataPtr()) = value;
      _size += getElementSize(sizeof(value));
      ++_elementCount;
      
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
      
      SDB_ASSERT(_elementCount < DPS_MERGE_BLOCK_MAX_DATA, "over max count");
      SDB_ASSERT(!isTagDuplicated(tag), "duplicated tag");
      
      rc = _ensureBufferToWrite(sizeof(value));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
         goto error;
      }

      getElementPtr()->tag = tag;
      getElementPtr()->len = sizeof(value);
      *((INT64 *)getElementDataPtr()) = value;
      _size += getElementSize(sizeof(value));
      ++_elementCount;
      
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

      SDB_ASSERT(_elementCount < DPS_MERGE_BLOCK_MAX_DATA, "over max count");
      SDB_ASSERT(!isTagDuplicated(tag), "duplicated tag");
      
      rc = _ensureBufferToWrite(size);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write:%d", rc);
         goto error;
      }

      getElementPtr()->tag = tag;
      getElementPtr()->len = size;
      ossMemcpy(getElementDataPtr(), value, size);
      _size += getElementSize(size);
      ++_elementCount;
      
   done:
      return rc;
   error:
      goto done;
   }

   template<class Allocator>
   INT32 _dpsJournalPad<Allocator>::_reserveBuffer(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < size, "can not be zero");
      constexpr UINT32 _MIN_RESERVE_SIZE = 512;
      constexpr UINT32 _DOUBLE_BUF_THRESHOLD = 512 << 10;
      UINT32 extendingSize = 0;
      CHAR *buffer = nullptr;

      if (size <= getFreeBufferSize())
      {
         goto done;
      }

      if (_bufferSize < _DOUBLE_BUF_THRESHOLD)
      {
         extendingSize = OSS_MAX(_MIN_RESERVE_SIZE, _bufferSize);
      }
      else
      {
         extendingSize = _MIN_RESERVE_SIZE;
      }

      if ((extendingSize + getFreeBufferSize()) < size)
      {
         extendingSize = size - getFreeBufferSize() + _MIN_RESERVE_SIZE;
      }

      extendingSize = ossAlign4(extendingSize); 

      if (nullptr == _buffer)
      {
         buffer = (CHAR *)Allocator.Malloc(extendingSize);
      }
      else
      {
         buffer = (CHAR *)Allocator.Realloc(_buffer, extendingSize + _bufferSize);
      }

      if (OSS_UNLIKELY(nullptr == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      _buffer = buffer;
      _bufferSize += extendingSize;
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
      UINT32 offset = sizeof(dpsLogHeader);
      for (UINT32 i = 0; i < _elementCount; ++i)
      {
         const dpsRecordEle *ele = (const dpsRecordEle *)(_buffer + offset);
         if (tag == ele->tag)
         {
            r = TRUE;
            break;
         }
         offset += getElementSize(ele->len);
      }
      return r;
   }
} // namespace engine


#endif//DPS_JOURNAL_PAD_HPP_
