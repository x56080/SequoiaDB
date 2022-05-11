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

   Source File Name = bufferPoolDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BUFFER_POOL_DEF_H_
#define VESSEL_BUFFER_POOL_DEF_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   enum class BUFFER_STATUS : UINT8
   {
      INVALID = 0,
      NORMAL,
      RECYCLING,
      DISCARDED,
   };//enum class BUFFER_STATUS

   typedef UINT32 BUFFER_CTL_FLAG_WORD;

   struct LOBC_BUFFER_CTL_FLAGS
   {
      static constexpr BUFFER_CTL_FLAG_WORD BUSY = 0x01;
      static constexpr BUFFER_CTL_FLAG_WORD IN_DIRTY_LIST = 0x02;
      static constexpr BUFFER_CTL_FLAG_WORD PENDING_FLUSH = 0x04;
   };//struct BUFFER_CTL_FLAGS

   struct LITE_IO_BUFFER_CTL_FLAGS
   {
      static constexpr BUFFER_CTL_FLAG_WORD IN_DIRTY_LIST = 0x01;
   };

} // namespace vessel

} // namespace engine


#endif//VESSEL_BUFFER_POOL_DEF_H_
