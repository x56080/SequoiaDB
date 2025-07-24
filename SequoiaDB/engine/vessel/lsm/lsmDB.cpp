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
#include "vessel/lsm/lsmLobcKeyComparator.h"
#include "vessel/lsm/lsmColumnFamilyContext.h"
#include "vessel/lsm/lsmCollector.h"
#include "vessel/lsm/lsmEventListener.h"
#include "vessel/lsm/lsmTableProperties.h"
#include "vessel/lsm/lsmIteratorBound.h"
#include "rocksdb/table.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/statistics.h"

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
      return nullptr != _db;
   }

   INT32 lsmDB::open(const CHAR *dbPath,
                     const lsmDBOptions *o)
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
         _o = *o;
      }

      opt.create_if_missing = TRUE;
      opt.create_missing_column_families = TRUE;
      opt.max_background_jobs = 4;
#if defined(_DEBUG)
      opt.statistics = rocksdb::CreateDBStatistics();
      opt.stats_dump_period_sec = 10;
#endif 

      for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
      {
         lsmColumnFamilyContext *context = 
               SDB_OSS_NEW lsmColumnFamilyContext(_getDefaultWriteOptions(
                                                     static_cast<LSM_CF_ID>(i)));
         if (nullptr == context)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "out of memory");
            goto error;
         }
         _contexts.emplace_back(context);
      }

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

      SDB_ASSERT(nullptr != _db, "can not be null");
      SDB_ASSERT(cfHandles.size() == _contexts.size(), "must be equal");
      for (UINT32 i = 0; i < cfHandles.size(); ++i)
      {
         _contexts[i]->setHandle(cfHandles[i]);
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
      if (nullptr != _db)
      {
         for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
         {
            if (nullptr != _contexts[i])
            {
               rocksdb::ColumnFamilyHandle *handle = _contexts[i]->getHandle();
               if (nullptr != handle)
               {
                  s = _db->DestroyColumnFamilyHandle(handle);
                  if (OSS_UNLIKELY(!s.ok()))
                  {
                     PD_LOG(PDERROR, "destroy column family[%d] handle failed, "
                           "status info:[%s]", i, s.ToString().c_str());
                  }
               }
               SAFE_OSS_DELETE(_contexts[i]);
            }
         }
         _contexts.clear();
         s = _db->Close();
         if (OSS_UNLIKELY(!s.ok()))
         {
            PD_LOG(PDERROR, "close db failed, status info:[%s]",
                   s.ToString().c_str());
         }
         delete _db;
         _db = nullptr;
      }
      else
      {
         for (UINT32 i = 0; i < _contexts.size(); ++i)
         {
            SAFE_OSS_DELETE(_contexts[i]);
         }
         _contexts.clear();
      }
      _o = lsmDBOptions();
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

      s = _db->Put(_getWriteOpt(id),
                   _getHandle(id),
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
                   _getHandle(id),
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
      
      s = _db->Delete(_getWriteOpt(id),
                       _getHandle(id),
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
      
      s = _db->DeleteRange(_getWriteOpt(id),
                           _getHandle(id),
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

   rocksdb::Iterator *lsmDB::newIterator(LSM_CF_ID id,
                                         const rocksdb::ReadOptions &opt)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      return _db->NewIterator(opt, _getHandle(id));
   }

   void lsmDB::openBatch(LSM_CF_ID id, lsmWriteBatch &batch)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
   
      batch._init(this, id, _getHandle(id));
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
      
      s = _db->Write(_getWriteOpt(batch._id),
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
      _getColumnFamilyCtx(id)->setMinDirtyLsn(lsn);
   }

   DPS_LSN_OFFSET lsmDB::getMinDirtyLsn(LSM_CF_ID id) const
   {
      SDB_ASSERT(isOpen(), "must be open");
      if (LSM_CF_INVALID != id)
      {
         return _getColumnFamilyCtx(id)->getMinDirtyLsn();
      }
      else
      {
         DPS_LSN_OFFSET r = DPS_INVALID_LSN_OFFSET;
         for (UINT32 i = LSM_CF_DEFAULT; i <= LSM_CF_MAX; ++i)
         {
            DPS_LSN_OFFSET tmp = _getColumnFamilyCtx(id)->getMinDirtyLsn();
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
      std::unique_lock<std::mutex> lock(_getColumnFamilyCtx(id)->getFlushLock());

      tmpLsn = _getColumnFamilyCtx(id)->beginToFlush();
      if (DPS_INVALID_LSN_OFFSET != tmpLsn)
      {
         s = _db->Flush(rocksdb::FlushOptions(), _getHandle(id));
         if (OSS_UNLIKELY(!s.ok()))
         {
            rc = SDB_IO;
            PD_LOG(PDERROR, "flush column family[%d] failed, status info:[%s]",
                   id, s.ToString().c_str());
            goto error;
         }
      }
      flushDone = TRUE;

   done:
      _getColumnFamilyCtx(id)->endToFlush(flushDone);
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
         cfOpt.write_buffer_size = _o.hitCfMemtableSize;
         cfOpt.comparator = getHitComparator();
         cfOpt.table_properties_collector_factories.emplace_back(newLsmCollectorFactory());
         cfOpt.prefix_extractor.reset(getLsmIndexPrefixTransform());
         cfOpt.level0_slowdown_writes_trigger = 1024;
         cfOpt.level0_stop_writes_trigger = 1536;
         cfOpt.compression = rocksdb::kNoCompression;
         //cfOpt.compression = rocksdb::kLZ4HCCompression;
         cfOpt.compaction_style = rocksdb::kCompactionStyleNone;

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

   INT32 lsmDB::loadSSTs(LSM_CF_ID id,
                         INT32 level,
                         BOOLEAN dirIncluded,
                         BOOLEAN creationAsc,
                         ossPoolVector<std::string> &ssts)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 == level, "welcome to DIY :-)");
      ssts.clear();

      if (OSS_UNLIKELY(LSM_CF_INVALID == id))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         rocksdb::ColumnFamilyMetaData meta;
         _db->GetColumnFamilyMetaData(_getHandle(id), &meta);
         if (!meta.levels.empty())
         {
            const std::vector<rocksdb::SstFileMetaData> &s = meta.levels.front().files;
            ssts.reserve(s.size());

            if (creationAsc)
            {
               for (auto i = s.crbegin(); i != s.crend(); ++i)
               {
                  std::string name;
                  if (dirIncluded)
                  {
                     name.append(i->directory).append(OSS_FILE_SEP).append(i->relative_filename);
                  }
                  else
                  {
                     name = i->relative_filename;
                  }
                  ssts.push_back(std::move(name));
               }
            }
            else
            {
               for (auto i = s.cbegin(); i != s.cend(); ++i)
               {
                  std::string name;
                  if (dirIncluded)
                  {
                     name.append(i->directory).append(OSS_FILE_SEP).append(i->relative_filename);
                  }
                  else
                  {
                     name = i->relative_filename;
                  }
                  ssts.push_back(std::move(name));
               }
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::removeSST(const std::string &name)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(name.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         rocksdb::Status s = _db->DeleteFile(name);
         if (!s.ok())
         {
            PD_LOG(PDERROR, "failed to delete file[%s], detail:%s",
                   name.c_str(), s.getState());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   lsmColumnFamilyContext *lsmDB::_getColumnFamilyCtx(LSM_CF_ID id)
   {
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      SDB_ASSERT((UINT32)id < _contexts.size(), "out of bound");
      return _contexts[id];
   }

   const lsmColumnFamilyContext *lsmDB::_getColumnFamilyCtx(LSM_CF_ID id) const
   {
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      SDB_ASSERT((UINT32)id < _contexts.size(), "out of bound");
      return _contexts[id];
   }

   rocksdb::ColumnFamilyHandle *lsmDB::_getHandle(LSM_CF_ID id) const
   {
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      SDB_ASSERT((UINT32)id < _contexts.size(), "out of bound");
      SDB_ASSERT(isOpen(), "must be open");
      return _contexts[id]->getHandle();
   }

   const rocksdb::WriteOptions &lsmDB::_getWriteOpt(LSM_CF_ID id) const
   {
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      SDB_ASSERT((UINT32)id < _contexts.size(), "out of bound");
      SDB_ASSERT(isOpen(), "must be open");
      return _contexts[id]->getWriteOpt();
   }

   INT32 lsmDB::restore(DPS_LSN_OFFSET checkpointLsn, DPS_LSN_OFFSET dpsMaxLsn)
   {
      INT32 rc = SDB_OK;
      
      if (DPS_INVALID_LSN_OFFSET == checkpointLsn ||
          DPS_INVALID_LSN_OFFSET == dpsMaxLsn ||
          checkpointLsn > dpsMaxLsn)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _restoreHybridIndexCF(checkpointLsn, dpsMaxLsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "restore hybrid index column family failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::_restoreHybridIndexCF(DPS_LSN_OFFSET checkpointLsn,
                                      DPS_LSN_OFFSET dpsMaxLsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != checkpointLsn, "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != dpsMaxLsn, "can not be invalid");
      SDB_ASSERT(checkpointLsn <= dpsMaxLsn, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      rocksdb::Status s;
      rocksdb::TablePropertiesCollection tpc;
      DPS_LSN_OFFSET minLsn = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET maxLsn = DPS_INVALID_LSN_OFFSET;
      BOOLEAN hasInvalid = FALSE;

      PD_LOG(PDINFO, "start to restore hybrid index column family, "
             "checkpoint lsn:[%llu], dps max lsn:[%llu]",
             checkpointLsn, dpsMaxLsn);

      s = _db->GetPropertiesOfAllTables(_getHandle(LSM_CF_HYBRID_INDEX), &tpc);
      if (OSS_UNLIKELY(!s.ok()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "get properties of cf[%d] failed, status info:[%s]",
                LSM_CF_HYBRID_INDEX, s.ToString().c_str());
         goto error;
      }

      if (tpc.empty())
      {
         PD_LOG(PDINFO, "restore hybrid index column family finished, "
                "no sst files exist in cf");
         goto done;
      }

      rc = _extractLsnFromProperties(tpc, minLsn, maxLsn);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "extract lsn from properties failed, rc:%d", rc);
         goto error;
      }

      PD_LOG(PDINFO, "total sst files count:[%zu], min lsn:[%llu], max lsn:[%llu]",
             tpc.size(), minLsn, maxLsn);

      if (DPS_INVALID_LSN_OFFSET == minLsn ||
          DPS_INVALID_LSN_OFFSET == maxLsn)
      {
         PD_LOG(PDINFO, "restore hybrid index column family finished, "
                "no sst files to restore in cf");
         goto done;
      }

      rc = _checkToRestore(checkpointLsn, dpsMaxLsn, minLsn, maxLsn, hasInvalid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "restore is not permitted, rc:%d", rc);
         goto error;
      }

      if (!hasInvalid)
      {
         PD_LOG(PDINFO, "restore hybrid index column family finished, "
                "all records in cf are valid");
         goto done;
      }
      else
      {
         PD_LOG(PDINFO, "will remove all sst files in hybrid index column family");
         rc = _recreateCFWhenRestore(LSM_CF_HYBRID_INDEX);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "drop cf[%d] to restore failed, rc:%d",
                   LSM_CF_HYBRID_INDEX, rc);
            goto error;
         }
         PD_LOG(PDINFO, "restore hybrid index column family finished, "
                "all sst files in cf are deleted");
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::_extractLsnFromProperties(const rocksdb::TablePropertiesCollection &tpc,
                                          DPS_LSN_OFFSET &minLsn,
                                          DPS_LSN_OFFSET &maxLsn) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!tpc.empty(), "can not be empty");
      SDB_ASSERT(isOpen(), "must be open");

      minLsn = DPS_INVALID_LSN_OFFSET;
      maxLsn = DPS_INVALID_LSN_OFFSET;
      for (auto itr = tpc.cbegin(); itr != tpc.cend(); ++itr)
      {
         DPS_LSN_OFFSET curMinLsn = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET curMaxLsn = DPS_INVALID_LSN_OFFSET;
         lsmTableProperties properties;
         properties.init(itr->second.get());
         if (!properties.hasUserDefinedProperties())
         {
            continue;
         }
         
         rc = properties.getLSNPair(curMinLsn, curMaxLsn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "get lsn pair failed, rc:%d", rc);
            goto error;
         }

         if (DPS_INVALID_LSN_OFFSET == minLsn ||
             curMinLsn < minLsn)
         {
            minLsn = curMinLsn;
         }

         if (DPS_INVALID_LSN_OFFSET == maxLsn ||
             curMaxLsn > maxLsn)
         {
            maxLsn = curMaxLsn;
         }
      }
      
   done:
      return rc;
   error:
      minLsn = DPS_INVALID_LSN_OFFSET;
      maxLsn = DPS_INVALID_LSN_OFFSET;
      goto done;
   }

   INT32 lsmDB::_checkToRestore(DPS_LSN_OFFSET checkpointLsn,
                                DPS_LSN_OFFSET dpsMaxLsn,
                                DPS_LSN_OFFSET minLsn,
                                DPS_LSN_OFFSET maxLsn,
                                BOOLEAN &hasInvalid) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != checkpointLsn, "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != dpsMaxLsn, "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != minLsn, "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != maxLsn, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      hasInvalid = FALSE;

      if (maxLsn > dpsMaxLsn)
      {
         // If there's a record's lsn less than checkpoint lsn,
         // delete records will cause the record to be lost,
         // restore is not permitted in this case.
         if (minLsn < checkpointLsn)
         {
            rc = SDB_INVALID_OPERATION;
            PD_LOG(PDERROR,
                   "min lsn[%llu] is less than checkpoint lsn[%llu]",
                   minLsn, checkpointLsn);
            goto error;
         }
         else
         {
            PD_LOG(PDINFO, "max lsn[%llu] is greater than dps max lsn[%llu], "
                   "sst files need to be deleted", maxLsn, dpsMaxLsn);
            hasInvalid = TRUE;
         }
      }
   
   done:
      return rc;
   error:
      hasInvalid = FALSE;
      goto done;
   }

   INT32 lsmDB::_recreateCFWhenRestore(LSM_CF_ID id)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(LSM_CF_INVALID != id, "can not be invalid");
      rocksdb::Status s;
      lsmColumnFamilyContext *ctx = nullptr;
      rocksdb::ColumnFamilyDescriptor desc;
      rocksdb::ColumnFamilyHandle *handle = _getHandle(id);
      SDB_ASSERT(nullptr != handle, "can not be null");

      SAFE_OSS_DELETE(_contexts[id]);

      s = handle->GetDescriptor(&desc);
      if (OSS_UNLIKELY(!s.ok()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "get cf[%d] descriptor failed, status info:[%s]",
                id, s.ToString().c_str());
         goto error;
      }

      s = _db->DropColumnFamily(handle);
      if (OSS_UNLIKELY(!s.ok()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "drop column family[%d] failed, status info:[%s]",
                id, s.ToString().c_str());
         goto error;
      }

      s = _db->DestroyColumnFamilyHandle(handle);
      if (OSS_UNLIKELY(!s.ok()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "destroy column family[%d] handle failed, "
                "status info:[%s]", id, s.ToString().c_str());
         goto error;
      }

      handle = nullptr;
      ctx = SDB_OSS_NEW lsmColumnFamilyContext(_getDefaultWriteOptions(id));
      if (OSS_UNLIKELY(nullptr == ctx))
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "allocate lsm column family context failed");
         goto error;
      }

      s = _db->CreateColumnFamily(desc.options, desc.name, &handle);
      if (OSS_UNLIKELY(!s.ok()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "create column family[%d] failed, "
                "status info:[%s]", id, s.ToString().c_str());
         goto error;
      }
      
      ctx->setHandle(handle);
      _contexts[id] = ctx;

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(ctx);
      goto done;
   }

   INT32 lsmDB::getSSTCount(LSM_CF_ID id, UINT32 &sstCount)
   {
      INT32 rc = SDB_OK;
      rocksdb::ColumnFamilyMetaData meta;
      sstCount = 0;

      if (OSS_UNLIKELY(LSM_CF_INVALID == id))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _db->GetColumnFamilyMetaData(_getHandle(id), &meta);
      sstCount = meta.file_count;

   done:
      return rc;
   error:
      goto done;
   }

////////////////////////////////
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
