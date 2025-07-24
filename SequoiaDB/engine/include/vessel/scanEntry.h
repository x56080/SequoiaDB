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

   Source File Name = scanEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SCAN_ENTRY_H_
#define VESSEL_SCAN_ENTRY_H_

#include "vessel/recordID.h"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
   class scanEntry : public SDBObject
   {  
      public:
         scanEntry(){}
         explicit scanEntry(UINT32 seq, RECORD_SLOT_POS pos):
         _seq(seq), _pos(pos){}
         ~scanEntry(){}
         scanEntry(const scanEntry &o):
         _seq(o._seq),
         _pos(o._pos){}
         scanEntry &operator=(const scanEntry &o)
         {
            _seq = o._seq;
            _pos = o._pos;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN operator<(const scanEntry &o)const
         {
            if (_seq < o._seq)
            {
               return TRUE;
            }
            else if (_seq > o._seq)
            {
               return FALSE;
            }
            else
            {
               return _pos < o._pos;
            }
         }

         OSS_INLINE BOOLEAN operator>(const scanEntry &o)const
         {
            if (_seq < o._seq)
            {
               return FALSE;
            }
            else if (_seq > o._seq)
            {
               return TRUE;
            }
            else
            {
               return _pos > o._pos;
            }
         }

         OSS_INLINE BOOLEAN operator==(const scanEntry &o)const
         {
            return _seq == o._seq && _pos == o._pos;
         }

         OSS_INLINE BOOLEAN operator!=(const scanEntry &o)const
         {
            return _seq != o._seq || _pos != o._pos;
         }

         OSS_INLINE BOOLEAN operator<=(const scanEntry &o)const
         {
            return *this < o || *this == o;
         }

      public:
         OSS_INLINE UINT32 getSeq()const
         {
            return _seq;
         }
         OSS_INLINE RECORD_SLOT_POS getPos()const
         {
            return _pos;
         }

         OSS_INLINE void reset(UINT32 seq = 0,
                               RECORD_SLOT_POS pos = 0)
         {
            _seq = seq;
            _pos = pos;
            return;
         }

         OSS_INLINE void incSeqAndZeroSlot()
         {
            ++_seq;
            _pos = 0;
            return;
         }

         OSS_INLINE void incPos()
         {
            ++_pos;
            return;
         }


      private:
         UINT32 _seq = 0;
         RECORD_SLOT_POS _pos = 0;
   };//class scanEntry
}//namespace vessel
}//namespace engine

#endif//VESSEL_SCAN_ENTRY_H_