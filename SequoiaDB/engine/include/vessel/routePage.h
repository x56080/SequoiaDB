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

   Source File Name = routePage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_ROUTE_PAGE_H_
#define VESSEL_ROUTE_PAGE_H_

#include "vessel/pageDef.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   static const UINT16 ROUTE_PAGE_VERSION = 1;

   const static INT32 COLLECTION_ROUTE_PAGE_LVL0 = 0;
   const static INT32 COLLECTION_ROUTE_PAGE_LVL1 = 1;
   const static INT32 COLLECTION_ROUTE_PAGE_LVL2 = 2;

#pragma pack(4)
   struct routePageHead
   {
      UINT16 version = 0;
      /// WARNING: size never shrinks.
      UINT16 size = 0;
      UINT32 logicalId = DMS_INVALID_LOGICCLID;
      INT32 lvl = -1;
      UINT64 pad = 0;
   };//struct routePageHead

   static const UINT32 ROUTE_PAGE_HEAD_SIZE = sizeof(routePageHead);

#pragma pack()

   UINT32 getCapacityOfRoutePage(UINT32 pageSize);

   OSS_INLINE BOOLEAN isValidRoutePageLvl(INT32 lvl)
   {
      return COLLECTION_ROUTE_PAGE_LVL0 <= lvl &&
             lvl <= COLLECTION_ROUTE_PAGE_LVL2;
   }

   BOOLEAN initRoutePage(UINT32 pageSize,
                         PAGE_ID pid,
                         PAGE_ID lpid,
                         PAGE_SNAPSHOT_VERION psv,
                         UINT32 logicalId,
                         INT32 lvl,
                         void *buf);

}//namespace vessel
}//namespace engine

#endif//VESSEL_ROUTE_PAGE_H_