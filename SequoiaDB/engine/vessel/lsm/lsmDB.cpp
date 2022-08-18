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
#include "vessel/lsm/lsmLobcKeyComparator.h"
#include "vessel/lsm/lsmColumnFamilyContext.h"
#include "vessel/lsm/lsmCollector.h"
#include "vessel/lsm/lsmEventListener.h"
#include "vessel/lsm/lsmIndexEntryValue.h"
#include "vessel/keyString.h"
#include "vessel/sliceTransfer.h"
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
   
      batch._db = this;
      batch._handle = _getHandle(id);
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
      
      s = _db->Write(_getWriteOpt(
                        static_cast<LSM_CF_ID>(batch._handle->GetID())),
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
         cfOpt.comparator = getHitComparator();
         cfOpt.table_properties_collector_factories.emplace_back(newLsmCollectorFactory());
         cfOpt.prefix_extractor.reset(getLsmIndexPrefixTransform());
         cfOpt.level0_slowdown_writes_trigger = 1024;
         cfOpt.level0_stop_writes_trigger = 1536;
         cfOpt.compression = rocksdb::kNoCompression;
         //cfOpt.compression = rocksdb::kLZ4HCCompression;
         cfOpt.disable_auto_compactions = TRUE;

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

   INT32 lsmDB::restore(DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      
      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _restoreHybridIndexCF(lsn);
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

   INT32 lsmDB::_restoreHybridIndexCF(DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      rocksdb::Status s;
      ossPoolVector<std::string> namelist;
      rocksdb::CompactRangeOptions crOpt;

      rc = loadSSTs(LSM_CF_HYBRID_INDEX, 0, TRUE, namelist);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "load sstables failed, rc:%d", rc);
         goto error;
      }
      else if (namelist.empty())
      {
         goto done;
      }

      rc = _restoreHybridIndexSsts(lsn, namelist);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "restore sst files failed, rc:%d", rc);
         goto error;
      }

      crOpt.change_level = TRUE;
      crOpt.target_level = 0;
      s = _db->CompactRange(crOpt, _getHandle(LSM_CF_HYBRID_INDEX), nullptr, nullptr);
      if (OSS_UNLIKELY(!s.ok()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "compact hybrid index column family failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

      s = _db->Flush(rocksdb::FlushOptions(), _getHandle(LSM_CF_HYBRID_INDEX));
      if (OSS_UNLIKELY(!s.ok()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "flush hybrid index column family failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }
      
   done:   
      return rc;
   error:
      goto done;
   }

   INT32 lsmDB::_restoreHybridIndexSsts(DPS_LSN_OFFSET lsn,
                                        const ossPoolVector<std::string> &namelist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(!namelist.empty(), "can not be empty");
      rocksdb::Status s;
      rocksdb::Options opt;

      for (UINT32 i = 0; i < namelist.size(); ++i)
      {
         DPS_LSN_OFFSET maxLsn = DPS_INVALID_LSN_OFFSET;
         std::unique_ptr<rocksdb::Iterator> itr;
         rocksdb::SstFileReader reader(opt);
         std::shared_ptr<const rocksdb::TableProperties> properties;
         rocksdb::UserCollectedProperties::const_iterator pItr;

         s = reader.Open(namelist[i]);
         if (OSS_UNLIKELY(!s.ok()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "open sst reader[%s] failed",
                   namelist[i].c_str());
            goto error;
         }

         properties = reader.GetTableProperties();
         if (OSS_UNLIKELY(nullptr == properties))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "get table properties in sst[%s] failed",
                   namelist[i].c_str());
            goto error;            
         }

         pItr = properties->user_collected_properties.find(LSM_COLLECTOR_FIELDNAME_MAX_LSN);
         // max lsn in properties will not be found when key-values in sst are deleted
         if (OSS_UNLIKELY(properties->user_collected_properties.cend() == pItr))
         {
            continue;
         }

         if (OSS_UNLIKELY(sizeof(DPS_LSN_OFFSET) != pItr->second.size()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid max lsn in sst[%s]", namelist[i].c_str());
            goto error;
         }

         maxLsn = *(reinterpret_cast<const DPS_LSN_OFFSET *>(pItr->second.c_str()));
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != maxLsn, "can not be invalid");

         if (lsn > maxLsn)
         {
            continue;
         }

         itr.reset(reader.NewIterator(rocksdb::ReadOptions()));
         if (OSS_UNLIKELY(nullptr == itr))
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "allocate sst[%s] iterator failed", namelist[i].c_str());
            goto error;
         }

         itr->SeekToFirst();
         while (itr->Valid())
         {
            lsmIndexEntryValueRef ref;
            keyString ks(toSlice(itr->key()));
            if (OSS_UNLIKELY(!ks.isValid()))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "invalid index key in sst[%s]",
                      namelist[i].c_str());
               goto error;
            }

            rc = ref.init(itr->value());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "invalid index entry value in sst[%s], rc:%d",
                      namelist[i].c_str(), rc);
               goto error;
            }

            if (ref.getValuePtr()->lsn > lsn)
            {
               rc = remove(LSM_CF_HYBRID_INDEX, itr->key());
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  PD_LOG(PDERROR, "remove key-value in sst[%s] failed, rc:%d",
                         namelist[i].c_str(), rc);
                  goto error;
               }
            }

            itr->Next();
         }

         if (OSS_UNLIKELY(!itr->status().ok()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid iterator in sst[%s], status info:[%s]",
                   namelist[i].c_str(), itr->status().ToString().c_str());
            goto error;
         }
      }

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
