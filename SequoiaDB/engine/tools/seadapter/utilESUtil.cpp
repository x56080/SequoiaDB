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

   Source File Name = utilESUtil.cpp

   Descriptive Name = Elasticsearch utility.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/03/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "ossUtil.hpp"
#include "utilESUtil.hpp"
#include "../../util/hex.h"
#include <sstream>
#include <algorithm>
#include "msgDef.hpp"
#include "seAdptDef.hpp"

using bson::BSONObjBuilder ;

#define UTIL_ESID_ENCODE_PREFIX     'x'

namespace seadapter
{
   _utilESMapping::_utilESMapping()
   {
   }

   _utilESMapping::~_utilESMapping()
   {
   }

   INT32 _utilESMapping::_processField( const BSONElement &eField )
   {
      INT32 rc = SDB_OK ;
      const CHAR* fieldName = eField.fieldName() ;

      /*
      eg:

      eField.embeddedObject() =
      {
         "a": { "Type": "text", "Index": true },
         "b": { "Index": false },
         "c": { "Type": "keyword" },
         "d.c": { "Type": "text" }
      }

      We will generate the following index templates:

      {
         "template_0" : {
            "match" : "b",
            "mappings" : {
               "index" : "false"
            }
         }
      }

      {
         "template_1" : {
            "path_match" : "d.c",
            "mappings" : {
               "type" : "text"
            }
         }
      }

      We will generate the following index properties:

      { "a": { "type" : "text", "index": true } }

      { "c": { "type" : "keyword" } }

      */

      try
      {
         BSONObjBuilder bob ;
         BSONObj field = eField.Obj() ;
         BSONObjIterator itr( field ) ;
         BOOLEAN isNested = ossStrstr( fieldName, "." ) ? TRUE : FALSE ;
         BOOLEAN hasType = field.hasField( FIELD_ES_NAME_TYPE ) ;
         StringBuilder buf ;
         string templateName ;

         if ( isNested || !hasType )
         {
            buf << SEADPT_PREFIX_TEMPLATE_NAME << _templates.size() ;
            templateName = buf.str() ;

            BSONObjBuilder templateBuilder( bob.subobjStart( templateName ) ) ;

            if ( isNested )
            {
               templateBuilder.append( SEADPT_DY_TMPL_RULE_PATH_MATCH, fieldName ) ;
            }
            else if ( !hasType )
            {
               templateBuilder.append( SEADPT_DY_TMPL_RULE_MATCH, fieldName ) ;
            }

            BSONObjBuilder mapBuilder( templateBuilder.subobjStart( SEADPT_FIELD_NAME_MAPPING ) ) ;

            while( itr.more() )
            {
               BSONElement ele = itr.next() ;
               ossPoolString fieldName = ele.fieldName() ;
               std::transform( fieldName.begin(), fieldName.end(), fieldName.begin(), ::tolower ) ;
               mapBuilder.appendAs( ele, fieldName.c_str() ) ;
            }
            mapBuilder.done() ;
            templateBuilder.done() ;

            _templates.push_back( bob.obj() ) ;
         }
         else
         {
            BSONObjBuilder fieldBuilder( bob.subobjStart( fieldName ) ) ;
            while( itr.more() )
            {
               BSONElement ele = itr.next() ;
               ossPoolString fieldName = ele.fieldName() ;
               std::transform( fieldName.begin(), fieldName.end(), fieldName.begin(), ::tolower ) ;
               fieldBuilder.appendAs( ele, fieldName.c_str() ) ;
            }
            fieldBuilder.done() ;

            _properties.push_back( bob.obj() ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when processing field: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESMapping::_generateStringTemplate()
   {
      INT32 rc = SDB_OK ;

      try
      {
         /*
         {
            "templatex":
            {
               "match_mapping_type": "string",
               "mappings":
               {
                  "type": "text"
               }
            }
         }
         */
         BSONObjBuilder bob ;
         StringBuilder buf ;
         string templateName ;

         buf << SEADPT_PREFIX_TEMPLATE_NAME << _templates.size() ;
         templateName = buf.str() ;

         BSONObjBuilder templateBuilder( bob.subobjStart( templateName ) ) ;
         templateBuilder.append( SEADPT_DY_TMPL_RULE_MACTH_MAP_TYPE,
                                 SEADPT_FIELD_NAME_STRING ) ;
         BSONObjBuilder mapBuilder( templateBuilder.subobjStart( SEADPT_FIELD_NAME_MAPPING ) ) ;
         mapBuilder.append( SEADPT_FIELD_NAME_TYPE, VALUE_ES_TYPE_NAME_TEXT ) ;
         mapBuilder.done() ;
         templateBuilder.done() ;

         _templates.push_back( bob.obj() ) ;

      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when generating string template: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESMapping::_generateDoubleTemplate()
   {
      INT32 rc = SDB_OK ;

      try
      {
         /*
         {
            "templatex":
            {
               "match_mapping_type": "double",
               "mappings":
               {
                  "type": "double"
               }
            }
         }
         */
         BSONObjBuilder bob ;
         StringBuilder buf ;
         string templateName ;

         buf << SEADPT_PREFIX_TEMPLATE_NAME << _templates.size() ;
         templateName = buf.str() ;

         BSONObjBuilder templateBuilder( bob.subobjStart( templateName ) ) ;
         templateBuilder.append( SEADPT_DY_TMPL_RULE_MACTH_MAP_TYPE,
                                 VALUE_ES_TYPE_NAME_DOUBLE ) ;
         BSONObjBuilder mapBuilder( templateBuilder.subobjStart( SEADPT_FIELD_NAME_MAPPING ) ) ;
         mapBuilder.append( SEADPT_FIELD_NAME_TYPE, VALUE_ES_TYPE_NAME_DOUBLE ) ;
         mapBuilder.done() ;
         templateBuilder.done() ;

         _templates.push_back( bob.obj() ) ;

      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when generating double template: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESMapping::_generateDefaultIndexTemplate()
   {
      INT32 rc = SDB_OK ;

      /*

      If we don't set this template,
      the double type field will be mapped to the float type by default, like this

      record = { a: 12.45 }

      mappings in ES =
      {
         "a": {
            "type" : "float"
         }
      }

      */
      rc = _generateDoubleTemplate() ;
      if ( rc )
      {
         goto error ;
      }

      /*

      If we don't set this template,
      the string type field will be mapped to the text type with keyword by default, like this

      record = { a: "aaa" }

      mappings in ES =
      {
         "a": {
            "type": "text",
            "fields": {
               "keyword": {
                  "type": "keyword",
                  "ignore_above": 256
               }
            }
         }
      }

      */
      rc = _generateStringTemplate() ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESMapping::generateIndexMapping( const BSONObj &mappings )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder indexMapping ;
         BSONObjIterator iMap( mappings ) ;
         while( iMap.more() )
         {
            BSONElement eMap = iMap.next() ;
            if ( 0 == ossStrcmp( eMap.fieldName(), FIELD_ES_NAME_FIELDS ) &&
                 Object == eMap.type() )
            {
               BSONObjIterator iFields( eMap.embeddedObject() ) ;
               while( iFields.more() )
               {
                  BSONElement eField = iFields.next() ;
                  if ( Object != eField.type() )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG( PDERROR, "The elements in Fileds must be Object" ) ;
                     goto error ;
                  }

                  rc = _processField( eField ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
               }
            }
         }

         rc = _generateDefaultIndexTemplate() ;
         PD_RC_CHECK( rc, PDERROR, "Generate default index mapping failed[%d]", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when generating index mappings: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESMapping::toObj( BSONObj &mapObj ) const
   {
      INT32 rc = SDB_OK ;
      try
      {
         /*

         {
            "mappings" : {
               "properties" : {
                  ...
               },
               "dynamic_templates" : [
                  {
                     ...
                  },
                  {
                     ...
                  },
                  ...
               ]
            }
         }

         */

         BSONObjBuilder builder ;
         BSONObjBuilder mapBuilder( builder.subobjStart( SEADPT_FIELD_NAME_MAPPINGS ) ) ;

         BSONObjBuilder propBuilder( builder.subobjStart( SEADPT_FIELD_NAME_PROPERTIES ) ) ;
         for ( ossPoolVector< BSONObj >::const_iterator it = _properties.begin() ;
               it != _properties.end() ; it++ )
         {
            propBuilder.append( (*it).firstElement() ) ;
         }
         propBuilder.done() ;

         BSONArrayBuilder templateBuilder( builder.subarrayStart( SEADPT_FIELD_NAME_DY_TMPL ) ) ;
         for ( ossPoolVector< BSONObj >::const_iterator it = _templates.begin() ;
               it != _templates.end() ; it++ )
         {
            templateBuilder.append( *it ) ;
         }
         templateBuilder.done() ;

         mapBuilder.done() ;
         mapObj = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // For OID type, the final id is the hex string of the _id value.
   // For other supported types, the format is as follows:
   //        x<type code><value code>
   // 'x' specifies this is an encoded id. The next char is its bson type.
   // After that, it's the raw data of the id value.
   // In case of any error or unsupported type, the id will be an empty string.
   // The caller need to check it.
   void encodeID( const BSONElement &idEle, string &id )
   {
      const CHAR *value = NULL ;
      INT32 valSize = 0 ;
      BOOLEAN idReady = FALSE ;
      BOOLEAN unsupport = FALSE ;

      try
      {
         switch ( idEle.type() )
         {
            case NumberDouble:
            case NumberInt:
            case NumberLong:
            case Object:
            case Bool:
            case Date:
            case Timestamp:
            case NumberDecimal:
               value = idEle.value() ;
               valSize = idEle.valuesize();
               break ;
            case String:
               value = idEle.valuestrsafe() ;
               valSize = idEle.valuestrsize() ;
               break ;
            case jstOID:
               // For oid type, it's already converted to hex. We do not encode
               // again.
               id = idEle.OID().str() ;
               idReady = TRUE ;
               break ;
            default:
               // Return empty string in case of unsupported type.
               id = "" ;
               unsupport = TRUE ;
               PD_LOG( PDWARNING, "Unsupported type[ %d ]", idEle.type() ) ;
               break ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      if ( !idReady && !unsupport )
      {
         CHAR bType = (CHAR)( idEle.type() ) ;
         id = UTIL_ESID_ENCODE_PREFIX +
              engine::toHexLower( (const void *)&bType, 1 )
              + engine::toHexLower( value, valSize ) ;
      }

   done:
      return ;
   error:
      goto done ;
   }

   INT32 decodeID( const CHAR *id, CHAR *raw, UINT32 &len, BSONType &type )
   {
      INT32 rc = SDB_OK ;
      const CHAR *p = NULL ;
      UINT32 idLen = 0 ;

      if ( !id )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "id to decode is empty" ) ;
         goto error ;
      }

      idLen = ossStrlen( id ) ;
      p = id ;
      if ( UTIL_ESID_ENCODE_PREFIX != *p )
      {
         // For compatibility with elder version(3.0).
         bson::OID oid ;
         SDB_ASSERT( 24 == idLen, "id size is not 24" ) ;
         if ( len <= sizeof( bson::OID ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Buffer size[ %u ] is too small", len ) ;
            goto error ;
         }

         {
            type = bson::jstOID ;
            const string idStr( id ) ;
            bson::OID oid( idStr ) ;
            len = sizeof( bson::OID ) ;
            ossMemcpy( raw, (CHAR *)&oid, len ) ;
         }
      }
      else
      {
         CHAR bType = 0 ;
         UINT32 targetLen = 0 ;

         p++ ;
         bType = engine::fromHex( p ) ;
         type = (BSONType)bType ;
         p += 2 ;

         targetLen = ( idLen - 3 ) / 2 ;
         if ( len <= targetLen )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Buffer size[ %u ] is too small", len ) ;
            goto error ;
         }

         for ( UINT32 i = 0; i < targetLen; ++i )
         {
            raw[ i ] = engine::fromHex( p ) ;
            p += 2 ;
         }

         raw[ targetLen ] = '\0' ;
         len = targetLen ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 seGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                              const CHAR **value )
   {
      SINT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName && value, "field name and value can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( fieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    fieldName,
                    obj.toString().c_str() ) ;
         PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                    "Unexpected field type : %s, supposed to be String",
                    obj.toString().c_str()) ;
         *value = ele.valuestr() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 seGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                           BSONObj &value )
   {
      SINT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( fieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    fieldName,
                    obj.toString().c_str() ) ;
         PD_CHECK ( Object == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                    "Unexpected field type : %s, supposed to be Object",
                    obj.toString().c_str()) ;
         value = ele.embeddedObject() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }
}

