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

   Source File Name = lsmIndexExecutor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmIndexExecutor.h"
#include "vessel/lsm/lsmDB.h"
#include "vessel/lsm/lsmIndexValue.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/threadContext.h"
#include "vessel/lsm/lsmIndexKeyPacker.h"

namespace engine
{
namespace vessel
{
   void lsmIndexExecutor::init(const lsmIndexMeta &meta)
   {
      SDB_ASSERT(meta.isValid(), "can not be invalid");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      fini();
      _cf = tc->getEnv()->lsm->getIdxColumnFamily();
      _meta = meta;
      _wOpt.disableWAL = TRUE;
   }

   void lsmIndexExecutor::fini()
   {
      _cf = lsmColumnFamily();
      _meta = lsmIndexMeta();
      _wOpt = rocksdb::WriteOptions();
      _rOpt = rocksdb::ReadOptions();
   }

   INT32 lsmIndexExecutor::put(const lsmKeyEntry &key)
   {
      INT32 rc = SDB_OK;
      lsmIndexKeyPacker packer;
      lsmIndexValue value;
      
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
      rc = packer.packFullKey(key, _meta);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "pack full key by packer failed, rc:%d", rc);
         goto error;
      }

      value.reset(LSM_VALUE_TYPE_INSERT);
      rc = _cf.put(packer.getFullKeySlice(), value.getSlice(), _wOpt);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put key-value into index column family failed, rc:%d", rc);
         goto error;
      }
   
   done:
      packer.reset();
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexExecutor::truncate()
   {
      INT32 rc = SDB_OK;
      globalIndexID upIdxId;
      CHAR lowKey[LSM_LOW_BOUND_KEY_SIZE];
      CHAR upKey[LSM_LOW_BOUND_KEY_SIZE];

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      lowKey[0] = LSM_ENTRY_TYPE_DATA;
      *((globalIndexID*)(lowKey + 1)) = _meta.getIdxId();

      upKey[0] = LSM_ENTRY_TYPE_DATA;
      upIdxId.reset(_meta.getIdxId().getLogicalCSID(),
                    _meta.getIdxId().getLogicalCLID(),
                    _meta.getIdxId().getLogicalIndexID() + 1);
      *((globalIndexID*)(upKey + 1)) = upIdxId;

      rc = _cf.truncate(rocksdb::Slice(lowKey, sizeof(lowKey)),
                        rocksdb::Slice(upKey, sizeof(upKey)),
                        _wOpt);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "truncate key-values in "
                "index column family failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine