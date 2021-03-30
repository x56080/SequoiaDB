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

   Source File Name = idMapPage.h

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

#ifndef VESSEL_ID_MAP_PAGE_H_
#define VESSEL_ID_MAP_PAGE_H_

#include "extentDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   const PAGE_ID SYSTEM_MAP_PAGE_ID = 2;

   const UINT16 CURRENT_ID_MAP_PAGE_VERSION = 1;
   struct idMapPageHead
   {
      UINT32 version;
      UINT32 flags;
      CHAR pad[16];
   };// struct idMapPageHead

   const UINT32 ID_MAP_PAGE_HEAD_LEN = sizeof(idMapPageHead);

   struct idMapSlot
   {
      SNAPSHOT_ID snapshot;
      PAGE_ID page;

      OSS_INLINE BOOLEAN free()const
      {
         return INVALID_PAGE_ID == page;
      }
   };// struct idMapExtentSlot
#pragma pack()
   INT32 getCapacityOfIMP(UINT32 pageSize, UINT32 &capacity);
}//namespace vessel
}//namespace engine

#endif//VESSEL_ID_MAP_PAGE_H_