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

   Source File Name = btreeItemLocation.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ITEM_LOCATION_H_
#define VESSEL_BTREE_ITEM_LOCATION_H_

#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   class btreeItemLocation : public SDBObject
   {
      public:
         btreeItemLocation(){}
         ~btreeItemLocation(){}
         btreeItemLocation(const btreeItemLocation &o):
         identical(o.identical),
         child(o.child),
         slotPos(o.slotPos),
         isUpperBound(o.isUpperBound){}
         btreeItemLocation &operator=(const btreeItemLocation &o)
         {
            identical = o.identical;
            child = o.child;
            slotPos = o.slotPos;
            isUpperBound = o.isUpperBound;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_RECORD_SLOT_ID != slotPos;
         }

      public:
         BOOLEAN identical = FALSE;
         PAGE_ID child = INVALID_PAGE_ID;
         RECORD_SLOT_ID slotPos = INVALID_RECORD_SLOT_ID;
         BOOLEAN isUpperBound = FALSE;
   };//class btreeItemLocation
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ITEM_LOCATION_H_
