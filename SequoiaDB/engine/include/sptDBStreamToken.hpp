/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
