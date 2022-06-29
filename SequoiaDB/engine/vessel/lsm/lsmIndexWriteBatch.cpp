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
#include "vessel/lsm/lsmDB.h"
#include "vessel/lsm/lsmIndexKeyPacker.h"

namespace engine
{
namespace vessel
{
   INT32 lsmIndexWriteBatch::put(const lsmIndexMeta &meta,
                                 const lsmPureKeyEntry &key,
                                 const lsmIndexValue &value)
   {
      INT32 rc = SDB_OK;
      lsmIndexKeyStackPacker packer;

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

      rc = packer.packFullKey(key, meta);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "pack full key by packer failed, rc:%d", rc);
         goto error;
      }

      rc = lsmWriteBatch::put(packer.getFullKeySlice(),
                              value.getSlice(),
                              key.getDataLsn());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put key-value into batch failed, rc:%d", rc);
         goto error;
      }

   done:
      packer.reset();
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine