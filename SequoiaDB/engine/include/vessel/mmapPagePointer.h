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

   Source File Name = mmapPagePointer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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