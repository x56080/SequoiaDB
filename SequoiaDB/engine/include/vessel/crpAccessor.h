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

#include "vessel/logicalPageAccessor.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/strSlice.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class crpAccessor : public logicalPageAccessor
   {
      public:
         crpAccessor();
         virtual ~crpAccessor();
      public:
         INT32 init(requestContext *context,
                    PAGE_ID lpid,
                    const pageAccessor::options &o,
                    logicalPageSpace *space,
                    DPS_LSN_OFFSET oplist=DPS_INVALID_LSN_OFFSET);

         INT32 createCL(requestContext *context,
                        const collectionRecord &record);

         /// record on disk must be valid.
         INT32 update(requestContext *context,
                      DPS_LOG_TYPE ddlType,
                      UINT64 mask,
                      const collectionRecord &record,
                      const slice &adjuncts);

         /// slot: [0, capacity)
         /// return SDB_DMS_NOTEXIST if slot is invalid.
         INT32 getClRecordBySlot(UINT32 slot, collectionRecord &record);

         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_COLLECTION_RECORD;
         }

      private:
         INT32 writeToSlot(UINT32 slot, const collectionRecord &record);
         INT32 getRecordPtr(UINT32 slot, const collectionRecord **record);
         INT32 getWritableRecordPtr(UINT32 slot, collectionRecord **record);

         INT32 prepareUpdateLog(requestContext *context,
                                logRecordContext *lrc,
                                DPS_LOG_TYPE ddlType,
                                BOOLEAN hasOld,
                                const slice &adjuncts);

         INT32 commitUpdateLog(requestContext *context,
                               logRecordContext *lrc,
                               DPS_LOG_TYPE ddlType,
                               UINT64 mask,
                               const collectionRecord *oldRecord,
                               const collectionRecord &newRecord,
                               const slice &adjuncts);

   };//class crpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_CRP_ACCESSOR_H_