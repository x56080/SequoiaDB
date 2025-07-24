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

   Source File Name = utilSecure.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/10/2022  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILSECURE_HPP_
#define UTILSECURE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

#define UTIL_SECURE_HEAD               "SDBSECURE"
#define UTIL_SECURE_BEGIN_SYMBOL       '('
#define UTIL_SECURE_END_SYMBOL         ')'
#define UTIL_SECURE_END_SYMBOL_STR     ")"
#define UTIL_SECURE_ENCRYPT_ALGORITHM  UTIL_SECURE_SDB_BASE64
#define UTIL_SECURE_ENCRYPT_VERSION    UTIL_SECURE_SDB_BASE64_V0
#define UTIL_SECURE_COMPRESS_ALGORITHM 0 // reserve
#define UTIL_SECURE_COMPRESS_VERSION   0 // reserve

#define UTIL_SECURE_SDB_BASE64     0
#define UTIL_SECURE_SDB_BASE64_V0  0

ossPoolString utilSecureObj( const bson::BSONObj &obj ) ;

ossPoolString utilSecureStr( const CHAR* data, INT32 size ) ;
ossPoolString utilSecureStr( const ossPoolString& str ) ;
ossPoolString utilSecureStr( const string& str ) ;

INT32 utilSecureDecrypt( const ossPoolString& str, ossPoolString& output ) ;

#endif
