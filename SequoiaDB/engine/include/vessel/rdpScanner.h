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

   Source File Name = rdpScanner.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RDP_SCANNER_H_
#define VESSEL_RDP_SCANNER_H_

#include "vessel/recordID.h"
#include "vessel/recordDataPage.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class logicalPageBuffer;

   class rdpScanner : public SDBObject
   {
      public:
         rdpScanner(){}
         ~rdpScanner(){}
         rdpScanner(const rdpScanner &) = delete;
         rdpScanner &operator=(const rdpScanner &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _head;
         }
         INT32 open(requestContext *context,
                    const logicalPageBuffer *lpb);
         void close();

         UINT32 getTotalSlotCount()const;

         INT32 getSlot(UINT32 pos, recordSlot &rs)const;

         ///WARNING: User must parse recordSlice according rh.
         INT32 getNormalRecordHeadAndBody(UINT32 pos,
                                          recordHead &rh,
                                          slice &bodySlice)const;

         const recordDataPageHead &getPageHead()const;


      private:
         const logicalPageBuffer *_lpb = NULL;
         const recordDataPageHead *_head = NULL;
      
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_SCANNER_H_