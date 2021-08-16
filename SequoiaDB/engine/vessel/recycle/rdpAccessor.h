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
   class logicalPageBuffer;
   class insertContext;
   class scanCLCursor;

   class rdpAccessor : public pageAccessor
   {
      public:
         rdpAccessor(){}
         virtual ~rdpAccessor(){}
      public:


         INT32 insertNormalRecord(insertContext *context,
                                  logicalPageBuffer *lpb);

         INT32 getMoreWhenScan(scanCLContext *context,
                               scanCLCursor *cursor);

         INT32 getRecordCount(requestContext *context,
                              UINT32 &count);

         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_RECORD;
         }
      private:
         INT32 validatePage(UINT32 logicalID);
         OSS_INLINE UINT32 getAlignedSizeOfNormalRecordAndHead(UINT32 recordSize)
         {
            return RDP_RECORD_HEAD_LEN + ossAlign4(recordSize);
         }

      private:
         INT32 readRecordInSlot(scanCLContext *context,
                                RECORD_SLOT_ID slotID,
                                const recordSlot &slot,
                                scanCLCursor *cursor);

         INT32 readNormalRecord(scanCLContext *context,
                                RECORD_SLOT_ID slotID,
                                const recordHead *head,
                                scanCLCursor *cursor);

      private:
         INT32 insertWithNormalRecordHead(insertContext *context,
                                          const recordData &rd);
   
         BOOLEAN hasSpaceToInsert(const recordDataPageHead *head,
                                  UINT32 sizeNeeded,
                                  BOOLEAN &needReorg);
      private:
         INT32 getSlot(RECORD_SLOT_ID slotID, recordSlot &slot);
         INT32 writeSlot(RECORD_SLOT_ID slotID, const recordSlot &slot);

         UINT32 getNonFreeBeginOffet(const recordDataPageHead *head);

         ///WARNING: May return invalid slot id. Which means
         /// not any more free slot exists.
         INT32 findNextFreeSlot(const recordDataPageHead *head,
                                RECORD_SLOT_ID &slotID);

         void updateMinMaxStriping(recordDataPageHead *head,
                                   STRIPING_ID striping);

      private:
         INT32 prepareInsertLog(insertContext *context,
                                logRecordContext *lrc,
                                UINT32 rhAndbodySize);

         INT32 commitInsertLog(insertContext *context,
                               logRecordContext *lrc,
                               const recordID &rid,
                               const recordDataPageHead *oldHead,
                               const recordDataPageHead *newHead,
                               const recordSlot &slot,
                               const recordHead *rh);
   };//class rdpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_ACCESSOR_H_