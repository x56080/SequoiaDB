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

   Source File Name = lsmLobcMetaStorage.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmLobcMetaStorage.h"
#include "vessel/lsm/lsmLobChunkValue.h"
#include "vessel/lsm/lsmDB.h"

namespace engine
{
namespace vessel
{
   void lsmLobcMetaStorage::init(const lsmColumnFamily &cf)
   {
      SDB_ASSERT(cf.isValid(), "can not be invalid");
      fini();
      _cf = cf;
   }

   void lsmLobcMetaStorage::fini()
   {
      _cf = lsmColumnFamily();
   }

   INT32 lsmLobcMetaStorage::put(const lsmLobChunkKey &key,
                                 const bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      rocksdb::WriteOptions opt;

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!key.isValid() || obj.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _cf.put(key.getSlice(),
                   rocksdb::Slice(obj.objdata(), obj.objsize()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put lob chunk into column family failed, rc:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmLobcMetaStorage::get(const lsmLobChunkKey &key,
                                 bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      std::string res;
      BOOLEAN notFound = FALSE;

      obj = bson::BSONObj();
      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!key.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _cf.get(key.getSlice(), res, notFound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get specified value in column family failed, rc:%d", rc);
         goto error;
      }

      if (!notFound)
      {
         try
         {
            obj = bson::BSONObj(res.c_str()).getOwned();
         }
         catch(std::exception &e)
         {
            PD_LOG(PDWARNING, "invalid lsm value, exception:%s", e.what());
            obj = bson::BSONObj();
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmLobcMetaStorage::remove(const lsmLobChunkKey &key)
   {
      INT32 rc = SDB_OK;

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!key.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _cf.remove(key.getSlice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "remove specified key-value in column family failed, rc:%d");
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmLobcMetaStorage::truncate(UINT32 csid)
   {
      INT32 rc = SDB_OK;
      lsmLobChunkKey lowKey;
      lsmLobChunkKey upKey;
      
      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (DMS_INVALID_LOGICCSID == csid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lowKey.setAsLowKey(csid);
      upKey.setAsUpKey(csid);

      rc = _cf.truncate(lowKey.getSlice(),
                        upKey.getSlice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "truncate key-values in column family failed, rc:%d", rc);
         goto error; 
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmLobcMetaStorage::truncate(UINT32 csid,
                                      UINT32 clid)
   {
      INT32 rc = SDB_OK;

      lsmLobChunkKey lowKey;
      lsmLobChunkKey upKey;
      
      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (DMS_INVALID_LOGICCSID == csid ||
               DMS_INVALID_LOGICCLID == clid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lowKey.setAsLowKey(csid, clid);
      upKey.setAsUpKey(csid, clid);

      rc = _cf.truncate(lowKey.getSlice(),
                        upKey.getSlice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "truncate key-values in column family failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmLobcMetaStorage::openIterator(UINT32 csid,
                                          lsmLobChunkIterator &itr)
   {
      INT32 rc = SDB_OK;
      rocksdb::ReadOptions opt;

      itr.close();
      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (DMS_INVALID_LOGICCSID == csid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      itr._lowBoundKey.setAsLowKey(csid);
      itr._upBoundKey.setAsUpKey(csid);
      itr._lowKey = itr._lowBoundKey.getSlice();
      itr._upKey = itr._upBoundKey.getSlice();

      opt.iterate_lower_bound = &itr._lowKey;
      opt.iterate_upper_bound = &itr._upKey;

      itr._itr = _cf.newIterator(opt);
      if (nullptr == itr._itr)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "allocate new iterator failed");
         goto error;
      }

      itr._itr->Seek(itr._lowKey);
   done:
      return rc;
   error:
      itr.close();
      goto done;
   }

} // namespace vessel
} // namespace engine
