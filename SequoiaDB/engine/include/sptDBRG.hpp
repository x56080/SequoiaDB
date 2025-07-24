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

   Source File Name = sptDBRG.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_RG_HPP
#define SPT_DB_RG_HPP
#include "client.hpp"
#include "sptApi.hpp"
using sdbclient::sdbReplicaGroup ;
using sdbclient::_sdbReplicaGroup ;
namespace engine
{
   #define SPT_RG_NAME_FIELD  "_name"
   #define SPT_RG_CONN_FIELD  "_conn"
   class _sptDBRG : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBRG )
   public:
      _sptDBRG( _sdbReplicaGroup *pRG = NULL ) ;
      ~_sptDBRG() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 getMaster( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 getSlave( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 start( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 stop( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 createNode( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 removeNode( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 getNode( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 reelect( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 reelectLocation( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      INT32 setActiveLocation( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      INT32 setAttributes( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      INT32 startCriticalMode( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      INT32 stopCriticalMode( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;

      INT32 startMaintenanceMode( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      INT32 stopMaintenanceMode( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      INT32 detachNode( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 attachNode( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 getNodeAndSetProperty( const string &hostname, const string &svcname,
                                    _sptReturnVal &rval, bson::BSONObj &detail ) ;
      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;
   private:
      sdbReplicaGroup _rg ;
   } ;
   typedef _sptDBRG sptDBRG ;

   #define SPT_SET_RG_TO_RETURNVAL( pRG )\
      do\
      {\
         sptDBRG *__sptRG__ = SDB_OSS_NEW sptDBRG( pRG ) ;\
         if( NULL == __sptRG__ )\
         {\
            rc = SDB_OOM ;\
            detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBRG" ) ;\
            goto error ;\
         }\
         rc = rval.setUsrObjectVal< sptDBRG >( __sptRG__ ) ;\
         if( SDB_OK != rc )\
         {\
            SAFE_OSS_DELETE( __sptRG__ ) ;\
            pRG = NULL ;\
            detail = BSON( SPT_ERR << "Failed to set return obj" ) ;\
            goto error ;\
         }\
         rval.getReturnVal().setName( pRG->getName() ) ;\
         rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;\
         rval.addReturnValProperty( SPT_RG_NAME_FIELD )\
            ->setValue( pRG->getName() ) ;\
         rval.addSelfToReturnValProperty( SPT_RG_CONN_FIELD ) ;\
      }while(0)
}
#endif
