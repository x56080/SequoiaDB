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

   Source File Name = keepHistoryPointerArray.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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