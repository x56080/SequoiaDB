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

   Source File Name = indexDescription.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexDescription.h"
#include "ixm_common.hpp"
#include "msgDef.h"


namespace engine
{
namespace vessel
{
   BOOLEAN indexDescription::isValid()const
   {
      BOOLEAN r = FALSE;

      if (INDEX_TYPE_BTREE != _type &&
          INDEX_TYPE_LSM != _type)
      {
         goto done;
      }

      if (INDEX_TYPE_LSM == _type)
      {
         if (0 != _columnFamily)
         {
            goto done;
         }
      }

      if (_name.empty() ||
          !_pattern.isValid() ||
          !_pattern.isOwned())
      {
         goto done;
      }

      r = TRUE;

   done:
      return r;

   }

   INT32 indexDescription::extractFromBson(const bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      bson::BSONElement ele;
      reset();

      if (!obj.isValid())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid bson object");
         goto error;
      }

      ele = obj.getField(IXM_NAME_FIELD);
      if (String != ele.type() || 0 == ele.valuestrsize())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid index name");
         goto error;
      }
      _name = ele.valuestr();
      
      ele = obj.getField(IXM_TYPE_FIELD);
      if (ele.eoo())
      {
         _type = INDEX_TYPE_LSM;
      }
      else if (0 == ossStrcmp(IXM_BTREE_FIELD, ele.valuestrsafe()))
      {
         _type = INDEX_TYPE_BTREE;
      }
      else if (0 == ossStrcmp(IXM_LSM_FIELD, ele.valuestrsafe()))
      {
         _type = INDEX_TYPE_LSM;
      }
      else
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid index type");
         goto error;
      }
      

      ele = obj.getField(IXM_INNERID_FIELD);
      if (ele.eoo())
      {
         // do nothing
      }
      else if(!ele.isNumber())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid index innerID");
         goto error;
      }
      else
      {
         _innerID = ele.numberInt();
      }

      ele = obj.getField(IXM_UNIQUE_FIELD);
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

      ele = obj.getField(IXM_ENFORCED_FIELD);
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

      ele = obj.getField(IXM_NOTNULL_FIELD);
      if (ele.eoo())
      {
         // do nothing
      }
      else if (!ele.isBoolean())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid NotNull field");
         goto error;
      }
      else if (ele.boolean())
      {
         setAsNotNull();
      }
      
      ele = obj.getField(IXM_NOTARRAY_FIELD);
      if (ele.eoo())
      {
         // do nothing
      }
      else if (!ele.isBoolean())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid NotArray field");
         goto error;
      }
      else if (ele.boolean())
      {
         setAsNotArray();
      }

      if (INDEX_TYPE_LSM == _type)
      {
         ele = obj.getField(IXM_COLUMN_FAMILY_FIELD);
         if (ele.eoo())
         {
            //do nothing
         }
         else if (!ele.isNumber())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid ColumnFamily field");
            goto error;
         }
         else
         {
            _columnFamily = ele.numberInt();
         }
      }

      if (INDEX_TYPE_BTREE == _type)
      {
         ele = obj.getField(IXM_BTREE_COMPRESSION_FIELD);
         if (ele.eoo())
         {
            // do nothing
         }
         else if (!ele.isBoolean())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid BTreeCompression field");
            goto error;
         }
         else if (ele.boolean())
         {
            setAsPrefixCompressionEnabled();
         }

         if (isPrefixCompressionEnabled())
         {
            ele = obj.getField(IXM_MAX_PREFIX_FIELD);
            if (ele.eoo())
            {
               // do nothing
            }
            else if (!ele.isNumber())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid MaxPrefixFields field");
               goto error;
            }
            else
            {
               _btreeMaxPrefixFields = ele.numberInt();
            }

            ele = obj.getField(IXM_BTREE_MIN_COMPRESSION_DEPTH_FIELD);
            if (ele.eoo())
            {
               // do nothing
            }
            else if (!ele.isNumber())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid BtreeMinCompressionDepth field");
               goto error;
            }
            else
            {
               _btreeMinCompressionDepth = ele.numberInt();
            }
         }
      }

      ele = obj.getField(IXM_KEY_FIELD);
      if (ele.eoo() || Object != ele.type() ||
          !ele.embeddedObject().isValid())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid key pattern");
         goto error;
      }
     _pattern.set(ele.embeddedObject());
      if (_pattern.getKeyCount() < _btreeMaxPrefixFields)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid key pattern");
         goto error;
      }
      _pattern.getOwned();

      SDB_ASSERT(isValid(), "can not be invalid");

   done:
      return rc;
   error:
      goto done;
   }

   void indexDescription::reset()
   {
      _name.clear();
      _pattern.reset();
      _type = INVALID_INDEX_TYPE;
      _innerID = UTIL_UNIQUEID_NULL;
      _flags = 0;
      _btreeMaxPrefixFields = 0;
      _btreeMinCompressionDepth = 2;
      _columnFamily = 0;
   }

   void indexDescription::exportToBson(bson::BSONObjBuilder &builder)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");

      builder.append(IXM_NAME_FIELD, _name.c_str());
      builder.append(IXM_KEY_FIELD, _pattern.getPattern());

      if (INDEX_TYPE_BTREE == _type)
      {
         builder.append(IXM_TYPE_FIELD, IXM_BTREE_FIELD);
      }
      else if (INDEX_TYPE_LSM == _type)
      {
         builder.append(IXM_TYPE_FIELD, IXM_LSM_FIELD);
      }

      builder.append(IXM_INNERID_FIELD, _innerID);

      builder.appendBool(IXM_UNIQUE_FIELD, isUnique());
      builder.appendBool(IXM_ENFORCED_FIELD, isEnforced());
      builder.appendBool(IXM_NOTNULL_FIELD, isNotNull());
      builder.appendBool(IXM_NOTARRAY_FIELD, isNotArray());
      
      if (INDEX_TYPE_BTREE == _type)
      {
         builder.appendBool(IXM_BTREE_COMPRESSION_FIELD, isPrefixCompressionEnabled());
         builder.append(IXM_MAX_PREFIX_FIELD,
                        _btreeMaxPrefixFields);
         builder.append(IXM_BTREE_MIN_COMPRESSION_DEPTH_FIELD,
                        _btreeMinCompressionDepth);
      }

      if (INDEX_TYPE_LSM == _type)
      {
         builder.append(IXM_COLUMN_FAMILY_FIELD, _columnFamily);
      }
   }

   strSlice indexDescription::getNameSlice()const
   {
      strSlice nameSlice;
      nameSlice.reset(_name.c_str());
      return nameSlice;
   }
}//namespace vessel

}//namespace engine
