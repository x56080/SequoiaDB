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

   Source File Name = lsmIndexWriteBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/lsm/lsmIndexWriteBatch.h"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/lsm/lsmDB.h"

namespace engine
{
namespace vessel
{
   void lsmIndexWriteBatch::open()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      tc->getEnv()->lsm->getIdxColumnFamily().openBatch(*this);
   }

   INT32 lsmIndexWriteBatch::put(const lsmIndexMeta &meta,
                                 const lsmKeyEntry &key,
                                 const lsmIndexValue &value)
   {
      INT32 rc = SDB_OK;
      CHAR *keyBuf = nullptr;
      UINT32 keySize = 0;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!key.isValid() || !meta.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      keySize = lsmCalFullDataKeyLen(key.getKey().dataSize());
      keyBuf = (CHAR*)SDB_THREAD_ALLOC(keySize);
      if (nullptr == keyBuf)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "out of memory");
         goto error;
      }

      rc = lsmPackIndexFullKey(keyBuf, keySize,
                               meta.getIdxId(),
                               meta.getOrdering(),
                               key.getKey(),
                               key.getRid(),
                               key.getDataLsn(),
                               key.getTransID());

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "pack index full key failed, rc:%d", rc);
         goto error;
      }

      rc = lsmColumnFamily::writeBatch::put(rocksdb::Slice(keyBuf, keySize),
                                            value.getSlice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put key-value into batch failed, rc:%d", rc);
         goto error;
      }

   done:
      if (nullptr == keyBuf)
      {
         SDB_THREAD_FREE(keyBuf);
      }
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine