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

   Source File Name = btreeIndexTuple.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexTuple.h"
#include "ixmKey.hpp"
#include "vessel/indexCompressedKey.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   void btreeIndexTuple::fini()
   {
      _slotNo = INVALID_RECORD_SLOT_ID;
      _slot = NULL;
      _keyData = NULL;
      return;
   }

   BOOLEAN btreeIndexTuple::init(RECORD_SLOT_ID slotNo,
                                 const btreeNodeSlot *slot,
                                 const CHAR *keyData)
   {
      BOOLEAN r = FALSE;
      fini();

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == slotNo ||
                       NULL == slot ||
                       !slot->isValid()))
      {
         goto done;
      }
      else if (!slot->isKeySavedInSlot() && NULL == keyData)
      {
         PD_LOG(PDERROR, "key data is null");
         goto done;
      }

      _slotNo = slotNo;
      _slot = slot;
      _keyData = keyData;

      r = TRUE;

   done:
      if (!r)
      {
         fini();
      }
      return r;
   }

   void btreeIndexTuple::getKeyWhenNotCompressed(ixmKey &key)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_slot->isKeyCompressed(), "can not be compressed");
      if (_slot->isKeySavedInSlot())
      {
         key.assign(_slot->data.keyData);
      }
      else
      {
         SDB_ASSERT(NULL != _keyData, "impossible");
         key.assign(_keyData);
      }
      return;
   }
} // namespace vessel

} // namespace engine

