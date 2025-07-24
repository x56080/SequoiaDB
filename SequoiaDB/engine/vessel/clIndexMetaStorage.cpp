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

   Source File Name = clIndexMetaStorage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/01/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/clIndexMetaStorage.h"
#include "vessel/globalIndexID.h"
#include "vessel/lsm/lsmIndexMetaKeyBuilder.h"
#include "ossLikely.hpp"
#include "ixm_common.hpp"
#include "vessel/lsm/lsmColumnFamily.h"

namespace engine
{
namespace vessel
{
   clIndexMetaStorage::clIndexMetaStorage(UINT32 lcsid, UINT32 lclid)
   {
      init(lcsid, lclid);
   }

   void clIndexMetaStorage::init(UINT32 csLid,
                                 UINT32 clLid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csLid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clLid, "can not be invalid");
      _csLid = csLid;
      _clLid = clLid;
   }

   INT32 clIndexMetaStorage::reload(indexObjectMap &im) const
   {
      INT32 rc = SDB_OK;
      ossPoolList<bson::BSONObj> l;

      im.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = list(l);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to list index entries:%d", rc);
         goto error;
      }

      for (auto itr = l.cbegin(); itr != l.cend(); ++itr)
      {
         std::unique_ptr<indexObject> ptr(SDB_OSS_NEW indexObject());
         if (OSS_UNLIKELY(!ptr))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         rc = ptr->initFromBson(*itr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init index obj from bson:%d", rc);
            goto error;
         }

         rc = im.insert(std::move(ptr));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert into map:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      im.reset();
      goto done;
   }

   INT32 clIndexMetaStorage::upsert(UINT32 indexLid,
                                    const indexObjectMap &im) const
   {
      INT32 rc = SDB_OK;
      const indexObject *obj = nullptr;

      if (OSS_UNLIKELY(INVALID_LOGICAL_INDEX_ID == indexLid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      obj = im.getIndexObj(indexLid);
      if (OSS_UNLIKELY(nullptr == obj ||
                       !obj->isValid() ||
                       !obj->isBuilding()))
      {
         rc = SDB_IXM_NOTEXIST;
         PD_LOG(PDERROR, "get index object[%d] failed", indexLid);
         goto error;
      }

      rc = upsert(indexLid, obj->toBson());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "upsert index meta data failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMetaStorage::upsert(UINT32 indexLid,
                                    const bson::BSONObj &indexEntry) const
   {
      INT32 rc = SDB_OK;
      lsmIndexMetaKeyBuilder key;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_LOGICAL_INDEX_ID == indexLid ||
               indexEntry.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!cf.isValid())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid column family");
         goto error;
      }

      rc = cf.put(key.build(globalIndexID(_csLid, _clLid, indexLid)),
                  rocksdb::Slice(indexEntry.objdata(),
                                 indexEntry.objsize()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put index meta entry failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMetaStorage::getIndexEntry(UINT32 indexLid,
                                           bson::BSONObj &obj) const
   {
      INT32 rc = SDB_OK;
      std::string res;
      lsmIndexMetaKeyBuilder key;
      BOOLEAN notFound = FALSE;

      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();

      if (OSS_UNLIKELY(!cf.isValid()))
      {
         SDB_ASSERT(FALSE, "invalid column family");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      obj = bson::BSONObj();

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_LOGICAL_INDEX_ID == indexLid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = cf.get(key.build(globalIndexID(_csLid, _csLid, indexLid)),
                  res, notFound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get index entry from lsmDB failed, rc:%d", rc);
         goto error;
      }
      
      if (notFound)
      {
         rc = SDB_IXM_NOTEXIST;
         PD_LOG(PDERROR, "index[%d] is not exist on collection[%d]",
                indexLid, _clLid);
         goto error;
      }
      else
      {
         try
         {
            obj = bson::BSONObj(res.c_str()).getOwned();
         }
         catch (std::exception &e)
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid index meta entry, exception:%s", e.what());
            goto error;
         }
      }
   
   done:
      return rc;
   error:
      obj = bson::BSONObj();
      goto done;
   }

   INT32 clIndexMetaStorage::list(ossPoolList<bson::BSONObj> &objs) const
   {
      INT32 rc = SDB_OK;
      rocksdb::ReadOptions rOpt;
      rocksdb::Iterator *itr = nullptr;
      lsmIndexMetaKeyBuilder lowKey, upKey;
      rocksdb::Slice lowKeySlice, upKeySlice;

      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();

      if (OSS_UNLIKELY(!cf.isValid()))
      {
         SDB_ASSERT(FALSE, "invalid column family");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      objs.clear();
      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      lowKeySlice = lowKey.build(globalIndexID(_csLid, _clLid, 0));
      upKeySlice = upKey.buildUpperKey(_csLid, _clLid);
      rOpt.iterate_lower_bound = &lowKeySlice;
      rOpt.iterate_upper_bound = &upKeySlice;

      itr = cf.newIterator(rOpt);
      if (nullptr == itr)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "out of memory");
         goto error;
      }

      for (itr->Seek(lowKeySlice); itr->Valid(); itr->Next())
      {
         try
         {
            objs.push_back(bson::BSONObj(itr->value().data()).getOwned());
         }
         catch(const std::exception& e)
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid index meta entry");
            goto error;
         }
      }

   done:
      if (nullptr != itr)
      {
         delete itr;
      }
      return rc;
   error:
      objs.clear();
      goto done;
   }

   INT32 clIndexMetaStorage::removeEntry(UINT32 indexLid) const
   {
      INT32 rc = SDB_OK;
      lsmIndexMetaKeyBuilder key;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();

      if (OSS_UNLIKELY(!cf.isValid()))
      {
         SDB_ASSERT(FALSE, "invalid column family");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_LOGICAL_INDEX_ID == indexLid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = cf.remove(key.build(globalIndexID(_csLid, _clLid, indexLid)));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "remove specified index meta data failed, rc:%d", rc);
         goto error;
      }
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMetaStorage::destroy() const
   {
      INT32 rc = SDB_OK;
      lsmIndexMetaKeyBuilder lowKey, upKey;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();

      if (OSS_UNLIKELY(!cf.isValid()))
      {
         SDB_ASSERT(FALSE, "invalid column family");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = cf.truncate(lowKey.build(_csLid, _clLid),
                       upKey.buildUpperKey(_csLid, _clLid));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to destroy indexes in collection[%d], rc:%d",
                _clLid, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMetaStorage::upsert(const indexObjectMap &im)
   {
      INT32 rc = SDB_OK;
      lsmWriteBatch batch;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();
      if (OSS_UNLIKELY(!cf.isValid()))
      {
         SDB_ASSERT(FALSE, "invalid column family");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      cf.openBatch(batch);
      for (auto itr = im.cbegin(); itr != im.cend(); ++itr)
      {
         bson::BSONObj o = itr->second->toBson();
         lsmIndexMetaKeyBuilder builder;
         rocksdb::Slice key = builder.build(globalIndexID(_csLid, _clLid, itr->first));
         rocksdb::Slice value = rocksdb::Slice(o.objdata(), o.objsize());
         rc = batch.put(key, value);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push entry into batch:%d", rc);
            goto error;
         }
      }

      rc = batch.commit();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit batch:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel
} // namespace engine
