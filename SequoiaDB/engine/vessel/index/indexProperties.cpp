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

   Source File Name = indexProperties.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/16/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexProperties.h"
#include "ixm_common.hpp"
#include "msgDef.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   constexpr CHAR * _LOWER_HYBRID_TREE_VALUE = "hybridtree";
   constexpr CHAR * _LOWER_BTREE_VALUE = "btree";
   constexpr CHAR * _LOWER_LSM_TREE_VALUE = "lsmtree";

   INT32 indexProperties::init(const bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(!obj.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
         const CHAR *indexName = obj.getStringField(IXM_NAME_FIELD);
         if (nullptr == indexName || 0 == ossStrlen(indexName))
         {
            PD_LOG(PDERROR, "failed to load index name from obj");
            rc = SDB_INVALIDARG;
            goto error;
         }
         _name.append(indexName);
      }

      {
         bson::BSONElement e = obj.getField(IXM_TYPE_FIELD);
         if (e.eoo())
         {
            _type = INDEX_TYPE_HYBRID_TREE;
         }
         else if (e.type() != String)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid index type");
            goto error;
         }
         else
         {
            ossPoolString typeStr(e.valuestr());
            std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);

            if (0 == typeStr.compare(_LOWER_HYBRID_TREE_VALUE))
            {
               _type = INDEX_TYPE_HYBRID_TREE;
            }
            else if (0 == typeStr.compare(_LOWER_BTREE_VALUE))
            {
               _type = INDEX_TYPE_BTREE;
            }
            else if (0 == typeStr.compare(_LOWER_LSM_TREE_VALUE))
            {
               _type = INDEX_TYPE_LSM;
            }
            else
            {
               PD_LOG(PDERROR, "invalid index type:%s", e.valuestr());
               rc = SDB_INVALIDARG;
               goto error;
            }
         }
      }

      {
         bson::BSONElement ele = obj.getField(IXM_UNIQUE_FIELD);
         if (ele.eoo())
         {
            // do nothing
         }
         else if (!ele.isBoolean())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid unique field");
            goto error;
         }
         else if (ele.boolean())
         {
            setAsUnique();
         }
      }

      {
         bson::BSONElement ele = obj.getField(IXM_ENFORCED_FIELD);
         if (ele.eoo())
         {
            // do nothing
         }
         else if (!ele.isBoolean())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid enforced field");
            goto error;
         }
         else if (ele.boolean())
         {
            setAsEnforced();
         }
      }

      {
         bson::BSONElement ele = obj.getField(IXM_NOTNULL_FIELD);
         if (ele.eoo())
         {
            // do nothing
         }
         else if (!ele.isBoolean())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid notnull field");
            goto error;
         }
         else if (ele.boolean())
         {
            setAsNotNull();
         }
      }

      {
         bson::BSONElement ele = obj.getField(IXM_NOTARRAY_FIELD);
         if (ele.eoo())
         {
            // do nothing
         }
         else if (!ele.isBoolean())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid notarray field");
            goto error;
         }
         else if (ele.boolean())
         {
            setAsNotArray();
         }
      }

      {
         bson::BSONElement ele = obj.getField(IXM_COMPRESSION);
         if (ele.eoo())
         {
            // do nothing
         }
         else if (!ele.isBoolean())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid compression field type:%d", ele.type());
            goto error;
         }
         else if (ele.boolean())
         {
            setCompressionEnabled();
         }
      }

      {
         bson::BSONElement ele = obj.getField(IXM_INNERID_FIELD);
         if (ele.eoo())
         {
            // do nothing
         }
         else if (!ele.isNumber())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid innerid field");
            goto error;
         }
         _innerID = ele.numberInt();
      }

      {
         bson::BSONElement ele = obj.getField(IXM_KEY_FIELD);
         if (Object != ele.type() ||
            !ele.embeddedObject().isValid())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid key pattern");
            goto error;
         }

         rc = _pattern.set(ele.embeddedObject());
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void indexProperties::reset()
   {
      _name.clear();
      _pattern.reset();
      _type = INDEX_TYPE_INVALID;
      _flags = 0;
      _innerID = UTIL_UNIQUEID_NULL;
      return;
   }

   void indexProperties::dump(bson::BSONObjBuilder &builder)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");

      builder.append(IXM_NAME_FIELD, _name.c_str());
      builder.append(IXM_KEY_FIELD, _pattern.getPattern());
      builder.append(IXM_TYPE_FIELD, IXM_HYBRID_TREE);
      builder.append(IXM_INNERID_FIELD, _innerID);
      builder.appendBool(IXM_UNIQUE_FIELD, isUnique());
      builder.appendBool(IXM_ENFORCED_FIELD, isEnforced());
      builder.appendBool(IXM_NOTNULL_FIELD, isNotNull());
      builder.appendBool(IXM_NOTARRAY_FIELD, isNotArray());
      builder.appendBool(IXM_COMPRESSION, isCompressionEnabled());

      return;
   }
}//namespace vessel

}//namespace engine
