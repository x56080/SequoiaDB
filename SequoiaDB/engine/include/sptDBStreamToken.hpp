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

   Source File Name = sptDBStreamToken.hpp

   Descriptive Name = Stream Token

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_STREAM_TOKEN_HPP
#define SPT_DB_STREAM_TOKEN_HPP

#include "sptApi.hpp"
#include "client.hpp"

namespace engine
{

   #define SPT_STREAM_TOKEN_NAME          "StreamToken"
   #define SPT_STREAM_TOKEN_TOKEN_FIELD   "_token"

   class _sptDBStreamToken : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBStreamToken )

   public:
      _sptDBStreamToken() ;
      _sptDBStreamToken( const sdbclient::sdbStreamToken &token ) ;
      virtual ~_sptDBStreamToken() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;

      const sdbclient::sdbStreamToken &getToken() const ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

   private:
      sdbclient::sdbStreamToken _token ;
   } ;

   typedef class _sptDBStreamToken sptDBStreamToken ;

}

#endif // SPT_DB_STREAM_TOKEN_HPP
