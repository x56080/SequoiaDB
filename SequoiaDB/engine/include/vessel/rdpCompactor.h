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

   Source File Name = pageCompactor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/31/2021  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_RDP_COMPACTOR_H_
#define VESSEL_RDP_COMPACTOR_H_
#include "vessel/strictBuffer.h"
#include "vessel/recordDataPage.h"

namespace engine
{
namespace vessel
{
   class rdpCompactor : public SDBObject
   {   
      public:
         rdpCompactor(){};
         rdpCompactor(CHAR* buf, UINT32 bufSize);
         virtual ~rdpCompactor(){};
         rdpCompactor(const rdpCompactor &) = delete;
         rdpCompactor &operator=(const rdpCompactor &) = delete;
         

      public:
         void reset(CHAR* buf, UINT32 bufSize);
         INT32 push(const UINT16 &slotFlags, 
                    const UINT8 &slotType, 
                    const slice &recordHeadAndData);
         INT32 push(const UINT16 &slotFlags,
                    const UINT8 &slotType, 
                    const slice &recordHead, 
                    const slice &recordData);

         INT32 pushEmptySlot();

         UINT32 getFrontOffset()const {return _frontOffset;}
         UINT32 getBackOffset()const {return _backOffset;}
         UINT32 getCorrectBackOffset()const 
         {
            return RECORD_PAGE_HEAD_SIZE + _backOffset;
         }
         UINT32 getSlotCount()const {return _totalSlotCount;}
         UINT32 getFreeSpace()const {return _totalFreeSpace;}
         UINT32 getBufferSize()const {return _bufferSize;}

         const CHAR* getBuffer();

      private:
         strictBuffer _buffer;
         UINT32 _bufferSize = 0;
         UINT32 _backOffset = 0;
         UINT32 _frontOffset = 0;  
         UINT32 _totalFreeSpace = 0;
         UINT32 _totalSlotCount = 0;
     
   };
}
}






#endif