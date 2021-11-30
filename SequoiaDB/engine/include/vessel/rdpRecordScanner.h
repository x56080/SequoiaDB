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

   Source File Name = rdpRecordScanner.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RDP_RECORD_SCANNER_H_
#define VESSEL_RDP_RECORD_SCANNER_H_

#include "vessel/rdpReader.h"
#include "vessel/memoryBlock.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   class rdpRecordScanner : public SDBObject
   {
      public:
         rdpRecordScanner(){}
         virtual ~rdpRecordScanner();

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _context;
         }

         /// lock lpid and prepare to read.
         /// min(endBound, real last slot) is inclusive end of scanning.
         INT32 open(requestContext *context,
                    PAGE_ID lpid,
                    memoryBlock *buffer=NULL,
                    RECORD_SLOT_ID endBound = INVALID_RECORD_SLOT_ID);

         void close();

         /// locate once at least before any fetching.
         /// it will automaticlly search visible slot begin from pos to the end.
         /// The end pos will be end bound(if set) or the last slot in the page.
         INT32 locate(RECORD_SLOT_ID pos);

         /// auto move the next valid slot fromm current pos.
         INT32 next();

         /// if it is not ready to fetch after locate/next,
         /// which means hit the end.
         BOOLEAN isReadyToFetch()const;

      public:/// ensure ready to fetch first
         recordID getCurrentRid()const;
         BOOLEAN isTombstoneRecord()const;
         BOOLEAN isOverflowRecord()const;
         BOOLEAN isBigRecord()const;

         INT32 fetchRecord();

      public:/// fetch record to reader first
         
         const DPS_TRANS_ID &getCurrentTransID()const
         {
            return _transID;
         }
         const slice &getCurrentRecord()const
         {
            return _recordData;
         }

      private:
         void clearDataCached();

         INT32 fetchNormalRecord();

         INT32 searchVisibleAndStableSlot(RECORD_SLOT_ID pos);

      private:
         requestContext *_context = NULL;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         RECORD_SLOT_ID _endBound = INVALID_RECORD_SLOT_ID;
         rdpReader _reader;

         RECORD_SLOT_ID _pos = INVALID_RECORD_SLOT_ID;
         recordSlot _rs;
         const recordHead *_rh = NULL;
         DPS_TRANS_ID _transID;
         slice _recordData;

         memoryBlock *_buffer = NULL;
         memoryBlock _mb;
   };//class rdpRecordScanner
} // namespace vessel

} // namespace engine


#endif//VESSEL_RDP_RECORD_SCANNER_H_