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

   Source File Name = sptDBMaxKey.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBMaxKey.hpp"
using namespace bson ;
namespace engine
{
   #define SPT_MAXKEY_NAME          "MaxKey"
   #define SPT_MAXKEY_SPECIAL_FIELD "$maxKey"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBMaxKey, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBMaxKey, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBMaxKey, help )

   JS_BEGIN_MAPPING( _sptDBMaxKey, SPT_MAXKEY_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_MAXKEY_SPECIAL_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBMaxKey::cvtToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBMaxKey::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBMaxKey::_sptDBMaxKey()
   {
   }

   _sptDBMaxKey::~_sptDBMaxKey()
   {
   }

   INT32 _sptDBMaxKey::construct( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() != 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "No arguments are required" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBMaxKey::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBMaxKey::cvtToBSON( const CHAR* key, const sptObject &value,
                                  BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                  string &errMsg )
   {
      builder.appendMaxKey( key ) ;
      return SDB_OK ;
   }

   INT32 _sptDBMaxKey::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                    _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBMaxKey *maxKey = SDB_OSS_NEW sptDBMaxKey() ;
      if( NULL == maxKey )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBMaxKey obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBMaxKey >( maxKey ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( maxKey ) ;
      goto done ;
   }

   INT32 _sptDBMaxKey::help( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"MaxKey\" : " << endl ;
      ss << "   { \"$maxKey\": 1 }" << endl ;
      ss << "   MaxKey()                   "
         << "- Data type: the maximum of all data types" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"MaxKey\" : " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"MaxKey\" : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}

