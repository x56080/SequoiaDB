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

   Source File Name = btreeNodeItem.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_ITEM_H_
#define VESSEL_BTREE_NODE_ITEM_H_

#include "vessel/btreeNodePage.h"
#include "vessel/btreeKeyStringEntry.h"

namespace engine
{
namespace vessel
{
   class btreeNodeItem : public SDBObject
   {
      public:
         btreeNodeItem() = default;
         ~btreeNodeItem() = default;
         btreeNodeItem(const btreeNodeItem &o) = delete;
         btreeNodeItem &operator=(const btreeNodeItem &o) = delete;
      public:
         OSS_INLINE const recordID &getIndexRid() const
         {
            return _rid;
         }

         OSS_INLINE const btreeItemSlot &getSlot() const
         {
            return _slot;
         }

         /// key slice may be empty
         OSS_INLINE const btreeKeyStringEntry &getEntry() const
         {
            return _entry;
         }
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _rid.isValid();
         }

         void reset();

         void init(PAGE_ID lpid,
                   RECORD_SLOT_POS pos,
                   const btreeItemSlot &slot,
                   const btreeKeyStringEntry &entry);
      private:
         btreeItemSlot _slot;
         btreeKeyStringEntry _entry;
         recordID _rid;/// index item rid
   };//class btreeNodeItem
} // namespace vessel

} // namespace engine

#endif//VESSEL_BTREE_NODE_ITEM_H_