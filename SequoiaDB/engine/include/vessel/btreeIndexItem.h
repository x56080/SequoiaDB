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
#include "ixmKey.hpp"
#include "../bson/ordering.h"

namespace engine
{
namespace vessel
{
   class btreeIndexItem : public SDBObject
   {
      public:
         btreeIndexItem(){}
         ~btreeIndexItem(){}
         btreeIndexItem(const btreeIndexItem &o):
         _slotPos(o._slotPos),
         _slot(o._slot),
         _keyData(o._keyData),
         _prefixData(o._prefixData)
         {}
         btreeIndexItem &operator=(const btreeIndexItem &o)
         {
            _slotPos = o._slotPos;
            _slot = o._slot;
            _keyData = o._keyData;
            _prefixData = o._prefixData;
            return *this;
         }

      public:
         OSS_INLINE RECORD_SLOT_ID getSlotPos()const
         {
            return _slotPos;
         }

         OSS_INLINE const btreeItemSlot *getSlot()const
         {
            return _slot;
         }

         /// key data may be null
         OSS_INLINE const CHAR *getKeyData()const
         {
            return _keyData;
         }

         OSS_INLINE const CHAR *getPrefixData()const
         {
            return _prefixData;
         }

         OSS_INLINE recordID getRid()const
         {
            return isValid() ?
                   recordID(_slot->ridPage, _slot->ridSlot) : recordID();
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_RECORD_SLOT_ID != _slotPos;
         }

         void fini();

         INT32 init(RECORD_SLOT_ID slotPos,
                    const btreeItemSlot *slot,
                    const CHAR *keyData,
                    const CHAR *prefix=NULL);

         UINT32 getSavingSize()const;

         void getKeyWhenNotCompressed(ixmKey &key)const;

         void buildKeyWhenCompressed(StackBufBuilder &builder)const;

         INT32 woCompare(const ixmKey &key,
                         const bson::Ordering &ordering)const;

      private:
         RECORD_SLOT_ID _slotPos = INVALID_RECORD_SLOT_ID;
         const btreeItemSlot *_slot = NULL;
         const CHAR *_keyData = NULL;
         const CHAR *_prefixData = NULL;
   };//class btreeIndexItem
} // namespace vessel

} // namespace engine

#endif//VESSEL_BTREE_INDEX_ITEM_H_