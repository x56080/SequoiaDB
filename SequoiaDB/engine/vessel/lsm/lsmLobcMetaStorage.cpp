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
#include "vessel/instanceEnv.h"
#include "vessel/threadContext.h"

namespace engine
{
namespace vessel
{
   void lsmLobcMetaStorage::init()
   {
      THREAD_CONTEXT *tc =  GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      fini();
      _cf = tc->getEnv()->lsm->getLobcColumnFamily();
      _wOpt.disableWAL = TRUE;
   }

   void lsmLobcMetaStorage::fini()
   {
      _cf = lsmColumnFamily();
      _wOpt = rocksdb::WriteOptions();
      _rOpt = rocksdb::ReadOptions();
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
                   rocksdb::Slice(obj.objdata(), obj.objsize()),
                   _wOpt);
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

      rc = _cf.get(key.getSlice(), res, notFound, _rOpt);
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

      rc = _cf.remove(key.getSlice(), _wOpt);
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
                        upKey.getSlice(),
                        _wOpt);
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
                        upKey.getSlice(),
                        _wOpt);
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
