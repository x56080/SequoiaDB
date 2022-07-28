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

   Source File Name = lsmDB.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/instanceEnv.h"
#include "vessel/threadContext.h"
#include "vessel/lsm/lsmDB.h"
#include "vessel/lsm/lsmIndexKey.h"
#include "vessel/lsm/lsmLobcKeyComparator.h"
#include "vessel/lsm/lsmColumnFamilyContext.h"
#include "vessel/lsm/lsmCollector.h"
#include "vessel/lsm/lsmEventListener.h"
#include "rocksdb/table.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/slice_transform.h"

namespace engine
{
namespace vessel
{
   extern const rocksdb::Comparator* getHitComparator();
   extern const rocksdb::SliceTransform *getLsmIndexPrefixTransform();

   lsmDB::~lsmDB()
   {
      close();
   }

   BOOLEAN lsmDB::isOpen()const
   {
      return nullptr != _db &&
             !_contexts.empty();
   }

   INT32 lsmDB::open(const CHAR *dbPath,
                     const rocksdb::Options *o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != dbPath, "can not be null");
      rocksdb::Status s;
      rocksdb::Options opt;
      std::vector<rocksdb::ColumnFamilyDescriptor> descs;
      std::vector<rocksdb::ColumnFamilyHandle *> cfHandles;

      close();

      if (nullptr != o)
      {
         opt = *o;
      }
      opt.create_if_missing = TRUE;
      opt.create_missing_column_families = TRUE;
      // opt.listeners.emplace_back(newLsmEventListener(this));
      opt.max_background_jobs = 4;

      // configure column family names and options
      for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
      {
         descs.push_back(_getDescriptor(static_cast<LSM_CF_ID>(i), opt));
      }

      s = rocksdb::DB::Open(opt, dbPath, descs, &cfHandles, &_db);
      if (!s.ok())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "open rocksdb failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

      for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
      {
         lsmColumnFamilyContext *context = 
               SDB_OSS_NEW lsmColumnFamilyContext(cfHandles[i], 
                                                  _getDefaultWriteOptions(
                                                     static_cast<LSM_CF_ID>(i)));
         if (nullptr == context)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "out of memory");
            goto error;
         }
         _contexts.emplace_back(context);
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void lsmDB::close()
   {
      rocksdb::Status s;
      if (isOpen())
      {
         for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
         {
            s = _db->DestroyColumnFamilyHandle(_contexts[i]->getHandle());
            SDB_ASSERT(s.ok(), "failed to destroy column family handle");
            SAFE_OSS_DELETE(_contexts[i]);
         }
         _db->Close();
         delete _db;
         _db = nullptr;
         // _journal = nullptr;
         _contexts.clear();
      }
   }

