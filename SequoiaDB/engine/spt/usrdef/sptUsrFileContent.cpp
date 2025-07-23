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

   Source File Name = sptUsrFileContent.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/03/2017  wujiaming  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrFileContent.hpp"
#include "../bson/lib/base64.h"
#include <string>
using std::string ;

namespace engine
{

JS_CONSTRUCT_FUNC_DEFINE( _sptUsrFileContent, construct )
JS_DESTRUCT_FUNC_DEFINE( _sptUsrFileContent, destruct )
JS_MEMBER_FUNC_DEFINE( _sptUsrFileContent, getLength )
JS_MEMBER_FUNC_DEFINE( _sptUsrFileContent, toBase64Code )
JS_MEMBER_FUNC_DEFINE( _sptUsrFileContent, clear )

JS_BEGIN_MAPPING( _sptUsrFileContent, "FileContent" )
   JS_ADD_CONSTRUCT_FUNC( construct )
   JS_ADD_DESTRUCT_FUNC( destruct )
   JS_ADD_MEMBER_FUNC( "getLength", getLength )
   JS_ADD_MEMBER_FUNC( "toBase64Code", toBase64Code )
   JS_ADD_MEMBER_FUNC( "clear", clear )
JS_MAPPING_END()

   _sptUsrFileContent::_sptUsrFileContent()
   {
      _buf = NULL ;
      _length = 0 ;
   }

   _sptUsrFileContent::~_sptUsrFileContent()
   {
      clear() ;
   }

   INT32 _sptUsrFileContent::init( const CHAR* buf, UINT32 len )
   {
      INT32 rc = SDB_OK ;

      if( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf must be not null" ) ;
         goto error ;
      }

      if( _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
         _length = 0 ;
      }

      _buf = ( CHAR* )SDB_OSS_MALLOC( len + 1 ) ;
      if( NULL == _buf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to init FileContent obj" ) ;
         goto error ;
      }
      ossMemcpy( _buf, buf, len ) ;
      _buf[ len ] = '\0' ;
      _length = len ;

   done:
      return rc ;
   error:
      if( _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
      }
      goto done ;
   }

   INT32 _sptUsrFileContent::construct( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      return SDB_OK ;
   }

   INT32 _sptUsrFileContent::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptUsrFileContent::getLength( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      rval.getReturnVal().setValue( _length ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrFileContent::toBase64Code( const _sptArguments &arg,
                                           _sptReturnVal &rval,
                                           bson::BSONObj &detail )
   {
      string retStr ;

      retStr = base64::encode( _buf, _length ) ;
      rval.getReturnVal().setValue( retStr ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrFileContent::clear( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      clear() ;
      return SDB_OK ;
   }

}
