/*******************************************************************************


Copyright (C) 2011-2018 SequoiaDB Ltd.

                        This program is free software: you can redistribute it
and/or modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the License, or
                                         (at your option) any later version.

                                         This program is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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