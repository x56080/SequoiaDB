/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rdpRecordScanner.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_RDP_RECORD_SCANNER_H_
#define VESSEL_RDP_RECORD_SCANNER_H_

#include "vessel/rdpAccessor.h"
#include "vessel/memoryBlock.h"
#include "vessel/recordID.h"
#include "vessel/logicalPageBuffer.h"
#include "dmsEngineOptions.hpp"

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
         class options : public SDBObject
         {
            public:
               options(){}
               ~options(){}
               options(const options &o):
               endBound(o.endBound),
               so(o.so),
               nolockWhenScanForNone(o.nolockWhenScanForNone){}
               options &operator=(const options &o)
               {
                  endBound = o.endBound;
                  so = o.so;
                  nolockWhenScanForNone = o.nolockWhenScanForNone;
                  return *this;
               }

            public:
               /// exclusive end
               RECORD_SLOT_POS  endBound = INVALID_RECORD_SLOT_POS ;
               dmsScanOptions so;

               /// do not hold rid latch when scan for none
               BOOLEAN nolockWhenScanForNone = FALSE;
         };//class options
      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _context;
         }

         /// lock lpid and locate the first visible slot
         /// from begin to end.
         INT32 open(requestContext *context,
                    PAGE_ID lpid,
                    RECORD_SLOT_POS  begin = 0,
                    const options *o = NULL);

         void close();

         /// auto move the next valid slot fromm current pos.
         INT32 next();

         /// if it is not ready to fetch after open/next,
         /// which means hit the end.
         BOOLEAN isReadyToRead()const;

         UINT32 getCurrentPageSeq()const;

      public:
         /// 1. user should lock rid/record outside first
         /// 2. always be ready to read if return ok.
         INT32 openToRead(requestContext *context,
                          const recordID &rid);

      public:/// ensure ready to fetch first
         recordID getCurrentRid()const;

         BOOLEAN isOverflow()const;

         BOOLEAN isBigRecord()const;
         
         OSS_INLINE const DPS_TRANS_ID &getCurrentTransID()const
         {
            return _transID;
         }
         OSS_INLINE const slice &getCurrentRecord()const
         {
            return _recordData;
         }

         OSS_INLINE const recordID &getOverflowAddr()const
         {
            return _overflowAddr;
         }

      private:
         /// it will automaticlly search visible slot from pos to the end.
         /// The end pos will be end bound(if set) or the last slot in the page.
         INT32 scanFrom(RECORD_SLOT_POS  pos);

         INT32 scanWithRU(RECORD_SLOT_POS  pos);

         INT32 scanWithLockingRecord(RECORD_SLOT_POS pos);

         void clearDataCached();

         INT32 initAccessor();

         INT32 fetchRecord(RECORD_SLOT_POS pos,
                           UINT8 type);

         INT32 fetchNormalRecord(RECORD_SLOT_POS pos);

         INT32 fetchOverflowedRecord(RECORD_SLOT_POS pos);

      private:
         static constexpr UINT8 _FLAG_BIG_RECORD = 0x01;

      private:
         requestContext *_context = NULL;
         options _o;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         logicalPageBuffer _lpb;
         rdpAccessor _accessor;

         /// data fetched
         RECORD_SLOT_POS  _pos = INVALID_RECORD_SLOT_POS;
         recordID _overflowAddr;
         UINT8 _recordType = RDP_RECORD_HEAD_TYPE_INVALID;
         UINT8 _flags = 0;
         DPS_TRANS_ID _transID;
         slice _recordData;
         /// used to save uncompressed/overflowed record 
         CHAR *_recordBuffer = NULL;
         UINT32 _recordBufferSize = 0;
   };//class rdpRecordScanner
} // namespace vessel

} // namespace engine


#endif//VESSEL_RDP_RECORD_SCANNER_H_