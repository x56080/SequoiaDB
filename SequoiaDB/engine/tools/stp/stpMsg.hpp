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

   Source File Name = stpMsg.hpp

   Descriptive Name = Serial Time Protocol messages

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef STP_MSG_HPP__
#define STP_MSG_HPP__

#include "msgDef.hpp"
#include "msg.hpp"
#include "msgMessageFormat.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   // net messages

   /*
      _stpServerReq define
    */
   // send server request to query servers, add server or remove server
   // | header | request type ( STP_SERVER_REQ_TYPE ) |
   struct _stpServerReq
   {
      MsgHeader   header ;
      UINT16      type ;
   } ;

   typedef struct _stpServerReq stpServerReq ;

   /*
      STP_MSG_SERVER_REQ_TYPE define
    */
   enum STP_SERVER_REQ_TYPE
   {
      // query servers
      STP_SERVER_REQ_QUERYSERVERS = 0,
      // add requester into servers
      STP_SERVER_REQ_ADDSERVER,
      // remove requester from servers
      STP_SERVER_REQ_REMOVESERVER
   } ;

   OSS_INLINE const CHAR *stpGetServerReqName( STP_SERVER_REQ_TYPE type )
   {
      switch ( type )
      {
         case STP_SERVER_REQ_QUERYSERVERS :
            return "QUERY" ;
         case STP_SERVER_REQ_ADDSERVER :
            return "ADD" ;
         case STP_SERVER_REQ_REMOVESERVER :
            return "REMOVE" ;
         default :
            break ;
      }
      return "UNKNOWN" ;
   }

   /*
      _stpServerRsp define
    */
   // response for server request
   // | reply header | BSON object for servers |
   struct _stpServerRsp
   {
      MsgInternalReplyHeader reply ;
   } ;

   typedef struct _stpServerRsp stpServerRsp ;

   /*
      _stpRegReq define
    */
   // send a register request to register a synchronize client into a source
   // | header | version of servers from requester | BSON object of requester |
   struct _stpRegReq
   {
      MsgHeader   header ;
      UINT32      version ;
   } ;

   typedef struct _stpRegReq stpRegReq ;

   /*
      _stpRegRsp define
    */
   // response for register request
   // | replay header | real routeID | verified OID | port ( not used yet ) |
   struct _stpRegRsp
   {
      MsgInternalReplyHeader  reply ;
      MsgRouteID              routeID ;
      bson::OID               oid ;
      UINT16                  port ;
   } ;

   typedef struct _stpRegRsp stpRegRsp ;

   /*
      _stpTimeSyncReq define
    */
   // send synchronize time request to synchronize time between client and
   // source
   // | header | flag or status | T1 | T2 | time error |
   struct _stpTimeSyncReq
   {
      MsgHeader   header ;
      UINT32      version ;
      UINT16      flag ;
      UINT16      status ;
      // request send time ( T1 )
      UINT64      sendTimeSec ;
      UINT64      sendTimeNanoSec ;
      // request receive time ( T2 )
      UINT64      receiveTimeSec ;
      UINT64      receiveTimeNanoSec ;
      // time error of client
      UINT32      timeError ;
   } ;

   typedef struct _stpTimeSyncReq stpTimeSyncReq ;

   /*
      _stpTimeSyncRsp define
    */
   // response for synchronize time request
   // | reply header | T1 | T2 | T3 | T4 | time errors |
   struct _stpTimeSyncRsp
   {
      MsgInternalReplyHeader  reply ;
      // request send time ( T1 )
      UINT64                  reqSendTimeSec ;
      UINT64                  reqSendTimeNanoSec ;
      // request receive time ( T2 )
      UINT64                  reqReceiveTimeSec ;
      UINT64                  reqReceiveTimeNanoSec ;
      // response send time ( T3 )
      UINT64                  rspSendTimeSec ;
      UINT64                  rspSendTimeNanoSec ;
      // response receive time ( T4 )
      UINT64                  rspReceiveTimeSec ;
      UINT64                  rspReceiveTimeNanoSec ;
      // time error in nanoseconds of client
      UINT32                  reqTimeError ;
      // new time error in nanoseconds of client adjusted by source
      UINT32                  rspTimeError ;
   } ;

   typedef struct _stpTimeSyncRsp stpTimeSyncRsp ;

   /*
      _stpMetaNotify define
    */
   // send notify of synchronize meta to launch secondary server to synchronize
   // meta LSN
   // NOTE: no need to response ( send synchronize meta request instead )
   // | header |
   struct _stpMetaNotify
   {
      MsgHeader header ;
   } ;

   typedef struct _stpMetaNotify stpMetaNotify ;

   /*
      _stpMetaSyncReq define
    */
   // send synchronize meta request to synchronize meta LSN between servers
   // | header |
   struct _stpMetaSyncReq
   {
      MsgHeader header ;
   } ;

   typedef struct _stpMetaSyncReq stpMetaSyncReq ;

   /*
      _stpMetaSyncRsp define
    */
   // response for synchronize meta request
   // NOTE: meta LSN contains time and version
   // | replay header | time | version |
   struct _stpMetaSyncRsp
   {
      MsgInternalReplyHeader  reply ;
      // time of meta LSN ( as offset )
      UINT64                  time ;
      // version of meta LSN ( increase when primary switch )
      UINT32                  version ;
   } ;

   typedef struct _stpMetaSyncRsp stpMetaSyncRsp ;

   // pipe messages

   // prefix for pipe message
   #define STP_PIPE_MSG_PREFIX   "$stp_"
   // test command to test alive of STP
   #define STP_PIPE_MSG_TEST     STP_PIPE_MSG_PREFIX "test"
   // synchronize command to tell STP to launch synchronize time
   #define STP_PIPE_MSG_SYNC     STP_PIPE_MSG_PREFIX "sync"

}

#endif // STP_MSG_HPP__
