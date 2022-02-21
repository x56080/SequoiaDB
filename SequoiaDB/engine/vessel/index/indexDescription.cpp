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
      bson::BSONObjIterator itr;
      indexKeyPattern pattern;
      BOOLEAN isBtreeCompression = FALSE;
      UINT32 btreeMaxPrefixFields = 0;
      UINT32 btreeMinCompressionDepth = 2;
      UINT32 columnFamily = 0;

      if (!obj.isValid())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid bson object");
         goto error;
      }

      itr = bson::BSONObjIterator(obj);
      while (itr.more())
      {
         bson::BSONElement ele(itr.next());

         if (String == ele.type() &&
             0 == ossStrcmp(IXM_NAME_FIELD, ele.fieldName()))
         {
            if (0 == ele.valuestrsize())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid index name");
               goto error;
            }
            _name = ele.valuestr();
            _nameSlice.reset(_name.c_str());
         }
         else if (Object == ele.type() &&
                  0 == ossStrcmp(IXM_KEY_FIELD, ele.fieldName()))
         {
            pattern.set(ele.embeddedObject());
         }
         else if (String == ele.type() &&
                  0 == ossStrcmp(IXM_TYPE_FIELD, ele.fieldName()))
         {
            if (0 == ossStrcmp(IXM_BTREE_FIELD, ele.valuestr()))
            {
               _type = INDEX_TYPE_BTREE;
            }
            else if(0 == ossStrcmp(IXM_LSM_FIELD, ele.valuestr()))
            {
               _type = INDEX_TYPE_LSM;
            }
            else
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid index type[%s]", ele.valuestr());
            }
         }
         else if (ele.isNumber() &&
                  0 == ossStrcmp(IXM_INNERID_FIELD, ele.fieldName()))
         {
            if (utilCheckIdxInnerID(ele.numberInt()))
            {
               _innerID = ele.numberInt();
            }
         }
         else if (ele.isBoolean() &&
                  0 == ossStrcmp(IXM_UNIQUE_FIELD, ele.fieldName()))
         {
            if (ele.boolean())
            {
               setAsUnique();
            }
         }
         else if (ele.isBoolean() &&
                  0 == ossStrcmp(IXM_ENFORCED_FIELD, ele.fieldName()))
         {
            if (ele.boolean())
            {
               setAsEnforced();
            }
         }
         else if (ele.isBoolean() &&
                  0 == ossStrcmp(IXM_NOTNULL_FIELD, ele.fieldName()))
         {
            if (ele.boolean())
            {
               setAsNotNull();
            }
         }
         else if (ele.isBoolean() &&
                  0 == ossStrcmp(IXM_NOTARRAY_FIELD, ele.fieldName()))
         {
            if (ele.boolean())
            {
               setAsNotArray();
            }
         }
         else if (ele.isBoolean() &&
                  0 == ossStrcmp(IXM_BTREE_COMPRESSION_FIELD, ele.fieldName()))
         {
            if (ele.boolean())
            {
               isBtreeCompression = TRUE;
            }
         }
         else if (ele.isNumber() &&
                  0 == ossStrcmp(IXM_MAX_PREFIX_FIELD, ele.fieldName()))
         {
            btreeMaxPrefixFields = ele.numberInt();
         }
         else if (ele.isNumber() &&
                  0 == ossStrcmp(IXM_BTREE_MIN_COMPRESSION_DEPTH_FIELD, 
                                 ele.fieldName()))
         {
            btreeMinCompressionDepth = ele.numberInt();
         }
         else if (ele.isNumber() &&
                  0 == ossStrcmp(IXM_COLUMN_FAMILY_FIELD, ele.fieldName()))
         {
            columnFamily = ele.numberInt();
         }
         else
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid argument[%s]", ele.fieldName());
            goto error;
         }
      }
      
      if (INVALID_INDEX_TYPE == _type)
      {
         _type = INDEX_TYPE_LSM;
      }
      if (INDEX_TYPE_BTREE == _type)
      {
         if(isBtreeCompression)
         {
            if (pattern.getKeyCount() < btreeMaxPrefixFields)
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid xxx");
               goto error;
            }
            setAsPrefixCompressionEnabled();
            _btreeMaxPrefixFields = btreeMaxPrefixFields;
            _btreeMinCompressionDepth = btreeMinCompressionDepth;
         }
      }
      else
      {
         _columnFamily = columnFamily;
      }
      _pattern = pattern;
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
      _nameSlice.reset();
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

      builder.append(IXM_NAME_FIELD, _nameSlice.str());
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
}
}
