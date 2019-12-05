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

   Source File Name = msgTp.hpp

   Descriptive Name = SequoiaDB Time Protocol Service Message

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MSG_TP_HPP__
#define MSG_TP_HPP__

#include "msgDef.hpp"
#include "msg.hpp"
#include "msgMessageFormat.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      MSG_TP_SERVER_REQ_TYPE define
    */
   enum MSG_TP_SERVER_REQ_TYPE
   {
      // query servers
      MSG_TP_QUERY_SERVER = 0,
      // add requester into servers
      MSG_TP_ADD_SERVER,
      // remove requester from servers
      MSG_TP_REMOVE_SERVER
   } ;

   OSS_INLINE const CHAR *msgGetTpServerReqName( MSG_TP_SERVER_REQ_TYPE type )
   {
      switch ( type )
      {
         case MSG_TP_QUERY_SERVER :
            return "QUERY" ;
         case MSG_TP_ADD_SERVER :
            return "ADD" ;
         case MSG_TP_REMOVE_SERVER :
            return "REMOVE" ;
         default :
            break ;
      }
      return "UNKNOWN" ;
   }

   /*
      _MsgTpServerReq define
    */
   struct _MsgTpServerReq
   {
      MsgHeader   header ;
      // MSG_TP_SERVER_REQ_TYPE
      UINT16      type ;
   } ;

   typedef struct _MsgTpServerReq MsgTpServerReq ;

   /*
      _MsgTpServerRsp define
    */
   struct _MsgTpServerRsp
   {
      MsgInternalReplyHeader reply ;
   } ;

   typedef struct _MsgTpServerRsp MsgTpServerRsp ;

   /*
      _MsgTpRegReq define
    */
   struct _MsgTpRegReq
   {
      MsgHeader   header ;
      UINT32      version ;
      UINT32      role ;
      UINT32      syncInterval ;
      UINT32      maxTimeError ;
      UINT32      timeError ;
      bson::OID   oid ;
   } ;

   typedef struct _MsgTpRegReq MsgTpRegReq ;

   /*
      _MsgTpRegRsp define
    */
   struct _MsgTpRegRsp
   {
      MsgInternalReplyHeader  reply ;
      bson::OID               oid ;
   } ;

   typedef struct _MsgTpRegRsp MsgTpRegRsp ;

   /*
      _MsgTpTimeSyncReq define
    */
   struct _MsgTpTimeSyncReq
   {
      MsgHeader   header ;
      UINT32      version ;
      UINT16      flag ;
      UINT16      status ;
      UINT64      sendTimeSec ;
      UINT64      sendTimeNanoSec ;
      UINT64      receiveTimeSec ;
      UINT64      receiveTimeNanoSec ;
      UINT32      timeError ;
   } ;

   typedef struct _MsgTpTimeSyncReq MsgTpTimeSyncReq ;

   /*
      _MsgTpSyncTimeRsp define
    */
   struct _MsgTpTimeSyncRsp
   {
      MsgInternalReplyHeader  reply ;
      UINT64                  reqSendTimeSec ;
      UINT64                  reqSendTimeNanoSec ;
      UINT64                  reqReceiveTimeSec ;
      UINT64                  reqReceiveTimeNanoSec ;
      UINT64                  rspSendTimeSec ;
      UINT64                  rspSendTimeNanoSec ;
      UINT64                  rspReceiveTimeSec ;
      UINT64                  rspReceiveTimeNanoSec ;
      UINT32                  reqTimeError ;
      UINT32                  rspTimeError ;
   } ;

   typedef struct _MsgTpTimeSyncRsp MsgTpTimeSyncRsp ;

   /*
      _MsgTpMetaNotify define
    */
   struct _MsgTpMetaNotify
   {
      MsgHeader header ;
   } ;

   typedef struct _MsgTpMetaNotify MsgTpMetaNotify ;

   /*
      _MsgTpMetaSyncReq define
    */
   struct _MsgTpMetaSyncReq
   {
      MsgHeader header ;
   } ;

   typedef struct _MsgTpMetaSyncReq MsgTpMetaSyncReq ;

   /*
      _MsgTpMetaSyncRsp define
    */
   struct _MsgTpMetaSyncRsp
   {
      MsgInternalReplyHeader  reply ;
      UINT64                  time ;
      UINT32                  version ;
   } ;

   typedef struct _MsgTpMetaSyncRsp MsgTpMetaSyncRsp ;

   // pipe message
   #define TP_PIPE_MSG_PREFIX         "$tps_"
   #define TP_PIPE_MSG_TEST           TP_PIPE_MSG_PREFIX "test"
   #define TP_PIPE_MSG_SYNC           TP_PIPE_MSG_PREFIX "sync"

}

#endif // MSG_TP_HPP__
