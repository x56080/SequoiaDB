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

#ifndef STP_CLIENT_INTERNAL_HPP__
#define STP_CLIENT_INTERNAL_HPP__

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossSocket.hpp"
#include "msg.hpp"
#include <string>

namespace engine
{

   /*
      _stpClientInternal define
    */
   // internal client to send commands and handle reply with STP nodes
   class _stpClientInternal : public SDBObject
   {
   public:
      // constructor and destructor
      _stpClientInternal() ;
      _stpClientInternal( const CHAR *hostName, const CHAR *serviceName ) ;
      _stpClientInternal( const _stpClientInternal &client ) ;
      virtual ~_stpClientInternal() ;

   public:
      // check if client is connected
      OSS_INLINE BOOLEAN isConnected()
      {
         return ( 0 != _handle ) ;
      }

      // get host name
      OSS_INLINE const CHAR *getHostName() const
      {
         return _hostName ;
      }

      // get service name
      OSS_INLINE const CHAR *getServiceName() const
      {
         return _serviceName ;
      }

      // set connection information of STP
      // input:
      // - hostName: host name of STP node
      // - serviceName: service name of STP ( port )
      // return:
      // - SDB_OK: succeed to connect
      // - other error code: failed to connect
      INT32 setConnInfo( const CHAR *hostName, const CHAR *serviceName ) ;

      // connect to STP
      // return:
      // - SDB_OK: succeed to connect
      // - other error code: failed to connect
      INT32 connect() ;

      // connect to STP
      // input:
      // - hostName: host name of STP node
      // - serviceName: service name of STP ( port )
      // return:
      // - SDB_OK: succeed to connect
      // - other error code: failed to connect
      INT32 connect( const CHAR *hostName, const CHAR *serviceName ) ;

      // disconnect from STP
      // return:
      // - SDB_OK: succeed to disconnect
      // - other error code: failed to disconnect
      INT32 disconnect() ;

      // run command on STP node
      // input:
      // - command: command to be executed
      // - argument: argument to be executed ( generally, it is in BSON format )
      // output:
      // - returnBuffer: result of command ( generally, it is in BSON format )
      // - returnCode: return code of command
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 runCommand( const CHAR *command,
                        const CHAR *argument,
                        CHAR **returnBuffer,
                        INT32 &returnCode ) ;

   protected:
      // connect to STP
      INT32 _connect() ;

      // run command
      INT32 _runCommand( ossValuePtr handle,
                         const CHAR *command,
                         const CHAR *options,
                         CHAR **returnBuffer,
                         INT32 &returnCode ) ;

      // send request and receive reply
      INT32 _sendAndReceive( ossValuePtr handle,
                             const MsgHeader *sendMessage,
                             MsgHeader **receiveReply,
                             INT32 *receiveSize,
                             BOOLEAN endianConvert ) ;

      // send request
      INT32 _sendMessage( ossValuePtr handle,
                          const MsgHeader *message,
                          BOOLEAN endianConvert ) ;

      // receive reply
      INT32 _receiveReply( ossValuePtr handle,
                           MsgHeader **reply,
                           INT32 *replySize,
                           BOOLEAN endianConvert ) ;

      // realloc buffer
      INT32 _reallocBuffer( CHAR **buffer,
                            INT32 *bufferSize,
                            INT32 newSize ) ;

      // extract reply
      INT32 _extractReply( MsgHeader *message,
                           INT32 messageSize,
                           INT64 &contextID,
                           BOOLEAN &extracted,
                           BOOLEAN endianConvert ) ;

      // extract error from return buffer
      INT32 _extractError( const CHAR *errorBuffer,
                           INT32 returnCode ) ;

      // get return buffer
      INT32 _getReturnBuffer( CHAR *returnMessage,
                              CHAR **returnBuffer ) ;

      // reset error information
      void _resetError() ;

      // set host name
      void _setHostName( const CHAR *hostName )
      {
         if ( NULL != hostName && '\0' != hostName[ 0 ] )
         {
            ossStrncpy( _hostName, hostName, OSS_MAX_HOSTNAME ) ;
         }
         else
         {
            _hostName[ 0 ] = '\0' ;
         }
      }

      // set service name
      void _setServiceName( const CHAR *serviceName )
      {
         if ( NULL != serviceName && '\0' != serviceName[ 0 ] )
         {
            ossStrncpy( _serviceName, serviceName, OSS_MAX_SERVICENAME ) ;
         }
         else
         {
            _serviceName[ 0 ] = '\0' ;
         }
      }

   protected:
      // handle of connection
      ossValuePtr _handle ;
      // error information: return code of the last error
      INT32       _lastErrorCode ;
      // error information: description of the last error
      std::string _lastErrorDescription ;
      // error information: detail of the last error
      std::string _lastErrorDetail ;

      // host name
      CHAR _hostName[ OSS_MAX_HOSTNAME + 1 ] ;

      // service name
      CHAR _serviceName[ OSS_MAX_SERVICENAME + 1 ] ;
   } ;

   typedef class _stpClientInternal stpClientInternal ;

}

#endif // STP_CLIENT_HPP__
