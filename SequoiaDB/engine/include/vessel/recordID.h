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

   Source File Name = recordID.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RECORD_ID_H_
#define VESSEL_RECORD_ID_H_

#include "vessel/pageDef.h"
#include "dms.hpp"
#include "xxHashInc.h"

namespace engine
{
namespace vessel
{
   typedef UINT16 RECORD_SLOT_ID;
   const RECORD_SLOT_ID INVALID_RECORD_SLOT_ID = 65535;
#pragma pack(2)
   class recordID
   {
      public:
         OSS_INLINE recordID()
         :_page(INVALID_PAGE_ID),
          _slot(INVALID_RECORD_SLOT_ID)
          {}

         OSS_INLINE explicit recordID(PAGE_ID pid, RECORD_SLOT_ID slotID)
         :_page(pid), _slot(slotID){}

         OSS_INLINE recordID(const recordID &id)
         :_page(id._page),
          _slot(id._slot)
         {
         }

         OSS_INLINE BOOLEAN operator==(const recordID &o)const
         {
            return _page == o._page && _slot == o._slot;
         }

         OSS_INLINE BOOLEAN operator!=(const recordID &o)const
         {
            return _page != o._page || _slot != o._slot;
         }

         OSS_INLINE recordID &operator=(const recordID &id)
         {
            _page = id._page;
            _slot = id._slot;
            return *this;
         }

         OSS_INLINE BOOLEAN operator<(const recordID &r)const
         {
            return compare(r) < 0;
         }

         OSS_INLINE BOOLEAN operator<=(const recordID &r)const
         {
            return compare(r) <= 0;
         }

         OSS_INLINE ~recordID(){}

         OSS_INLINE INT32 compare(const recordID &rid)const
         {
            INT32 res = (INT32)_page - (INT32)rid._page;
            if (0 == res)
            {
               res = (INT32)_slot - (INT32)rid._slot;
            }
            return res;
         }

         OSS_INLINE void setPageID(PAGE_ID id)
         {
            _page = id;
         }

         OSS_INLINE PAGE_ID getPageID()const
         {
            return _page;
         }

         OSS_INLINE void setSlotID(RECORD_SLOT_ID id)
         {
            _slot = id;
         }

         OSS_INLINE RECORD_SLOT_ID getSlotID()const
         {
            return _slot;
         }

         OSS_INLINE BOOLEAN valid()const
         {
            return INVALID_RECORD_SLOT_ID != _slot &&
                   INVALID_PAGE_ID != _page;
         }

         OSS_INLINE dmsRecordID toDMSRid()const
         {
            if (valid())
            {
               return dmsRecordID(_page, _slot);
            }
            return dmsRecordID();
         }

         OSS_INLINE void reset(const dmsRecordID &rid)
         {
            if (rid.isValid())
            {
               SDB_ASSERT(rid._offset < INVALID_RECORD_SLOT_ID, "out of bound");
               _page = rid._extent;
               _slot = rid._offset;
            }
            else
            {
               _page = INVALID_PAGE_ID;
               _slot = INVALID_RECORD_SLOT_ID;
            }
         }

         OSS_INLINE UINT32 hash()const
         {
            UINT64 v = _page;
            v <<= 32;
            v |= _slot;
            return XXH3_64bits(&v, sizeof(v));
         }

      private:
         PAGE_ID _page;
         RECORD_SLOT_ID _slot;
   }; /// end of class recordID
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_ID_H_