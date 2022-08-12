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
          06/25/2022  ZHY  Reimplementation

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/lsm/lsmDB.h"
#include "pd.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   lsmColumnFamily::lsmColumnFamily(lsmDB *db, LSM_CF_ID cfId)
   {
      SDB_ASSERT(nullptr != db, "can not be null");
      SDB_ASSERT(LSM_CF_INVALID != cfId, "can not be invalid");
      _db = db;
      _cfId = cfId;
   }

   INT32 lsmColumnFamily::put(const rocksdb::Slice &key,
                              const rocksdb::Slice &value,
                              DPS_LSN_OFFSET lsn) const
   {
      SDB_ASSERT(isValid(), "must be valid");
      INT32 rc = SDB_OK;
      rc = _db->put(_cfId, key, value);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to put, cf[%d], rc: %d", _cfId, rc);
         goto error;
      }
      
      if (DPS_INVALID_LSN_OFFSET != lsn)
      {
         _db->setMinDirtyLsn(_cfId, lsn);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::get(const rocksdb::Slice &key,
                              std::string &value,
                              BOOLEAN &notFound) const
   {
      SDB_ASSERT(isValid(), "must be valid");
      value.clear();
      notFound = TRUE;
      INT32 rc = SDB_OK;
      rc = _db->get(_cfId, key, value, notFound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get, cf[%d], rc: %d", _cfId, rc);
         goto error;
      }
   done:
      return rc;
   error:
      value.clear();
      notFound = TRUE;
      goto done;
   }

   INT32 lsmColumnFamily::remove(const rocksdb::Slice &key) const
   {
      SDB_ASSERT(isValid(), "must be valid");
      INT32 rc = SDB_OK;
      rc = _db->remove(_cfId, key);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove, cf[%d], rc: %d", _cfId, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::truncate(const rocksdb::Slice &lowKey,
                                   const rocksdb::Slice &upKey) const
   {
      SDB_ASSERT(isValid(), "must be valid");
      INT32 rc = SDB_OK;
      rc = _db->truncate(_cfId, lowKey, upKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate, cf[%d], rc: %d", _cfId, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::compact(const rocksdb::Slice *lowKey,
                                  const rocksdb::Slice *upKey) const
   {
      SDB_ASSERT(isValid(), "must be valid");
      INT32 rc = SDB_OK;
      rc = _db->compact(_cfId, lowKey, upKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to compact, cf[%d], rc: %d", _cfId, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   rocksdb::Iterator *lsmColumnFamily::newIterator(const rocksdb::ReadOptions &opt)
   {
      SDB_ASSERT(isValid(), "must be valid");
      return _db->newIterator(_cfId, opt);
   }

   INT32 lsmColumnFamily::flush() const
   {
      SDB_ASSERT(isValid(), "must be valid");
      INT32 rc = SDB_OK;
      rc = _db->flush(_cfId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to flush, cf[%d], rc: %d", _cfId, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lsmColumnFamily::openBatch(lsmWriteBatch &batch)
   {
      SDB_ASSERT(isValid(), "must be valid");
      _db->openBatch(_cfId, batch);
   }

   DPS_LSN_OFFSET lsmColumnFamily::getMinDirtyLsn() const
   {
      SDB_ASSERT(isValid(), "must be valid");
      return _db->getMinDirtyLsn(_cfId);
   }

   INT32 lsmColumnFamily::loadSSTs(INT32 level, BOOLEAN dirIncluded, ossPoolVector<std::string> &ssts)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _db->loadSSTs(_cfId, level, dirIncluded, ssts);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load sst files:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine