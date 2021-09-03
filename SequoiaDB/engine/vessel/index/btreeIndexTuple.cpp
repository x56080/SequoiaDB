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
   btreeIndexTuple::btreeIndexTuple(const btreeNodeSlot *slot,
                                    const CHAR *data,
                                    BOOLEAN isLeaf)
   {
      init(slot, data, isLeaf);
   }

   void btreeIndexTuple::fini()
   {
      _slot = NULL;
      _data = NULL;
      _keyDataSize = 0;
      _isLeaf = FALSE;
      return;
   }

   BOOLEAN btreeIndexTuple::init(const btreeNodeSlot *slot,
                                 const CHAR *data,
                                 BOOLEAN isLeaf)
   {
      BOOLEAN r = FALSE;
      fini();

      if (OSS_UNLIKELY(NULL == slot ||
                       !slot->isValid() ||
                       NULL == data))
      {
         goto done;
      }

      _slot = slot;
      _data = data;
      _isLeaf = isLeaf;

      if (slot->isCompletelyCompressed())
      {
         _keyDataSize = 0;
      }
      else if (slot->isCompressed())
      {
         _keyDataSize = indexCompressedKey(data).getKeyDataSize();
         if (0 == _keyDataSize)
         {
            PD_LOG(PDERROR, "failed to get key data size");
            goto done;  
         }
      }
      else
      {
         _keyDataSize = ixmKey(data).dataSize();
      }

      r = TRUE;

   done:
      if (!r)
      {
         fini();
      }
      return r;
   }

   PAGE_ID btreeIndexTuple::getLeftNode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      const PAGE_ID *left = (const UINT32 *)(_data + _keyDataSize);
      return *left;
   }
} // namespace vessel

} // namespace engine

