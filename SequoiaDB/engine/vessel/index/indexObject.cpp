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

   Source File Name = indexObject.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexObject.h"
#include "ossLikely.hpp"
#include "vessel/buildingIndexContext.h"
#include "ixm_common.hpp"
#include "msgDef.h"
#include "vessel/indexUtils.h"

namespace engine
{
namespace vessel
{
   INT32 indexObject::init(UINT32 indexLid,
                           const indexProperties &properties,
                           INDEX_STATUS status)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(INVALID_LOGICAL_INDEX_ID == indexLid ||
                       !properties.isValid() ||
                       INDEX_STATUS_INVALID == status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _logicalID = indexLid;
      _properties = properties;
      _status = status;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void indexObject::reset()
   {
      _logicalID = INVALID_LOGICAL_INDEX_ID;
      _rebornLSN = DPS_INVALID_LSN_OFFSET;
      _properties.reset();
      _status = INDEX_STATUS_INVALID;
      _entryAddr.reset();
      return;
   }

   bson::BSONObj indexObject::toBson()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      bson::BSONObjBuilder builder;
      builder.append(IXM_LOGICALID_FIELD, _logicalID);
      builder.append(IXM_REBORN_LSN, (long long)_rebornLSN);
      bson::BSONObjBuilder subBuilder(builder.subobjStart(IXM_PROPERTIES));
      _properties.dump(subBuilder);
      subBuilder.doneFast();
      builder.append(IXM_STATUS_FIELD, (INT32)_status);
      btreeEntryAddr addr = _entryAddr.get();
      if (addr.isValid())
      {
         builder.append(IXM_BTREE_ENTRY, addr.pid);
      }

      return builder.obj();
   }

   INT32 indexObject::initFromBson(const bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      reset();

      {
         bson::BSONElement e = obj.getField(IXM_LOGICALID_FIELD);
         if (NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         _logicalID = e.Int();
      }

      {
         bson::BSONElement e = obj.getField(IXM_REBORN_LSN);
         if (NumberLong != e.type())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         _rebornLSN = e.Long();
      }

      {
         bson::BSONElement e = obj.getField(IXM_STATUS_FIELD);
         if (NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         _status = (INDEX_STATUS)e.Int();
      }

      {
         bson::BSONElement e = obj.getField(IXM_BTREE_ENTRY);
         if (e.eoo())
         {
            /// do nothing
         }
         else if (NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         else
         {
            _entryAddr.set(e.Int(), 0);
         }
      }

      {
         bson::BSONElement e = obj.getField(IXM_PROPERTIES);
         if (Object != e.type())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = _properties.init(e.embeddedObject());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init properties:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      PD_LOG(PDERROR, "failed to init index obj from[%s]", obj.toPoolString().c_str());
      reset();
      goto done;
   }

   BOOLEAN indexObject::associates(const CHAR *fieldName)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(NULL != fieldName, "can not be null");

      BOOLEAN r = FALSE;
      const bson::BSONObj &pattern = _properties.getPattern().getPattern();
      bson::BSONObjIterator itr(pattern);
      while (itr.more())
      {
         const CHAR *patternFieldName = itr.next().fieldName();
         if (indexUtils::fieldNameAssociate(fieldName, patternFieldName))
         {
            r = TRUE;
            goto done;
         }
      }

   done:
      return r;
   }

} // namespace vessel

}//namespace engine
