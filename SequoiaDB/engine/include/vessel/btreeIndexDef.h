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

   Source File Name = btreeIndexDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_INDEX_DEF_H_
#define VESSEL_BTREE_INDEX_DEF_H_

#include "vessel/indexDef.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   /// min key size is 1.
   static const UINT32 BTREE_MIN_ENTRY_SIZE = sizeof(recordID) + sizeof(recordID) + 1;

   class insertPosition : public SDBObject
   {
      public:
         OSS_INLINE insertPosition(){}
         OSS_INLINE ~insertPosition(){}
         OSS_INLINE insertPosition(const insertPosition &o):
         child(o.child),
         slotNo(o.slotNo),
         identical(o.identical){}
         OSS_INLINE insertPosition &operator=(const insertPosition &o)
         {
            child = o.child;
            slotNo = o.slotNo;
            identical = o.identical;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_PAGE_ID != child ||
                   INVALID_RECORD_SLOT_ID != slotNo;
         }

      public:
         PAGE_ID child = INVALID_PAGE_ID;
         RECORD_SLOT_ID slotNo = INVALID_RECORD_SLOT_ID;
         BOOLEAN identical = FALSE;
   };//class insertPosition
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_INDEX_DEF_H_
