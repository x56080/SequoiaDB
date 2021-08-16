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

   Source File Name = recordReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RECORD_READER_H_
#define VESSEL_RECORD_READER_H_

#include "vessel/rdpScanner.h"
#include "vessel/memoryBlock.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/recordID.h"
#include "vessel/recordDataPage.h"
#include "vessel/sharedObjectMap.hpp"

namespace engine
{
namespace vessel
{
   class mainDataSpace;
   class requestContext;

   class recordReader : public SDBObject
   {
      public:
         recordReader();
         ~recordReader();
         recordReader(const recordReader &) = delete;
         recordReader &operator=(const recordReader &) = delete;

      public:
         INT32 init(requestContext *context,
                    PAGE_ID lpid,
                    mainDataSpace *mds,
                    RECORD_SLOT_ID seek = 0,
                    memoryBlock *mb=NULL);

         void fini();

         /// Always fetch first before any reading.
         /// WARNING:Any data of current record will be released
         /// when call "fetchNextToReader".
         INT32 fetchNextToReader(BOOLEAN &hitTheEnd);

         BOOLEAN isCurrentRecordIsTombstone()const;
         recordID getCurrentRid()const;
         const recordHead &getCurrentRecordHead()const
         {
            return _currentRecordHead;
         }
         slice getCurrentRecordBody()const
         {
            return _currentRecord;
         }

      private:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _context;
         }
         OSS_INLINE BOOLEAN isCurrentRecordAvailable()const
         {
            return INVALID_RECORD_SLOT_ID != _currentSlot;
         }

         void clearCurrentRecord();

         INT32 openScanner();

         void closeScanner();

         INT32 fetchRecord(RECORD_SLOT_ID slotID,
                           const recordSlot &slot);

      private:
         requestContext *_context = NULL;
         rdpScanner _scanner;
         logicalPageBuffer _lpb;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         mainDataSpace *_mds = NULL;
         memoryBlock *_buffer = NULL;
         memoryBlock _mb;
         RECORD_SLOT_ID _nextSlot = 0;
         RECORD_SLOT_ID _currentSlot = INVALID_RECORD_SLOT_ID;
         recordHead _currentRecordHead;
         slice _currentRecord;
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_READER_H_