/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = mongoReplyHelper.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/05/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongoReplyHelper.hpp"
#include "sdbInterface.hpp"
#include "../../bson/bson.hpp"
#include "../../bson/lib/nonce.h"
namespace fap
{
   namespace mongo
   {
      void buildIsMasterReplyMsg( engine::IResource *resource,
                                  engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         bob.append( "ismaster", TRUE ) ;
         bob.append("msg", "isdbgrid");
         // build
         // config at last
         bob.append( "maxBsonObjectSize", 16*1024*1024 ) ;
         bob.append( "maxMessageSizeBytes", SDB_MAX_MSG_LENGTH ) ;
         bob.append( "maxWriteBatchSize", 1000 ) ;
         bob.append( "localTime", 100 ) ;
         bob.append( "maxWireVersion", 2 ) ;
         bob.append( "minWireVersion", 2 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildGetNonceReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         static Nonce::Security security ;
         UINT64 nonce = security.getNonce() ;

         std::stringstream ss ;
         ss << std::hex << nonce ;
         bob.append( "nonce", ss.str() ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildGetLastErrorReplyMsg( const bson::BSONObj &err,
                                      engine::rtnContextBuf &buff )
      {
         INT32 rc = SDB_OK ;
         bson::BSONObjBuilder bob ;
         rc = err.getIntField( OP_ERRNOFIELD ) ;
         if ( SDB_OK == rc )
         {
            bob.append( "ok", 1.0 ) ;
            bob.appendNull( "err" ) ;
         }
         if ( SDB_OK != rc && !err.isEmpty() )
         {
            bob.append( "ok", 0.0 ) ;
            bob.append( "code", rc ) ;
            bob.append( "errmsg", err.getStringField( OP_ERRDESP_FIELD) ) ;
            bob.append( "err", err.getStringField( OP_ERRDESP_FIELD) ) ;
         }
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildNotSupportReplyMsg( engine::rtnContextBuf &buff,
                                    const CHAR *cmdName )
      {
         bson::BSONObjBuilder bob ;
         std::string err = "no such cmd:";
         err += cmdName ;
         bob.append( "ok", 0 ) ;
         bob.append( "code", 59 ) ;
         bob.append( "errmsg", err.c_str() ) ;
         bob.append( "err", err.c_str() ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildPingReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob;
         bob.append( "ok", 1 );
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildGetMoreMsg( msgBuffer &out )
      {
         if ( !out.empty() )
         {
            out.zero() ;
         }
         out.reverse( sizeof( MsgOpGetMore ) ) ;
         out.advance( sizeof( MsgOpGetMore ) ) ;

         MsgOpGetMore *getmore = (MsgOpGetMore *)out.data() ;
         getmore->header.messageLength = sizeof( MsgOpGetMore ) ;
         getmore->header.opCode = MSG_BS_GETMORE_REQ ;
         getmore->header.requestID = 0 ;
         getmore->header.routeID.value = 0 ;
         getmore->header.TID = 0 ;
         getmore->contextID = -1 ;
         getmore->numToReturn = -1 ;
      }
   }
}
