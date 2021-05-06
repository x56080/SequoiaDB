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

   Source File Name = routePageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ROUTE_PAGE_ACCESSOR_H_
#define VESSEL_ROUTE_PAGE_ACCESSOR_H_

#include "vessel/logicalPageAccessor.h"
#include "vessel/routePage.h"

namespace engine
{
namespace vessel
{
   class logRecordContext;

   class routePageAccessor : public logicalPageAccessor
   {
      public:
         routePageAccessor(){}
         virtual ~routePageAccessor(){}

      public:
         INT32 init(requestContext *context,
                    PAGE_ID lpid,
                    const pageAccessor::options &o,
                    logicalPageSpace *space,
                    DPS_LSN_OFFSET oplist = DPS_INVALID_LSN_OFFSET);

         INT32 appendSlots(requestContext *context,
                           UINT32 logicalId,
                           UINT32 slot,
                           UINT32 count,
                           const PAGE_ID *lpids);

         /// users should ensure slot is lower than capacity and count.
         INT32 readSlot(requestContext *context,
                        UINT32 slot,
                        PAGE_ID &lpid);

         /// WARNING: SDB_OK deos means slot is valid.
         /// users should alwasy validate lpid by themselves.
         INT32 readLastSlot(requestContext *context,
                            PAGE_ID &lpid,
                            UINT32 &slot);

      public:
         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_ROUTE;
         }

      private:
         INT32 readSlot(UINT32 slot, PAGE_ID &lpid);
         INT32 writeSlots(UINT32 slot, UINT32 count, const PAGE_ID *lpids);
         INT32 prpareAppendLog(requestContext *context,
                               logRecordContext *lrc,
                               UINT32 count);
         INT32 commitAppendLog(requestContext *context,
                               logRecordContext *lrc,
                               PAGE_ID lpid,
                               const routePageHead &oldHead,
                               const routePageHead &newHead,
                               UINT32 count,
                               const PAGE_ID *lpids);

   };//class routePageAccessor
}//namespace vessel
}//namespace engine
#endif//VESSEL_ROUTE_PAGE_ACCESSOR_H_