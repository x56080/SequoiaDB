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

   Source File Name = sptDBUser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          26/05/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_USER_HPP
#define SPT_DB_USER_HPP

#include "sptApi.hpp"

namespace engine
{
   #define SPT_USER_NAME           "User"
   #define SPT_USER_USER_FIELD     "_user"

   class _sptDBUser : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBUser )
   public:
      _sptDBUser() ;
      virtual ~_sptDBUser() ;
   public:
      INT32          construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;
      INT32          destruct() ;
#if !defined( SDB_FMP )
      INT32          promptPassword( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail ) ;
#endif
      const CHAR*    getPasswd() ;
      static INT32   cvtToBSON( const CHAR* key,
                                const sptObject &value,
                                BOOLEAN isSpecialObj,
                                BSONObjBuilder& builder,
                                string &errMsg ) ;
      static INT32   fmpToBSON( const sptObject &value,
                                BSONObj &retObj,
                                string &errMsg ) ;
      static INT32   bsonToJSObj( sdbclient::sdb &db,
                                  const BSONObj &data,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;
      static INT32   help( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;
   private:
      string _passwd ;
   } ;

   typedef _sptDBUser sptDBUser ;

}

#endif

