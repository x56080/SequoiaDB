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

   Source File Name = lazyArray.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LAZY_ARRAY_HPP_
#define VESSEL_LAZY_ARRAY_HPP_

#include "ossUtil.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "ossLatch.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   template <class T>
   class lazyArray : public SDBObject
   {
      public:
         lazyArray(){}
         ~lazyArray()
         {
            fini();
         }
         lazyArray(const lazyArray &) = delete;
         lazyArray &operator=(const lazyArray &) = delete;

      public:
         UINT32 capacity()const
         {
            return _capacity;
         }
         BOOLEAN isInitialized()const
         {
            return NULL != _matrix;
         }

      private:
         class _objectSlot : public SDBObject
         {
            public:
            _objectSlot(){}
            ~_objectSlot()
            {
               SAFE_OSS_DELETE(_obj);
            }
            _objectSlot(const _objectSlot &) = delete;
            _objectSlot &operator=(const _objectSlot &) = delete;

            BOOLEAN isFree()const
            {
               return NULL == _obj;
            }
            T *allocate()
            {
               SDB_ASSERT(NULL == _obj, "must be null");
               _obj = SDB_OSS_NEW T();
               return _obj;
            }
            void release()
            {
               SAFE_OSS_DELETE(_obj);
            }
            T *getObj()
            {
               return _obj;
            }
            private:
               T *_obj = NULL;
         };//class _objectSlot

      public:
         INT32 init(UINT32 capacity, UINT32 chunkSize)
         {
            INT32 rc = SDB_OK;
            fini();
            UINT32 bufferSize = 0;
            UINT32 sequare = 0;
            
            if (!ossIsPowerOf2(capacity) ||
                !ossIsPowerOf2(chunkSize, &sequare) ||
                capacity < chunkSize)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            _capacity = capacity;
            _chunkSize = chunkSize;
            _chunkBitwise = sequare;

            bufferSize = getXSize() * sizeof(_objectSlot*);
            _matrix = (_objectSlot**)SDB_OSS_MALLOC(bufferSize);
            if (NULL == _matrix)
            {
               rc = SDB_OOM;
               goto error;
            }
            ossMemset(_matrix, 0, bufferSize);
         done:
            return rc;
         error:
            fini();
            goto done;
         }

         /// not thread safe
         void clear()
         {
            if (NULL != _matrix)
            {
               UINT32 size = getXSize();
               for (UINT32 i = 0; i < size; ++i)
               {
                  if (NULL != _matrix[i])
                  {
                     SDB_OSS_DEL [](_matrix[i]);
                     _matrix[i] = NULL;
                  }
               }
            }
            return;
         }

         void fini()
         {
            if (NULL != _matrix)
            {
               UINT32 size = getXSize();
               for (UINT32 i = 0; i < size; ++i)
               {
                  if (NULL != _matrix[i])
                  {
                     SDB_OSS_DEL [](_matrix[i]);
                     _matrix[i] = NULL;
                  }
               }
               SDB_OSS_FREE(_matrix);
               _matrix = NULL;
            }
            _capacity = 0;
            _chunkBitwise = 0;
            _chunkSize = 0;
            return;
         }

         INT32 ensure(UINT32 i, T **out)
         {
            INT32 rc = SDB_OK;
            UINT32 x = 0;
            UINT32 y = 0;
            T *obj = NULL;

            if (OSS_UNLIKELY(!isInitialized()))
            {
               rc = SDB_VESSEL_RESOURCES_NOT_INIT;
               goto error;
            }
            else if (OSS_UNLIKELY(_capacity <= i))
            {
               rc = SDB_OUT_OF_BOUND;
               goto error;
            }

            x = getX(i);
            y = getY(i);

            obj = ensure(x, y);
            if (NULL == obj)
            {
               rc = SDB_OOM;
               goto error;
            }
            if (NULL != out)
            {
               *out = obj;
            }
         done:
            return rc;
         error:
            goto done;
         }

         INT32 get(UINT32 i, T **out)const
         {
            INT32 rc = SDB_OK;
            UINT32 x = 0;
            UINT32 y = 0;
            T *obj = NULL;

            if (OSS_UNLIKELY(!isInitialized()))
            {
               rc = SDB_VESSEL_RESOURCES_NOT_INIT;
               goto error;
            }
            else if (OSS_UNLIKELY(_capacity <= i))
            {
               rc = SDB_OUT_OF_BOUND;
               goto error;
            }
            else if (OSS_UNLIKELY(NULL == out))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            x = getX(i);
            y = getY(i);
            obj = get(x, y);

            if (NULL == obj)
            {
               rc = SDB_VESSEL_RESOURCES_NOT_INIT;
               goto error;
            }
            *out = obj;
         done:
            return rc;
         error:
            goto done;
         }

         T *get(UINT32 i)
         {
            T *out = nullptr;
            if (OSS_UNLIKELY(!isInitialized()))
            {
               SDB_ASSERT(FALSE, "not inited");
               goto done;
            }
            else if (OSS_UNLIKELY(_capacity <= i))
            {
               SDB_ASSERT(FALSE, "out of bound");
               goto done;
            }
            else
            {
               UINT32 x = getX(i);
               UINT32 y = getY(i);
               out = get(x, y);
            }
         done:
            return out;
         }

         void release(UINT32 i)
         {
            if (OSS_LIKELY(isInitialized() && (i < _capacity)))
            {
               UINT32 x = getX(i);
               UINT32 y = getY(i);
               release(x, y);
            }
            return;
         }
      private:
         void release(UINT32 x, UINT32 y)
         {
            SDB_ASSERT(isInitialized(), "must be inited");
            if (NULL != _matrix[x])
            {
               _matrix[x][y].release();
            }
            return;
         }

         T *get(UINT32 x, UINT32 y)const
         {
            SDB_ASSERT(isInitialized(), "must be inited");
            T *out = NULL;
            if (NULL == _matrix[x] ||
                _matrix[x][y].isFree())
            {
               goto done;
            }

            out = _matrix[x][y].getObj();
         done:
            return out;
         }

         T *ensure(UINT32 x, UINT32 y)
         {
            T *r = NULL;
            SDB_ASSERT(isInitialized(), "must be inited");
            ossXLatchGuard guard(&_latch, FALSE);

            if (NULL == _matrix[x])
            {
               guard.lock();
               if (NULL == _matrix[x])
               {
                  _objectSlot *tmp = SDB_OSS_NEW _objectSlot[_chunkSize];
                  if (NULL == tmp)
                  {
                     goto done;
                  }
                  _matrix[x] = tmp;
               }
            }

            if (_matrix[x][y].isFree())
            {
               if (!guard.isLocked())
               {
                  guard.lock();
               }
               if (_matrix[x][y].isFree())
               {
                  r = _matrix[x][y].allocate();
                  goto done;
               }
            }

            r = _matrix[x][y].getObj();
         done:
            return r;
         }

      private:
         UINT32 getXSize()const
         {
            return _capacity >> _chunkBitwise;
         }
         UINT32 getX(UINT32 i)const
         {
            return i >> _chunkBitwise;
         }
         UINT32 getY(UINT32 i)const
         {
            return i & (_chunkSize - 1);
         }
      private:
         UINT32 _capacity = 0;
         UINT32 _chunkSize = 0;
         UINT32 _chunkBitwise = 0;
         ossSpinXLatch _latch;
         _objectSlot **_matrix = NULL;
   };//class lazyArray

}//namespace vessel
}//namespace engine

#endif//VESSEL_LAZY_ARRAY_HPP_