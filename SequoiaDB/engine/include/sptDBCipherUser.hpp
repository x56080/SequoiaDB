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

   Source File Name = sptDBCipherUser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          26/05/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_CIPHER_USER_HPP
#define SPT_DB_CIPHER_USER_HPP

#include "sptApi.hpp"

namespace engine
{
   #define SPT_CIPHERUSER_NAME                      "CipherUser"
   #define SPT_CIPHERUSER_USER_FIELD                "_user"
   #define SPT_CIPHERUSER_CLUSTER_NAME_FIELD        "_clusterName"
   #define SPT_CIPHERUSER_CIPHER_FILE_FIELD         "_cipherFile"

   #define SPT_CIPHERUSER_FIELD_NAME_CLUSTER_NAME   "ClusterName"
   #define SPT_CIPHERUSER_FIELD_NAME_CIPHER_FILE    "CipherFile"

   class _sptDBCipherUser : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBCipherUser )
   public:
      _sptDBCipherUser() ;
      virtual ~_sptDBCipherUser() ;
   public:
      INT32          construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;
      INT32          destruct() ;
      INT32          setToken( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;
      const CHAR*    getToken() ;
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
      string _token ;
   } ;

   typedef _sptDBCipherUser sptDBCipherUser ;

}

#endif

