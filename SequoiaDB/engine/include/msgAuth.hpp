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

   Source File Name = msgAuth.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MSGAUTH_HPP_
#define MSGAUTH_HPP_
#include "core.hpp"
#include "msg.h"
#include "authDef.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   /* struct _MsgAuthReplyV0
   {
      MsgInternalReplyHeader header ;
      _MsgAuthReplyV0()
      {
         header.res = 0 ;
         header.header.messageLength = sizeof( _MsgAuthReplyV0 ) ;
         header.header.opCode = MSG_AUTH_VERIFY_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID = 0 ;
         header.header.requestID = 0 ;
      }
   } ; */
   typedef MsgOpReply   MsgAuthReply ;

   /* struct _MsgAuthCrtReplyV0
   {
      MsgInternalReplyHeader header ;
      _MsgAuthCrtReplyV0()
      {
         header.res = 0 ;
         header.header.messageLength = sizeof(_MsgAuthCrtReplyV0 ) ;
         header.header.opCode = MSG_AUTH_CRTUSR_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID = 0 ;
         header.header.requestID = 0 ;
      }
   } ; */
   typedef MsgOpReply   MsgAuthCrtReply ;

   /* struct _MsgAuthDelReplyV0
   {
      MsgInternalReplyHeader header ;
      _MsgAuthDelReplyV0()
      {
         header.res = 0 ;
         header.header.messageLength = sizeof(_MsgAuthDelReplyV0 ) ;
         header.header.opCode = MSG_AUTH_DELUSR_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID = 0 ;
         header.header.requestID = 0 ;
      }
   } ; */
   typedef MsgOpReply   MsgAuthDelReply ;

   INT32 extractAuthMsg( MsgHeader *header, BSONObj &obj ) ;

   INT32 msgBuildAuthMsg( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *username,
                          const CHAR *password,
                          UINT64 reqID ) ;

}

#endif

