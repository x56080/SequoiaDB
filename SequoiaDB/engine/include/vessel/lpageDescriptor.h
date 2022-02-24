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
   class lpageDescriptor
   {
      public:
         lpageDescriptor(){}
         lpageDescriptor(PAGE_ID pi,
                         INT32 b,
                         PAGE_SNAPSHOT_VERION pv)
         :pid(pi),
          birthTick(b),
          psv(pv){}
         ~lpageDescriptor(){}
         lpageDescriptor(const lpageDescriptor &o):
         pid(o.pid),
         birthTick(o.birthTick),
         psv(o.psv){}
         lpageDescriptor &operator=(const lpageDescriptor &o)
         {
            pid = o.pid;
            birthTick = o.birthTick;
            psv = o.psv;
            return *this;
         }
      
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_PAGE_ID != pid &&
                   INVALID_PAGE_SNAPSHOT_VERSION != psv;
         }
         OSS_INLINE void reset(PAGE_ID pid=INVALID_PAGE_ID,
                               UINT32 birthTick = -1,
                               PAGE_SNAPSHOT_VERION psv=INVALID_PAGE_SNAPSHOT_VERSION)
         {
            this->pid = pid;
            this->birthTick = birthTick;
            this->psv = psv;
            return;
         }

      public:
         PAGE_ID pid = INVALID_PAGE_ID;
         INT32 birthTick = -1;
         PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
   };//class lpageDescriptor
   constexpr UINT32 LPAGE_DESC_SIZE = sizeof(lpageDescriptor);
#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_DESCRIPTOR_H_
