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

   Source File Name = dpsOplistDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_OPLIST_DEF_HPP__
#define DPS_OPLIST_DEF_HPP__

#include "dpsDef.hpp"

namespace engine
{
   enum DPS_OPLIST_STATUS : INT32
   {
      START = 0x00,
      BUILDING = 0x01,
      ROLLING_BACK = 0x02,
      COMPLETED = 0x03,
   };

} // namespace engine


#endif//DPS_OPLIST_DEF_HPP__