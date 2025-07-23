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

   Source File Name = sptDBSnapshotOption.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          16/07/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_SNAPSHOTOPTION_HPP
#define SPT_DB_SNAPSHOTOPTION_HPP

#include "sptApi.hpp"
#include "sptDBOptionBase.hpp"

using namespace bson ;

namespace engine
{
   #define SPT_SNAPSHOTOPTION_NAME      "SdbSnapshotOption"

   /*
      _sptDBSnapshotOption define
   */
   class _sptDBSnapshotOption : public _sptDBOptionBase
   {
      JS_DECLARE_CLASS( _sptDBSnapshotOption )
   public:
      _sptDBSnapshotOption() ;
      virtual ~_sptDBSnapshotOption() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;

      static INT32 bsonToJSObj( sdbclient::sdb &db,
                                const BSONObj &data,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   } ;

   typedef _sptDBSnapshotOption sptDBSnapshotOption ;
}

#endif // SPT_DB_SNAPSHOTOPTION_HPP
