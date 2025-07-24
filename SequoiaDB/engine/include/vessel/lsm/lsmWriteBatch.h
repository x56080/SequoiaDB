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

   Source File Name = lsmWriteBatch.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/24/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_WRITE_BATCH_H_
#define VESSEL_LSM_WRITE_BATCH_H_

#include "oss.hpp"
#include "dpsDef.hpp"
#include "vessel/lsm/lsmDBDef.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/db.h"

namespace engine
{
namespace vessel
{
   class lsmDB;
   class lsmWriteBatch : public SDBObject
   {
      friend class lsmDB;
      public:
         lsmWriteBatch() = default;
         ~lsmWriteBatch() = default;
         lsmWriteBatch(const lsmWriteBatch &) = delete;
         lsmWriteBatch &operator= (const lsmWriteBatch &) = delete;
      
      public:
         OSS_INLINE BOOLEAN isOpen() const
         {
            return nullptr != _db &&
                   LSM_CF_INVALID != _id &&
                   nullptr != _handle;
         }

         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _batch.Count();
         }

      public:
         void reset();

         INT32 put(const rocksdb::Slice &key,
                   const rocksdb::Slice &value,
                   DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET);

         INT32 remove(const rocksdb::Slice &key);

         INT32 commit();

         void setMinDirtyLsn(DPS_LSN_OFFSET lsn);

      private:
         // init by lsmDB
         void _init(lsmDB *db,
                    LSM_CF_ID id,
                    rocksdb::ColumnFamilyHandle *handle);
      
      private:
         lsmDB *_db = nullptr;
         LSM_CF_ID _id = LSM_CF_INVALID;
         rocksdb::ColumnFamilyHandle *_handle = nullptr;
         rocksdb::WriteBatch _batch;
         DPS_LSN_OFFSET _minDirtyLsn = DPS_INVALID_LSN_OFFSET;
   };
} // namespace vessel
} // namespace engine


#endif // VESSEL_LSM_WRITE_BATCH_H_