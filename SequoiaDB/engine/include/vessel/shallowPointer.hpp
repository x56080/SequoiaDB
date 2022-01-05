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

   Source File Name = shallowPointer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SHALLOW_POINTER_H_
#define VESSEL_SHALLOW_POINTER_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   template <class T>
   class shallowPointer : public SDBObject
   {
      public:
         shallowPointer(){}
         ~shallowPointer(){_ptr = NULL;}
         explicit shallowPointer(T *ptr):
         _ptr(ptr){}
         shallowPointer(const shallowPointer &o):
         _ptr(o._ptr){}
         shallowPointer &operator=(const shallowPointer &o)
         {
            _ptr = o._ptr;
            return *this;
         }

         T *operator->()const
         {
            return _ptr;
         }

      public:
         BOOLEAN isValid()const{return NULL != _ptr;}
         T *get(){return _ptr;}
         void reset(){_ptr = NULL;}

      private:
         T *_ptr = NULL;
   };//class shallowPointer

   template <class T>
   class shallowArray : public SDBObject
   {
      public:
         shallowArray(){}
         explicit shallowArray(T *ptr, UINT32 size):
         _ptr(ptr), _size(size){}
         ~shallowArray(){}
         shallowArray(const shallowArray &o):
         _ptr(o._ptr), _size(o._size){}
         shallowArray &operator=(const shallowArray &o)
         {
            _ptr = o._ptr;
            _size = o._size;
            return *this;
         }

      public:
         OSS_INLINE T &operator[](UINT32 i)
         {
            return at(i);
         }
         OSS_INLINE const T &operator[](UINT32 i)const
         {
            return at(i);
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _ptr && 0 < _size;
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE T *data()
         {
            return _ptr;
         }
         OSS_INLINE const T *data()const
         {
            return _ptr;
         }
         OSS_INLINE const T &at(UINT32 i)const
         {
            return _ptr[i];
         }
         OSS_INLINE T &at(UINT32 i)
         {
            return _ptr[i];
         }
         
         void fill(const T &v)
         {
            for (UINT32 i = 0; i < _size; ++i)
            {
               _ptr[i] = v;
            }
            return;
         }

         void fill()
         {
            T v;
            for (UINT32 i = 0; i < _size; ++i)
            {
               _ptr[i] = v;
            }
            return;
         }

      private:
         T *_ptr = NULL;
         UINT32 _size = 0;
   };//class shallowArray
} // namespace vessel

} // namespace engine


#endif//VESSEL_SHALLOW_POINTER_H_
