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

   Source File Name = pageCompactor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/31/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
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
         UINT32 getBufferSize()const;

         const CHAR* getBuffer();

      private:
         strictBuffer _buffer;
         UINT32 _backOffset = 0;
         UINT32 _frontOffset = 0;  
         UINT32 _totalFreeSpace = 0;
         UINT32 _totalSlotCount = 0;
     
   };
}
}






#endif