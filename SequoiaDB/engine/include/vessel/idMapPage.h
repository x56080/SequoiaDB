/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = idMapPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         return INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                INVALID_PAGE_ID == pid;
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
      static_assert(4096 == ID_MAP_PAGE_CAPACITY, "must be power of 2");
      return lpid & (ID_MAP_PAGE_CAPACITY - 1);
   }
}//namespace vessel
}//namespace engine

#endif//VESSEL_ID_MAP_PAGE_H_