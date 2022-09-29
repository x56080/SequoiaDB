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

   Source File Name = dpsBlackHoleInst.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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