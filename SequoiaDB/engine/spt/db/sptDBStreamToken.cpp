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

   Source File Name = sptDBStreamToken.cpp

   Descriptive Name = Stream Token

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBStreamToken.hpp"

using namespace bson ;
using namespace sdbclient ;

namespace engine
{

   JS_CONSTRUCT_FUNC_DEFINE( _sptDBStreamToken, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBStreamToken, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBStreamToken, help )

   JS_BEGIN_MAPPING( _sptDBStreamToken, SPT_STREAM_TOKEN_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
   JS_MAPPING_END()

   _sptDBStreamToken::_sptDBStreamToken()
   {
   }

   _sptDBStreamToken::_sptDBStreamToken( const sdbStreamToken &token )
   : _token( token )
   {
   }

   _sptDBStreamToken::~_sptDBStreamToken()
   {
   }

   INT32 _sptDBStreamToken::construct( const _sptArguments &arg,
                                       _sptReturnVal &rval,
                                       BSONObj &detail )
   {
      INT32  rc = SDB_OK ;

      string tokenString ;

      if ( 0 == arg.argc() )
      {
         _token.resetToken() ;
      }
      else if ( arg.argc() > 0 )
      {
         rc = arg.getString( 0, tokenString ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get token string" ) ;
            goto error ;
         }
         _token.setToken( tokenString ) ;
      }

      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addSelfProperty( SPT_STREAM_TOKEN_TOKEN_FIELD )->setValue( _token.getToken() ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sptDBStreamToken::destruct()
   {
      return SDB_OK ;
   }

   const sdbStreamToken &_sptDBStreamToken::getToken() const
   {
      return _token ;
   }

   INT32 _sptDBStreamToken::help( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"StreamToken\": " << endl ;
      ss << "   StreamToken( [<token>] ) "
         << "- Create a StreamToken obj" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"StreamToken\": " << endl ;
      ss << "   toString()               "
         << "- Convert StreamToken to string format" << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"StreamToken\": " << endl ;
      ss << "   getToken()               "
         << "- Get token in string format" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}

