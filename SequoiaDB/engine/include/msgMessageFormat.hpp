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

   Source File Name = msgMessageFormat.hpp

   Descriptive Name = Message Client Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Messaging component. This file contains message structure for
   client-server communication.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/10/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MSGMESSAGE_FORMAT_HPP_
#define MSGMESSAGE_FORMAT_HPP_

#include "msg.hpp"
#include "netDef.hpp"
#include "../bson/bson.h"
#include "../bson/oid.h"
#include <string>

using namespace bson;
using namespace std;

const CHAR* serviceID2String( UINT32 serviceID ) ;

string routeID2String( MsgRouteID routeID ) ;
string routeID2String( UINT64 nodeID ) ;

///  get the msg type string desp
const CHAR* msgType2String( MSG_TYPE msgType, BOOLEAN isCommand = FALSE ) ;

/*
   Message to string functions
*/
#define MSG_HEADER_MASK_LEN            0x00000001
#define MSG_HEADER_MASK_OPCODE         0x00000002
#define MSG_HEADER_MASK_TID            0x00000004
#define MSG_HEADER_MASK_ROUTEID        0x00000008
#define MSG_HEADER_MASK_REQID          0x00000010

#define MSG_MASK_ALL                   0xFFFFFFFF

string  msg2String( const MsgHeader *pMsg,
                    UINT32 headMask = MSG_MASK_ALL,
                    UINT32 expandMask = MSG_MASK_ALL ) ;

/*
   Message expand to string functions
*/
typedef void (*EXPAND_MSG_2_STRING_FUNC)( stringstream &ss,
                                          const MsgHeader *pMsg,
                                          UINT32 expandMask ) ;

void msgExpandComSessionInit2String( stringstream &ss,
                                     const MsgHeader *pMsg,
                                     UINT32 expandMask ) ;

#define MSG_EXP_MASK_CLNAME            0x00000001
#define MSG_EXP_MASK_MATCHER           0x00000002
#define MSG_EXP_MASK_SELECTOR          0x00000004
#define MSG_EXP_MASK_ORDERBY           0x00000008
#define MSG_EXP_MASK_HINT              0x00000010
#define MSG_EXP_MASK_OTHER             0x00000020
void msgExpandBSQuery2String( stringstream &ss,
                              const MsgHeader *pMsg,
                              UINT32 expandMask ) ;

#endif // MSGMESSAGE_FORMAT_HPP_

