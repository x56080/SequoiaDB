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

#include "stpClient.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _stpClient implement
    */
   _stpClient::_stpClient()
   : _stpClientInternal()
   {
   }

   _stpClient::_stpClient( const CHAR *hostName, const CHAR *serviceName )
   : _stpClientInternal( hostName, serviceName )
   {
   }

   _stpClient::_stpClient( const _stpClient &client )
   : _stpClientInternal( client )
   {
   }


   _stpClient::~_stpClient()
   {
      // _stpClientInternal will do the disconnect
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_RUNCOMMAND, "_stpClient::runCommand" )
   INT32 _stpClient::runCommand( const CHAR *command,
                                 const BSONObj &argument,
                                 BSONObj &returnObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_RUNCOMMAND ) ;

      INT32 returnCode = SDB_OK ;
      rc = runCommand( command, argument, returnObject, returnCode ) ;
      if ( SDB_OK == rc && SDB_OK != returnCode )
      {
         rc = returnCode ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   command, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_RUNCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_RUNCOMMAND_RC, "_stpClient::runCommand" )
   INT32 _stpClient::runCommand( const CHAR *command,
                                 const BSONObj &argument,
                                 BSONObj &returnObject,
                                 INT32 &returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_RUNCOMMAND_RC ) ;

      CHAR *returnBuffer = NULL ;

      // check if connected, try connect to STP is not connected
      if ( !isConnected() )
      {
         rc = connect() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to connect to STP, rc: %d", rc ) ;
      }

      // run command
      rc = stpClientInternal::runCommand( command,
                                          argument.objdata(),
                                          &returnBuffer,
                                          returnCode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s] in STP node, "
                   "rc: %d", command, rc ) ;

      // check if an error happened
      if ( SDB_OK == returnCode && NULL != returnBuffer )
      {
         try
         {
            returnObject = BSONObj( returnBuffer ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse BSON object from result, "
                    "error: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_RUNCOMMAND_RC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETCONF, "_stpClient::getConf" )
   INT32 _stpClient::getConf( stpOptions &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETCONF ) ;

      BSONObj argument, result, errorResult ;
      pmdCfgRecord::controlParams cp( TRUE ) ;

      // run command
      rc = runCommand( CMD_NAME_STP_GET_CONFIG, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_CONFIG, rc ) ;
                   
      // parse config from BSON
      rc = options.update( result, FALSE,  cp, errorResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update STP options, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETCONF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETMETA, "_stpClient::getMetaData" )
   INT32 _stpClient::getMetaData( string &shmKey,
                                  stpMetaData &metaData,
                                  DPS_LSN &metaLSN )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETMETA ) ;

      BSONObj argument, result ;

      // run command
      rc = runCommand( CMD_NAME_STP_GET_META, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_META, rc ) ;

      // parse BSON to get meta
      try
      {
         BSONElement subElement ;

         // get shared memory key
         subElement = result.getField( STP_FIELD_NAME_META_SHMKEY ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_META_SHMKEY ) ;
         PD_CHECK( String == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not a string",
                   STP_FIELD_NAME_META_SHMKEY ) ;
         shmKey = subElement.str() ;

         // get meta data
         subElement = result.getField( STP_FIELD_NAME_META_DATA ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_META_DATA ) ;
         PD_CHECK( Object == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an object",
                   STP_FIELD_NAME_META_DATA ) ;

         // parse meta data
         rc = metaData.fromBSON( subElement.embeddedObject() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse meta data from BSON, "
                      "rc: %d", rc ) ;

         // get meta LSN
         subElement = result.getField( STP_FIELD_NAME_META_LSN ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_META_LSN ) ;
         PD_CHECK( Object == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an object",
                   STP_FIELD_NAME_META_LSN ) ;

         {
            // parse meta LSN
            BSONObj lsnObject = subElement.embeddedObject() ;
            BSONElement lsnElement ;

            // get offset
            lsnElement = lsnObject.getField( STP_FIELD_NAME_META_OFFSET ) ;
            PD_CHECK( EOO != lsnElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_META_OFFSET ) ;
            PD_CHECK( NumberLong == lsnElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, "
                      "it is not a long number", STP_FIELD_NAME_META_OFFSET ) ;
            metaLSN.offset = (DPS_LSN_OFFSET)( lsnElement.numberLong() ) ;

            // get version
            lsnElement = lsnObject.getField( STP_FIELD_NAME_META_VERSION ) ;
            PD_CHECK( EOO != lsnElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_META_VERSION ) ;
            PD_CHECK( NumberInt == lsnElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, "
                      "it is not an integer", STP_FIELD_NAME_META_VERSION ) ;
            metaLSN.version = (DPS_LSN_VER)( lsnElement.numberInt() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for meta, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETMETA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETTIME, "_stpClient::getTime" )
   INT32 _stpClient::getTime( stpLogicalTimeNS &logicalTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETTIME ) ;

      BSONObj argument, result ;

      // run command
      rc = runCommand( CMD_NAME_STP_GET_TIME, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_TIME, rc ) ;

      // parse time from BSON
      rc = logicalTime.fromBSON( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for logical time, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETTIMEUS, "_stpClient::getTimeUS" )
   INT32 _stpClient::getTimeUS( stpLogicalTimeUS &logicalTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETTIMEUS ) ;

      BSONObj argument, result ;

      // build argument
      try
      {
         argument = BSON( STP_FIELD_NAME_TYPE <<
                          STP_FORMAT_NAME_LOGICAL_TIME_US ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to generate argument, occur exception: %s",
                 e.what() ) ;
         rc = SDB_OK ;
         goto error ;
      }

      // run command
      rc = runCommand( CMD_NAME_STP_GET_TIME, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_TIME, rc ) ;

      // parse time from BSON
      rc = logicalTime.fromBSON( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for logical time "
                   "in microsecond, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETTIMEUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETSERVERS, "_stpClient::getServers" )
   INT32 _stpClient::getServers( UINT32 &version,
                                 STP_SERVER_LIST &serverList,
                                 stpServerNode &primaryNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETSERVERS ) ;

      BSONObj argument, result ;

      // run command
      rc = runCommand( CMD_NAME_STP_GET_SERVERS, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_SERVERS, rc ) ;

      // parse servers from BSON
      try
      {
         BSONElement subElement ;

         // get version
         subElement = result.getField( STP_FIELD_NAME_VERSION ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_VERSION ) ;
         PD_CHECK( NumberInt == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an integer",
                   STP_FIELD_NAME_VERSION ) ;
         version = (UINT32)( subElement.numberInt() ) ;

         // get server group
         subElement = result.getField( STP_FIELD_NAME_GROUP ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_GROUP ) ;
         PD_CHECK( Array == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an array",
                   STP_FIELD_NAME_GROUP ) ;

         {
            // parse servers
            BSONObj serverInfo = subElement.embeddedObject() ;
            BSONObjIterator serverEleIter( serverInfo ) ;
            while ( serverEleIter.more() )
            {
               stpServerNode server ;
               BSONElement serverElement = serverEleIter.next() ;
               PD_CHECK( Object == serverElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get server from BSON, "
                         "it is not an object" ) ;
               rc = server.fromBSON( serverElement.embeddedObject(), TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for server, "
                            "rc: %d", rc ) ;
               serverList.push_back( server ) ;
            }
         }

         // parse primary
         subElement = result.getField( STP_FIELD_NAME_PRIMARY ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_PRIMARY ) ;
         PD_CHECK( Object == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an object",
                   STP_FIELD_NAME_PRIMARY ) ;

         {
            // parse primary node
            BSONObj primaryObject = subElement.embeddedObject() ;
            BSONElement primaryElement ;

            // get host name
            primaryElement = primaryObject.getField( STP_FIELD_NAME_HOST ) ;
            PD_CHECK( EOO != primaryElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_HOST ) ;
            PD_CHECK( String == primaryElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is not a string",
                      STP_FIELD_NAME_HOST ) ;
            primaryNode.setHostName( primaryElement.valuestr() ) ;

            // get service name
            primaryElement = primaryObject.getField( STP_FIELD_NAME_SERVICE ) ;
            PD_CHECK( EOO != primaryElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_SERVICE ) ;
            PD_CHECK( String == primaryElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is not a string",
                      STP_FIELD_NAME_SERVICE ) ;
            primaryNode.setServiceName( primaryElement.valuestr() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for servers, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETSYNCCLIENTS, "_stpClient::getSyncClients" )
   INT32 _stpClient::getSyncClients( stpSourceNode &sourceNode,
                                     STP_CLIENT_LIST &clientList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETSYNCCLIENTS ) ;

      BSONObj argument, result ;

      // run command
      rc = runCommand( CMD_NAME_STP_GET_SYNC_CLIENTS, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_SYNC_CLIENTS, rc ) ;

      // parse servers from BSON
      try
      {
         BSONElement subElement ;

         subElement = result.getField( STP_FIELD_NAME_SYNC_SOURCE ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_SYNC_SOURCE ) ;
         PD_CHECK( Object == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an object",
                   STP_FIELD_NAME_SYNC_SOURCE ) ;

         {
            // parse source
            BSONObj sourceObj = subElement.embeddedObject() ;
            BSONElement sourceElement ;

            sourceElement = sourceObj.getField( STP_FIELD_NAME_HOST ) ;
            PD_CHECK( EOO != sourceElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_HOST ) ;
            PD_CHECK( String == sourceElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is not a string",
                      STP_FIELD_NAME_HOST ) ;
            sourceNode.setHostName( sourceElement.valuestr() ) ;

            sourceElement = sourceObj.getField( STP_FIELD_NAME_SERVICE ) ;
            PD_CHECK( EOO != sourceElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_SERVICE ) ;
            PD_CHECK( String == sourceElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is not a string",
                      STP_FIELD_NAME_SERVICE ) ;
            sourceNode.setServiceName( sourceElement.valuestr() ) ;
         }

         // get clients
         subElement = result.getField( STP_FIELD_NAME_SYNC_CLIENTS ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_SYNC_CLIENTS ) ;
         PD_CHECK( Array == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an array",
                   STP_FIELD_NAME_SYNC_CLIENTS ) ;
         {
            // parse clients
            BSONObj clientInfo = subElement.embeddedObject() ;
            BSONObjIterator clientEleIter( clientInfo ) ;
            while ( clientEleIter.more() )
            {
               stpClientNode client ;
               BSONElement clientElement = clientEleIter.next() ;
               PD_CHECK( Object == clientElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get client from BSON, "
                         "it is not an object" ) ;
               rc = client.fromBSON( clientElement.embeddedObject(), TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for client, "
                            "rc: %d", rc ) ;
               clientList.push_back( client ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for synchronize clients, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETSYNCCLIENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETSYNCSTATUS, "_stpClient::getSyncStatus" )
   INT32 _stpClient::getSyncStatus( STP_ROLE &role,
                                    BOOLEAN &isPrimary,
                                    STP_SYNC_STATUS &status,
                                    stpSourceNode &sourceNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETSYNCSTATUS ) ;

      BSONObj argument, result ;

      // run command
      rc = runCommand( CMD_NAME_STP_GET_SYNC_STATUS, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_SYNC_STATUS, rc ) ;

      // parse source from BSON
      try
      {
         BSONElement subElement ;
         const CHAR *roleName = NULL ;

         // parse role
         subElement = result.getField( STP_FIELD_NAME_ROLE ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_ROLE ) ;
         PD_CHECK( String == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not a string",
                   STP_FIELD_NAME_ROLE ) ;
         roleName = subElement.valuestr() ;
         role = stpGetRoleByName( roleName ) ;

         // parse primary
         subElement = result.getField( STP_FIELD_NAME_IS_PRIMARY ) ;
         PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_IS_PRIMARY ) ;
         PD_CHECK( Bool == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not a boolean",
                   STP_FIELD_NAME_IS_PRIMARY ) ;
         isPrimary = subElement.boolean() ;

         if ( isPrimary )
         {
            status = STP_SYNC_NOSOURCE ;
         }
         else
         {
            BSONObj subObject ;
            const CHAR *statusName = NULL ;

            subElement = result.getField( STP_FIELD_NAME_SYNC_STATUS ) ;
            PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_SYNC_STATUS ) ;
            PD_CHECK( String == subElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is not a string",
                      STP_FIELD_NAME_SYNC_STATUS ) ;
            statusName = subElement.valuestr() ;
            status = stpGetSyncStatusByName( statusName ) ;

            // get source
            subElement = result.getField( STP_FIELD_NAME_SYNC_SOURCE ) ;
            PD_CHECK( EOO != subElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is empty",
                      STP_FIELD_NAME_SYNC_SOURCE ) ;
            PD_CHECK( Object == subElement.type(), SDB_SYS, error, PDERROR,
                      "Failed to get [%s] field from BSON, it is not an array",
                      STP_FIELD_NAME_SYNC_SOURCE ) ;
            subObject = subElement.embeddedObject() ;
            if ( subObject.nFields() > 0 )
            {
               rc = sourceNode.fromBSON( subObject, TRUE, TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for source, "
                            "rc: %d", rc ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for synchronize status, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETSYNCSTATUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_GETSYNCHIST, "_stpClient::getSyncHistory" )
   INT32 _stpClient::getSyncHistory( STP_SOURCE_LIST &sourceList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_GETSYNCHIST ) ;

      BSONObj argument, result ;

      // run command
      rc = runCommand( CMD_NAME_STP_GET_SYNC_HISTORY, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_GET_SYNC_HISTORY, rc ) ;

      // parse history from BSON
      try
      {
         BSONElement element ;

         element = result.getField( STP_FIELD_NAME_SYNC_SOURCES ) ;
         PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is empty",
                   STP_FIELD_NAME_SYNC_SOURCE ) ;
         PD_CHECK( Array == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get [%s] field from BSON, it is not an array",
                   STP_FIELD_NAME_SYNC_SOURCE ) ;
         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               stpSourceNode source ;
               BSONElement sourceElement = iter.next() ;

               PD_CHECK( Object == sourceElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get source from BSON, "
                         "it is not an object" ) ;

               rc = source.fromBSON( sourceElement.embeddedObject(),
                                     FALSE, TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for source, "
                            "rc: %d", rc ) ;

               sourceList.push_back( source ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for synchronize history, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_GETSYNCHIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_REELECT, "_stpClient::reelect" )
   INT32 _stpClient::reelect( UINT32 timeout,
                              const CHAR *targetHost )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_REELECT ) ;

      BSONObj argument, result ;

      // build argument
      try
      {
         BSONObjBuilder builder ;
         if ( STP_REELECT_DFT_TIMEOUT != timeout )
         {
            builder.append( STP_FIELD_NAME_TYPE, timeout ) ;
         }
         if ( NULL != targetHost )
         {
            builder.append( STP_FIELD_NAME_HOST, targetHost ) ;
         }
         argument = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to generate argument, occur exception: %s",
                 e.what() ) ;
         rc = SDB_OK ;
         goto error ;
      }

      // run command
      rc = runCommand( CMD_NAME_STP_REELECT, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_REELECT, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_REELECT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_CONVREALTOLOGICAL_SIM, "_stpClient::convRealTimeToLogicalTime" )
   INT32 _stpClient::convRealTimeToLogicalTime( const ossTimestamp &realTime,
                                                UINT64 &logicalTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_CONVREALTOLOGICAL_SIM ) ;

      stpHPTime realHPTime, logicalHPTime ;

      realHPTime.fromMicroSecond( STP_SEC_TO_MICROSEC( realTime.time ) +
                                  realTime.microtm ) ;

      rc = convRealTimeToLogicalTime( realHPTime, logicalHPTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to convert real time to logical time, "
                   "rc: %d", rc ) ;

      logicalTime = logicalHPTime.toMicroSecond() ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_CONVREALTOLOGICAL_SIM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_CONVLOGICALTOREAL_SIM, "_stpClient::convLogicalTimeToRealTime" )
   INT32 _stpClient::convLogicalTimeToRealTime( UINT64 logicalTime,
                                                ossTimestamp &realTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_CONVLOGICALTOREAL_SIM ) ;

      stpHPTime realHPTime, logicalHPTime ;

      logicalHPTime.fromMicroSecond( logicalTime ) ;

      rc = convLogicalTimeToRealTime( logicalHPTime, realHPTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to convert logical time to real time, "
                   "rc: %d", rc ) ;

      realTime.time = realHPTime.getSecond() ;
      realTime.microtm =
            STP_NANOSEC_TO_MICROSEC( realHPTime.getNanoSecond() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_CONVLOGICALTOREAL_SIM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_CONVREALTOLOGICAL, "_stpClient::convRealTimeToLogicalTime" )
   INT32 _stpClient::convRealTimeToLogicalTime( const stpHPTime &realTime,
                                                stpHPTime &logicalTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_CONVREALTOLOGICAL ) ;

      rc = _convTime( realTime, logicalTime, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to convert logical time to "
                   "real time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_CONVREALTOLOGICAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_CONVLOGICALTOREAL, "_stpClient::convLogicalTimeToRealTime" )
   INT32 _stpClient::convLogicalTimeToRealTime( const stpHPTime &logicalTime,
                                                stpHPTime &realTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_CONVLOGICALTOREAL ) ;

      rc = _convTime( logicalTime, realTime, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to convert logical time to "
                   "real time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_CONVLOGICALTOREAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT__CONVTIME, "_stpClient::_convTime" )
   INT32 _stpClient::_convTime( const stpHPTime &fromTime,
                                stpHPTime &toTime,
                                BOOLEAN isRTimeToLTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT__CONVTIME ) ;

      BSONObj argument, result ;

      // assign fields by convert direction
      const CHAR *fromField = isRTimeToLTime ?
                              STP_FIELD_NAME_REAL_TIME :
                              STP_FIELD_NAME_LOGICAL_TIME ;
      const CHAR *toField = isRTimeToLTime ?
                            STP_FIELD_NAME_LOGICAL_TIME :
                            STP_FIELD_NAME_REAL_TIME ;

      // build argument
      try
      {
         BSONObjBuilder builder ;

         BSONObjBuilder subBuilder( builder.subobjStart( fromField ) ) ;
         rc = fromTime.toBSON( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], "
                      "rc: %d", fromField, rc ) ;
         subBuilder.doneFast() ;

         argument = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for [%s], "
                 "occur exception %s", fromField, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // run command
      rc = runCommand( CMD_NAME_STP_CONV_TIME, argument, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   CMD_NAME_STP_CONV_TIME, rc ) ;

      // parse result
      try
      {
         BSONElement element = result.getField( toField ) ;
         PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to parse BSON for [%s], it is not an object",
                   toField ) ;

         rc = toTime.fromBSON( element.embeddedObject() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for [%s], "
                      "rc: %d", toField, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for [%s], "
                 "occur exception %s", toField, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT__CONVTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
