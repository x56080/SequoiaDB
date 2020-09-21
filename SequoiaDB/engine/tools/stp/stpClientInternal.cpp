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

   Source File Name = stpClientInternal.hpp

   Descriptive Name = Serial Time Protocol

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

#include "stpClientInternal.hpp"
#include "client.h"
#include "client_internal.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "msgDef.h"
#include "ossUtil.h"
#include "stpToolCommon.hpp"

#if defined( _LINUX ) || defined (_AIX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

using namespace std ;

namespace engine
{

   #define STP_CLIENT_DFT_NETWORK_TIMEOUT ( 10000 )

   /*
      _stpClientInternal implement
    */
   _stpClientInternal::_stpClientInternal()
   : _handle( 0 ),
     _lastErrorCode( SDB_OK )
   {
      _hostName[ 0 ] = '\0' ;
      _serviceName[ 0 ] = '\0' ;
   }

   _stpClientInternal::_stpClientInternal( const CHAR *hostName,
                                           const CHAR *serviceName )
   : _handle( 0 ),
     _lastErrorCode( SDB_OK )
   {
      _setHostName( hostName ) ;
      _setServiceName( serviceName ) ;
   }

   _stpClientInternal::_stpClientInternal( const _stpClientInternal &client )
   : _handle( 0 ),
     _lastErrorCode( SDB_OK )
   {
      _setHostName( client.getHostName() ) ;
      _setServiceName( client.getServiceName() ) ;
   }

   _stpClientInternal::~_stpClientInternal()
   {
      disconnect() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT_SETCONNINFO, "_stpClientInternal::setConnInfo" )
   INT32 _stpClientInternal::setConnInfo( const CHAR *hostName,
                                          const CHAR *serviceName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT_SETCONNINFO ) ;

      // disconnect from old connection first
      disconnect() ;

      PD_CHECK( NULL != hostName && '\0' != hostName[ 0 ],
                SDB_INVALIDARG, error, PDERROR,
                "Failed to connect to STP, host name is empty" ) ;
      PD_CHECK( NULL != serviceName && '\0' != serviceName[ 0 ],
                SDB_INVALIDARG, error, PDERROR,
                "Failed to connect to STP, service name is empty" ) ;

