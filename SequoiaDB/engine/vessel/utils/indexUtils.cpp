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

   Source File Name = indexUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexUtils.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/indexDef.h"
#include "ixm_common.hpp"

namespace engine
{
namespace vessel
{
   bson::BSONObj indexUtils::buildIndexDefObj(const strSlice &indexName,
                                              const indexKeyPattern &keyPattern,
                                              const indexParameters &params)
   {
      bson::BSONObjBuilder builder;
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(keyPattern.isValid(), "can not be invalid");
      SDB_ASSERT(params.isValid(), "can not be inavlid");

      builder.append(IXM_NAME_FIELD, indexName.str());
      builder.append(IXM_KEY_FIELD, keyPattern.getPattern());
      params.exportToBson(builder);
      return builder.obj();
   }

   INT32 indexUtils::parseIndexDefObj(const bson::BSONObj &obj,
                                       strSlice *indexName,
                                       indexKeyPattern *keyPattern,
                                       indexParameters *params)
   {
      INT32 rc = SDB_OK;

      if (NULL != indexName)
      {
         indexName->reset();
         bson::BSONElement ele = obj.getField(IXM_NAME_FIELD);
         if (bson::String != ele.type())
         {
            PD_LOG(PDERROR, "index name not found");
            rc = SDB_INVALIDARG;
            goto error;
         }
         indexName->reset(ele.valuestr());
      }

      if (NULL != keyPattern)
      {
         keyPattern->reset();
         bson::BSONElement ele = obj.getField(IXM_KEY_FIELD);
         if (bson::Object != ele.type())
         {
            PD_LOG(PDERROR, "index key define not found");
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = keyPattern->set(ele.embeddedObject());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to set key pattern from obj:%d", rc);
            goto error;
         }
      }

      if (NULL != params)
      {
         if (!params->extractFromBson(obj))
         {
            PD_LOG(PDERROR, "failed to extract common options");
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine