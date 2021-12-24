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

   Source File Name = scanCLCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_SCAN_COLLECTION;
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
         OSS_INLINE void setToScanSlot(RECORD_SLOT_ID slot)
         {
            _toScan.reset(_toScan.getSeq(), slot);
         }
         OSS_INLINE void incToScanSlot()
         {
            _toScan.incSlot();
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