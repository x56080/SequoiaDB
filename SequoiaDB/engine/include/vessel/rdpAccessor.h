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
#include "vessel/recordDataPage.h"
#include "vessel/recordID.h"
#include "vessel/slice.h"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class insertContext;
   class modifyRecordContext;
   class logicalPageBuffer;
   class logRecordContext;

   class rdpAccessor : public pageAccessor
   {
      public:
         rdpAccessor(){}
         virtual ~rdpAccessor(){}

      public:
         INT32 init(requestContext *context,
                    logicalPageBuffer *lpb);

         OSS_INLINE void fini() {_lpb = NULL;}

         /// non-big-record
         INT32 insertNormalRecord(insertContext *context);


         INT32 updateNormalRecord(modifyRecordContext *context,
                                  const slice &newRowData,
                                  BOOLEAN &outOfSpace);

         INT32 deleteRecord(modifyRecordContext *context);

      public:
         UINT32 getTotalSlotCount()const;
         INT32 getSlot(RECORD_SLOT_ID pos, recordSlot &rs)const;

         INT32 getRecord(RECORD_SLOT_ID pos,
                         recordHead &rh,
                         slice &data)const;

         const recordDataPageHead *getReadablePageHead()const;

      private:
         INT32 insertNormalRecordToPos(insertContext *context,
                                       RECORD_SLOT_ID pos,
                                       UINT16 offset);

      private:
         void updatePageHeadWhenInsert(RECORD_SLOT_ID pos,
                                       const recordSlot &slot,
                                       const recordHead &rh,
                                       STRIPING_ID striping);

         void updateStripingInfo(recordDataPageHead *head,
                                 STRIPING_ID striping);
         void updateMaxTransSN(recordDataPageHead *head,
                               UINT64 transSN);

         INT32 getPosToInsert(const recordDataPageHead *head,
                              UINT32 alignedHeadAndBodySize,
                              UINT32 minFreeSize,
                              RECORD_SLOT_ID &slotId,
                              UINT16 &offset,
                              UINT32 &totalSize)const;

         BOOLEAN findPositionToInsert(UINT32 recordSize,
                                      FLOAT32 minFreePercent,
                                      RECORD_SLOT_ID &pos,
                                      UINT16 &offset)const;

      private:
         INT32 inplaceUpdate(modifyRecordContext *context,
                             const slice &row);

      private:
         INT32 createTombstone(modifyRecordContext *context);

      private:
         const recordSlot *getReadableSlot(RECORD_SLOT_ID pos)const;
         
         UINT32 getFrontOffset(const recordDataPageHead *head)const;

         recordSlot *getWritableSlot(strictBuffer &buffer,
                                     RECORD_SLOT_ID pos);
      private:
         INT32 validatePage(requestContext *context,
                            logicalPageBuffer *lpb)const;

         INT32 prepareInsertLog(insertContext *context,
                                UINT32 recordHeadAndBodySize,
                                const runtimePageBuffer *rpb,
                                logRecordContext *lrc);

         INT32 commitInsertLog(insertContext *context,
                               const recordID &rid,
                               const recordSlot &slot,
                               const void *record,
                               const recordDataPageHead *oldHead,
                               const recordDataPageHead *newHead,
                               const runtimePageBuffer *rpb,
                               logRecordContext *lrc);

         INT32 prepareInplaceUpdateLog(modifyRecordContext *context,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc);
         INT32 prepareDeleteLog(modifyRecordContext *context,
                                const runtimePageBuffer *rpb,
                                logRecordContext *lrc);

      private:
         logicalPageBuffer *_lpb = NULL;
   };//class rdpAccessor 
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_ACCESSOR_H_