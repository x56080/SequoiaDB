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

   Source File Name = crpAccessor.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CRP_ACCESSOR_H_
#define VESSEL_CRP_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   class crpAccessor : public pageAccessor
   {
      public:
         crpAccessor();
         virtual ~crpAccessor();
      public:
         INT32 initPage(requestContext *context, PAGE_ID lpid);

         INT32 createCL(requestContext *context,
                        const CHAR *csName,
                        const collectionRecord &record);

         /// slot: [0, capacity)
         INT32 getClRecordBySlot(UINT32 slot, collectionRecord &record);

         INT32 getHeadContent(UINT32 &capacity, UINT64 &bitmap);

         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_COLLECTION_RECORD;
         }

      private:
         INT32 writeToSlot(UINT32 slot, const collectionRecord &record);

         INT32 readFromSlot(UINT32 slot, collectionRecord &record);

         INT32 prepareCreateCLLog(requestContext *context,
                                  logRecordContext *lrc,
                                  const strSlice &csName,
                                  const GLOBAL_FULL_PAGE_ID &id,
                                  const collectionRecord &record);

         INT32 commitCreateCLLog(requestContext *context,
                                 logRecordContext *lrc,
                                 const strSlice &csName,
                                 const GLOBAL_FULL_PAGE_ID &id,
                                 const collectionRecord &record);

   };//class crpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_CRP_ACCESSOR_H_