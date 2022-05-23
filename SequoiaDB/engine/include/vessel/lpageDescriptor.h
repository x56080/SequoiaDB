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

   Source File Name = lpageDescriptor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
