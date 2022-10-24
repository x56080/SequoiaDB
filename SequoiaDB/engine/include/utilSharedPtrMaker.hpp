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

   Source File Name = utilSharedPtrMaker.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_UTIL_SHARED_PTR_MAKER_HPP_
#define SDB_UTIL_SHARED_PTR_MAKER_HPP_

#include "ossMemPool.hpp"
#include <memory> // c++ 11

namespace engine
{
   /// allocate_shared can avoid twice memory allocating(obj and control block).
   template<class T, class ... Args>
   std::shared_ptr<T> makeSharedPtrFromPool(Args &&... args)
   {
      typename ossPoolAllocator<T>::Type alloc;
      return std::allocate_shared<T>(alloc, std::forward<Args>(args)...);
   }
} // namespace engine


#endif//SDB_UTIL_SHARED_PTR_MAKER_HPP_