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

   Source File Name = scanCLCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SCAN_CL_CURSOR_H_
#define VESSEL_SCAN_CL_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "vessel/scanEntry.h"
#include "vessel/objectIdentifier.h"
#include "dmsEngineOptions.hpp"

namespace engine
{
namespace vessel
{
   class scanCLCursor : public cursorKernal
   {
      public:
         scanCLCursor(){}
         virtual ~scanCLCursor(){}

      public:
         virtual const CHAR *getName()const override
         {
            return "vessel.scanCLCursor";
         }

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_SCAN_COLLECTION;
         }
         virtual slice getDataSlice()const override
         {
            constexpr UINT32 _SIZE = sizeof(dmsRecordID) + sizeof(DPS_TRANS_ID);
            slice s;
            slice raw = cursorKernal::getRawData();
            if (_SIZE < raw.getSize())
            {
               s = raw.getSlice(_SIZE, raw.getSize() - _SIZE);
            }
            return s;
         }

         void resetToScan(const globalCollectionId &gcid,
                          const dmsScanOptions &options)
         {
            _gcid = gcid;
            _options = options;
            _lpid = INVALID_PAGE_ID;
            _toScan.reset();
            return;
         }
         OSS_INLINE const globalCollectionId &getCollectionId()const
         {
            return _gcid;
         }
         OSS_INLINE const dmsScanOptions &getOptions()const
         {
            return _options;
         }
         OSS_INLINE PAGE_ID getLpid()const
         {
            return _lpid;
         }
         OSS_INLINE void incToScanPage()
         {
            _lpid = INVALID_PAGE_ID;
            _toScan.incSeqAndZeroSlot();
         }
         OSS_INLINE void setToScanSlot(RECORD_SLOT_POS slot)
         {
            _toScan.reset(_toScan.getSeq(), slot);
         }
         OSS_INLINE void incToScanSlot()
         {
            _toScan.incPos();
         }

         OSS_INLINE void setLpid(PAGE_ID lpid)
         {
            _lpid = lpid;
         }
         OSS_INLINE const scanEntry &getToScanEntry()const
         {
            return _toScan;
         }

      private:
         dmsScanOptions _options;
         globalCollectionId _gcid;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         scanEntry _toScan;
   };//class scanCLCursor
}//namespace vessel
}//namespace engine

#endif//VESSEL_SCAN_CL_CURSOR_H_