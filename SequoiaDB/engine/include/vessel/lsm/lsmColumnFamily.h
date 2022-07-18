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

   Source File Name = lsmColumnFamily.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_COLUMN_FAMILY_H_
#define VESSEL_LSM_COLUMN_FAMILY_H_

#include "oss.hpp"
#include "dpsDef.hpp"
#include "vessel/lsm/lsmDBDef.h"
#include "vessel/lsm/lsmWriteBatch.h"
#include "rocksdb/slice.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/options.h"
#include "rocksdb/iterator.h"

namespace engine
{
namespace vessel
{
   class lsmDB;
   class lsmColumnFamily : public SDBObject
   {
   public:
      lsmColumnFamily() = default;
      lsmColumnFamily(lsmDB *db, LSM_CF_ID cfId);
      ~lsmColumnFamily() = default;
      lsmColumnFamily(const lsmColumnFamily &cf) = default;
      lsmColumnFamily &operator=(const lsmColumnFamily &cf) = default;
   
   public:
      OSS_INLINE BOOLEAN isValid() const
      {
         return nullptr != _db &&
                LSM_CF_INVALID != _cfId;
      }

   public:
      INT32 put(const rocksdb::Slice &key,
                const rocksdb::Slice &value,
                DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET) const;
      INT32 get(const rocksdb::Slice &key,
                std::string &value,
                BOOLEAN &notFound) const;
      INT32 remove(const rocksdb::Slice &key) const;
      INT32 truncate(const rocksdb::Slice &lowKey,
                     const rocksdb::Slice &upKey) const;
      INT32 compact(const rocksdb::Slice *lowKey = nullptr,
                    const rocksdb::Slice *upKey = nullptr) const;
      rocksdb::Iterator *newIterator(const rocksdb::ReadOptions &opt);
      INT32 flush() const;
      void openBatch(lsmWriteBatch &batch);
      DPS_LSN_OFFSET getMinDirtyLsn() const;

   public:
      lsmDB *_db = nullptr;
      LSM_CF_ID _cfId = LSM_CF_INVALID;
   };

   extern lsmColumnFamily GET_HYBRID_INDEX_COLUMN_FAMILY();
   extern lsmColumnFamily GET_LOBM_COLUMN_FAMILY();
   extern lsmColumnFamily GET_INDEX_META_COLUMN_FAMILY();
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_COLUMN_FAMILY_H_