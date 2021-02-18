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

   Source File Name = fsmPage.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FSM_PAGE_H_
#define VESSEL_FSM_PAGE_H_

#include "vessel/vesselDef.h"

namespace engine
{
namespace vessel
{
   struct fsmGlobalPageHead
   {
      UINT32 filePageCount; /// 16KB per page
      UINT32 freeExtentCount; /// 4KB per extent

   }; // struct fsmGlobalPage;

   struct fsmStripingPageHead
   {
      UINT16 totalCount;
      UINT16 pad1;
      UINT32 pad2;
   }; // struct fsmStripingPageHead

   struct fsmStripingRecord
   {
      UINT16 min;
      UINT16 max;
      UINT32 pageCount;
      PAGE_ID lastExtent;
   };

   const UINT32 FSM_SLOT_COUNT = 60;
   const UINT32 FSM_BIT_COUNT = 15;
   struct fsmDataPage
   {
      UINT16 totalCount;
      UINT16 fullCount;
      UINT32 lastFsmExtent;
      CHAR pad[24];
      UINT32 slots[FSM_SLOT_COUNT];
      UINT32 map1[FSM_BIT_COUNT];
      UINT32 map2[FSM_BIT_COUNT];
      UINT32 map3[FSM_BIT_COUNT];
      UINT32 map4[FSM_BIT_COUNT];
   }; // struct fsmDataPage
} // namespace vessel
} // namespace engine

#endif // VESSEL_FSM_PAGE_H_