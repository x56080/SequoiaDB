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

   Source File Name = sptDBQueryOption.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/07/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBQueryOption.hpp"
#include "sptBsonobj.hpp"

using namespace bson ;

namespace engine
{

   JS_CONSTRUCT_FUNC_DEFINE( _sptDBQueryOption, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBQueryOption, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBQueryOption, help )

   JS_BEGIN_MAPPING_WITHPARENT( _sptDBQueryOption, SPT_QUERYOPTION_NAME,
                                _sptDBOptionBase )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBQueryOption::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBQueryOption::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBQueryOption::bsonToJSObj )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
   JS_MAPPING_END()

   _sptDBQueryOption::_sptDBQueryOption()
   {
   }

   _sptDBQueryOption::~_sptDBQueryOption()
   {
   }

   INT32 _sptDBQueryOption::construct( const _sptArguments &arg,
                                       _sptReturnVal &rval,
                                       bson::BSONObj &detail )
   {
      _sptDBOptionBase::construct( arg, rval, detail ) ;

      rval.addSelfProperty( SPT_QUERYOPTION_OPTIONS_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( BSONObj() ) ;

      return SDB_OK ;
   }

   INT32 _sptDBQueryOption::destruct()
   {
      _sptDBOptionBase::destruct() ;
      return SDB_OK ;
   }

   INT32 _sptDBQueryOption::cvtToBSON( const CHAR* key,
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

   INT32 _sptDBQueryOption::fmpToBSON( const sptObject &value,
                                       BSONObj &retObj,
                                       string &errMsg )
   {
      INT32 rc = SDB_OK ;
      sptObjectPtr ptr ;
      sptBsonobj *pBsonObj = NULL ;

      BSONObj baseRetObj ;
      BSONObjBuilder builder ;

      rc = _sptDBOptionBase::fmpToBSON( value, baseRetObj, errMsg ) ;
      if ( rc )
      {
         goto error ;
      }

      builder.appendElements( baseRetObj ) ;

      if ( value.isFieldExist( SPT_QUERYOPTION_OPTIONS_FIELD ) )
      {
         rc = value.getObjectField( SPT_QUERYOPTION_OPTIONS_FIELD,
                                    ptr ) ;
         if ( rc )
         {
            errMsg = "Failed to get options field" ;
            goto error ;
         }

         rc = ptr->getUserObj( _sptBsonobj::__desc,
                               (const void**)&pBsonObj ) ;
         if ( rc )
         {
            errMsg = "Failed to get option data field" ;
            goto error ;
         }
         builder.append( SPT_QUERYOPTION_OPTIONS_FIELD,
                         pBsonObj->getBson() ) ;
      }

      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBQueryOption::bsonToJSObj( sdbclient::sdb &db,
                                         const BSONObj &data,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBQueryOption *pSnapOption = NULL ;

      pSnapOption = SDB_OSS_NEW sptDBQueryOption() ;
      if( NULL == pSnapOption )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBQueryOption obj" ) ;
         goto error ;
      }

      rc = rval.setUsrObjectVal< sptDBQueryOption >( pSnapOption ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }

      _setReturnVal( data, rval ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pSnapOption ) ;
      goto done ;
   }

   void _sptDBQueryOption::_setReturnVal( const BSONObj &data,
                                          _sptReturnVal &rval )
   {
      _sptDBOptionBase::_setReturnVal(  data, rval ) ;

      BSONObj obj ;
      obj = data.getObjectField( SPT_QUERYOPTION_OPTIONS_FIELD ) ;
      rval.addSelfProperty( SPT_QUERYOPTION_OPTIONS_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( obj ) ;
   }

   INT32 _sptDBQueryOption::help( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"SdbQueryOption\" : " << endl ;
      ss << "   SdbQueryOption[.cond(<cond>)]" << endl ;
      ss << "                 [.sel(<sel>)]" << endl ;
      ss << "                 [.sort(<sort>)]" << endl ;
      ss << "                 [.options(<options>)]" << endl ;
      ss << "                 [.skip(<skipNum>)]" << endl ;
      ss << "                 [.limit(<retNum>)]" << endl ;
      ss << "                 [.update(<rule>, [returnNew], [options])]" << endl ;
      ss << "                 [.remove()]" << endl ;
      ss << "                              "
         << "- Create a SdbQueryOption object" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"SdbQueryOption\" : " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"SdbQueryOption\" : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }
}
