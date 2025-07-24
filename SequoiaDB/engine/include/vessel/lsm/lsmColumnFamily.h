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
      rocksdb::Iterator *newIterator(const rocksdb::ReadOptions &opt);
      INT32 flush() const;
      void openBatch(lsmWriteBatch &batch);
      DPS_LSN_OFFSET getMinDirtyLsn() const;

   public:
      INT32 loadSSTs(INT32 level,
                     BOOLEAN dirIncluded,
                     BOOLEAN creationAsc,
                     ossPoolVector<std::string> &ssts);
      
      INT32 getSSTCount(UINT32 &sstCount);

   private:
      lsmDB *_db = nullptr;
      LSM_CF_ID _cfId = LSM_CF_INVALID;
   };

   extern lsmColumnFamily GET_HYBRID_INDEX_COLUMN_FAMILY();
   extern lsmColumnFamily GET_LOBM_COLUMN_FAMILY();
   extern lsmColumnFamily GET_INDEX_META_COLUMN_FAMILY();
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_COLUMN_FAMILY_H_