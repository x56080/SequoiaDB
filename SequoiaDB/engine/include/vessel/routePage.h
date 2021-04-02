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

   Source File Name = routePage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ROUTE_PAGE_H_
#define VESSEL_ROUTE_PAGE_H_

#include "vessel/extentDef.h"

namespace engine
{
namespace vessel
{
   static const UINT16 ROUTE_PAGE_VERSION = 1;

#pragma pack(4)
   struct routePageHead
   {
      OSS_INLINE routePageHead &operator=(const routePageHead &o)
      {
         version = o.version;
         count = o.count;
         logicalId = o.logicalId;
         pad = o.pad;
         return *this;
      }

      UINT16 version;
      /// WARNING: count can never shrink. It means slot ever allocated.
      UINT16 count;
      UINT32 logicalId;
      UINT64 pad;
   };//struct routePageHead

   static const UINT32 ROUTE_PAGE_HEAD_LEN = sizeof(routePageHead);

#pragma pack()

   UINT32 getCapacityOfRoutePage(UINT32 pageSize);
}//namespace vessel
}//namespace engine

#endif//VESSEL_ROUTE_PAGE_H_