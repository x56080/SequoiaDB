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
          06/25/2022  ZHY  Reimplementation

   Last Changed =

*******************************************************************************/
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

   INT32 lsmColumnFamily::loadSSTs(INT32 level,
                                   BOOLEAN dirIncluded,
                                   BOOLEAN creationAsc,
                                   ossPoolVector<std::string> &ssts)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _db->loadSSTs(_cfId, level, dirIncluded, creationAsc, ssts);
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

   INT32 lsmColumnFamily::getSSTCount(UINT32 &sstCount)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _db->getSSTCount(_cfId, sstCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get sst file count failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine