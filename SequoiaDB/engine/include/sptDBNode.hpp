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

   Source File Name = sptDBNode.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_NODE_HPP
#define SPT_DB_NODE_HPP
#include "client.hpp"
#include "sptApi.hpp"
using sdbclient::sdbNode ;
using sdbclient::_sdbNode ;
namespace engine
{
   #define SPT_NODE_NAME_FIELD      "_nodename"
   #define SPT_NODE_HOSTNAME_FIELD  "_hostname"
   #define SPT_NODE_SVCNAME_FIELD   "_servicename"
   #define SPT_NODE_NODEID_FIELD    "_nodeid"
   #define SPT_NODE_RG_FIELD        "_rg"
   #define SPT_NODE_RGNAME_FIELD    "_name"

   class _sptDBNode : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBNode )
   public:
      _sptDBNode( _sdbNode* pNode = NULL ) ;
      ~_sptDBNode() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 start( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 stop( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 connect( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 setLocation( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 setAttributes( const _sptArguments &arg,
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
      sdbNode _node ;
   } ;
   typedef _sptDBNode sptDBNode ;

   #define SPT_SET_NODE_TO_RETURNVAL( pNode )   \
      do\
      {\
         sptDBNode *__sptNode__ = SDB_OSS_NEW sptDBNode( pNode ) ;\
         if( NULL == __sptNode__ )\
         {\
            rc = SDB_OOM ;\
            detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBNode" ) ;\
            goto error ;\
         }\
         rc = rval.setUsrObjectVal< sptDBNode >( __sptNode__ ) ;\
         if( SDB_OK != rc )\
         {\
            SAFE_OSS_DELETE( __sptNode__ ) ;\
            pNode = NULL ;\
            detail = BSON( SPT_ERR << "Failed to set return obj" ) ;\
            goto error ;\
         }\
      }while(0)

}
#endif
