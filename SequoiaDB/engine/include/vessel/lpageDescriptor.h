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

   Source File Name = lpageDescriptor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPAGE_DESCRIPTOR_H_
#define VESSEL_LPAGE_DESCRIPTOR_H_

#include "vessel/pageIdentifier.h"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct lpageDescriptor
   {
      lpageDescriptor() = default;
      explicit lpageDescriptor(PAGE_ID pi,
                               PAGE_SNAPSHOT_VERION pv):
      pid(pi),
      psv(pv){}
      ~lpageDescriptor() = default;
      lpageDescriptor(const lpageDescriptor &o) = default;
      lpageDescriptor &operator=(const lpageDescriptor &o) = default;
   

      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_PAGE_SNAPSHOT_VERSION != psv &&
                INVALID_PAGE_ID != pid;
      }
      OSS_INLINE void reset(PAGE_ID pid=INVALID_PAGE_ID,
                            PAGE_SNAPSHOT_VERION psv=INVALID_PAGE_SNAPSHOT_VERSION)
      {
         this->pid = pid;
         this->psv = psv;
         return;
      }

      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
   };//class lpageDescriptor
   constexpr UINT32 LPAGE_DESC_SIZE = sizeof(lpageDescriptor);
#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_DESCRIPTOR_H_
