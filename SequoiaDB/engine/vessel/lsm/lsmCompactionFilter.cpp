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

   Source File Name = lsmCompactionFilter.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/16/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmCompactionFilter.hpp"
#include "vessel/lsm/lsmIndexValue.hpp"

namespace engine
{
namespace vessel
{
   /*
      filter out the old version and deleted entry in compaction process.
      Return:
         TRUE: this entry should be deleted.
         FALSE: this entry should be saved.
      Filter process:
         1. check if full key and value's size is valid.
         2. check if value is valid.
         3. unpack the current key.
      If key and value is invalid, it means the entry is not inserted by normal way and don't need to cache for next.
         4. unpack the last key in cache.
         5. if value type is deleted, delete the entry.
         6. if indexID changed, save the entry.
         7. if ixmKey or rid changed, save the entry.
      Other condition means current entry is the old version.
         8. cache current key for next compare.

   */
   bool lsmIdxCompactionFilter::Filter(INT32 level,
                                    const Slice& key,
                                    const Slice& existing_value,
                                    std::string* new_value,
                                    bool* value_changed) const
   {
      bool result = FALSE;
      INT32 rc = SDB_OK;
      const lsmIndexValue *vl = NULL;

      lsmIdxFullKeySlice fullKey(key.data(), key.size(), TRUE);

      // check if full key and value's size is valid
      if (!fullKey.isValid() || 
          LSM_VALUE_SIZE != existing_value.size())
      {
         result = FALSE;
         ++_invalidCount;
         goto done;
      }

      vl = (const lsmIndexValue*)existing_value.data();
      // check if value is valid
      if (!vl->isValid())
      {
         result = FALSE;
         ++_invalidCount;
         goto done;
      }

      // check if current is the first latest version
      if (0 == _cachedFullKeyBuilder.len())
      {
         result = vl->isDeleted();
      }
      else
      {
         // unpack the last full key
         lsmIdxFullKeySlice lastFullKey(_cachedFullKeyBuilder.buf(),
                                        _cachedFullKeyBuilder.getSize(), FALSE);
         SDB_ASSERT(lastFullKey.isValid(), "impossible");
         if (vl->isDeleted())
         {
            result = TRUE;
         }
         else if (fullKey.getFixedKey()->indexid != lastFullKey.getFixedKey()->indexid)
         {
            result = FALSE;
         }
         else if (fullKey.getFixedKey()->rid != lastFullKey.getFixedKey()->rid ||
                  !ixmKey(fullKey.getIxmKeyData()).woEqual(ixmKey(lastFullKey.getIxmKeyData())))
         {
            result = FALSE;
         }
         else
         {
            result = TRUE;
         }
      }
      
      // cache current key and value
      _cachedFullKeyBuilder.reset();
      _cachedFullKeyBuilder.appendBuf(key.data(), key.size());
      _cachedValue = *vl;
   done:
      return result;
   }

   shared_ptr<CompactionFilterFactory> createIdxCompactionFilterFactory()
   {
      return shared_ptr<CompactionFilterFactory>(
                     new lsmIdxCompactionFilterFactory());
   }
}
}