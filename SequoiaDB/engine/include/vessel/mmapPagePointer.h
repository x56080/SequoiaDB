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

   Source File Name = mmapPagePointer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_MMAP_PAGE_POINTER_H_
#define VESSEL_MMAP_PAGE_POINTER_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   class mmapPagePointer : public SDBObject
   {
      public:
         OSS_INLINE mmapPagePointer(){}
         OSS_INLINE ~mmapPagePointer(){}
         OSS_INLINE mmapPagePointer(const mmapPagePointer &o):
         _ptr(o._ptr)
         {}

         OSS_INLINE mmapPagePointer &operator=(const mmapPagePointer &o)
         {
            _ptr = o._ptr;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return 0 != _ptr;
         }
         OSS_INLINE ossValuePtr get()const
         {
            return _ptr;
         }
         OSS_INLINE CHAR *getBuf()const
         {
            return (CHAR *)_ptr;
         }
         OSS_INLINE void reset(ossValuePtr ptr = 0)
         {
            _ptr = ptr;
            return;
         }
      private:
         ossValuePtr _ptr = 0;

   };//class mmapPagePointer
}//namespace vessel
}//namespace engine

#endif//VESSEL_MMAP_PAGE_POINTER_H_