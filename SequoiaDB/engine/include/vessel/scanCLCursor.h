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
#include "vessel/collectionHandle.h"
#include "vessel/scanCLOptions.h"
#include "vessel/scanEntry.h"

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

         void resetToScan(const collectionHandle &handle,
                          const scanCLOptions &options)
         {
            _handle = handle;
            _options = options;
            _lpid = INVALID_PAGE_ID;
            _toScan.reset();
            return;
         }
         OSS_INLINE const collectionHandle &getHandle()const
         {
            return _handle;
         }
         OSS_INLINE const scanCLOptions &getOptions()const
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

         OSS_INLINE void setLpid(PAGE_ID lpid)
         {
            _lpid = lpid;
         }
         OSS_INLINE const scanEntry &getToScanEntry()const
         {
            return _toScan;
         }

      public:
         virtual INT32 getNextRow(ISession *session,
                                  cursorRow *row);

      private:
         scanCLOptions _options;
         collectionHandle _handle;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         scanEntry _toScan;
   };//class scanCLCursor
}//namespace vessel
}//namespace engine

#endif//VESSEL_SCAN_CL_CURSOR_H_