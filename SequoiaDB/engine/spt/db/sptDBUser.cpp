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

   Source File Name = sptDBUser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          26/05/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBUser.hpp"
#include "../util/linenoise.h"
#include "msgDef.h"
#include "ossPrimitiveFileOp.hpp"

using namespace bson ;

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBUser, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBUser, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBUser, help )
#if !defined( SDB_FMP )
   JS_MEMBER_FUNC_DEFINE( _sptDBUser, promptPassword )
#endif

   JS_BEGIN_MAPPING( _sptDBUser, SPT_USER_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
#if !defined( SDB_FMP )
      JS_ADD_MEMBER_FUNC( "_promptPassword", promptPassword )
#endif
      JS_SET_CVT_TO_BSON_FUNC( _sptDBUser::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBUser::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBUser::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBUser::_sptDBUser()
   {
   }

   _sptDBUser::~_sptDBUser()
   {
   }

   INT32 _sptDBUser::construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32  rc = SDB_OK ;
      string username ;

      if( arg.argc() == 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "You should input username" ) ;
         goto error ;
      }

      if ( arg.argc() > 1 )
      {
         // if password has been input, we don't save command to history file.
         sdbSetIsNeedSaveHistory( FALSE ) ;
      }

      if( arg.isString( 0 ) )
      {
         rc = arg.getString( 0, username ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get username" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Username must be string" ) ;
         goto error ;
      }

      if ( arg.argc() > 1 )
      {
         if( arg.isString( 1 ) )
         {
            rc = arg.getString( 1, _passwd ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Failed to get password" ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Password must be string" ) ;
            goto error ;
         }
      }

      rval.addSelfProperty( SPT_USER_USER_FIELD )->setValue( username ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBUser::destruct()
   {
      return SDB_OK ;
   }

#if !defined( SDB_FMP )
   INT32 _sptDBUser::promptPassword( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc   = SDB_OK ;
      CHAR* line = NULL ;

      setEchoOff() ;
      if ( ( line = linenoise( "password: " ) ) != NULL )
      {
         _passwd = line ;
      }
      else
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Failed to get password" ) ;
         goto error ;
      }

done:
   SDB_OSS_ORIGINAL_FREE( line ) ;
   setEchoOn() ;
   return rc ;
error:
   goto done ;
   }
#endif

   const CHAR* _sptDBUser::getPasswd()
   {
      return _passwd.c_str() ;
   }

   INT32 _sptDBUser::cvtToBSON( const CHAR* key,
                                const sptObject &value,
                                BOOLEAN isSpecialObj,
                                BSONObjBuilder& builder,
                                string &errMsg )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      rc = fmpToBSON( value, obj, errMsg ) ;
      if ( rc )
      {
         goto error ;
      }

      builder.append( key, obj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBUser::fmpToBSON( const sptObject &value,
                                BSONObj &retObj,
                                string &errMsg )
   {
      INT32  rc = SDB_OK ;
      string username ;

      rc = value.getStringField( SPT_USER_USER_FIELD, username ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get username" ;
         goto error ;
      }

      retObj = BSON( SDB_AUTH_USER << username.c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBUser::bsonToJSObj( sdbclient::sdb &db,
                                  const BSONObj &data,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBUser *pUser = NULL ;

      pUser = SDB_OSS_NEW sptDBUser() ;
      if ( NULL == pUser )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBUser obj" ) ;
         goto error ;
      }

      rc = rval.setUsrObjectVal< sptDBUser >( pUser ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }

      rval.addReturnValProperty( SPT_USER_USER_FIELD )
      ->setValue( data.getStringField( SDB_AUTH_USER ) ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pUser ) ;
      goto done ;
   }

   INT32 _sptDBUser::help( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"User\": " << endl ;
      ss << "   User( <username>, [passwd] )" << endl ;
      ss << "                              "
         << "- Create a User obj" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"User\": " << endl ;
      ss << "   toString()                 "
         << "- Convert User to string format" << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"User\": " << endl ;
      ss << "   promptPassword()           "
         << "- Input password" << endl ;
      ss << "   getUsername()              "
         << "- Get username" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}

