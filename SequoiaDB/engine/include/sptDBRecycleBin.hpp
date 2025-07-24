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

   Source File Name = sptDBRecycleBin.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_RECYCLEBIN_HPP
#define SPT_DB_RECYCLEBIN_HPP

#include "client.hpp"
#include "sptApi.hpp"

using sdbclient::sdbRecycleBin ;
using sdbclient::_sdbRecycleBin ;

namespace engine
{

   class _sptDBRecycleBin : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBRecycleBin )

   public:
      _sptDBRecycleBin( _sdbRecycleBin *pRecycleBin = NULL ) ;
      ~_sptDBRecycleBin() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 getDetail( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 enable( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 disable( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 setAttributes( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      INT32 alter( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 list( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 snapshot( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 count( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 dropItem( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;
      INT32 dropAll( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;
      INT32 returnItem( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;
      INT32 returnItemToName( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;
      static INT32 cvtToBSON( const CHAR *key,
                              const sptObject &value,
                              BOOLEAN isSpecialObj,
                              BSONObjBuilder &builder,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db,
                                const BSONObj &data,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;
   protected:
      sdbRecycleBin _recycleBin ;
   } ;

   typedef class _sptDBRecycleBin sptDBRecycleBin ;
}

#endif // SPT_DB_RECYCLEBIN_HPP
