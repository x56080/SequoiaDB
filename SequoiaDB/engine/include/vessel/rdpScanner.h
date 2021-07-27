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

#include "vessel/recordDataPage.h"

namespace engine
{
namespace vessel
{
   class scanCLContext;
   class requestContext;
   class scanCLCursor;
   class logicalPageBuffer;
   class runtimePageBuffer;

   class rdpScanner : public SDBObject
   {
      public:
         rdpScanner(){}
         ~rdpScanner(){}
         rdpScanner(const rdpScanner &) = delete;
         rdpScanner &operator=(const rdpScanner &) = delete;

      public:
         INT32 getMore(scanCLContext *context,
                       const logicalPageBuffer *lpb,
                       scanCLCursor *cursor)const;

         INT32 getRecourdCountInHead(requestContext *context,
                                     const logicalPageBuffer *lpb,
                                     UINT32 &count)const;

      private:

         INT32 getNormalRecord(scanCLContext *context,
                               const recordSlot &slot,
                               const runtimePageBuffer *rpb,
                               scanCLCursor *cursor)const;
      
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_SCANNER_H_