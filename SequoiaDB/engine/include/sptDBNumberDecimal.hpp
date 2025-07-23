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

   Source File Name = sptDBNumberDecimal.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_NUMBERDECIMAL_HPP
#define SPT_DB_NUMBERDECIMAL_HPP
#include "sptApi.hpp"

namespace engine
{
   class _sptDBNumberDecimal : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBNumberDecimal )
   public:
      _sptDBNumberDecimal() ;
      virtual ~_sptDBNumberDecimal() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;
      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;
      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   private:
      static INT32 _getDecimalPrecision( const CHAR *precisionStr,
                                         INT32 *precision, INT32 *scale ) ;
      static INT32 _checkParameters( const bson::StringData& strDecimal,
                                     INT32 precision, INT32 scale,
                                     BOOLEAN isPrecisionNull,
                                     bson::BSONObj &detail ) ;
   } ;
   typedef _sptDBNumberDecimal sptDBNumberDecimal ;
}
#endif
