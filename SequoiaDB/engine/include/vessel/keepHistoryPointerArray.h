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

   Source File Name = keepHistoryPointerArray.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_KEEP_HISTORY_POINTER_ARRAY_H_
#define VESSEL_KEEP_HISTORY_POINTER_ARRAY_H_

#include "core.hpp"
#include "oss.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   class keepHistoryPointerArray : public SDBObject
   {
      public:
         keepHistoryPointerArray();
         ~keepHistoryPointerArray();
         keepHistoryPointerArray(const keepHistoryPointerArray &) = delete;
         keepHistoryPointerArray &operator=(const keepHistoryPointerArray &) = delete;

      public:
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         INT32 init(UINT32 initCapacity = 4);
         void fini();

         template<typename T>
         INT32 pushBack(T *ptr)
         {
            INT32 rc = SDB_OK;
            rc = ensureCapacity(_size + 1);
            if (SDB_OK != rc)
            {
               goto error;
            }

            _current[_size] = (ossValuePtr)ptr;
            ++_size;
         done:
            return rc;
         error:
            goto done;
         }

         template<typename T>
         INT32 set(UINT32 pos, T *ptr)
         {
            INT32 rc = SDB_OK;
            rc = ensureCapacity(pos + 1);
            if (SDB_OK != rc)
            {
               goto error;
            }

            _current[pos] = (ossValuePtr)ptr;
            if (_size <= pos)
            {
               _size = pos + 1;
            }
         done:
            return rc;
         error:
            goto done;
         }

         template<typename T>
         INT32 get(UINT32 pos, T *&ptr)const
         {
            INT32 rc = SDB_OK;
            if (_size <= pos)
            {
               rc = SDB_OUT_OF_BOUND;
               goto error;
            }

            ptr = (T *)(_current[pos]);
         done:
            return rc;
         error:
            goto done;
         }

         template<typename T>
         T *get(UINT32 pos)const
         {
            T *ptr = NULL;
            if (pos < _size)
            {
               ptr = (T *)(_current[pos]);
            }
            else
            {
               SDB_ASSERT(FALSE, "out of bound");
            }
            return ptr;
         }

         template<typename T>
         const T *getFront()const
         {
            return 0 < _size ?
                   reinterpret_cast<const T*>(_current[0]) : nullptr;
         }

         template<typename T>
         T *getFront()
         {
            return 0 < _size ?
                   reinterpret_cast<T*>(_current[0]) : nullptr;
         }

         template<typename T>
         const T *getBack()const
         {
            return 0 < _size ?
                   reinterpret_cast<const T*>(_current[_size - 1]) : nullptr;
         }

         template<typename T>
         T *getBack()
         {
            return 0 < _size ?
                   reinterpret_cast<T*>(_current[_size - 1]) : nullptr;
         }

      private:
         INT32 ensureCapacity(UINT32 capacity);

      private:
         UINT32 _capacity = 0;
         UINT32 _size = 0;
         ossValuePtr *_current = NULL;
         ossValuePtr *_histroy = NULL;
   };//class keepHistoryPointerArray
}//namespace vessel
}//namespace engine

#endif//VESSEL_KEEP_HISTORY_POINTER_ARRAY_H_