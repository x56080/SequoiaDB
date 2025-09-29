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

   Source File Name = coordRemoteConnection.hpp

   Descriptive Name = Coordinator remote connection header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_REMOTE_CONNECTION_HPP__
#define COORD_REMOTE_CONNECTION_HPP__

#include "ossSocket.hpp"
#include "msg.hpp"
#include "pmdDef.hpp"
#include "pmdEDU.hpp"
#include "msgConvertor.hpp"

#define COORD_SDB_CONNECTION_FORCE_TIMEOUT         ( 60 * OSS_ONE_SEC )

namespace engine
{
   class _coordRemoteConnection : public SDBObject
   {
   public:
      _coordRemoteConnection() ;
      ~_coordRemoteConnection() ;

      /**
       * Initialize a remote connection with an existing socket. TPC connection
       * should have been established by the socket.
       * @param socket
       * @return
       */
      INT32 init( ossSocket *socket ) ;

      /**
       * Initialize a remote connection by the specified address.
       * @param host       Target host.
       * @param service    Target service name.
       * @param timeout    Time out for the socket.
       * @return
       *
       * A new socket will be created inside, and will be destroyed when the
       * connection is destroyed.
       */
      INT32 init( const CHAR *host, const CHAR *service,
                  INT32 timeout = OSS_SOCKET_DFT_TIMEOUT ) ;

      BOOLEAN isConnected() const ;

      /**
       * Do authentication on the connection.
       * @param user       User name
       * @param password   Pass word
       * @param cb
       * @return
       */
      INT32 authenticate( const CHAR *user, const CHAR *password,
                          pmdEDUCB *cb ) ;

      /**
       * Send a message to peer by the connection, and get the reply.
       * @param header
       * @param recvEvent Receive event for the reply. If no reply will be sent,
       *        set it to NULL.
       * @param cb
       * @param timeout
       * @param forceTimeout
       * @return
       *
       * The recvEvent contain the reply. Don't forget to release the memory
       * hold by the recvEvent by pmdEduEventRelease.
       */
      INT32 syncSend( MsgHeader *header, pmdEDUEvent &recvEvent, pmdEDUCB *cb,
                      INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                      INT32 forceTimeout = -1 ) ;

      /**
       * Disconnect the connection with peer. A disconnect message will be sent.
       * @param cb
       * @return
       */
      INT32 disconnect( pmdEDUCB *cb ) ;

   private:
      INT32 _doSysInfo( _pmdEDUCB *cb ) ;
      INT32 _doAuth( const CHAR *user, const CHAR *encryptPasswd,
                     pmdEDUCB *cb ) ;

      INT32 _checkProtocolCompatibility( const MsgSysInfoReply &sysInfoReply ) ;

      INT32 _onSendMsg( MsgHeader *origMsg, CHAR *&finalMsg,
                        UINT32 &finalLen ) ;

   private:
      ossSocket        *_socket ;
      BOOLEAN           _newSocket ;
      IMsgConvertor    *_msgConvertor ;
   } ;
   typedef _coordRemoteConnection coordRemoteConnection ;
}
#endif /* COORD_REMOTE_CONNECTION_HPP__ */
