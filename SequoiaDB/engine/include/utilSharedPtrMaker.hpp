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