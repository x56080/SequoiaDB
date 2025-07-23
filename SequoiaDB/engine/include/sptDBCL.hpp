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

   Source File Name = sptDBCL.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_CL_HPP
#define SPT_DB_CL_HPP
#include "client.hpp"
#include "sptApi.hpp"
using sdbclient::_sdbCollection ;
using sdbclient::sdbCollection ;
namespace engine
{
   #define SPT_CL_NAME_FIELD     "_name"
   #define SPT_CL_CS_FIELD       "_cs"
   class _sptDBCL : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBCL )
   public:
      _sptDBCL( _sdbCollection *pCL = NULL ) ;
      ~_sptDBCL() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 rawFind( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 insert( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 update( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 upsert( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 remove( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 pop( const _sptArguments &arg,
                 _sptReturnVal &rval,
                 bson::BSONObj &detail ) ;

      INT32 count( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 lobCount( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 createIndex( const _sptArguments &arg,
                         _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 createIndexAsync( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;

      INT32 getIndexes( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 dropIndex( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 dropIndexAsync( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 copyIndex( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 copyIndexAsync( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 bulkInsert( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 split( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 splitAsync( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 aggregate( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 snapshotIndexes( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      INT32 alter( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 attachCL( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 detachCL( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 explain( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 putLob( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 getLob( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 getLobRTimeDetail( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      INT32 lobExplain( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 deleteLob( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 truncateLob( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 listLobs( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 createLobID( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 listLobPieces( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      INT32 truncate( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 createIdIndex( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      INT32 dropIdIndex( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 createAutoIncrement( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      INT32 dropAutoIncrement( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;

      INT32 getQueryMeta( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      INT32 enableSharding( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 disableSharding( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      INT32 enableCompression( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      INT32 disableCompression( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      INT32 setAttributes( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      INT32 getDetail( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 getIndexStat( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      INT32 getCollectionStat( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      INT32 query( const BSONObj &cond, const BSONObj &sel,
                   const BSONObj &order, const BSONObj &hint,
                   const BSONObj &options, INT32 numToSkip,
                   INT32 numToRet, INT32 flags,
                   sdbclient::_sdbCursor **curosr ) ;

      INT32 getCount( SINT64 &count, const BSONObj &condition,
                      const BSONObj &hint ) ;
      INT32 setConsistencyStrategy( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;
      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;

   private:
      INT32 _parseInsertOptions( const _sptArguments &arg, SINT32 &flags,
                                 BSONObj &hint, bson::BSONObj &detail ) ;

      INT32 _createIndex( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail,
                          BOOLEAN isAsync ) ;

      INT32 _dropIndex( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail,
                        BOOLEAN isAsync ) ;

      INT32 _copyIndex( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail,
                        BOOLEAN isAsync ) ;

   private:
      sdbCollection _cl ;
   } ;
   typedef _sptDBCL sptDBCL ;
}
#endif
