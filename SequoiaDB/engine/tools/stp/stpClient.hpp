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

   Source File Name = stpClient.hpp

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

#ifndef STP_CLIENT_HPP__
#define STP_CLIENT_HPP__

#include "oss.hpp"
#include "ossSocket.hpp"
#include "stpClientInternal.hpp"
#include "stpMetaData.hpp"
#include "stpOptions.hpp"
#include "stpNode.hpp"
#include "dpsLogDef.hpp"
#include "../bson/bson.h"

namespace engine
{

   /*
      _stpClient define
    */
   // client to send commands and handle reply with STP nodes
   // command and reply are in BSON format
   class _stpClient : public stpClientInternal
   {
   public:
      _stpClient() ;
      _stpClient( const CHAR *hostName, const CHAR *serviceName ) ;
      _stpClient( const _stpClient &client ) ;
      virtual ~_stpClient() ;

   public:
      // run command on STP node to get back BSON object
      // input:
      // - command: command to be executed
      // - argument: argument to be executed in BSON format
      // output:
      // - returnObject: result of command in BSON format
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 runCommand( const CHAR *command,
                        const bson::BSONObj &argument,
                        bson::BSONObj &returnObject ) ;

      // run command on STP node to get back BSON object and return code
      // input:
      // - command: command to be executed
      // - argument: argument to be executed in BSON format
      // output:
      // - returnObject: result of command in BSON format
      // - returnCode: return code of command
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 runCommand( const CHAR *command,
                        const bson::BSONObj &argument,
                        bson::BSONObj &returnObject,
                        INT32 &returnCode ) ;

      // get configure options from STP
      // output:
      // - options: configure options of STP
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 getConf( stpOptions &options ) ;

      // get meta data from STP
      // output:
      // - shmKey: key of shared memory to get meta data of STP
      // - metaData: current meta data of STP
      // - metaLSN: current LSN of STP
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 getMetaData( std::string &shmKey,
                         stpMetaData &metaData,
                         DPS_LSN &metaLSN ) ;

      // get logical time in nanoseconds from STP
      // output:
      // - logicalTime: current logical time in nanoseconds
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 getTime( stpLogicalTimeNS &logicalTime ) ;

      // get logical time in microseconds from STP
      // output:
      // - logicalTime: current logical time in microseconds
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 getTimeUS( stpLogicalTimeUS &logicalTime ) ;

      // get server information from STP
      // output:
      // - version: version of servers
      // - serverList: list of servers
      // - primaryNode: primary node of servers
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 getServers( UINT32 &version,
                        STP_SERVER_LIST &serverList,
                        stpServerNode &primaryNode ) ;

      // get synchronizing clients from STP
      // output:
      // - sourceNode: current source node ( primary node )
      // - clientList: list of synchronize clients
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      // NOTE: will redirect to primary node
      INT32 getSyncClients( stpSourceNode &sourceNode,
                            STP_CLIENT_LIST &clientList ) ;

      // get synchronize status
      // output:
      // - role: role of STP
      // - isPrimary: indicates if STP is primary
      // - sourceNode: current source node to synchronize, and with
      //               synchronize status
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 getSyncStatus( STP_ROLE &role,
                           BOOLEAN &isPrimary,
                           STP_SYNC_STATUS &status,
                           stpSourceNode &sourceNode ) ;

      // get synchronize history
      // output:
      // - sourceList: list of history source nodes with synchronize
      //               statistics
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 getSyncHistory( STP_SOURCE_LIST &sourceList ) ;

      // reelect STP servers
      // input:
      // - timeout: timeout of reelection
      // - targetHost: host name of new primary
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 reelect( UINT32 timeout = STP_REELECT_DFT_TIMEOUT,
                     const CHAR *targetHost = NULL ) ;

      // convert real time to logical time in simple mode
      // input:
      // - realTime: real time in local time zone
      // output:
      // - logicalTime: logical time in microseconds
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 convRealTimeToLogicalTime( const ossTimestamp &realTime,
                                       UINT64 &logicalTime ) ;

      // convert logical time to real time in simple mode
      // input:
      // - logicalTime: logical time in microseconds
      // output:
      // - realTime: real time in local time zone
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 convLogicalTimeToRealTime( UINT64 logicalTime,
                                       ossTimestamp &realTime ) ;

      // convert real time to logical time in nanoseconds
      // input:
      // - realTime: real time in nanoseconds
      // output:
      // - logicalTime: logical time in nanoseconds
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 convRealTimeToLogicalTime( const stpHPTime &realTime,
                                       stpHPTime &logicalTime ) ;

      // convert logical time in nanoseconds to real time
      // input:
      // - logicalTime: logical time in nanoseconds
      // output:
      // - realTime: real time in nanoseconds
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 convLogicalTimeToRealTime( const stpHPTime &logicalTime,
                                       stpHPTime &realTime ) ;

   protected:
      // helper to convert times
      // convert times between logical time and real time
      // input:
      // - fromTime: time to convert from ( in nanoseconds )
      // - isRTimeToLTime: convert direction from real time to logical time
      //                   or reverse
      // output:
      // - toTime: time to convert to ( in nanoseconds )
      INT32 _convTime( const stpHPTime &fromTime,
                       stpHPTime &toTime,
                       BOOLEAN isRTimeToLTime ) ;
   } ;

   typedef class _stpClient stpClient ;

}

#endif // STP_CLIENT_HPP__
