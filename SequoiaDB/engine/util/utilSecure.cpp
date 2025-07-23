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

   Source File Name = utilSecure.cpp

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
#include "utilSecure.hpp"
#include "base64.hpp"

using namespace bson ;
using namespace std ;

#define UTIL_SECURE_BASE64_CODETABLE (unsigned char*) \
                                     "IKJNHGOPCQRZSTBUVDWXELAYFM" \
                                     "ikjnhgopcqrzstbuvdwxelayfm" \
                                     "/4+593168207"

engine::base64::Alphabet alphabet1( UTIL_SECURE_BASE64_CODETABLE ) ;

ossPoolString utilSecureObj( const BSONObj &obj )
{
   return engine::base64::encodeEx( obj.objdata(), obj.objsize(), &alphabet1 ) ;
}

ossPoolString utilSecureStr( const CHAR* data, INT32 size )
{
   ossPoolStringStream ss ;

   try
   {
      ss << UTIL_SECURE_HEAD
         << UTIL_SECURE_ENCRYPT_ALGORITHM
         << UTIL_SECURE_ENCRYPT_VERSION
         << UTIL_SECURE_COMPRESS_ALGORITHM
         << UTIL_SECURE_COMPRESS_VERSION
         << UTIL_SECURE_BEGIN_SYMBOL ;
      engine::base64::encodeEx( ss, data, size, &alphabet1 ) ;
      ss << UTIL_SECURE_END_SYMBOL ;
      return ss.str() ;
   }
   catch ( std::exception &e )
   {
      try
      {
         ss << e.what() ;
         return ss.str() ;
      }
      catch (...)
      {
         return "Out-of-memory" ;
      }
   }
}

ossPoolString utilSecureStr( const ossPoolString& str )
{
   return utilSecureStr( str.c_str(), str.size() ) ;
}

ossPoolString utilSecureStr( const string& str )
{
   return utilSecureStr( str.c_str(), str.size() ) ;
}

static INT32 _utilChar2Int( CHAR c )
{
   return c - '0' ;
}

INT32 utilSecureDecrypt( const ossPoolString& str, ossPoolString& output )
{
   INT32 rc = SDB_OK ;
   INT32 strLen = str.size() ;
   INT32 headLen = sizeof( UTIL_SECURE_HEAD ) - 1 ;
   INT32 ciphertextLen = 0 ;
   INT32 i = 0 ;

   if ( strLen <= headLen + 4 + 1 ) // 0000(
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 0 != ossStrncmp( str.c_str(), UTIL_SECURE_HEAD, headLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   i = headLen ;

   if ( _utilChar2Int( str[i++] ) != UTIL_SECURE_SDB_BASE64 ||
        _utilChar2Int( str[i++] ) != UTIL_SECURE_SDB_BASE64_V0 ||
        _utilChar2Int( str[i++] ) != UTIL_SECURE_COMPRESS_ALGORITHM ||
        _utilChar2Int( str[i++] ) != UTIL_SECURE_COMPRESS_VERSION )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( str[i++] != UTIL_SECURE_BEGIN_SYMBOL )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ciphertextLen = strLen - i ;
   if ( str[strLen-1] == UTIL_SECURE_END_SYMBOL )
   {
      ciphertextLen -= 1 ;
   }

   try
   {
      output = engine::base64::decodeEx( str.c_str() + i, ciphertextLen,
                                         &alphabet1 ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

