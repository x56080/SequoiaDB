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

   Source File Name = fsmSizeLvl.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/fsmSizeLvl.h"
#include "pdTrace.hpp"
#include "dms.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   void fsmSizeLvl::init(UINT32 pageSize, UINT32 size)
   {
      if (DMS_PAGE_SIZE32K == pageSize)
      {
         initUnder32KBPage(size);
      }
      else if (DMS_PAGE_SIZE64K == pageSize)
      {
         initUnder64KBPage(size);
      }
      return;
   }

   void fsmSizeLvl::initUnder32KBPage(UINT16 size)
   {
      SDB_ASSERT(size <= DMS_PAGE_SIZE32K, "impossible");
      reset();
      if (OSS_UNLIKELY(DMS_PAGE_SIZE32K < size))
      {
         goto done;
      }
      
      if (FSM_32KB_LVL3 < size)
      {
         _lvl = FSM_SPACE_LVL3;
         _delta = (size - FSM_32KB_LVL3) / FSM_32KB_LVL3_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }
      else if (FSM_32KB_LVL2 < size)
      {
         _lvl = FSM_SPACE_LVL2;
         _delta = (size - FSM_32KB_LVL2) / FSM_32KB_LVL2_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }
      else if (FSM_32KB_LVL1 < size)
      {
         _lvl = FSM_SPACE_LVL1;
         _delta = (size - FSM_32KB_LVL1) / FSM_32KB_LVL1_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }
      else
      {
         _lvl = FSM_SPACE_LVL0;
         _delta = size / FSM_32KB_LVL0_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }
   done:
      return;
   }

   void fsmSizeLvl::initUnder64KBPage(UINT16 size)
   {
      reset();
      
      if (FSM_64KB_LVL3 < size)
      {
         _lvl = FSM_SPACE_LVL3;
         _delta = (size - FSM_64KB_LVL3) / FSM_64KB_LVL3_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }
      else if (FSM_64KB_LVL2 < size)
      {
         _lvl = FSM_SPACE_LVL2;
         _delta = (size - FSM_64KB_LVL2) / FSM_64KB_LVL2_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }
      else if (FSM_64KB_LVL1 < size)
      {
         _lvl = FSM_SPACE_LVL1;
         _delta = (size - FSM_64KB_LVL1) / FSM_64KB_LVL1_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }
      else
      {
         _lvl = FSM_SPACE_LVL0;
         _delta = size / FSM_64KB_LVL0_DELTA_RANGE;
         if (FSM_LVL_DELTA_COUNT == _delta)
         {
            --_delta;
         }
      }

      return;
   }

   void fsmSizeLvl::reset(INT16 lvl, UINT16 delta)
   {
      SDB_ASSERT(FSM_LVL_DELTA_COUNT == 16, "impossible");
      if (FSM_SPACE_LVL_MAX < lvl ||
               lvl < FSM_SPACE_LVL_MIN)
      {
         _lvl = FSM_SPACE_LVL_INVALID;
         _delta = 0;
      }

      _lvl = lvl;
      _delta = delta & (0xf);
      return;
   }

   BOOLEAN fsmSizeLvl::isSimilarWithHigherLvl()const
   {
      return (isValid() && FSM_SPACE_LVL_MAX != _lvl &&
              FSM_LVL_DELTA_COUNT == _delta + 1);
   }

   void fsmSizeLvl::incDelta()
   {
      if (!isValid())
      {
         goto done;
      }
      else if (FSM_LVL_DELTA_COUNT > (_delta + (UINT32)1))
      {
         ++_delta;
      }
      else if (FSM_SPACE_LVL_MAX != _lvl)
      {
         ++_lvl;
         _delta = 0;
      }
   done:
      return;
   }

   UINT32 fsmSizeLvl::getMinFreeSize(UINT32 pageSize)const
   {
      SDB_ASSERT(DMS_PAGE_SIZE32K == pageSize ||
                 DMS_PAGE_SIZE64K == pageSize, "impossible");
      UINT32 size = 0;
      UINT32 lvlSize = 0;
      UINT32 deltaRange = 0;
      if (!isValid())
      {
         goto done;
      }
      else if (DMS_PAGE_SIZE32K == pageSize)
      {
         if (FSM_SPACE_LVL0 == _lvl)
         {
            deltaRange = FSM_32KB_LVL0_DELTA_RANGE;
         }
         else if (FSM_SPACE_LVL1 == _lvl)
         {
            lvlSize = FSM_32KB_LVL1;
            deltaRange = FSM_32KB_LVL1_DELTA_RANGE;
         }
         else if (FSM_SPACE_LVL2 == _lvl)
         {
            lvlSize = FSM_32KB_LVL2;
            deltaRange = FSM_32KB_LVL2_DELTA_RANGE;
         }
         else if (FSM_SPACE_LVL3 == _lvl)
         {
            lvlSize = FSM_32KB_LVL3;
            deltaRange = FSM_32KB_LVL3_DELTA_RANGE;
         }
      }
      else if (DMS_PAGE_SIZE64K == pageSize)
      {
         if (FSM_SPACE_LVL0 == _lvl)
         {
            deltaRange = FSM_64KB_LVL0_DELTA_RANGE;
         }
         else if (FSM_SPACE_LVL1 == _lvl)
         {
            lvlSize = FSM_64KB_LVL1;
            deltaRange = FSM_64KB_LVL1_DELTA_RANGE;
         }
         else if (FSM_SPACE_LVL2 == _lvl)
         {
            lvlSize = FSM_64KB_LVL2;
            deltaRange = FSM_64KB_LVL2_DELTA_RANGE;
         }
         else if (FSM_SPACE_LVL3 == _lvl)
         {
            lvlSize = FSM_64KB_LVL3;
            deltaRange = FSM_64KB_LVL3_DELTA_RANGE;
         }
      }

      size = lvlSize + _delta * deltaRange;
   done:
      return size;
   }

}//namespace vessel
}//namespace engine