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

   Source File Name = sptDBDataSource.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_DATASOURCE_HPP__
#define SPT_DB_DATASOURCE_HPP__

#include "client.hpp"
#include "sptApi.hpp"

using sdbclient::_sdbDataSource ;
using sdbclient::sdbDataSource ;

namespace engine
{
   #define SPT_DS_NAME_FIELD  "_name"
   class _sptDBDataSource : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBDataSource ) ;
   public:
      _sptDBDataSource( _sdbDataSource *pDataSource = NULL ) ;
      ~_sptDBDataSource() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 alter( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

   private:
      sdbDataSource _dataSource ;
   } ;
   typedef _sptDBDataSource sptDBDataSource ;
}

#endif /* SPT_DB_DATASOURCE_HPP__ */
