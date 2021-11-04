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
      _slot.reset();
      _keyData = NULL;
      _prefixData = NULL;
      _extKeyBuffer.release();
      return;
   }

   void btreeIndexItem::initWhenNormal(RECORD_SLOT_ID slotPos,
                                       const btreeItemSlot *slot,
                                       const CHAR *keyData)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotPos , "can not be invalid");
      SDB_ASSERT(NULL != slot && slot->isValid(), "can not be invalid");
      SDB_ASSERT(!slot->isKeyCompressed() || !slot->isKeyInExtPage(), "can not be invalid");
      SDB_ASSERT(NULL != keyData, "can not be invalid");
      _slotPos = slotPos;
      _slot = *slot;
      _keyData = keyData;
      _prefixData = NULL;
      _extKeyBuffer.release();

      return;
   }

   void btreeIndexItem::initWhenCompressed(RECORD_SLOT_ID slotPos,
                                           const btreeItemSlot *slot,
                                           const CHAR *suffixData,
                                           const CHAR *prefix)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotPos , "can not be invalid");
      SDB_ASSERT(NULL != slot && slot->isValid(), "can not be invalid");
      SDB_ASSERT(slot->isKeyCompressed(), "must be compressed");
      SDB_ASSERT(NULL != prefix, "can not be null");
      SDB_ASSERT(!(slot->data.key.size < 0 && NULL == suffixData), "can not be invlaid");
      _slotPos = slotPos;
      _slot = *slot;
      _keyData = suffixData;
      _prefixData = prefix;
      _extKeyBuffer.release();
      return;
   }

   INT32 btreeIndexItem::initWhenExtKey(RECORD_SLOT_ID slotPos,
                                        const btreeItemSlot *slot,
                                        UINT32 keySize,
                                        const CHAR *keyData)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotPos , "can not be invalid");
      SDB_ASSERT(NULL != slot && slot->isValid(), "can not be invalid");
      SDB_ASSERT(slot->isKeyInExtPage(), "must be ext key");
      SDB_ASSERT(0 < keySize && NULL != keyData, "can not be invalid");
      INT32 rc = SDB_OK;
      _slotPos = slotPos;
      _slot = *slot;
      rc = _extKeyBuffer.copy(keySize, keyData);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy ext key:%d", rc);
         goto error;
      }

      _keyData = _extKeyBuffer.getBuffer();
      _prefixData = NULL;

   done:
      return rc;
   error:
      goto done;
   }

   UINT32 btreeIndexItem::getSavingSize()const
   {      
      return isValid() ? (BTREE_NODE_SLOT_SIZE + getSavedKeyDataSize()) : 0;
   }

   INT32 btreeIndexItem::woCompare(const ixmKey &key,
                                   const bson::Ordering &ordering)const
   {
      INT32 r = 0;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      ixmKey localKey;

      if (_slot.isKeyCompressed())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      else
      {
         SDB_ASSERT(NULL != _keyData, "impossible");
         r = ixmKey(_keyData).woCompare(key, ordering);
      }

      return r;
   }

   UINT32 btreeIndexItem::getSavedKeyDataSize()const
   {
      if (isValid())
      {
         return _slot.isKeyInExtPage() ?
                _extKeyBuffer.getSize() : _slot.data.key.size;
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         return 0;
      }
   }

   UINT32 btreeIndexItem::getOriginalKeySize()const
   {
      if (isValid())
      {
         if (_slot.isKeyInExtPage())
         {
            return _extKeyBuffer.getSize();
         }
         else if (_slot.isKeyCompressed())
         {
            return ixmKey(_prefixData).dataSize() +
                   _slot.data.key.size;
         }
         else
         {
            return _slot.data.key.size;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         return 0;
      }
   }

   void btreeIndexItem::exportOriginalKey(StackBufBuilder &builder)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_slot.isKeyCompressed(), "TODO");
      builder.appendBuf(_keyData, getSavedKeyDataSize());
      return;
   }

/*
   void btreeIndexItem::cacheOriginalKey()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_slot.isKeyCompressed(), "TODO");
      if (0 == _keyBuffer.len())
      {
         exportOriginalKey(_keyBuffer);
      }
   }*/
} // namespace vessel

} // namespace engine

