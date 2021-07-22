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

   Source File Name = rdpInsertExecutor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RDP_INSERT_EXECUTOR_H_
#define VESSEL_RDP_INSERT_EXECUTOR_H_

#include "vessel/pageAccessor.h"

namespace engine
{
namespace vessel
{
   class insertContext;
   class logicalPageBuffer;
   class logRecordContext;

   class rdpInsertExecutor : public pageAccessor
   {
      public:
         rdpInsertExecutor();
         virtual ~rdpInsertExecutor();

      public:
         /// non-big-record
         INT32 insertNormalRecord(insertContext *context,
                                  logicalPageBuffer *lpb);

      private:
         INT32 insertWithNormalHead(insertContext *context,
                                    RECORD_SLOT_ID slotId,
                                    const recordSlot &slot,
                                    const recordHead &rh,
                                    logicalPageBuffer *lpb);

      private:
         void updatePageHead(recordDataPageHead *head,
                             RECORD_SLOT_ID slotId,
                             const recordSlot &slot,
                             const recordHead &rh,
                             STRIPING_ID striping);

         INT32 getPosToInsert(const recordDataPageHead *head,
                              UINT32 alignedHeadAndBodySize,
                              RECORD_SLOT_ID &slotId,
                              UINT16 &offset)const;

      private:
         INT32 prepareInsertLog(insertContext *context,
                                UINT32 recordHeadAndBodySize,
                                const runtimePageBuffer *rpb,
                                logRecordContext *lrc);

         INT32 commitInsertLog(insertContext *context,
                               const recordID &rid,
                               const recordSlot &slot,
                               const recordHead *record,
                               const recordDataPageHead *oldHead,
                               const recordDataPageHead *newHead,
                               const runtimePageBuffer *rpb,
                               logRecordContext *lrc);
   };//class rdpInsertExecutor 
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_INSERT_EXECUTOR_H_