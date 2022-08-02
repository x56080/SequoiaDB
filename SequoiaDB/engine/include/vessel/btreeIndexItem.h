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

   Source File Name = btreeIndexItem.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_INDEX_ITEM_H_
#define VESSEL_BTREE_INDEX_ITEM_H_

#include "vessel/btreeNodePage.h"
#include "vessel/keyString.h"

namespace engine
{
namespace vessel
{
   class btreeIndexItem : public SDBObject
   {
      public:
         btreeIndexItem(){}
         ~btreeIndexItem(){}
         btreeIndexItem(const btreeIndexItem &o) = delete;
         btreeIndexItem &operator=(const btreeIndexItem &o) = delete;
      public:
         OSS_INLINE RECORD_SLOT_POS getSlotPos()const
         {
            return _slotPos;
         }

         OSS_INLINE const btreeItemSlot &getSlot()const
         {
            return _slot;
         }

         /// key slice may be empty
         OSS_INLINE const slice &getKeySlice() const
         {
            return _keySlice;
         }

         OSS_INLINE const slice &getPrefix()const
         {
            return _prefix;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return isValidRecordSlotPosition(_slotPos);
         }

         void reset();

         /// not compressed and not ext key
         void initWhenNormal(RECORD_SLOT_POS slotPos,
                             const btreeItemSlot &slot,
                             const slice &key);

         void initWhenCompressed(RECORD_SLOT_POS slotPos,
                                 const btreeItemSlot &slot,
                                 const slice &prefix,
                                 const slice &suffix);
                                 
         INT32 getOwnedKeyString(keyString &ks) const;

         keyString getOwnedKeyString() const;

      private:
         RECORD_SLOT_POS _slotPos = INVALID_RECORD_SLOT_POS;
         btreeItemSlot _slot;
         slice _prefix;
         slice _keySlice;
   };//class btreeIndexItem
} // namespace vessel

} // namespace engine

#endif//VESSEL_BTREE_INDEX_ITEM_H_