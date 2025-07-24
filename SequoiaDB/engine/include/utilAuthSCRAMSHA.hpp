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

   Source File Name = utilAuthSCRAMSHA.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          15/04/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _UTIL_AUTH_SCRAMSHA_HPP_
#define _UTIL_AUTH_SCRAMSHA_HPP_

#include "ossUtil.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <string>
#include "../bson/lib/base64.h"

using namespace std ;

#define UTIL_AUTH_CLIENT_KEY                         "Client Key"
#define UTIL_AUTH_SERVER_KEY                         "Server Key"
#define UTIL_AUTH_SCRAMSHA256_HASH_SIZE              32
#define UTIL_AUTH_SCRAMSHA256_HASH_BASE64_SIZE       44
#define UTIL_AUTH_SCRAMSHA256_SALT_LEN               28
#define UTIL_AUTH_SCRAMSHA256_SALT_BASE64_LEN        40
#define UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT         15000

#define UTIL_AUTH_SCRAMSHA1_HASH_SIZE                20
#define UTIL_AUTH_SCRAMSHA1_HASH_BASE64_SIZE         28
#define UTIL_AUTH_SCRAMSHA1_SALT_LEN                 16
#define UTIL_AUTH_SCRAMSHA1_SALT_BASE64_LEN          24
#define UTIL_AUTH_SCRAMSHA1_ITERATIONCOUNT           10000

#define UTIL_AUTH_SCRAMSHA_NONCE_LEN                 24
#define UTIL_AUTH_SCRAMSHA_NONCE_BASE64_LEN          32

#define UTIL_AUTH_MD5SUM_LEN                         32

/*

For a detailed introduction to the SCRAM-SHA256 authentication mechanism, please
refer to the official website document

http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1432190676-edition_id-0

*/

INT32 utilAuthCaculateKey( const CHAR *originalPassword,
                           const BYTE *salt, INT32 saltLen,
                           UINT32 iterationCount,
                           string &storedKeyBase64,
                           string &serverKeyBase64,
                           string &clientKeyBase64 ) ;

INT32 utilAuthCaculateKey1( const CHAR *originalPassword,
                            const BYTE *salt, INT32 saltLen,
                            UINT32 iterationCount,
                            string &storedKeyBase64,
                            string &serverKeyBase64,
                            string &clientKeyBase64 ) ;

INT32 utilAuthCaculateClientProof( const CHAR *originalPassword,
                                   const CHAR *username,
                                   UINT32 iterationCount,
                                   const CHAR *saltBase64,
                                   const CHAR *combineNonce,
                                   const CHAR *identify,
                                   BOOLEAN fromSdb,
                                   string &clientProofBase64 ) ;

INT32 utilAuthCaculateServerProof( const CHAR *username,
                                   UINT32 iterationCount,
                                   const CHAR *saltBase64,
                                   const CHAR *combineNonceBase64,
                                   const CHAR *identify,
                                   const CHAR *serverKeyBase64,
                                   BOOLEAN fromSdb,
                                   string &serverProofBase64 ) ;

INT32 utilAuthCaculateServerProof1( const CHAR *username,
                                    UINT32 iterationCount,
                                    const CHAR *saltBase64,
                                    const CHAR *combineNonceBase64,
                                    const CHAR *identify,
                                    const CHAR *serverKeyBase64,
                                    string &serverProofBase64 ) ;

INT32 utilAuthGenerateNonce( BYTE *nonce, UINT32 nonceLen ) ;

INT32 utilAuthVerifyClientProof( const CHAR *clientProofBase64,
                                 const CHAR *username,
                                 UINT32 iterationCount,
                                 const CHAR *saltBase64,
                                 const CHAR *combineNonceBase64,
                                 const CHAR *identify,
                                 const CHAR *storedKeyBase64,
                                 BOOLEAN fromSdb,
                                 BOOLEAN &isValid ) ;

INT32 utilAuthVerifyClientProof1( const CHAR *clientProofBase64,
                                  const CHAR *username,
                                  UINT32 iterationCount,
                                  const CHAR *saltBase64,
                                  const CHAR *combineNonceBase64,
                                  const CHAR *identify,
                                  const CHAR *storedKeyBase64,
                                  BOOLEAN &isValid ) ;

INT32 utilAuthVerifyServerProof( const CHAR *serverProofBase64,
                                 const CHAR *username,
                                 UINT32 iterationCount,
                                 const CHAR *saltBase64,
                                 const CHAR *combineNonceBase64,
                                 const CHAR *identify,
                                 const CHAR *serverKeyBase64,
                                 BOOLEAN fromSdb,
                                 BOOLEAN &isValid ) ;

#endif
