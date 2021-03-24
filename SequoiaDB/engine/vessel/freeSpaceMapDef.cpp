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

namespace engine
{
namespace vessel
{
   const static FLOAT32 WORTH_TO_SCAN_PERCENT = 0.05;

   BOOLEAN isWorthToScanDisk(UINT32 needLvl,
                             UINT32 totalCnt,
                             INT32 lvl0,
                             INT32 lvl1,
                             INT32 lvl2,
                             INT32 lvl3,
                             const FLOAT32 *minPercent)
   {
      BOOLEAN r = FALSE;
      FLOAT32 cnt = 0;
      FLOAT32 percent = 0.0;
      FLOAT32 min = NULL == minPercent ? WORTH_TO_SCAN_PERCENT : *minPercent;
      if (0 == totalCnt)
      {
         goto done;
      }
      switch (needLvl)
      {
      case FSM_SPACE_LVL0:
         if (0 < lvl0) {cnt += lvl0;}
      case FSM_SPACE_LVL1:
         if (0 < lvl1) {cnt += lvl1;}
      case FSM_SPACE_LVL2:
         if (0 < lvl2) {cnt += lvl2;}
      case FSM_SPACE_LVL3:
         if (0 < lvl3) {cnt += lvl3;}
         break;
      default:
         break;
      }
   
      percent = cnt / totalCnt;
      r = min <= percent;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine