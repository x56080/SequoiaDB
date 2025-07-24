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

   Source File Name = lsmWriteBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/27/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmDB.h"
#include "vessel/lsm/lsmWriteBatch.h"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   void lsmWriteBatch::reset()
   {
      _db = nullptr;
      _id = LSM_CF_INVALID;
      _handle = nullptr;
      _batch.Clear();
      _minDirtyLsn = DPS_INVALID_LSN_OFFSET;
   }

   void lsmWriteBatch::_init(lsmDB *db,
                             LSM_CF_ID id,
                             rocksdb::ColumnFamilyHandle *handle)
   {
      SDB_ASSERT(nullptr != db, "can not be null");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be null");
      SDB_ASSERT(nullptr != handle, "can not be null");

      reset();
      _db = db;
      _id = id;
      _handle = handle;
   }


   INT32 lsmWriteBatch::put(const rocksdb::Slice &key,
                            const rocksdb::Slice &value,
                            DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;
      if (nullptr == _db)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         PD_LOG(PDERROR, "lsm write batch must be initialized");
         goto error;
      }

      s = _batch.Put(_handle ,key, value);
      if (!s.ok())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to put to batch, rc: %d", rc);
         goto error;
      }

      if (lsn < _minDirtyLsn)
      {
         _minDirtyLsn = lsn;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmWriteBatch::remove(const rocksdb::Slice &key)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;

      if (nullptr == _db)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         PD_LOG(PDERROR, "lsm write batch must be initialized");
         goto error;
      }

      s = _batch.Delete(_handle, key);
      if (!s.ok())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "remove specified key failed");
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmWriteBatch::commit()
   {
      INT32 rc = SDB_OK;

      if (nullptr == _db)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         PD_LOG(PDERROR, "lsm write batch must be initialized");
         goto error;
      }

      rc = _db->write(*this);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit batch, rc: %d",rc);
         goto error;
      }

      _db->setMinDirtyLsn(_id, _minDirtyLsn);
   done:
      return rc;
   error:
      goto done;
   }

   void lsmWriteBatch::setMinDirtyLsn(DPS_LSN_OFFSET lsn)
   {
      if (lsn < _minDirtyLsn)
      {
         _minDirtyLsn = lsn;
      }
   }
} // namespace vessel
} // namespace engine