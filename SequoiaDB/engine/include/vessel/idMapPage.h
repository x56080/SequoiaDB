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
#include "vessel/idMapFileDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct idMapSlot
   {
      OSS_INLINE idMapSlot(){}
      OSS_INLINE ~idMapSlot(){}
      OSS_INLINE idMapSlot(const idMapSlot &o):
      psv(o.psv),
      pid(o.pid){}
      OSS_INLINE explicit idMapSlot(PAGE_SNAPSHOT_VERION v, PAGE_ID p):
      psv(v),
      pid(p){}
      OSS_INLINE idMapSlot &operator=(const idMapSlot &o)
      {
         psv = o.psv;
         pid = o.pid;
         return *this;
      }
      OSS_INLINE void reset()
      {
         psv = INVALID_PAGE_SNAPSHOT_VERSION;
         pid = INVALID_PAGE_ID;
      }


      UINT32 psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT32 pid = INVALID_PAGE_ID;

      OSS_INLINE BOOLEAN isFree()const
      {
         return INVALID_PAGE_ID == pid;
      }
   };// struct idMapSlot
#pragma pack()

   constexpr UINT32 ID_MAP_PAGE_CAPACITY = (ID_MAP_FILE_PAGE_SIZE / sizeof(idMapSlot));

   idMapSlot getIdMapSlot(ossValuePtr ptr, UINT32 slot);

   OSS_INLINE PAGE_ID getImpPidOfLpid(PAGE_ID lpid)
   {
      return lpid / ID_MAP_PAGE_CAPACITY;
   }

   OSS_INLINE UINT32 getIdMapSlotNo(PAGE_ID lpid)
   {
      return lpid & (ID_MAP_PAGE_CAPACITY - 1);
   }
}//namespace vessel
}//namespace engine

#endif//VESSEL_ID_MAP_PAGE_H_