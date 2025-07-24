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

   Source File Name = sptDBMinKey.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBMinKey.hpp"
using namespace bson ;
namespace engine
{
   #define SPT_MINKEY_NAME          "MinKey"
   #define SPT_MINKEY_SPECIAL_FIELD "$minKey"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBMinKey, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBMinKey, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBMinKey, help )

   JS_BEGIN_MAPPING( _sptDBMinKey, SPT_MINKEY_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_MINKEY_SPECIAL_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBMinKey::cvtToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBMinKey::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBMinKey::_sptDBMinKey()
   {
   }

   _sptDBMinKey::~_sptDBMinKey()
   {
   }

   INT32 _sptDBMinKey::construct( const _sptArguments &arg,
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

   INT32 _sptDBMinKey::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBMinKey::cvtToBSON( const CHAR* key, const sptObject &value,
                                  BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                  string &errMsg )
   {
      builder.appendMinKey( key ) ;
      return SDB_OK ;
   }

   INT32 _sptDBMinKey::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBMinKey *minKey = SDB_OSS_NEW sptDBMinKey() ;
      if( NULL == minKey )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBMinKey obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBMinKey >( minKey ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( minKey ) ;
      goto done ;
   }

   INT32 _sptDBMinKey::help( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"MinKey\" : " << endl ;
      ss << "   { \"$minKey\": 1 }" << endl ;
      ss << "   MinKey()                   "
         << "- Data type: the minimum of all data types" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"MinKey\" : " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"MinKey\" : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }
}

