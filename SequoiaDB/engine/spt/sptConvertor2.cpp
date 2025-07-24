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

   Source File Name = sptConvertor2.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/13/2013  YW Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossUtil.hpp"
#include "sptConvertor2.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "utilStr.hpp"
#include "../client/base64c.h"
#include <boost/lexical_cast.hpp>

#define SPT_CONVERTOR_SPE_OBJSTART '$'
#define SPT_SPEOBJ_MINKEY "$minKey"
#define SPT_SPEOBJ_MAXKEY "$maxKey"
#define SPT_SPEOBJ_TIMESTAMP "$timestamp"
#define SPT_SPEOBJ_DATE "$date"
#define SPT_SPEOBJ_REGEX "$regex"
#define SPT_SPEOBJ_OPTION "$options"
#define SPT_SPEOBJ_BINARY "$binary"
#define SPT_SPEOBJ_TYPE "$type"
#define SPT_SPEOBJ_OID "$oid"
#define SPT_SPEOBJ_NUMBERLONG "$numberLong"
#define SPT_SPEOBJ_DECIMAL "$decimal"
#define SPT_SPEOBJ_PRESICION "$precision"


using namespace bson ;

INT32 sptConvertor2::toBson( JSObject *obj , bson::BSONObj &bsobj )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != _cx, "can not be NULL" ) ;

   BSONObjBuilder builder ;
   rc = _traverse( obj, builder ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   bsobj = builder.obj() ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor2::_traverse( JSObject *obj , bson::BSONObjBuilder &builder )
{
   INT32 rc = SDB_OK ;
   JSIdArray *properties = NULL ;
   if ( NULL == obj )
   {
      goto done ;
   }

   properties = JS_Enumerate( _cx, obj ) ;
   if ( NULL == properties )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   for ( jsint i = 0; i < properties->length; i++ )
   {
      jsid id = properties->vector[i] ;
      jsval fieldName, fieldValue ;
      std::string name ;
      if ( !JS_IdToValue( _cx, id, &fieldName ))
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( fieldName, name ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( !JS_GetProperty( _cx, obj, name.c_str(), &fieldValue ))
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _appendToBson( name, fieldValue, builder ) ;
   }
done:
   if ( NULL != properties )
   {
      JS_DestroyIdArray( _cx, properties ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor2::_getDecimalPrecision( const CHAR *precisionStr, 
                                           INT32 *precision, INT32 *scale )
{
   //precisionStr:10,6

   INT32 rc = SDB_OK ;
   rc = sscanf ( precisionStr, "%d,%d", precision, scale ) ;
   if ( 2 != rc )
   {
      return SDB_INVALIDARG ;
   }

   return SDB_OK ;
}

BOOLEAN sptConvertor2::_addSpecialObj( JSObject *obj,
                                      const CHAR *key,
                                      bson::BSONObjBuilder &builder )
{
   BOOLEAN ret = TRUE ;
   INT32 rc = SDB_OK ;
   JSIdArray *properties = JS_Enumerate( _cx, obj ) ;
   if ( NULL == properties || 0 == properties->length )
   {
      goto error ;
   }

   {
   /// get the first ele
   jsid id = properties->vector[0] ;
   jsval fieldName ;
   std::string name ;
   if ( !JS_IdToValue( _cx, id, &fieldName ))
   {
      goto error ;
   }

   rc = _toString( fieldName, name ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( name.length() <= 1 )
   {
      goto error ;
   }

   /// start with '$'
   if ( SPT_CONVERTOR_SPE_OBJSTART != name.at(0) )
   {
      goto error ;
   }

   if ( 0 == name.compare( SPT_SPEOBJ_MINKEY ) &&
        1 == properties->length )
   {
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_NUMBER, value ) )
      {
         goto error ;
      }

      builder.appendMinKey( key ) ;
   }
   else if ( 0 == name.compare(SPT_SPEOBJ_MAXKEY) &&
             1 == properties->length )
   {
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_NUMBER, value ) )
      {
         goto error ;
      }

      builder.appendMaxKey( key ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_OID ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_STRING, value ))
      {
         goto error ;
      }

      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( 24 != strValue.length() )
      {
         goto error ;
      }

      {
      OID id( strValue ) ;
      builder.appendOID( key, &id ) ;
      }
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_TIMESTAMP ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      time_t tm ;
      UINT64 usec = 0 ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_STRING, value ))
      {
         goto error ;
      }

      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( SDB_OK != engine::utilStr2TimeT( strValue.c_str(),
                                            tm,
                                            &usec ))
      {
         goto error ;
      }

      {
      builder.appendTimestamp( key, tm * 1000, usec ) ;
      }
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_NUMBERLONG ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_STRING, value ))
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_isValidNumberLong(strValue.c_str()) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      builder.appendNumber( key, ossAtoll(strValue.c_str()) ) ;
   }    
   else if ( 0 == name.compare( SPT_SPEOBJ_DATE ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      Date_t dt ;
      UINT64 millis = 0 ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_STRING, value ))
      {
         goto error ;
      }

      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( SDB_OK != engine::utilStr2Date( strValue.c_str(), millis ) )
      {
         goto error ;
      }

      dt.millis = millis ;
      builder.appendDate( key, dt ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_REGEX ) &&
             2 == properties->length )
   {
      std::string optionName ;
      std::string strRegex, strOption ;
      jsval jsRegex, jsOption ;
      jsid optionid = properties->vector[1] ;
      jsval optionValName ;

      if ( !JS_IdToValue( _cx, optionid, &optionValName ))
      {
         goto error ;
      }

      rc = _toString( optionValName, optionName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( 0 != optionName.compare( SPT_SPEOBJ_OPTION ) )
      {
         goto error ;
      }

      if ( !_getProperty( obj, name.c_str(),
                          JSTYPE_STRING, jsRegex ))
      {
         goto error ;
      }

      if ( !_getProperty( obj, optionName.c_str(),
                          JSTYPE_STRING, jsOption ))
      {
         goto error ;
      }

      rc = _toString( jsRegex, strRegex ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _toString( jsOption, strOption ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      builder.appendRegex( key, strRegex, strOption ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_BINARY ) &&
             2 == properties->length )
   {
      std::string typeName ;
      std::string strBin, strType ;
      jsval jsBin, jsType ;
      jsid typeId = properties->vector[1] ;
      jsval typeValName ;
      BinDataType binType ;
      CHAR *decode = NULL ;
      INT32 decodeSize = 0 ; 

      if ( !JS_IdToValue( _cx, typeId, &typeValName ))
      {
         goto error ;
      }

      rc = _toString( typeValName, typeName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( 0 != typeName.compare( SPT_SPEOBJ_TYPE ) )
      {
         goto error ;
      }

      if ( !_getProperty( obj, name.c_str(),
                          JSTYPE_STRING, jsBin ))
      {
         goto error ;
      }

      if ( !_getProperty( obj, typeName.c_str(),
                          JSTYPE_STRING, jsType ))
      {
         goto error ;
      }

      rc = _toString( jsBin, strBin ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _toString( jsType, strType ) ;
      if ( SDB_OK != rc || strType.empty())
      {
         goto error ;
      }

      try
      {
         binType = (BinDataType)(boost::lexical_cast<INT32>(strType)) ;
      }      
      catch ( std::bad_cast &e )
      {
         PD_LOG( PDERROR, "invalid bindata type:%s", strType.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      decodeSize = getDeBase64Size( strBin.c_str() ) ;
      if( decodeSize < 0 )
      {
         PD_LOG( PDERROR, "invalid bindata %s", strBin.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( decodeSize > 0 )
      {
         decode = ( CHAR * )SDB_OSS_MALLOC( decodeSize ) ;
         if ( NULL == decode )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         memset ( decode, 0, decodeSize ) ;
         if ( base64Decode( strBin.c_str(), decode, decodeSize ) < 0 )
         {
            PD_LOG( PDERROR, "failed to decode base64 code" ) ;
            rc = SDB_INVALIDARG ;
            SDB_OSS_FREE( decode ) ;
            goto error ;
         }
         builder.appendBinData( key, decodeSize,
                                binType, decode ) ;
         SDB_OSS_FREE( decode ) ;
      }
      else
      {
         builder.appendBinData( key, 0,
                                binType, "" ) ;
      }
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_DECIMAL ) )
   {
      std::string optionName ;
      std::string strDecimal, strOption ;
      jsval jsDecimal, jsOption ;
      jsval optionValName ;
      int precision ;
      int scale ;
      if ( 1 != properties->length && 2 != properties->length )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_getProperty( obj, name.c_str(),
                          JSTYPE_STRING, jsDecimal ))
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( jsDecimal, strDecimal ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 2 == properties->length )
      {
         // 2 == properties->length
         jsid optionid = properties->vector[1] ;

         if ( !JS_IdToValue( _cx, optionid, &optionValName ))
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = _toString( optionValName, optionName ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( 0 != optionName.compare( SPT_SPEOBJ_PRESICION ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( !_getProperty( obj, optionName.c_str(),
                             JSTYPE_OBJECT, jsOption ))
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = _toString( jsOption, strOption ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = _getDecimalPrecision( strOption.c_str(), &precision, &scale ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "invalid precision %s", strOption.c_str() ) ;
            goto error ;
         }

         rc = builder.appendDecimal( key, strDecimal, precision, scale ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "invalid decimal[%s],precision[%s]", 
                    strDecimal.c_str(), strOption.c_str() ) ;
            goto error ;
         }
      }
      else
      {
         // 1 == properties->length
         rc = builder.appendDecimal( key, strDecimal ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "invalid decimal %s", strDecimal.c_str() ) ;
            goto error ;
         }
      }

      if ( 0 != rc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      goto error ;
   }
   }

done:
   return ret ;
error:
   ret = FALSE ;
   goto done ;
}

INT32 sptConvertor2::_appendToBson( const std::string &name,
                                   const jsval &val,
                                   bson::BSONObjBuilder &builder )
{
   INT32 rc = SDB_OK ;
   switch (JS_TypeOfValue( _cx, val ))
   {
      case JSTYPE_VOID :
      {
         /// do nothing. 
         break ;
      }
      case JSTYPE_NULL :
      {
         builder.appendNull( name ) ;
         break ;
      }
      case JSTYPE_NUMBER :
      {
         if ( JSVAL_IS_INT( val ) )
         {
            INT32 iN = 0 ;
            rc = _toInt( val, iN ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            builder.appendNumber( name, iN ) ;
         }
         else
         {
            FLOAT64 fV = 0 ;
            rc = _toDouble( val, fV ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            builder.appendNumber( name, fV ) ;
         }
         break ;
      }
      case JSTYPE_STRING :
      {
         std::string str ;
         rc = _toString( val, str ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         builder.append( name, str ) ;
         break ;
      }
      case JSTYPE_BOOLEAN :
      {
         BOOLEAN bL = TRUE ;
         rc = _toBoolean( val, bL ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         builder.appendBool( name, bL ) ;
         break ;
      }
      case JSTYPE_OBJECT :
      {
         if ( JSVAL_IS_NULL( val ) )
         {
            builder.appendNull( name ) ;
         }
         else
         {
            JSObject *obj = JSVAL_TO_OBJECT( val ) ;
            if ( NULL == obj )
            {
               builder.appendNull( name ) ;
            }
            else if ( !_addSpecialObj( obj, name.c_str(), builder ) )
            {
               BSONObj bsonobj ;
               rc = toBson( obj, bsonobj ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               if ( JS_IsArrayObject( _cx, obj ) )
               {
                  builder.appendArray( name, bsonobj ) ;
               }
               else
               {
                  builder.append( name, bsonobj ) ;
               }
            }
            else
            {
               /// do noting
            }
         }
         break ;
      }
      case JSTYPE_FUNCTION :
      {
         std::string str ;
         rc = _toString( val, str ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         builder.appendCode( name, str ) ;
         break ;
      }
      default :
      {
         SDB_ASSERT( FALSE, "unexpected type" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN sptConvertor2::_getProperty( JSObject *obj,
                                    const CHAR *name,
                                    JSType type,
                                    jsval &val )
{
   if ( !JS_GetProperty( _cx, obj, name, &val ) )
   {
      return FALSE ;
   }
   else if ( type != JS_TypeOfValue( _cx, val ) )
   {
      return FALSE ;
   }
   else
   {
      return TRUE ;
   }
}

INT32 sptConvertor2::toString( JSContext *cx,
                              const jsval &val,
                              std::string &str )
{
   INT32 rc = SDB_OK ;
   //CHAR *utf8 = NULL ;
   SDB_ASSERT( NULL != cx, "impossible" ) ;
   size_t len = 0 ;
   JSString *jsStr = JS_ValueToString( cx, val ) ;
   if ( NULL == jsStr )
   {
      goto done ;
   }

   len = JS_GetStringLength( jsStr ) ;
   if ( 0 == len )
   {
      goto done ;
   }
   else
   {
/*      size_t cLen = len * 6 + 1 ;
      const jschar *utf16 = JS_GetStringCharsZ( cx, jsStr ) ; ;
      utf8 = (CHAR *)SDB_OSS_MALLOC( cLen ) ;
      if ( NULL == utf8 )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      if ( !JS_EncodeCharacters( cx, utf16, len, utf8, &cLen ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      str.assign( utf8, cLen ) ;
*/

      CHAR *p = JS_EncodeString ( cx , jsStr ) ;
      if ( NULL != p )
      {
         str.assign( p ) ;
         free( p ) ;
      }
   }
done:
//   if ( NULL != utf8 )
//   {
//      SDB_OSS_FREE( utf8 ) ;
//   }
   return rc ;
}

INT32 sptConvertor2::_toString( const jsval &val, std::string &str )
{
   return toString( _cx, val, str ) ;
}

INT32 sptConvertor2::_toInt( const jsval &val, INT32 &iN )
{
   INT32 rc = SDB_OK ;
   int32 ip = 0 ;
   if ( !JS_ValueToInt32( _cx, val, &ip ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   iN = ip ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor2::_toDouble( const jsval &val, FLOAT64 &fV )
{
   INT32 rc = SDB_OK ;
   jsdouble dp = 0 ;
   if ( !JS_ValueToNumber( _cx, val, &dp ))
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   fV = dp ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor2::_toBoolean( const jsval &val, BOOLEAN &bL )
{
   INT32 rc = SDB_OK ;
   JSBool bp = TRUE ;
   if ( !JS_ValueToBoolean( _cx, val, &bp ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   bL = bp ;
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN sptConvertor2::_isValidNumberLong( const CHAR *value )
{
   UINT32 len = 0 ;
   UINT32 i = 0;
   if ( NULL == value  )
      return FALSE ;
   len = ossStrlen( value ) ;
   if ( len > 0 )
   {
      if ( value[0] == '-' )
      {
         ++i ;
      }
   }
   for ( ; i < len; ++i )
   {
      if ( ! ( value[i] >= '0' && value[i] <= '9' ) )
      {
         return FALSE ;
      }
   }
   return TRUE ;
}