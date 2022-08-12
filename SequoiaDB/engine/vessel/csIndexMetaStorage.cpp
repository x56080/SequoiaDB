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

   Source File Name = csIndexMetaStorage.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#include "vessel/csIndexMetaStorage.h"
#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/lsm/lsmIndexMetaKeyBuilder.h"
#include "ixm_common.hpp"

namespace engine
{
namespace vessel
{
   csIndexMetaStorage::csIndexMetaStorage(UINT32 csLid)
   {
      init(csLid);
   }

   void csIndexMetaStorage::init(UINT32 csLid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csLid, "can not be invalid");
      _csLid = csLid;
   }

   INT32 csIndexMetaStorage::upsert(const bson::BSONObj &manifest) const
   {
      INT32 rc = SDB_OK;
      lsmIndexMetaKeyBuilder key;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (manifest.isEmpty())
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

      rc = cf.put(key.build(_csLid),
                  rocksdb::Slice(manifest.objdata(),
                                 manifest.objsize()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put index manifest data failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 csIndexMetaStorage::getMaxIndexLid(UINT32 &maxIndexLid) const
   {
      INT32 rc = SDB_OK;
      std::string res;
      lsmIndexMetaKeyBuilder key;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();
      BOOLEAN notFound = TRUE;
      maxIndexLid = INVALID_LOGICAL_INDEX_ID;

      if (!isValid())
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

      rc = cf.get(key.build(_csLid), res, notFound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get index manifest failed, rc:%d", rc);
         goto error;
      }
      else if (!notFound)
      {
         bson::BSONObj manifest(res.c_str());
         bson::BSONElement e = manifest.getField(IXM_MAX_LOGICAL_ID);
         if (NumberInt != e.type())
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid index manifest");
            goto error;
         }
         maxIndexLid = static_cast<UINT32>(e.Int());
      }
   
   done:
      return rc;
   error:
      maxIndexLid = INVALID_LOGICAL_INDEX_ID;
      goto done;
   }

   INT32 csIndexMetaStorage::getIndexManifest(BOOLEAN &notFound,
                                              bson::BSONObj &obj) const
   {
      INT32 rc = SDB_OK;
      std::string res;
      lsmIndexMetaKeyBuilder key;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();
      notFound = TRUE;
      obj = bson::BSONObj();

      if (!isValid())
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

      rc = cf.get(key.build(_csLid), res, notFound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get index manifest failed, rc:%d", rc);
         goto error;
      }
      else if(!notFound)
      {
         try
         {
            obj = bson::BSONObj(res.c_str()).getOwned();
         }
         catch(const std::exception& e)
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid index manifest, exception:%s",
                   e.what());
            goto error;
         }
      }

   done:
      return rc;
   error:
      notFound = TRUE;
      obj = bson::BSONObj();
      goto done;
   }

   INT32 csIndexMetaStorage::destroy() const
   {
      INT32 rc = SDB_OK;
      lsmIndexMetaKeyBuilder lowKey, upKey;
      lsmColumnFamily cf = GET_INDEX_META_COLUMN_FAMILY();

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!cf.isValid())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid column family");
         goto error;
      }

      rc = cf.truncate(lowKey.build(_csLid), upKey.buildUpperKey(_csLid));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "destroy cs[%d] failed, rc:%d",
                _csLid, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine