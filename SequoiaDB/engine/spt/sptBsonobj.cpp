/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptBsonobj.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptBsonobj.hpp"

using namespace bson ;

namespace engine
{

   /*
      _sptBsonobj implement
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptBsonobj, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptBsonobj, destruct)
   JS_MEMBER_FUNC_DEFINE_NORESET( _sptBsonobj, toJson )

   JS_BEGIN_MAPPING( _sptBsonobj, "BSONObj" )
     JS_ADD_MEMBER_FUNC( "toJson", toJson )
     JS_ADD_CONSTRUCT_FUNC( construct )
     JS_ADD_DESTRUCT_FUNC( destruct )
   JS_MAPPING_END()

   _sptBsonobj::_sptBsonobj()
   {

   }

   _sptBsonobj::_sptBsonobj( const bson::BSONObj &obj )
   {
      _obj = obj.copy() ;
   }

   _sptBsonobj::~_sptBsonobj()
   {

   }

   INT32 _sptBsonobj::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail)
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      rc = arg.getBsonobj( 0, obj ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "The 1st param must be Object" ) ;
         goto error ;
      }
      _obj = obj ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptBsonobj::toJson( const _sptArguments &arg,
                              _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      rval.getReturnVal().setValue( _obj.toString( false, true ) ) ;
      return SDB_OK ;
   }

   INT32 _sptBsonobj::destruct()
   {
      return SDB_OK ;
   }

}

