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

#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
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
      OSS_INLINE idMapSlot():
      snapshot(INVALID_SNAPSHOT_ID),
      page(INVALID_PAGE_ID){}
      OSS_INLINE ~idMapSlot(){}
      OSS_INLINE idMapSlot(const idMapSlot &o):
      snapshot(o.snapshot),
      page(o.page){}
      OSS_INLINE idMapSlot &operator=(const idMapSlot &o)
      {
         snapshot = o.snapshot;
         page = o.page;
         return *this;
      }
      OSS_INLINE void reset()
      {
         snapshot = INVALID_SNAPSHOT_ID;
         page = INVALID_PAGE_ID;
      }


      SNAPSHOT_ID snapshot;
      PAGE_ID page;

      OSS_INLINE BOOLEAN free()const
      {
         return INVALID_PAGE_ID == page;
      }
   };// struct idMapExtentSlot
#pragma pack()
   INT32 get64AlignedCapacityOfIMP(UINT32 pageSize, UINT32 &capacity);
}//namespace vessel
}//namespace engine

#endif//VESSEL_ID_MAP_PAGE_H_