   lsmColumnFamily lsmDB::getHitColumnFamily()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return lsmColumnFamily(this, LSM_CF_HYBRID_INDEX);
   }

   lsmColumnFamily lsmDB::getLobmColumnFamily()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return lsmColumnFamily(this, LSM_CF_LOBM);
   }

   lsmColumnFamily lsmDB::getIdxMetaColumnFamily()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return lsmColumnFamily(this, LSM_CF_INDEX_META);
   }

   INT32 lsmDB::put(LSM_CF_ID id,
                    const rocksdb::Slice &key,
                    const rocksdb::Slice &value)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (LSM_CF_INVALID == id)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      s = _db->Put(_contexts[id]->getWriteOpt(),
                   _contexts[id]->getHandle(),
                   key, value);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "put key-value into lsmDB failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::get(LSM_CF_ID id,
                    const rocksdb::Slice &key,
                    std::string &value,
                    BOOLEAN &notFound)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;

      value.clear();
      notFound = FALSE;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (LSM_CF_INVALID == id)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      s = _db->Get(rocksdb::ReadOptions(),
                   _contexts[id]->getHandle(),
                   key, &value);
      if (s.IsNotFound())
      {
         value.clear();
         notFound = TRUE;
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
      value.clear();
      notFound = TRUE;
      goto done;
   }

   INT32 lsmDB::remove(LSM_CF_ID id,
                       const rocksdb::Slice &key)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (LSM_CF_INVALID == id)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      s = _db->Delete(_contexts[id]->getWriteOpt(),
                      _contexts[id]->getHandle(),
                      key);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "delete specified key-value in RocksDB failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::truncate(LSM_CF_ID id,
                         const rocksdb::Slice &lowKey,
                         const rocksdb::Slice &upKey)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (LSM_CF_INVALID == id)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      s = _db->DeleteRange(_contexts[id]->getWriteOpt(),
                           _contexts[id]->getHandle(),
                           lowKey, upKey);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "range delete key-values in RocksDB failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::compact(LSM_CF_ID id,
                        const rocksdb::Slice *lowKey,
                        const rocksdb::Slice *upKey)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (LSM_CF_INVALID == id)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      s = _db->CompactRange(rocksdb::CompactRangeOptions(),
                            _contexts[id]->getHandle(),
                            lowKey, upKey);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "compact key-values in RocksDB failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done; 
   }

   rocksdb::Iterator *lsmDB::newIterator(LSM_CF_ID id,
                                         const rocksdb::ReadOptions &opt)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      return _db->NewIterator(opt, _contexts[id]->getHandle());
   }

   void lsmDB::openBatch(LSM_CF_ID id, lsmWriteBatch &batch)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
   
      batch._db = this;
      batch._handle = _contexts[id]->getHandle();
      batch._batch.Clear();
      batch._minDirtyLsn = DPS_INVALID_LSN_OFFSET;
   }

   INT32 lsmDB::write(lsmWriteBatch &batch)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      s = _db->Write(_contexts[batch._handle->GetID()]->getWriteOpt(),
                     &batch._batch);
      if (!s.ok())
      {
         rc = SDB_IO;
         PD_LOG(PDERROR, "write batch into RocksDB failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::flush(LSM_CF_ID id)
   {
      INT32 rc = SDB_OK;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (LSM_CF_INVALID != id)
      {
         rc = _flushCF(id);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "flush column family[%d] failed, rc:%d",
                   id, rc);
            goto error;
         }
      }
      else
      {
         rc = _flushDB();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "flush lsmDB failed, rc:%d",
                   id, rc);
            goto error;
         }
      }
   
   done:
      return rc;
   error:
      goto done;
   }

   // void lsmDB::onFlush(DPS_LSN_OFFSET maxLsn)
   // {
   //    SDB_ASSERT(nullptr != _journal, "can not be null");
   //    _journal->flush(maxLsn);
   // }

   void lsmDB::setMinDirtyLsn(LSM_CF_ID id,
                              DPS_LSN_OFFSET lsn)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be null");
      _contexts[id]->setMinDirtyLsn(lsn);
   }

   DPS_LSN_OFFSET lsmDB::getMinDirtyLsn(LSM_CF_ID id) const
   {
      SDB_ASSERT(isOpen(), "must be open");
      if (LSM_CF_INVALID != id)
      {
         return _contexts[id]->getMinDirtyLsn();
      }
      else
      {
         DPS_LSN_OFFSET r = DPS_INVALID_LSN_OFFSET;
         for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
         {
            DPS_LSN_OFFSET tmp = _contexts[i]->getMinDirtyLsn();
            if (tmp < r)
            {
               r = tmp;
            }
         }
         return r;
      }
   }

   // void lsmDB::setJournal(IDataJournal *journal)
   // {
   //    SDB_ASSERT(nullptr != journal, "can not be nullptr");
   //    _journal=journal;
   // }

   INT32 lsmDB::_flushDB()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
      {
         rc = _flushCF(static_cast<LSM_CF_ID>(i));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "flush column family[%d] failed, rc:%d", i, rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::_flushCF(LSM_CF_ID id)
   {
      INT32 rc = SDB_OK;
      rocksdb::Status s;
      DPS_LSN_OFFSET tmpLsn = DPS_INVALID_LSN_OFFSET;
      BOOLEAN flushDone = FALSE;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      std::unique_lock<std::mutex> lock(_contexts[id]->getFlushLock());

      tmpLsn = _contexts[id]->beginToFlush();
      if (DPS_INVALID_LSN_OFFSET != tmpLsn)
      {
         s = _db->Flush(rocksdb::FlushOptions(),
                        _contexts[id]->getHandle());
         if (!s.ok())
         {
            rc = SDB_IO;
            PD_LOG(PDERROR, "flush column family[%d] failed, status info:[%s]",
                  s.ToString().c_str());
            goto error;
         }
      }
      flushDone = TRUE;

   done:
      _contexts[id]->endToFlush(flushDone);
      return rc;
   error:
      goto done;
   }

   rocksdb::ColumnFamilyDescriptor lsmDB::_getDescriptor(LSM_CF_ID id,
                                             const rocksdb::Options &opt)const
   {
      std::string cfName;
      rocksdb::ColumnFamilyOptions cfOpt(opt);

      if (LSM_CF_DEFAULT == id)
      {
         cfName = LSM_DEFAULT_CF_NAME;
      }
      else if (LSM_CF_HYBRID_INDEX == id)
      {
         cfName = LSM_HYBRID_INDEX_CF_NAME;
         cfOpt.comparator = getHitComparator();
         cfOpt.table_properties_collector_factories.emplace_back(newLsmCollectorFactory());
         cfOpt.prefix_extractor.reset(getLsmIndexPrefixTransform());
         //cfOpt.level0_slowdown_writes_trigger = 1024;
         //cfOpt.level0_stop_writes_trigger = 1536;
         cfOpt.compression = rocksdb::kNoCompression;
         //cfOpt.compaction_style = rocksdb::kCompactionStyleNone;

         rocksdb::BlockBasedTableOptions tableOpt;
         tableOpt.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
         tableOpt.whole_key_filtering = FALSE;
         cfOpt.table_factory.reset(rocksdb::NewBlockBasedTableFactory(tableOpt));
      }
      else if (LSM_CF_LOBM == id)
      {
         cfName = LSM_LOBM_CF_NAME;
         cfOpt.comparator = lsmLobcKeyComparator();
      }
      else if (LSM_CF_INDEX_META == id)
      {
         cfName = LSM_INDEX_META_CF_NAME;
         cfOpt.write_buffer_size = 64 * 1024;
      }

      return rocksdb::ColumnFamilyDescriptor(cfName, cfOpt);
   }

   rocksdb::WriteOptions lsmDB::_getDefaultWriteOptions(LSM_CF_ID id)const
   {
      rocksdb::WriteOptions wOpt;
      if (LSM_CF_HYBRID_INDEX == id)
      {
         wOpt.disableWAL = TRUE;
      }
      else if (LSM_CF_LOBM == id)
      {
         wOpt.disableWAL = TRUE;
      }
      else if (LSM_CF_INDEX_META == id)
      {
      }

      return wOpt; 
   }

   lsmColumnFamily GET_HYBRID_INDEX_COLUMN_FAMILY()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return tc->getEnv()->lsm->getHitColumnFamily();
   }

   lsmColumnFamily GET_LOBM_COLUMN_FAMILY()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return tc->getEnv()->lsm->getLobmColumnFamily();
   }

   lsmColumnFamily GET_INDEX_META_COLUMN_FAMILY()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return tc->getEnv()->lsm->getIdxMetaColumnFamily();
   }

} // namespace vessel
} // namespace engine