      _setHostName( hostName ) ;
      _setServiceName( serviceName ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT_SETCONNINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT_CONNECT, "_stpClientInternal::connect" )
   INT32 _stpClientInternal::connect()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT_CONNECT ) ;

      rc = _connect() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to connect to STP, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT_CONNECT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT_CONNECT_HOST, "_stpClientInternal::connect" )
   INT32 _stpClientInternal::connect( const CHAR *hostName,
                                      const CHAR *serviceName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT_CONNECT_HOST ) ;

      rc = setConnInfo( hostName, serviceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set connection information, "
                   "rc: %d", rc ) ;

      rc = _connect() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to connect to STP, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT_CONNECT_HOST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT_DISCONNECT, "_stpClientInternal::disconnect" )
   INT32 _stpClientInternal::disconnect()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT_DISCONNECT ) ;

      // check handle
      if ( 0 != _handle )
      {
         sdbDisconnect( (sdbConnectionHandle)_handle ) ;
         sdbReleaseConnection( (sdbConnectionHandle) _handle ) ;
         _handle = 0 ;
      }

      PD_TRACE_EXITRC( SDB__STPCLIENTINT_DISCONNECT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT_RUNCOMMAND, "_stpClientInternal::runCommand" )
   INT32 _stpClientInternal::runCommand( const CHAR *command,
                                         const CHAR *argument,
                                         CHAR **returnBuffer,
                                         INT32 &returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT_RUNCOMMAND ) ;

      string commandString ;

      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to run command, command name is invalid" ) ;

      PD_CHECK( 0 != _handle, SDB_NETWORK, error, PDERROR,
                "Failed to run command [%s] on invalid handle", command ) ;

      // add command prefix
      commandString += CMD_ADMIN_PREFIX ;
      commandString += command ;

      rc = _runCommand( _handle,
                        commandString.c_str(),
                        argument,
                        returnBuffer,
                        returnCode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s] in STP node, "
                   "rc: %d", command, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT_RUNCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__CONNECT, "_stpClientInternal::_connect" )
   INT32 _stpClientInternal::_connect()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__CONNECT ) ;

      SDB_ASSERT( NULL != _hostName && '\0' != _hostName[ 0 ],
                  "host name is invalid" ) ;
      SDB_ASSERT( NULL != _serviceName && '\0' != _serviceName[ 0 ],
                  "service name is invalid" ) ;

      // close old connection before connect
      rc = disconnect() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to disconnect" ) ;

      // connect
      // NOTE: currently we only use default user and password
      rc = sdbConnect( _hostName, _serviceName, STP_USER, STP_USERPASSWD,
                       &_handle ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to connect to %s:%s, rc: %d",
                   _hostName, _serviceName, rc ) ;

      PD_LOG( PDINFO, "Connected to STP [ host: %s, service: %s ]",
              _hostName, _serviceName ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__CONNECT, rc ) ;
      return rc ;

   error:
      disconnect() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__RUNCOMMAND, "_stpClientInternal::_runCommand" )
   INT32 _stpClientInternal::_runCommand( ossValuePtr handle,
                                          const CHAR *command,
                                          const CHAR *options,
                                          CHAR **returnBuffer,
                                          INT32 &returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__RUNCOMMAND ) ;

      SDB_ASSERT( isConnected(), "handle is invalid" ) ;
      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      BOOLEAN extracted = FALSE ;
      INT64 contextID = 0 ;
      sdbConnectionStruct *connection = 0 ;

      returnCode = SDB_OK ;

      // check connection
      PD_CHECK( isConnected(), SDB_NETWORK, error, PDERROR,
                "Failed to run command [%s] on STP node, network is closed",
                command ) ;

      connection = (sdbConnectionStruct *)handle ;

      // build request
      rc = clientBuildQueryMsgCpp( &( connection->_pSendBuffer ),
                                   &( connection->_sendBufferSize ),
                                   command, 0, 0, -1, -1,
                                   options, NULL, NULL, NULL,
                                   connection->_endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build query message for "
                   "command [%s], rc: %d", command, rc ) ;

      // send request and get reply
      rc = _sendAndReceive( handle,
                            ( const MsgHeader* )connection->_pSendBuffer,
                            ( MsgHeader** )&( connection->_pReceiveBuffer ),
                            &( connection->_receiveBufferSize ),
                            connection->_endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build send and receive message "
                   "for command [%s], rc: %d", command, rc ) ;

      // extract reply
      rc = _extractReply( (MsgHeader *)( connection->_pReceiveBuffer ),
                          connection->_receiveBufferSize,
                          contextID,
                          extracted,
                          connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         if ( !extracted )
         {
            PD_LOG( PDERROR, "Failed to extract result from reply "
                    "for command [%s], rc: %d", command, rc ) ;
            goto error ;
         }
         else
         {
            PD_LOG( PDINFO, "Failed to run command [%s] in STP, rc: %d",
                    command, rc ) ;
            returnCode = rc ;
            rc = SDB_OK ;
            goto error ;
         }
      }

      // set return buffer if needed
      if ( NULL != returnBuffer )
      {
         rc = _getReturnBuffer( connection->_pReceiveBuffer,
                                returnBuffer ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get result from reply "
                      "for command [%s], rc: %d", command, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__RUNCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__SENDANDRECEIVE, "_stpClientInternal::_sendAndReceive" )
   INT32 _stpClientInternal::_sendAndReceive( ossValuePtr handle,
                                              const MsgHeader *sendMessage,
                                              MsgHeader **receiveReply,
                                              INT32 *receiveSize,
                                              BOOLEAN endianConvert )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__SENDANDRECEIVE ) ;

      BOOLEAN hasLock = FALSE ;
      sdbConnectionStruct *connection = (sdbConnectionStruct*)handle ;

      PD_CHECK( NULL != connection->_sock, SDB_NOT_CONNECTED, error, PDERROR,
                "Failed to send message, socket is not connected" ) ;
      PD_CHECK( NULL != sendMessage, SDB_INVALIDARG, error, PDERROR,
                "Failed to send message, message is invalid" ) ;

      // lock socket
      ossMutexLock( &( connection->_sockMutex ) ) ;
      hasLock = TRUE ;

      // send message
      rc = _sendMessage( handle, sendMessage, endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send message, rc: %d", rc ) ;

      // receive reply
      rc = _receiveReply( handle, receiveReply, receiveSize,
                          endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to receive reply, rc: %d", rc ) ;

   done:
      if ( hasLock )
      {
         ossMutexUnlock( &( connection->_sockMutex ) ) ;
      }
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__SENDANDRECEIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__SENDMESSAGE, "_stpClientInternal::_sendMessage" )
   INT32 _stpClientInternal::_sendMessage( ossValuePtr handle,
                                           const MsgHeader *message,
                                           BOOLEAN endianConvert )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__SENDMESSAGE ) ;

      SDB_ASSERT( 0 != handle, "handle is invalid" ) ;
      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      INT32 messageSize = 0 ;
      INT32 sentSize = 0 ;
      INT32 remainSize = 0 ;
      CHAR *buffer = (CHAR *)message ;

      sdbConnectionStruct *connection = (sdbConnectionStruct *)handle ;

      PD_CHECK( NULL != connection->_sock, SDB_INVALIDARG, error, PDERROR,
                "Failed to send message, socket is invalid" ) ;

      // convert endian
      ossEndianConvertIf4( message->messageLength, messageSize,
                           endianConvert ) ;

      // send all message
      remainSize = messageSize ;
      while ( remainSize > 0 )
      {
         rc = clientSend( connection->_sock, buffer, remainSize,
                          &sentSize, STP_CLIENT_DFT_NETWORK_TIMEOUT ) ;
         buffer += sentSize ;
         remainSize -= sentSize ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to send message, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__SENDMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__RECEIVEREPLY, "_stpClientInternal::_receiveReply" )
   INT32 _stpClientInternal::_receiveReply( ossValuePtr handle,
                                            MsgHeader **reply,
                                            INT32 *replySize,
                                            BOOLEAN endianConvert )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__RECEIVEREPLY ) ;

      CHAR **buffer = (CHAR **)reply ;
      INT32 receiveSize = 0 ;
      INT32 realSize = 0 ;
      INT32 receivedSize = 0 ;
      INT32 totalReceivedSize = 0 ;

      sdbConnectionStruct *connection = (sdbConnectionStruct *)handle ;

      PD_CHECK( NULL != connection->_sock, SDB_INVALIDARG, error, PDERROR,
                "Failed to receive reply, socket is invalid" ) ;

      while ( TRUE )
      {
         // get length first
         rc = clientRecv( connection->_sock,
                          ( (CHAR *)&receiveSize ) + totalReceivedSize,
                           sizeof( receiveSize ) - totalReceivedSize,
                           &receivedSize,
                           STP_CLIENT_DFT_NETWORK_TIMEOUT ) ;
         totalReceivedSize += receivedSize ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get receive size, rc: %d", rc ) ;

#if defined( _LINUX ) || defined (_AIX)
#if defined (_AIX)
   #define TCP_QUICKACK TCP_NODELAYACK
#endif
         // quick ack
         {
            INT32 i = 1 ;
            setsockopt( clientGetRawSocket( connection->_sock ),
                        IPPROTO_TCP, TCP_QUICKACK, (void *)( &i ),
                        sizeof( i ) ) ;
         }
#endif
         break ;
      }

      ossEndianConvertIf4( receiveSize, realSize, endianConvert ) ;
      rc = _reallocBuffer( buffer, replySize , realSize + 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to realloc buffer, rc: %d", rc ) ;

      // use the original recvLength before convert
      *(INT32 *)( *buffer ) = receiveSize ;
      totalReceivedSize = 0 ;
      receivedSize = 0 ;
      while ( TRUE )
      {
         // get residual message
         rc = clientRecv( connection->_sock,
                          &( *buffer )[ sizeof( realSize ) +
                                        totalReceivedSize ],
                          realSize - sizeof( realSize ) - totalReceivedSize,
                          &receivedSize,
                          STP_CLIENT_DFT_NETWORK_TIMEOUT ) ;
         totalReceivedSize += receivedSize ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get reply message, "
                      "rc: %d", rc ) ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__RECEIVEREPLY, rc ) ;
      return rc ;

   error:
      goto done ;

   }

   INT32 _stpClientInternal::_reallocBuffer( CHAR **buffer,
                                             INT32 *bufferSize,
                                             INT32 newSize )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != buffer, "buffer is invalid" ) ;

      if ( *bufferSize < newSize )
      {
         CHAR *newBuffer = (CHAR *)SDB_OSS_REALLOC( *buffer, newSize ) ;
         PD_CHECK( NULL != newBuffer, SDB_OOM, error, PDERROR,
                   "Failed to realloc buffer with size [%d], rc: %d",
                   newSize, rc) ;
         *buffer = newBuffer ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__EXTRACTREPLY, "_stpClientInternal::_extractReply" )
   INT32 _stpClientInternal::_extractReply( MsgHeader *message,
                                            INT32 messageSize,
                                            INT64 &contextID,
                                            BOOLEAN &extracted,
                                            BOOLEAN endianConvert )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__EXTRACTREPLY ) ;

      INT32 replyFlag = -1 ;
      INT32 numReturned = -1 ;
      INT32 startFrom = -1 ;
      CHAR *buffer = (CHAR *)message ;

      // extract reply
      extracted = FALSE ;
      rc = clientExtractReply( buffer, &replyFlag, &contextID, &startFrom,
                               &numReturned, endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract reply, rc: %d", rc ) ;

      rc = replyFlag ;
      extracted = TRUE ;

      // check return code
      if ( SDB_OK != replyFlag && SDB_DMS_EOC != replyFlag )
      {
         INT32 dataOffset = 0 ;
         INT32 dataSize = 0 ;
         const CHAR *errorBuffer = NULL ;

         dataOffset = ossRoundUpToMultipleX( sizeof( MsgOpReply ), 4 ) ;
         dataSize = message->messageLength - dataOffset ;
         /// save error info
         if ( dataSize > 0 )
         {
            INT32 tmpRC = SDB_OK ;
            errorBuffer = (const CHAR *)message + dataOffset ;
            tmpRC = _extractError( errorBuffer, replyFlag ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDERROR, "Failed to extract error, rc: %d", tmpRC ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__EXTRACTREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__EXTRACTERROR, "_stpClientInternal::_extractError" )
   INT32 _stpClientInternal::_extractError( const CHAR *errorBuffer,
                                            INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__EXTRACTERROR ) ;

      SDB_ASSERT( NULL != errorBuffer, "error buffer is invalid" ) ;

      INT32 bsonRC = BSON_OK ;
      bson errorObject ;
      bson_iterator iter ;
      BOOLEAN errorInited = FALSE ;

      _resetError() ;

      PD_CHECK( NULL != errorBuffer, SDB_INVALIDARG, error, PDERROR,
                "Failed to extract error message, error buffer is invalid" ) ;

      bson_init( &errorObject ) ;
      errorInited = TRUE ;

      bsonRC = bson_init_finished_data( &errorObject, errorBuffer ) ;
      PD_CHECK( BSON_OK == bsonRC, SDB_CORRUPTED_RECORD, error, PDERROR,
                "Failed to initialize BSON object, rc: %d", bsonRC ) ;

      // get return code
      if ( BSON_INT == bson_find( &iter, &errorObject, OP_ERRNOFIELD ) )
      {
         _lastErrorCode = bson_iterator_int( &iter ) ;
      }
      // get error description
      if ( BSON_STRING == bson_find( &iter, &errorObject, OP_ERRDESP_FIELD ) )
      {
         _lastErrorDescription.assign( bson_iterator_string( &iter ) ) ;
      }
      // get error detail
      if ( BSON_STRING == bson_find( &iter, &errorObject, OP_ERR_DETAIL ) )
      {
         _lastErrorDetail.assign( bson_iterator_string( &iter ) ) ;
      }

   done:
      if ( errorInited )
      {
         bson_destroy( &errorObject ) ;
      }
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__EXTRACTERROR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__GETRETBUF, "_stpClientInternal::_getReturnBuffer" )
   INT32 _stpClientInternal::_getReturnBuffer( CHAR *returnMessage,
                                               CHAR **returnBuffer )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTINT__GETRETBUF ) ;

      INT32 offset = ossRoundUpToMultipleX( sizeof( MsgOpReply ), 4 ) ;

      MsgOpReply *reply = (MsgOpReply *)returnMessage ;

      PD_CHECK( NULL != returnBuffer, SDB_INVALIDARG, error, PDERROR,
                "Failed to get return buffer, rc: %d", rc ) ;
      PD_CHECK( offset < reply->header.messageLength,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize return buffer, "
                "size is unexpected, expected [%llu], given [%llu], "
                "rc: %d", offset, reply->header.messageLength, rc ) ;

      *returnBuffer = &returnMessage[ offset ] ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTINT__GETRETBUF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTINT__RESETERROR, "_stpClientInternal::_resetError" )
   void _stpClientInternal::_resetError()
   {
      PD_TRACE_ENTRY( SDB__STPCLIENTINT__RESETERROR ) ;

      _lastErrorCode = SDB_OK ;
      _lastErrorDescription.clear() ;
      _lastErrorDetail.clear() ;

      PD_TRACE_EXIT( SDB__STPCLIENTINT__RESETERROR ) ;
   }

}
