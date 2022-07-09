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

   Source File Name = indexMetaStorage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/01/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexMetaStorage.h"
#include "vessel/globalIndexID.h"
#include "vessel/objectIdentifier.h"
#include "vessel/lsm/lsmIndexMetaKey.h"
#include "ossLikely.hpp"
#include "ixm_common.hpp"
#include "vessel/lsm/lsmColumnFamily.h"

namespace engine
{
namespace vessel
{
   indexMetaStorage::indexMetaStorage(UINT32 lcsid, UINT32 lclid)
   {
      init(lcsid, lclid);
   }

   void indexMetaStorage::init(UINT32 csLid,
                               UINT32 clLid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csLid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clLid, "can not be invalid");
      _csLid = csLid;
      _clLid = clLid;
   }

   INT32 indexMetaStorage::commit(UINT32 indexLid,
                                  const indexObjectMap &im,
                                  BOOLEAN commitManifest)
   {
      INT32 rc = SDB_OK;
      const indexObject *obj = nullptr;
      bson::BSONObj indexEntry;
      bson::BSONObj manifestEntry;

      if (OSS_UNLIKELY(INVALID_LOGICAL_INDEX_ID == indexLid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }

      obj = im.getIndexObj(indexLid);
      if (nullptr == obj)
      {
         rc = SDB_VESSEL_KEY_NOT_FOUND;
         goto error;
      }

      indexEntry = obj->toBson();
      if (commitManifest)
      {
         manifestEntry = _getManifestEntry(im);
      }

      rc = _upsert(indexLid, indexEntry, manifestEntry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upsert index entry:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexMetaStorage::reload(indexObjectMap &im)
   {
      INT32 rc = SDB_OK;
      ossPoolList<bson::BSONObj> l;
      bson::BSONObj manifest;
      BOOLEAN found = FALSE;

      im.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getIndexManifest(found, manifest);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get manifest obj:%d", rc);
         goto error;
      }
      else if (found)
      {
         UINT32 maxLogicalId = INVALID_LOGICAL_INDEX_ID;
         bson::BSONElement e = manifest.getField(IXM_MAX_LOGICAL_ID);
         if (NumberInt != e.type())
         {
            PD_LOG(PDERROR, "failed to parse manifest obj[%s]", manifest.toPoolString().c_str());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         maxLogicalId = static_cast<UINT32>(e.Int());

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

         im.setMaxIndexLid(maxLogicalId);
      }

   done:
      return rc;
   error:
      im.reset();
      goto done;
   }

   INT32 indexMetaStorage::upsert(UINT32 indexLid,
                                  const bson::BSONObj &indexEntry)   
   {
      INT32 rc = SDB_OK;
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

      rc = _upsert(indexLid, indexEntry, bson::BSONObj());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "fialed to upsert:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexMetaStorage::upsert(UINT32 indexLid,
                                  const bson::BSONObj &indexEntry,
                                  const bson::BSONObj &manifest)   
   {
      INT32 rc = SDB_OK;
      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_LOGICAL_INDEX_ID == indexLid ||
               indexEntry.isEmpty() ||
               manifest.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _upsert(indexLid, indexEntry, manifest);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "fialed to upsert:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexMetaStorage::_upsert(UINT32 indexLid,
                                   const bson::BSONObj &indexEntry,
                                   const bson::BSONObj &manifest)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");
      SDB_ASSERT(!indexEntry.isEmpty(), "can not be empty");

      lsmWriteBatch batch;
      lsmIndexIdKey indexKey;
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();

      if (OSS_UNLIKELY(!cf.isValid()))
      {
         SDB_ASSERT(FALSE, "invalid column family");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      cf.openBatch(batch);
      indexKey.init(globalIndexID(_csLid, _clLid, indexLid));

      rc = batch.put(indexKey.getKeySlice(),
                     rocksdb::Slice(indexEntry.objdata(),
                                    indexEntry.objsize()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put index entry data into batch failed, rc:%d");
         goto error;
      }

      if (!manifest.isEmpty())
      {
         lsmIndexManifestKey mKey;
         mKey.init(_csLid, _clLid);
         rc = batch.put(mKey.getKeySlice(),
                        rocksdb::Slice(manifest.objdata(),
                                       manifest.objsize()));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "put cl entry data into batch failed, rc:%d");
            goto error;
         }
      }

      rc = batch.commit();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "commit batch into lsmDB failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexMetaStorage::getIndexEntry(UINT32 indexLid,
                                         bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      std::string res;
      lsmIndexIdKey indexKey;
      BOOLEAN notFound = FALSE;

      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();

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

      indexKey.init(globalIndexID(_csLid, _clLid, indexLid));

      rc = cf.get(indexKey.getKeySlice(),
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

   INT32 indexMetaStorage::getIndexManifest(BOOLEAN &found, bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      std::string res;
      lsmIndexManifestKey mKey;
      BOOLEAN notFound = FALSE;
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();

      if (OSS_UNLIKELY(!cf.isValid()))
      {
         SDB_ASSERT(FALSE, "invalid column family");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      obj = bson::BSONObj();
      found = FALSE;

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      mKey.init(_csLid, _clLid);

      rc = cf.get(mKey.getKeySlice(),
                  res, notFound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get index entry from lsmDB failed, rc:%d", rc);
         goto error;
      }
      
      if (!notFound)
      {
         try
         {
            obj = bson::BSONObj(res.c_str()).getOwned();
            found = TRUE;
         }
         catch (std::exception &e)
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid cl entry, exception:%s, cl logical Id:%d",
                   e.what(), _clLid);
            goto error;
         }
      }
   
   done:
      return rc;
   error:
      obj = bson::BSONObj();
      goto done;
   }

   INT32 indexMetaStorage::list(ossPoolList<bson::BSONObj> &objs)
   {
      INT32 rc = SDB_OK;
      rocksdb::ReadOptions rOpt;
      rocksdb::Iterator *itr = nullptr;
      lsmIndexIdKey lowKey;
      lsmIndexManifestKey upKey;
      rocksdb::Slice lowKeySlice;
      rocksdb::Slice upKeySlice;

      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();

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

      lowKey.init(globalIndexID(_csLid, _clLid, 0));
      upKey.initAsUpKey(_csLid, _clLid);
      lowKeySlice = rocksdb::Slice(lowKey.getKeySlice());
      upKeySlice = rocksdb::Slice(upKey.getKeySlice());
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

   INT32 indexMetaStorage::removeEntry(UINT32 indexLid)
   {
      INT32 rc = SDB_OK;
      lsmIndexIdKey indexKey;
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();

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

      indexKey.init(globalIndexID(_csLid, _clLid, indexLid));
      rc = cf.remove(indexKey.getKeySlice());
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

   INT32 indexMetaStorage::destroy()
   {
      INT32 rc = SDB_OK;
      lsmIndexManifestKey lowKey;
      lsmIndexManifestKey upKey;
      rocksdb::Slice lowKeySlice;
      rocksdb::Slice upKeySlice;
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();

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

      lowKey.init(_csLid, _clLid);
      upKey.initAsUpKey(_csLid, _clLid);
      lowKeySlice = rocksdb::Slice(lowKey.getKeySlice());
      upKeySlice = rocksdb::Slice(upKey.getKeySlice());

      rc = cf.truncate(lowKeySlice, upKeySlice);
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

   bson::BSONObj indexMetaStorage::_getManifestEntry(const indexObjectMap &im)const
   {
      bson::BSONObjBuilder builder;
      builder.append(IXM_MAX_LOGICAL_ID, (INT32)im.getMaxIndexLid());
      return builder.obj();
   }
} // namespace vessel
} // namespace engine
