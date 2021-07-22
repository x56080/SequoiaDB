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

   Source File Name = pageInitializer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         INT32 prepareInitLog(requestContext *context,
                              UINT32 adjunctSize,
                              const runtimePageBuffer *rpb,
                              logRecordContext *lrc);

         INT32 commitInitLog(requestContext *context,
                             const GLOBAL_PAGE_ID &gpid,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             PAGE_TYPE type,
                             const slice &adjunct,
                             logRecordContext *lrc);
   };//class pageInitializer
}//class vessel
}//class engine

#endif//VESSEL_PAGE_INITIALIZER_H_
