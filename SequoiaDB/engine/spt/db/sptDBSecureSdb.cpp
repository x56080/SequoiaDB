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

   Source File Name = sptDBSecureSdb.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBSecureSdb.hpp"
#include "sptDBSdb.hpp"
namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBSecureSdb, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBSecureSdb, destruct )
   JS_RESOLVE_FUNC_DEFINE( _sptDBSecureSdb, resolve )
   JS_BEGIN_MAPPING_WITHPARENT( _sptDBSecureSdb, "SecureSdb", _sptDBSdb )
   JS_ADD_CONSTRUCT_FUNC(construct)
   JS_ADD_DESTRUCT_FUNC(destruct)
   JS_ADD_RESOLVE_FUNC(resolve)
   JS_SET_CVT_TO_BSON_FUNC( _sptDBSecureSdb::cvtToBSON )
   JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBSecureSdb::fmpToBSON )
   JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBSecureSdb::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBSecureSdb::_sptDBSecureSdb(): _sptDBSdb( TRUE )
   {
   }

   _sptDBSecureSdb::~_sptDBSecureSdb()
   {
   }

   INT32 _sptDBSecureSdb::cvtToBSON( const CHAR* key, const sptObject &value,
                                     BOOLEAN isSpecialObj,
                                     BSONObjBuilder& builder,
                                     string &errMsg )
   {
      errMsg = "SecureSdb can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBSecureSdb::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                     string &errMsg )
   {
      errMsg = "SecureSdb obj can not be return" ;
      return SDB_SYS ;
   }

   INT32 _sptDBSecureSdb::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                       _sptReturnVal &rval,
                                       bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "SecureSdb obj can not be return" ) ;
      return SDB_SYS ;
   }
}