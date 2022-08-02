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
   void btreeIndexItem::reset()
   {
      _slotPos = INVALID_RECORD_SLOT_POS;
      _slot.reset();
      _prefix = slice();
      _keySlice = slice();
      return;
   }

   void btreeIndexItem::initWhenNormal(RECORD_SLOT_POS slotPos,
                                       const btreeItemSlot &slot,
                                       const slice &key)
   {
      SDB_ASSERT(isValidRecordSlotPosition(slotPos), "can not be invalid");
      SDB_ASSERT(slot.isValid(), "can not be invalid");
      SDB_ASSERT(!slot.isKeyCompressed(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      _slotPos = slotPos;
      _slot = slot;
      _keySlice = key;
      _prefix = slice();
      return;
   }

   void btreeIndexItem::initWhenCompressed(RECORD_SLOT_POS slotPos,
                                           const btreeItemSlot &slot,
                                           const slice &prefix,
                                           const slice &suffix)
   {
      SDB_ASSERT(isValidRecordSlotPosition(slotPos), "can not be invalid");
      SDB_ASSERT(slot.isValid(), "can not be invalid");
      SDB_ASSERT(slot.isKeyCompressed(), "must be compressed");
      SDB_ASSERT(prefix.isValid(), "can not be null");
      SDB_ASSERT(!(0 < slot.data.key.size && !suffix.isValid()), "can not be invlaid");
      _slotPos = slotPos;
      _slot = slot;
      _prefix = prefix;
      _keySlice = suffix;
      return;
   }

   INT32 btreeIndexItem::getOwnedKeyString(keyString &ks) const
   {
      INT32 rc = SDB_OK;
      ks.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(!_slot.isKeyCompressed(), "TODO");
      rc = ks.init(_keySlice);
      {
         PD_LOG(PDERROR, "failed to init key string:%d", rc);
         goto error;
      }

      rc = ks.getOwned();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get owned:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      ks.reset();
      goto done;
   }

   keyString btreeIndexItem::getOwnedKeyString() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      keyString ks(_keySlice);
      if (ks.isValid())
      {
         INT32 rc = ks.getOwned();
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to get owned key string:%d", rc);
            ks.reset();
         }
      }

      return std::move(ks);
   }

} // namespace vessel

} // namespace engine

