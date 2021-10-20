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

   Source File Name = btreeIndexItem.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexItem.h"
#include "ixmKey.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   void btreeIndexItem::fini()
   {
      _slotPos = INVALID_RECORD_SLOT_ID;
      _slot = NULL;
      _keyData = NULL;
      _prefixData = NULL;
      return;
   }

   INT32 btreeIndexItem::init(RECORD_SLOT_ID slotPos,
                              const btreeItemSlot *slot,
                              const CHAR *keyData,
                              const CHAR *prefix)
   {
      INT32 rc = SDB_OK;
      fini();

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == slotPos ||
                       NULL == slot ||
                       !slot->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == keyData &&
                            (slot->isDataInExtPage() ||
                             slot->isDataInPageBody())))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(slot->isKeyCompressed() &&
                            NULL == prefix))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _slotPos = slotPos;
      _slot = slot;
      _keyData = keyData;
      _prefixData = prefix;

   done:
      return rc;
   error:
      goto done;
   }

   UINT32 btreeIndexItem::getSavingSize()const
   {
      UINT32 size = 0;
      
      if (isValid())
      {
         size = BTREE_NODE_SLOT_SIZE;
         if (_slot->isDataInPageBody() ||
             _slot->isDataInExtPage())
         {
            size += _slot->data.pointer.size;
         }
      }
      
      return size;
   }

   void btreeIndexItem::getKeyWhenNotCompressed(ixmKey &key)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_slot->isKeyCompressed(), "can not be compressed");

      if (_slot->isDataInPageBody())
      {
         key.assign(_keyData);
      }
      else
      {
         key.assign(_slot->data.getKeyData());
      }
      return;
   }

   void btreeIndexItem::buildKeyWhenCompressed(StackBufBuilder &builder)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(_slot->isKeyCompressed(), "must be compressed");
      
   }

   INT32 btreeIndexItem::woCompare(const ixmKey &key,
                                   const bson::Ordering &ordering)const
   {
      INT32 r = 0;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      ixmKey localKey;

      if (_slot->isKeyCompressed())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      else
      {
         getKeyWhenNotCompressed(localKey);
         r = localKey.woCompare(key, ordering);
      }

      return r;
   }
} // namespace vessel

} // namespace engine

