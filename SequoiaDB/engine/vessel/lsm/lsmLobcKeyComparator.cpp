/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = lsmDB.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmLobcKeyComparator.h"
#include "vessel/lsm/lsmLobChunkKey.h"

namespace engine
{
namespace vessel
{
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