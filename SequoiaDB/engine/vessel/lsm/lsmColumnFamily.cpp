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

******************************************************************************/

#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/lsm/lsmDB.h"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   lsmColumnFamily::lsmColumnFamily(lsmDB *db,
                                    rocksdb::ColumnFamilyHandle *handle)
   {
      SDB_ASSERT(nullptr != db, "can not be null");
      SDB_ASSERT(nullptr != handle, "can not be null");
      _db = db;
      _handle = handle;
   }

   INT32 lsmColumnFamily::put(const rocksdb::Slice &key,
                              const rocksdb::Slice &value,
                              const rocksdb::WriteOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      rocksdb::Status s;

      s = _db->getDBPtr()->Put(o, _handle, key, value);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "put key-value into RocksDB failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::get(const rocksdb::Slice &key,
                              std::string &value,
                              BOOLEAN &notFound,
                              const rocksdb::ReadOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      rocksdb::Status s;

      value.clear();
      notFound = FALSE;
      s = _db->getDBPtr()->Get(o, _handle, key, &value);
      if (s.IsNotFound())
      {
         notFound = TRUE;
         value.clear();
         goto done;
      }
      else if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "get specified value in RocksDB failed, "
                "status info:[%s]", s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::remove(const rocksdb::Slice &key,
                                 const rocksdb::WriteOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      rocksdb::Status s;

      s = _db->getDBPtr()->Delete(o, _handle, key);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "delete specified key-value in RocksDB failed, "
                "status info:[%s]", s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::truncate(const rocksdb::Slice &lowKey,
                                   const rocksdb::Slice &upKey,
                                   const rocksdb::WriteOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      rocksdb::Status s;

      s = _db->getDBPtr()->DeleteRange(o, _handle, lowKey, upKey);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "range delete key-values in RocksDB failed, "
                "status info:[%s]", s.ToString().c_str());
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::compact(const rocksdb::Slice *beginKey,
                                  const rocksdb::Slice *endKey,
                                  const rocksdb::CompactRangeOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      rocksdb::Status s;

      s = _db->getDBPtr()->CompactRange(o, _handle, beginKey, endKey);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "compact key-values failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   rocksdb::Iterator *lsmColumnFamily::newIterator(const rocksdb::ReadOptions &o)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _db->getDBPtr()->NewIterator(o, _handle);
   }

   void lsmColumnFamily::openBatch(writeBatch &batch)
   {
      batch.reset();
      batch._db = _db->getDBPtr();
      batch._handle = _handle;
      batch._wOpt.disableWAL = TRUE;
   }

   lsmColumnFamily::writeBatch::~writeBatch()
   {
      reset();
   }

   void lsmColumnFamily::writeBatch::reset()
   {
      _db = nullptr;
      _handle = nullptr;
      _batch.Clear();
      _wOpt = rocksdb::WriteOptions();
   }

   INT32 lsmColumnFamily::writeBatch::put(const rocksdb::Slice &key,
                                          const rocksdb::Slice &value)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;
      
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      s = _batch.Put(_handle, key, value);
      if (!s.ok())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "put key-value into batch failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmColumnFamily::writeBatch::commit()
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;
      
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      s = _db->Write(_wOpt, &_batch);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "write batch into rocksdb failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }
   
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine