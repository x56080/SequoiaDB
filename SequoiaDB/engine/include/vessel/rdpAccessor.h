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

   Source File Name = rdpAccessor.h

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

#ifndef VESSEL_RDP_ACCESSOR_H_
#define VESSEL_RDP_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/recordData.h"
#include "vessel/strSlice.h"
#include "utilCompression.hpp"
#include "vessel/vesselOptions.h"
#include "vessel/recordID.h"
#include "recordDataPage.h"

namespace engine
{
namespace vessel
{
   class logRecordContext;
   class insertContext;

   class rdpAccessor : public pageAccessor
   {
      public:
         rdpAccessor();
         virtual ~rdpAccessor();
      public:
         INT32 initRdp(requestContext *context,
                       PAGE_ID lpid,
                       UINT32 logicalID,
                       UINT32 sequence);

         INT32 insert(insertContext *context);
      private:
         INT32 validatePage(UINT32 logicalID);

      private:
         INT32 insertWithOutCompression(insertContext *context);
   
         BOOLEAN hasSpaceToInsert(const recordDataPageHead *head,
                                  UINT32 sizeNeeded,
                                  BOOLEAN &needReorg);
      private:
         INT32 getSlot(RECORD_SLOT_ID slotID, recordSlot &slot);
         INT32 writeSlot(RECORD_SLOT_ID slotID,
                         const recordSlot &slot);
         UINT32 getAlignedSizeOfNormalRecordAndHead(UINT32 recordSize);
         INT32 findFreeSlot(const recordDataPageHead *head,
                            RECORD_SLOT_ID &slotID);
         UINT32 getNonFreeBeginOffet(const recordDataPageHead *head);

         void updateMinMaxStriping(recordDataPageHead *head,
                                   STRIPING_ID striping);

      private:
         /// with out compression
         INT32 prepareInsertWOCLog(insertContext *context,
                                   logRecordContext *lrc,
                                   UINT32 rhAndbodySize);
         INT32 commitInsertWOCLog(insertContext *context,
                                  logRecordContext *lrc,
                                  const recordID &rid,
                                  const recordDataPageHead *head,
                                  const recordSlot &slot,
                                  const recordHead *rh);
   };//class rdpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_ACCESSOR_H_