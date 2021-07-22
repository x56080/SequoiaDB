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

   Source File Name = scanCLContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SCAN_CL_CONTEXT_H_
#define VESSEL_SCAN_CL_CONTEXT_H_

#include "vessel/requestContext.h"
#include "vessel/recordID.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class scanCLContext : public requestContext
   {
      public:
         scanCLContext(){}
         virtual ~scanCLContext(){}
      public:

         void setScanning(const recordID &rid)
         {
            _scanning = rid;
         }
         const recordID &getScanning()const
         {
            return _scanning;
         }
         const recordID &getNext()const
         {
            return _next;
         }
         void setNext(const recordID &rid)
         {
            _next = rid;
         }
         BOOLEAN hasNext()const
         {
            return _next.valid();
         }
         memoryBlock &getMemBlock()
         {
            return _mb;
         }
         BOOLEAN isOverflow()const
         {
            return _overflow;
         }
         BOOLEAN isBigRecord()const
         {
            return _bigRecord;
         }
         void setOverflow()
         {
            _overflow = TRUE;
         }
         void setBigRecord()
         {
            _bigRecord = TRUE;
         }
      private:
         BOOLEAN _overflow = FALSE;
         BOOLEAN _bigRecord = FALSE;
         recordID _scanning;
         recordID _next;
         memoryBlock _mb;
   };//class scanCLContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_SCAN_CL_CONTEXT_H_