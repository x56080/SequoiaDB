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

   Source File Name = sptDBNumberLong.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_NUMBERLONG_HPP
#define SPT_DB_NUMBERLONG_HPP
#include "sptApi.hpp"

#define SPT_NUMBERLONG_NAME             "NumberLong"
#define SPT_NUMBERLONG_VALUE_FIELD      "_v"
#define SPT_NUMBERLONG_SPECIALOBJ_FIELD "$numberLong"

namespace engine
{
   class _sptDBNumberLong : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBNumberLong )
   public:
      _sptDBNumberLong() ;
      virtual ~_sptDBNumberLong() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;
      INT64 getValue() { return _value ; }
      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;

      static INT32 cvtToString( const sptObject &obj, BOOLEAN isSpecialObj,
                                string &retVal ) ;

      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;
      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   private:
      static BOOLEAN _isValidNumberLong( const CHAR *value ) ;
      INT64 _value ;
   } ;
   typedef _sptDBNumberLong sptDBNumberLong ;
}
#endif

