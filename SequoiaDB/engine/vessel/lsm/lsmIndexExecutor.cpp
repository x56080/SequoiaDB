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
#include "vessel/lsm/lsmIndexKeyPacker.h"

namespace engine
{
namespace vessel
{
   void lsmIndexExecutor::init(const lsmColumnFamily &cf,
                               const lsmIndexMeta &meta)
   {
      SDB_ASSERT(cf.isValid(), "can not be invalid");
      SDB_ASSERT(meta.isValid(), "can not be invalid");
      fini();
      _cf = cf;
      _meta = meta;
   }

   void lsmIndexExecutor::fini()
   {
      _cf = lsmColumnFamily();
      _meta = lsmIndexMeta();
   }

   INT32 lsmIndexExecutor::put(const lsmPureKeyEntry &key)
   {
      INT32 rc = SDB_OK;
      lsmIndexKeyStackPacker packer;
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

      value.reset(LSM_IDX_VALUE_TYPE_INSERT);
      rc = _cf.put(packer.getFullKeySlice(),
                   value.getSlice(),
                   key.getDataLsn());
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
      LSM_IDX_KEY_BOUNDARY low;
      LSM_IDX_KEY_BOUNDARY up;
      rocksdb::Slice lowSlice, upSlice;

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      low = _meta.getIdxId();
      up.reset(low.getLogicalCSID(), low.getLogicalCLID(), low.getLogicalIndexID() + 1);
      lowSlice = rocksdb::Slice((const CHAR *)(&low), LSM_IDX_BOUNDARY_SIZE);
      upSlice = rocksdb::Slice((const CHAR *)(&up), LSM_IDX_BOUNDARY_SIZE);

      rc = _cf.truncate(lowSlice, upSlice);
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