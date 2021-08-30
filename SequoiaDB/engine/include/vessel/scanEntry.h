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

   Source File Name = scanEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         explicit scanEntry(UINT32 seq, RECORD_SLOT_ID slot):
         _seq(seq), _slot(slot){}
         ~scanEntry(){}
         scanEntry(const scanEntry &o):
         _seq(o._seq),
         _slot(o._slot){}
         scanEntry &operator=(const scanEntry &o)
         {
            _seq = o._seq;
            _slot = o._slot;
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
               return _slot < o._slot;
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
               return _slot > o._slot;
            }
         }

         OSS_INLINE BOOLEAN operator==(const scanEntry &o)const
         {
            return _seq == o._seq && _slot == o._slot;
         }

         OSS_INLINE BOOLEAN operator!=(const scanEntry &o)const
         {
            return _seq != o._seq || _slot != o._slot;
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
         OSS_INLINE RECORD_SLOT_ID getSlot()const
         {
            return _slot;
         }

         OSS_INLINE void reset(UINT32 seq = 0,
                               RECORD_SLOT_ID slot = 0)
         {
            _seq = seq;
            _slot = slot;
            return;
         }

         OSS_INLINE void incSeqAndZeroSlot()
         {
            ++_seq;
            _slot = 0;
            return;
         }

         OSS_INLINE void incSlot()
         {
            ++_slot;
            return;
         }


      private:
         UINT32 _seq = 0;
         RECORD_SLOT_ID _slot = 0;
   };//class scanEntry
}//namespace vessel
}//namespace engine

#endif//VESSEL_SCAN_ENTRY_H_