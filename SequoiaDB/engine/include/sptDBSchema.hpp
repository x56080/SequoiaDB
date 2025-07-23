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

   Source File Name = sptDBSchema.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_SCHEMA_HPP
#define SPT_DB_SCHEMA_HPP

#include "client.hpp"
#include "sptApi.hpp"

using sdbclient::sdbSchema ;
using sdbclient::_sdbSchema ;

namespace engine
{
   #define SPT_SCHEMA_NAME_FIELD            "_name"

   class _sptDBSchema : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBSchema ) ;
   public:
      _sptDBSchema( _sdbSchema *pSchema = NULL ) ;
      ~_sptDBSchema() ;

   public:
      INT32 construct( const _sptArguments &arg, _sptReturnVal &val, bson::BSONObj &detail ) ;
      INT32 destruct() ;

      INT32 addColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 alterColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 renameColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 dropColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 dropColumnDefault( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 setAttributes( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 alter( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;

   private:
      sdbSchema _schema ;
   } ;
   typedef _sptDBSchema sptDBSchema ;
}

#endif /* SPT_DB_SCHEMA_HPP */
