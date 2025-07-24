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

   Source File Name = bufferOwnedSorter.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BUFFER_OWNED_SORTER_HPP_
#define VESSEL_BUFFER_OWNED_SORTER_HPP_

#include "vessel/memoryBlock.h"
#include "../../bson/util/builder.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   template<class T, class COMPARER>
   class bufferOwnedSortor : public SDBObject
   {
      public:
         bufferOwnedSortor(){}
         ~bufferOwnedSortor(){}
         bufferOwnedSortor(const bufferOwnedSortor &) = delete;
         bufferOwnedSortor &operator=(const bufferOwnedSortor &) = delete;

      public:
         class batch : public SDBObject
         {
            friend class bufferOwnedSortor;
            public:
               batch(){}
               ~batch(){}
               batch(const batch &) = delete;
               batch &operator=(const batch &) = delete;

            public:
               void push(const slice &obj)
               {
                  SDB_ASSERT(obj.isValid(), "can not be empty");
                  _bb.appendBuf(obj.data(), obj.getSize());
                  ++_count;
                  return;
               }

               void pushFragments(std::initializer_list<slice> il)
               {
                  for (auto i = il.begin(); i != il.end(); ++i)
                  {
                     SDB_ASSERT(i->isValid(), "can not be empty");
                     _bb.appendBuf(i->data(), i->getSize());
                  }
                  ++_count;
                  return;
               }

               void reset()
               {
                  _count = 0;
                  _bb.reset();
                  return;
               }

            private:
               UINT32 _count = 0;
               bson::StackBufBuilder _bb;
         };//class batch

      public:
         INT32 init(COMPARER *cmp, UINT32 bufferSize, memoryBlock *outer=NULL)
         {
            INT32 rc = SDB_OK;
            fini();

            if (NULL == cmp ||
                0 == bufferSize)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            _cmp = cmp;
            _outer = outer;
            _buffer = NULL == _outer ? &_inner : _outer;
            _buffer->resize(0);
            rc = _buffer->reserve(bufferSize);
            if (SDB_OK != rc)
            {
               goto error;
            }

            _backOffset = _buffer->getCapacity();
         done:
            return rc;
         error:
            fini();
            goto done;
         }

         void fini()
         {
            _cmp = NULL;
            _outer = NULL;
            _inner.release();
            _buffer = NULL;
            _count = 0;
            _backOffset = 0;
         }

         void resetData()
         {
            SDB_ASSERT(isValid(), "can not be invalid");
            _count = 0;
            _backOffset = _buffer->getCapacity();
            return;
         }

         BOOLEAN isValid()const
         {
            return NULL != _cmp;
         }

         BOOLEAN push(const batch &b)
         {
            BOOLEAN r = FALSE;
            SDB_ASSERT(NULL != _cmp, "not inited");
            SDB_ASSERT(0 < b._count, "can not be empty");
            CHAR *buffer = _buffer->getBuffer();
            UINT32 offset = 0;

            if (!isFreeToPush(b))
            {
               goto done;
            }

            for (UINT32 i = 0; i < b._count; ++i)
            {
               const CHAR *rptr = (const CHAR *)(b._bb.buf()) + offset;
               T obj;
               obj.assign(rptr);
               UINT32 objSize = obj.size();
               CHAR *backPtr = buffer + _backOffset - objSize;
               ossMemcpy(backPtr, rptr, objSize);
               CHAR **frontPtr = (CHAR **)(buffer + getFrontOffset());
               *frontPtr = backPtr;
               ++_count;
               _backOffset -= objSize;
               offset += objSize;
            }

            r = TRUE;

         done:
            return r;
         }

         void sort()
         {
            SDB_ASSERT(NULL != _cmp, "not inited");
            if (0 < _count)
            {
               CHAR *buffer = _buffer->getBuffer();
               CHAR **begin = (CHAR **)(buffer);
               CHAR **end = (CHAR **)(buffer + sizeof(CHAR *) * _count);
               std::sort(begin, end, *_cmp);
            }
            return;
         }

         UINT64 getCount()const
         {
            return _count;
         }

         void get(UINT64 i, T &obj)
         {
            SDB_ASSERT(i < _count, "out of bound");
            CHAR *buffer = _buffer->getBuffer();
            const CHAR **ptr = (const CHAR **)(buffer + sizeof(CHAR *) * i);
            obj.assign(*ptr);
            return;
         }

      private:
         INT64 getFrontOffset()const
         {
            return sizeof(CHAR *) * _count;
         }
         INT64 getFreeSpaceSize()const
         {
            SDB_ASSERT(NULL != _cmp, "not inited");
            return _backOffset - getFrontOffset();
         }

         BOOLEAN isFreeToPush(const batch &b)const
         {
            SDB_ASSERT(NULL != _cmp, "not inited");

            INT64 size = b._bb.len() + (b._count * sizeof(CHAR *));
            return size <= getFreeSpaceSize();
         }

      private:
         COMPARER *_cmp = NULL;
         memoryBlock *_outer = NULL;
         memoryBlock _inner;
         memoryBlock *_buffer = NULL;
         UINT64 _count = 0;
         INT64 _backOffset = 0;
   };//class bufferOwnedSortor
} // namespace vessel

} // namespace engine


#endif//VESSEL_BUFFER_OWNED_SORTER_HPP_