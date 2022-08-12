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

   Source File Name = indexObject.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      _btreeEntryAddr = INVALID_PAGE_ID;
      _btreeEntryPSN = 0;
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
      if (INVALID_PAGE_ID != _btreeEntryAddr)
      {
         builder.append(IXM_BTREE_ENTRY, _btreeEntryAddr);
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
            _btreeEntryAddr = INVALID_PAGE_ID;
         }
         else if (NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         else
         {
            _btreeEntryAddr = e.Int();
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
