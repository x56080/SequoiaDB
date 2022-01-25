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
#include "vessel/dmlRequest.h"
#include "dmsStripingId.hpp"
#include "vessel/bigRecordStream.h"

namespace engine
{
namespace vessel
{
   class dmlContext;
   class logicalPageBuffer;
   class logRecordContext;
   class requestContext;

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
         INT32 insertNormalRecord(dmlContext *context,
                                  const dmlInsertRequest &request);

         INT32 insertOverflowedRecord(dmlContext *context,
                                      const slice &record,
                                      recordID &rid);

         INT32 insertBigRecordSlice(dmlContext *context,
                                    bigRecordStream &recordStream);
         
         INT32 updateNormalRecord(dmlContext *context,
                                  RECORD_SLOT_POS pos,
                                  const dmsStripingId &striping,
                                  const slice &newRowData,
                                  BOOLEAN &outOfSpace);
         
         INT32 setRecordOverflowed(dmlContext *context,
                                   RECORD_SLOT_POS pos,
                                   const dmsStripingId &striping,
                                   const recordID &overflowAddr);

         INT32 updateOverflowedInfo(dmlContext *context,
                                    RECORD_SLOT_POS pos,
                                    const dmsStripingId &striping,
                                    const recordID &overflowAddr);

         INT32 deleteRecord(dmlContext *context,
                            RECORD_SLOT_POS  pos);

         INT32 destroySlotAndData(dmlContext *context,
                                  RECORD_SLOT_POS pos);

      public:
         UINT32 getFreeSpaceAfterLastSlot()const;
         UINT32 getTotalSlotCount()const;
         INT32 getSlot(RECORD_SLOT_POS  pos, recordSlot &rs)const;

         INT32 getNormalRecord(RECORD_SLOT_POS  pos,
                               normalRecordHead &rh,
                               slice &data)const;

         INT32 getOverflowedRecord(RECORD_SLOT_POS pos,
                                   overflowedRecord &ofr)const;

         INT32 getBigRecordEntrySlice(RECORD_SLOT_POS pos,
                                      bigRecordEntrySlice &entry,
                                      slice &data)const;

         INT32 getBigRecordBodySlice(RECORD_SLOT_POS pos,
                                     bigRecordBodySlice &body,
                                     slice &data)const;

         const recordDataPageHead *getReadablePageHead()const;

         INT32 getRecordCount(UINT32 &count)const;

         FLOAT32 getFreeSpacePercent()const;

      private:
         INT32 insertBigRecordEntrySlice(dmlContext *context,
                                         bigRecordStream &recordStream);

         INT32 insertBigRecordBodySlice(dmlContext *context,
                                        bigRecordStream &recordStream);

      private:
         INT32 insertNormalRecordToPos(dmlContext *context,
                                       const dmlInsertRequest &request,
                                       RECORD_SLOT_POS  pos,
                                       UINT16 offset);
         
         INT32 insertOverflowedRecordToPos(dmlContext *context,
                                           const slice &row,
                                           RECORD_SLOT_POS pos,
                                           UINT16 offset);

      private:
         void updatePageHeadWhenInsert(RECORD_SLOT_POS  pos,
                                       const recordSlot &slot,
                                       const DPS_TRANS_ID &transID,
                                       const dmsStripingId &striping);

         void updateStripingInfo(recordDataPageHead *head,
                                 const dmsStripingId &striping);
         void updateMaxTransSN(recordDataPageHead *head,
                               UINT64 transSN);

         INT32 getPosToInsert(const recordDataPageHead *head,
                              UINT32 alignedHeadAndBodySize,
                              UINT32 minFreeSize,
                              RECORD_SLOT_POS  &slotId,
                              UINT16 &offset,
                              UINT32 &totalSize)const;

         BOOLEAN findPositionToInsert(UINT32 recordSize,
                                      FLOAT32 minFreePercent,
                                      RECORD_SLOT_POS  &pos,
                                      UINT16 &offset)const;

      private:
         INT32 inplaceUpdate(dmlContext *context,
                             RECORD_SLOT_POS  pos,
                             const dmsStripingId &striping,
                             const slice &row);

         INT32 updateByResaving(dmlContext *context,
                                RECORD_SLOT_POS  pos,
                                const dmsStripingId &striping,
                                const slice &row);

         INT32 updateByCompaction(dmlContext *context,
                                  RECORD_SLOT_POS  pos,
                                  const dmsStripingId &striping,
                                  const slice &row);

      private:
         INT32 createTombstone(dmlContext *context,
                               RECORD_SLOT_POS  pos);

      private:
         const recordSlot *getReadableSlot(RECORD_SLOT_POS  pos)const;
         
         UINT32 getFrontOffset(const recordDataPageHead *head)const;

         recordSlot *getWritableSlot(strictBuffer &buffer,
                                     RECORD_SLOT_POS  pos);
      private:
         INT32 validatePage(requestContext *context,
                            logicalPageBuffer *lpb)const;

         INT32 prepareInsertLog(dmlContext *context,
                                UINT32 recordHeadAndBodySize,
                                const runtimePageBuffer *rpb,
                                logRecordContext *lrc);

         INT32 commitInsertLog(dmlContext *context,
                               const recordID &rid,
                               const recordSlot &slot,
                               const void *record,
                               const recordDataPageHead *oldHead,
                               const recordDataPageHead *newHead,
                               const runtimePageBuffer *rpb,
                               logRecordContext *lrc);

         INT32 prepareInplaceUpdateLog(dmlContext *context,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc);
         INT32 prepareDeleteLog(dmlContext *context,
                                const runtimePageBuffer *rpb,
                                logRecordContext *lrc);

      private:
         logicalPageBuffer *_lpb = NULL;
   };//class rdpAccessor 
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_ACCESSOR_H_