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

   Source File Name = btreeNodeItem.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNodeItem.h"
#include "ixmKey.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   void btreeNodeItem::reset()
   {
      _rid.reset();
      _slot.reset();
      _entry.reset();
      return;
   }

   void btreeNodeItem::init(PAGE_ID lpid,
                            RECORD_SLOT_POS pos,
                            const btreeItemSlot &slot,
                            const btreeKeyStringEntry &entry)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(slot.isValid(), "can not be invalid");
      SDB_ASSERT(entry.isValid(), "can not be invalid");
      _rid.setPid(lpid);
      _rid.setPos(pos);
      _slot = slot;
      _entry = entry;
      return;
   }

} // namespace vessel

} // namespace engine

