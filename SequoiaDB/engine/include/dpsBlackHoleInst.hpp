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

   Source File Name = dpsBlackHoleInst.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_BLACK_HOLE_HPP_
#define DPS_BLACK_HOLE_HPP_

#include "interface/IDataProtectionService.h"
#include <atomic>

namespace engine
{
   class dpsBlackHoleInst : public IDataProtectionService
   {
      public:
         dpsBlackHoleInst() = default;
         virtual ~dpsBlackHoleInst() = default;

      public:

      private:
         
      
      private:
         OSS_INLINE UINT32 _getVersion()const
         {
            return _version.load(std::memory_order_relaxed);
         }
         OSS_INLINE UINT64 _getOffset()const
         {
            return _offset.load(std::memory_order_relaxed);
         }

      private:
         std::atomic<UINT32> _version{1};
         std::atomic<UINT64> _offset{0};
   };//class dpsBlackHole
} // namespace engine


#endif//DPS_BLACK_HOLE_HPP_