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
      if (INDEX_TYPE_LSM == type)
      {
         builder.append(IXM_TYPE_FIELD, IXM_LSM_FIELD);
      }
      else if (INDEX_TYPE_BTREE == type)
      {
         builder.append(IXM_TYPE_FIELD, IXM_BTREE_FIELD);
      }
      
      builder.appendBool(IXM_UNIQUE_FIELD, isUnique);
      builder.appendBool(IXM_ENFORCED_FIELD, enforeced);
      builder.appendBool(IXM_NOTNULL_FIELD, notNull);
      builder.appendBool(IXM_NOTARRAY_FIELD, notArray);
      if (INDEX_TYPE_BTREE == type)
      {
         builder.append(IXM_MAX_PREFIX_FIELD,
                        btreeMaxPrefixFields);
         builder.append(IXM_BTREE_MIN_COMPRESSION_DELTH_FIELD,
                        btreeMinCompressionDepth);
      }
      if (INDEX_TYPE_LSM == type)
      {
         builder.append(IXM_COLUMN_FAMILY_FIELD, columnFamily);
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

      ele = obj.getField(IXM_TYPE_FIELD);
      if (ele.eoo())
      {
         type = INDEX_TYPE_LSM;
      }
      else if (String != ele.type())
      {
         goto done;
      }
      else if (0 == ossStrcmp(ele.valuestr(), IXM_BTREE_FIELD))
      {
         type = INDEX_TYPE_BTREE;
      }
      else if (0 == ossStrcmp(ele.valuestr(), IXM_LSM_FIELD))
      {
         type = INDEX_TYPE_LSM;
      }
      else
      {
         goto done;
      }

      isUnique = obj.getBoolField(IXM_UNIQUE_FIELD);
      enforeced = obj.getBoolField(IXM_ENFORCED_FIELD);
      notNull = obj.getBoolField(IXM_NOTNULL_FIELD);
      notArray = obj.getBoolField(IXM_NOTARRAY_FIELD);

      if (INDEX_TYPE_BTREE == type)
      {
         ele = obj.getField(IXM_BTREE_COMPRESSION_FIELD);
         if (ele.eoo())
         {
            btreeCompressionEnabled = FALSE;
         }
         else if (!ele.isBoolean())
         {
            goto done;
         }
         else
         {
            btreeCompressionEnabled = ele.booleanSafe();
         }

         if (btreeCompressionEnabled)
         {
            ele = obj.getField(IXM_MAX_PREFIX_FIELD);
            if (ele.eoo())
            {
               btreeMaxPrefixFields = 0;
            }
            else if (!ele.isNumber())
            {
               goto done;
            }
            btreeMaxPrefixFields = ele.numberInt();

            ele = obj.getField(IXM_BTREE_MIN_COMPRESSION_DELTH_FIELD);
            if (ele.eoo())
            {
               btreeMinCompressionDepth = 2;
            }
            else if (!ele.isNumber())
            {
               goto done;
            }
            else
            {
               btreeMinCompressionDepth = ele.numberInt();
            }
         }
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
      return btreeCompressionEnabled;
   }
}//namespace vessel
}//namespace engine