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

   Source File Name = freeSpaceMapDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/freeSpaceMapDef.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   /// lvl size range: (x, y]
   INT32 getFsmSpaceLvl(UINT32 pageSize, UINT32 size)
   {
      INT32 lvl = FSM_INVALID_SPACE_LVL;
      if (isValidPageSize(pageSize) &&
          size <= pageSize &&
          0 < size)
      {
         lvl = (size - 1) / (pageSize / FSM_SPACE_LVL_COUNT);
      }
      return lvl;
   }

   INT32 getAdjustedFsmSpaceLvl(UINT32 pageSize,
                                UINT32 size,
                                UINT32 factor)
   {
      INT32 lvl = FSM_INVALID_SPACE_LVL;
      if (isValidPageSize(pageSize) &&
          size <= pageSize &&
          0 < size)
      {
         UINT32 range = pageSize / FSM_SPACE_LVL_COUNT;
         lvl = (size - 1) / range;
         if (0 <= lvl &&
             ((size % range) < factor))
         {
            --lvl;
         }
      }
      return lvl;
   }
}//namespace vessel
}//namespace engine