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

   Source File Name = lsmTableProperties.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#include "vessel/lsm/lsmTableProperties.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "dpsDef.hpp"
#include "vessel/keyStringCoder.h"

namespace engine
{
namespace vessel
{
   lsmTableProperties::lsmTableProperties(const rocksdb::TableProperties *properties):
   _properties(properties)
   {
      SDB_ASSERT(nullptr != properties, "can not be invalid");
   }

   void lsmTableProperties::init(const rocksdb::TableProperties *properties)
   {
      SDB_ASSERT(nullptr != properties, "can not be invalid");
      _properties = properties;
   }

   BOOLEAN lsmTableProperties::hasUserDefinedProperties()const
   {
      return isValid() && !_properties->user_collected_properties.empty();
   }

   INT32 lsmTableProperties::getLSNPair(UINT64 &minLSN, UINT64 &maxLSN) const
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         rc = _extractLSN(LSM_TABLE_PROPERTIES_MIN_LSN, minLSN);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extract min lsn:%d", rc);
            goto error;
         }

         rc = _extractLSN(LSM_TABLE_PROPERTIES_MAX_LSN, maxLSN);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extract max lsn:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      minLSN = DPS_INVALID_LSN_OFFSET;
      maxLSN = DPS_INVALID_LSN_OFFSET;
      goto done;
   }

   INT32 lsmTableProperties::_extractLSN(const CHAR *name, UINT64 &lsn)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid() && nullptr != name, "can not be invalid");
      rocksdb::UserCollectedProperties::const_iterator itr =
            _properties->user_collected_properties.find(name);
      if (_properties->user_collected_properties.cend() == itr)
      {
         PD_LOG(PDERROR, "property[%s] not found", name);
         rc = SDB_VESSEL_KEY_NOT_FOUND;
         goto error;
      }
      else if (sizeof(UINT64) != itr->second.size())
      {
         PD_LOG(PDERROR, "invalid property[%s] value size[%d]", name, itr->second.size());
         rc = SDB_VESSEL_KEY_NOT_FOUND;
         goto error;
      }
      else
      {
         lsn = *reinterpret_cast<const UINT64 *>(itr->second.data());
         if (DPS_INVALID_LSN_OFFSET == lsn)
         {
            PD_LOG(PDERROR, "invalid lsn value extracted");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      return rc;
   error:
      lsn = DPS_INVALID_LSN_OFFSET;
      goto done;
   }

   INT32 lsmTableProperties::getIndexIdPair(globalIndexID &minId,
                                            globalIndexID &maxId)const
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         rc = _extractIndexId(LSM_TABLE_PROPERTIES_MIN_IDX_ID, minId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extract min lsn:%d", rc);
            goto error;
         }

         rc = _extractIndexId(LSM_TABLE_PROPERTIES_MAX_IDX_ID, maxId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extract max lsn:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      minId.reset();
      maxId.reset();
      goto done;
   }

   INT32 lsmTableProperties::_extractIndexId(const CHAR *name, globalIndexID &indexId)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid() && nullptr != name, "can not be invalid");
      rocksdb::UserCollectedProperties::const_iterator itr =
            _properties->user_collected_properties.find(name);
      if (_properties->user_collected_properties.cend() == itr)
      {
         PD_LOG(PDERROR, "property[%s] not found", name);
         rc = SDB_VESSEL_KEY_NOT_FOUND;
         goto error;
      }
      else if (keyStringCoder::INDEX_ID_ENCODEING_SIZE != itr->second.size())
      {
         PD_LOG(PDERROR, "invalid property[%s] value size[%d]", name, itr->second.size());
         rc = SDB_VESSEL_KEY_NOT_FOUND;
         goto error;
      }
      else
      {
         indexId = keyStringCoder().decodeToIndexId(itr->second.data());
         if (!indexId.isValid())
         {
            PD_LOG(PDERROR, "invalid index id extracted");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      return rc;
   error:
      indexId.reset();
      goto done;
   }
} // namespace vessel

} // namespace engine

