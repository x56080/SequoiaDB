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

   Source File Name = lsmLobChunkKey.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmLobChunkKey.h"
#include "rocksdb/comparator.h"

namespace engine
{
namespace vessel
{
   INT32 lsmLobChunkKey::compare(const lsmLobChunkKey &key)const
   {
      INT32 res = 0;
      INT32 oidcmp = 0;

      if (_csId < key._csId)
      {
         res = -1;
         goto done;
      }
      else if (_csId > key._csId)
      {
         res = 1;
         goto done;
      }

      if (_clId < key._clId)
      {
         res = -1;
         goto done;
      }
      else if (_clId > key._clId)
      {
         res = 1;
         goto done;
      }

      oidcmp = _oid.compare(key._oid);
      if (oidcmp > 0)
      {
         res = 1;
         goto done;
      }
      else if (oidcmp < 0)
      {
         res = -1;
         goto done;
      }

      if (_chunkId < key._chunkId)
      {
         res = -1;
         goto done;
      }
      else if (_chunkId > key._chunkId)
      {
         res = 1;
         goto done;
      }

   done:
      return res;
   }

   INT32 lsmLobcKeyComparatorImpl::Compare(const rocksdb::Slice &a,
                                           const rocksdb::Slice &b)const
   {
      if (LSM_LOB_CHUNK_KEY_SIZE != a.size() ||
          LSM_LOB_CHUNK_KEY_SIZE != b.size())
      {
         if (a.size() > b.size())
         {
            return 1;
         }
         else if (a.size() < b.size())
         {
            return -1;
         }
         else
         {
            return 0;
         }
      }

      const lsmLobChunkKey *aKey = (const lsmLobChunkKey *)a.data();
      const lsmLobChunkKey *bKey = (const lsmLobChunkKey *)b.data();
      return aKey->compare(*bKey);
   }

   const rocksdb::Comparator* lsmLobcKeyComparator()
   {
      static lsmLobcKeyComparatorImpl _lsmLobcKeyComparator;
      return &_lsmLobcKeyComparator;
   }

} // namespace vessel

} // namespace engine
