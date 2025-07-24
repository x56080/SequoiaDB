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

   Source File Name = routePageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_ROUTE_PAGE_ACCESSOR_H_
#define VESSEL_ROUTE_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/routePage.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;

   class routePageAccessor : public pageAccessor
   {
      public:
         routePageAccessor(){}
         virtual ~routePageAccessor(){}

      public:
         INT32 append(requestContext *context,
                      UINT32 count,
                      const PAGE_ID *lpids,
                      logicalPageBuffer *lpb);

         /// pos can not be out of current size
         INT32 get(requestContext *context,
                   UINT32 pos,
                   const logicalPageBuffer *lpb,
                   PAGE_ID &lpid)const;

         INT32 getSizeAndLast(requestContext *context,
                              INT32 targetLvl,
                              const logicalPageBuffer *lpb,
                              UINT32 &size,
                              PAGE_ID &last)const;

         INT32 dumpValidPages(requestContext *context,
                              INT32 targetLvl,
                              logicalPageBuffer *lpb,
                              ossPoolVector<PAGE_ID> &lpids);

      private:
         INT32 validatePage(requestContext *context,
                            INT32 targetLvl,
                            const logicalPageBuffer *lpb);

         INT32 writeJournal(requestContext *context,
                            const GLOBAL_PAGE_ID &gpid,
                            UINT16 oldSize,
                            UINT16 size,
                            const PAGE_ID *lpids,
                            DPS_LSN_OFFSET &lsn);

   };//class routePageAccessor
}//namespace vessel
}//namespace engine
#endif//VESSEL_ROUTE_PAGE_ACCESSOR_H_