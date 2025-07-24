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

   Source File Name = pageInitializer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_PAGE_INITIALIZER_H_
#define VESSEL_PAGE_INITIALIZER_H_

#include "vessel/pageAccessor.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class pageInitializer : public pageAccessor
   {
      public:
         pageInitializer(){}
         virtual ~pageInitializer(){}

      public:
         virtual INT32 initPage(requestContext *context,
                                PAGE_ID lpid,
                                PAGE_SNAPSHOT_VERION psv,
                                runtimePageBuffer *rpb) = 0;

         /// Used for batch allocating.
         /// "i" will be set as [0, count) in turns.
         virtual INT32 initInTurns(requestContext *context,
                                   UINT32 i,
                                   PAGE_ID lpid,
                                   PAGE_SNAPSHOT_VERION psv,
                                   runtimePageBuffer *rpb)
         {
            if (0 == i)
            {
               return initPage(context, lpid, psv, rpb);
            }
            else
            {
               return SDB_VESSEL_INTERNAL_ERR;
            }
         }

      protected:
         INT32 writeJournal(requestContext *context,
                            const GLOBAL_PAGE_ID &gpid,
                            PAGE_TYPE type,
                            const slice &adjunct,
                            DPS_LSN_OFFSET &lsn);
   };//class pageInitializer
}//class vessel
}//class engine

#endif//VESSEL_PAGE_INITIALIZER_H_
