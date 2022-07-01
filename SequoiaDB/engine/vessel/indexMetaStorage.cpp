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
#include "vessel/lsm/lsmDB.h"
#include "vessel/lsm/lsmIndexMetaKey.h"

namespace engine
{
namespace vessel
{
   void indexMetaStorage::init(UINT32 csLid,
                               UINT32 clLid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csLid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clLid, "can not be invalid");
      _cf = GET_INDEX_META_COLUMN_FAMILY();
      _csLid = csLid;
      _clLid = clLid;
   }

   INT32 indexMetaStorage::upsert(UINT32 indexLid,
                                  const bson::BSONObj &indexEntry,
                                  const bson::BSONObj *manifest)
   {
      INT32 rc = SDB_OK;
      lsmWriteBatch batch;
      lsmIndexIdKey indexKey;

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_LOGICAL_INDEX_ID == indexLid ||
               indexEntry.isEmpty() ||
               (nullptr != manifest && manifest->isEmpty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _cf.openBatch(batch);
      indexKey.init(globalIndexID(_csLid, _clLid, indexLid));

      rc = batch.put(indexKey.getKeySlice(),
                     rocksdb::Slice(indexEntry.objdata(),
                                    indexEntry.objsize()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put index entry data into batch failed, rc:%d");
         goto error;
      }

      if (nullptr != manifest)
      {
         lsmIndexManifestKey mKey;
         mKey.init(_csLid, _clLid);
         rc = batch.put(mKey.getKeySlice(),
                        rocksdb::Slice(manifest->objdata(),
                                       manifest->objsize()));
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

      rc = _cf.get(indexKey.getKeySlice(),
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

   INT32 indexMetaStorage::getIndexManifest(bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      std::string res;
      lsmIndexManifestKey mKey;
      BOOLEAN notFound = FALSE;
      obj = bson::BSONObj();

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      mKey.init(_csLid, _clLid);

      rc = _cf.get(mKey.getKeySlice(),
                   res, notFound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get index entry from lsmDB failed, rc:%d", rc);
         goto error;
      }
      
      if (notFound)
      {
         rc = SDB_IXM_NOTEXIST;
         PD_LOG(PDERROR, "index is not exist on collection[%d]", _clLid);
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

      itr = _cf.newIterator(rOpt);
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
            objs.clear();
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
      goto done;
   }

   INT32 indexMetaStorage::removeEntry(UINT32 indexLid)
   {
      INT32 rc = SDB_OK;
      lsmIndexIdKey indexKey;

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
      rc = _cf.remove(indexKey.getKeySlice());
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

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      lowKey.init(_csLid, _clLid);
      upKey.initAsUpKey(_csLid, _clLid);
      lowKeySlice = rocksdb::Slice(lowKey.getKeySlice());
      upKeySlice = rocksdb::Slice(upKey.getKeySlice());

      rc = _cf.truncate(lowKeySlice, upKeySlice);
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


} // namespace vessel
} // namespace engine
