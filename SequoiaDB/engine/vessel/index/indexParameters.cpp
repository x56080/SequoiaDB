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

   Source File Name = indexParameters.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexParameters.h"
#include "pdTrace.hpp"
#include "ixm_common.hpp"

namespace engine
{
namespace vessel
{
   BOOLEAN indexParameters::isValid()const
   {
      BOOLEAN r = FALSE;
      if (INDEX_TYPE_BTREE != type &&
          INDEX_TYPE_LSM != type)
      {
         goto done;
      }

      if (INDEX_TYPE_LSM == type)
      {
         if (0 != columnFamily)
         {
            goto done;
         }
      }
      
      r = TRUE;
   done:
      return r;
   }

   void indexParameters::exportToBson(bson::BSONObjBuilder &builder)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      builder.append(VESSEL_INDEX_FIELD_NAME_TYPE, type);
      builder.appendBool(IXM_UNIQUE_FIELD, isUnique);
      builder.appendBool(IXM_ENFORCED_FIELD, enforeced);
      builder.appendBool(IXM_NOTNULL_FIELD, notNull);
      builder.appendBool(IXM_NOTARRAY_FIELD, notArray);
      if (INDEX_TYPE_BTREE == type)
      {
         builder.append(VESSEL_INDEX_FIELD_NAME_BTREE_MAX_PREFIX_FIELDS,
                        btreeMaxPrefixFields);
         builder.append(VESSEL_INDEX_FIELD_NAME_BTREE_MIN_COMPRESSION_DEPTH,
                        btreeMinCompressionDepth);
      }
      if (INDEX_TYPE_LSM == type)
      {
         builder.append(VESSEL_INDEX_FIELD_NAME_COLUMN_FAMILY, columnFamily);
      }
      return;
   }

   BOOLEAN indexParameters::extractFromBson(const bson::BSONObj &obj)
   {
      BOOLEAN r = FALSE;
      bson::BSONElement ele;

      if (!obj.isValid())
      {
         goto done;
      }

      ele = obj.getField(VESSEL_INDEX_FIELD_NAME_TYPE);
      if (bson::NumberInt != ele.type())
      {
         goto done;
      }
      type = ele.Int();
      if (INDEX_TYPE_LSM != type && INDEX_TYPE_BTREE != type)
      {
         goto done;
      }

      isUnique = obj.getBoolField(IXM_UNIQUE_FIELD);
      enforeced = obj.getBoolField(IXM_ENFORCED_FIELD);
      notNull = obj.getBoolField(IXM_NOTNULL_FIELD);
      notArray = obj.getBoolField(IXM_NOTARRAY_FIELD);

      if (INDEX_TYPE_LSM == type)
      {
         ele = obj.getField(VESSEL_INDEX_FIELD_NAME_COLUMN_FAMILY);
         if (!ele.isNumber())
         {
            goto done;
         }
         columnFamily = ele.Number();
      }

      if (INDEX_TYPE_BTREE == type)
      {
         ele = obj.getField(VESSEL_INDEX_FIELD_NAME_BTREE_MAX_PREFIX_FIELDS);
         if (!ele.isNumber())
         {
            goto done;
         }
         btreeMaxPrefixFields = ele.Number();

         ele = obj.getField(VESSEL_INDEX_FIELD_NAME_BTREE_MIN_COMPRESSION_DEPTH);
         if (!ele.isNumber())
         {
            goto done;
         }
         btreeMinCompressionDepth = ele.Number();
      }

      r = isValid();
   done:
      if (!r)
      {
         *this = indexParameters();
      }
      return r;
   }

   BOOLEAN indexParameters::isPrefixCompressionEnabled()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return 0 < btreeMaxPrefixFields;
   }
}//namespace vessel
}//namespace engine