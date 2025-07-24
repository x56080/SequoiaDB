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

   Source File Name = stpCommand.cpp

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
#include "stpCommand.hpp"
#include "stpCB.hpp"
#include "stpSession.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "dpsLogDef.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _stpCommand implement
    */
   _stpCommand::_stpCommand( STPCB *stpCB )
   : _stpCB( stpCB )
   {
   }

   _stpCommand::~_stpCommand()
   {
   }

   INT32 _stpCommand::initialize( const CHAR *option )
   {
      return SDB_OK ;
   }

   INT32 _stpCommand::finalize()
   {
      return SDB_OK ;
   }

   /*
      _stpCommandAssit implement
    */
   _stpCommandAssit::_stpCommandAssit( STP_CMD_NEW_FUNC func )
   {
      if ( NULL != func )
      {
         stpCommand *command = (*func)( NULL ) ;
         if ( NULL != command )
         {
            const CHAR *name = command->getName() ;
            stpGetCommandBuilder()->_registerCommand( name, func ) ;
         }
         SAFE_OSS_DELETE( command ) ;
      }
   }

   _stpCommandAssit::~_stpCommandAssit()
   {
   }

   /*
      _stpCommandBuilder implement
    */
   _stpCommandBuilder::_stpCommandBuilder()
   {
   }

   _stpCommandBuilder::~_stpCommandBuilder()
   {
   }

   stpCommand *_stpCommandBuilder::createCommand( STPCB *stpCB,
                                                  const CHAR *name )
   {
      // find command by name
      STP_CMD_NEW_FUNC func = _findCommand( name ) ;
      if ( NULL != func )
      {
         // call new function to create command
         return (*func)( stpCB ) ;
      }
      return NULL ;
   }

   void _stpCommandBuilder::releaseCommand( stpCommand *command )
   {
      // release command
      SAFE_OSS_DELETE( command ) ;
   }

   void _stpCommandBuilder::_registerCommand( const CHAR *name,
                                              STP_CMD_NEW_FUNC func )
   {
      // insert new command
      _commandMap.insert( make_pair( name, func ) ) ;
   }

   STP_CMD_NEW_FUNC _stpCommandBuilder::_findCommand( const CHAR *name )
   {
      if ( NULL != name )
      {
         // find command by name
         STP_CMD_MAP::iterator iter = _commandMap.find( name ) ;
         if ( iter != _commandMap.end() )
         {
            return iter->second ;
         }
      }
      return NULL ;
   }

   /*
      STP command helper functions
    */

   stpCommandBuilder *stpGetCommandBuilder()
   {
      static stpCommandBuilder commandBuilder ;
      return ( &commandBuilder ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETCOMMAND, "stpGetCommand" )
   INT32 stpGetCommand( STPCB *stpCB, const CHAR *name, stpCommand **command )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETCOMMAND ) ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      stpCommand *tmpCommand = NULL ;

      // check if command start with '$'
      PD_CHECK( NULL != name && '$' == name[ 0 ] && '\0' != name[ 1 ],
                SDB_INVALIDARG, error, PDERROR, "Failed to get command, "
                "name is invalid" ) ;

      // create command
      tmpCommand = stpGetCommandBuilder()->createCommand( stpCB, name + 1 ) ;
      PD_CHECK( NULL != tmpCommand, SDB_INVALIDARG, error, PDERROR,
                "Failed to get command with name [%s]", name ) ;

      // set command to output
      *command = tmpCommand ;

   done:
      PD_TRACE_EXITRC( SDB__STPGETCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPINITCOMMAND, "stpInitCommand" )
   INT32 stpInitCommand( stpCommand *command, const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPINITCOMMAND ) ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      // check if command is valid
      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize command, command is invalid" ) ;

      try
      {
         // initialize command with option
         rc = command->initialize( option ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize command [%s], "
                      "rc: %d", command->getName(), rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG ( PDERROR, "Failed to initialize command [%s], "
                  "occurred error: %s",
                  command->getName(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPINITCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPRUNCOMMAND, "stpRunCommand" )
   INT32 stpRunCommand( stpCommand *command,
                        stpSession *session,
                        MsgHeader *message,
                        BSONObj &result,
                        BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPRUNCOMMAND ) ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;
      SDB_ASSERT( NULL != session, "session is invalid" ) ;

      // check if command is valid
      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to run command, command is invalid" ) ;

      finished = FALSE ;

      try
      {
         // run command and get result in BSON format
         rc = command->doit( session, message, result, finished ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], "
                      "rc: %d", command->getName(), rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG ( PDERROR, "Failed to run command [%s], "
                  "occurred error: %s",
                  command->getName(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPRUNCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPRELEASECOMMAND, "stpReleaseCommand" )
   INT32 stpReleaseCommand( stpCommand *command )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPRELEASECOMMAND ) ;

      if ( NULL != command )
      {
         // finalize command
         command->finalize() ;

         // release command
         stpGetCommandBuilder()->releaseCommand( command ) ;
      }

      PD_TRACE_EXITRC( SDB__STPRELEASECOMMAND, rc ) ;

      return rc ;
   }

   /*
      _stpGetTimeCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetTimeCMD )

   _stpGetTimeCMD::_stpGetTimeCMD( STPCB *stpCB )
   : stpCommand( stpCB ),
     _format( STP_FORMAT_UNKNOWN )
   {
   }

   _stpGetTimeCMD::~_stpGetTimeCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETTIMECMD_INITIALIZE, "_stpGetTimeCMD::initialize" )
   INT32 _stpGetTimeCMD::initialize( const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETTIMECMD_INITIALIZE ) ;

      try
      {
         // parse new configs
         BSONObj temp( option ) ;
         BSONElement subElement ;
         const CHAR *formatName = NULL ;

         _options = temp.getOwned() ;
         subElement = _options.getField( STP_FIELD_NAME_TYPE ) ;
         if ( EOO == subElement.type() )
         {
            // by default
            _format = STP_FORMAT_LOGICAL_TIME_NS ;
         }
         else
         {
            PD_CHECK( String == subElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s], it is not a string",
                      STP_FIELD_NAME_TYPE ) ;
            formatName = subElement.valuestr() ;

            _format = stpGetTimeFormat( formatName ) ;
         }
         PD_CHECK( STP_FORMAT_UNKNOWN != _format,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse option for command [%s], "
                   "format [%s] is unknown", getName(), formatName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse option for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPGETTIMECMD_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETTIMECMD_DOIT, "_stpGetTimeCMD::doit" )
   INT32 _stpGetTimeCMD::doit( stpSession *session,
                               MsgHeader *message,
                               BSONObj &result,
                               BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETTIMECMD_DOIT ) ;

      try
      {
         switch ( _format )
         {
            case STP_FORMAT_LOGICAL_TIME_NS :
            {
               stpLogicalTimeNS time ;

               // get logical time in nanoseconds
               rc = _stpCB->getMetaData()->getLogicalTimeNS( time ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get logical time "
                            "in nanoseconds, rc: %d", rc ) ;

               rc = time.toBSON( result ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "logical time in nanoseconds, rc: %d", rc ) ;

               break ;
            }
            case STP_FORMAT_LOGICAL_TIME_US :
            {
               stpLogicalTimeUS time ;
               stpLogicalTimeNS tempTime ;

               // get logical time in microseconds
               rc = _stpCB->getMetaData()->getLogicalTimeNS( tempTime ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get logical time "
                            "in nanoseconds, rc: %d", rc ) ;

               // convert to microseconds
               time = tempTime ;
               rc = time.toBSON( result ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "logical time in microseconds, rc: %d", rc ) ;

               break ;
            }
            default :
            {
               PD_LOG( PDERROR, "Failed to get time, unknown format" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETTIMECMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpGetMetaCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetMetaCMD )

   _stpGetMetaCMD::_stpGetMetaCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpGetMetaCMD::~_stpGetMetaCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETMETACMD_DOIT, "_stpGetMetaCMD::doit" )
   INT32 _stpGetMetaCMD::doit( stpSession *session,
                               MsgHeader *message,
                               BSONObj &result,
                               BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETMETACMD_DOIT ) ;

      const CHAR *metaSHMKey = NULL ;
      stpMetaData *metaData = NULL ;
      DPS_LSN metaLSN ;

      // get key of shared memory of meta data
      metaSHMKey = _stpCB->getMetaManager()->getSHMKey() ;
      PD_CHECK( NULL != metaSHMKey, SDB_SYS, error, PDERROR,
                "Failed to get meta shared memory key, "
                "meta shared memory key is invalid" ) ;

      // get meta data
      metaData = _stpCB->getMetaData() ;
      PD_CHECK( NULL != metaData, SDB_SYS, error, PDERROR,
                "Failed to get meta data, meta data is invalid" ) ;

      // if meta manager is activate, get meta LSN
      if ( _stpCB->getMetaManager()->isActivated() )
      {
         _stpCB->getMetaManager()->getMetaLSN( metaLSN ) ;
      }

      try
      {
         BSONObjBuilder builder ;

         // append key of shared memory
         builder.append( STP_FIELD_NAME_META_SHMKEY, metaSHMKey ) ;

         // append meta data
         rc = metaData->toBSON( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for meta data, "
                      "rc: %d", rc ) ;

         {
            // append meta LSN
            BSONObjBuilder lsnBuilder(
                           builder.subobjStart( STP_FIELD_NAME_META_LSN ) ) ;
            // append meta LSN time ( as offset )
            lsnBuilder.append( STP_FIELD_NAME_META_OFFSET,
                               (INT64)metaLSN.offset ) ;
            // append meta LSN version
            lsnBuilder.append( STP_FIELD_NAME_META_VERSION,
                               (INT32)metaLSN.version ) ;
            lsnBuilder.doneFast() ;
         }

         result = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETMETACMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpGetServersCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetServersCMD )

   _stpGetServersCMD::_stpGetServersCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpGetServersCMD::~_stpGetServersCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETSERVERSCMD_DOIT, "_stpGetServersCMD::doit" )
   INT32 _stpGetServersCMD::doit( stpSession *session,
                                  MsgHeader *message,
                                  BSONObj &result,
                                  BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSERVERSCMD_DOIT ) ;

      // get servers into BSON format
      rc = _stpCB->getNodeManager()->getServers( result, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETSERVERSCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpGetSyncClientsCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetSyncClientsCMD )

   _stpGetSyncClientsCMD::_stpGetSyncClientsCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpGetSyncClientsCMD::~_stpGetSyncClientsCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETSYNCCLIENTSCMD_DOIT, "_stpGetSyncClientsCMD::doit" )
   INT32 _stpGetSyncClientsCMD::doit( stpSession *session,
                                      MsgHeader *message,
                                      BSONObj &result,
                                      BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSYNCCLIENTSCMD_DOIT ) ;

      STP_CLIENT_MAP clients ;
      stpClientNode localNode = _stpCB->getNodeManager()->getLocal() ;

      // get all clients
      rc = _stpCB->getSyncSourceManager()->dumpClients( clients ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump clients, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;

         // build address ( host name and service name ) for source
         BSONObjBuilder sourceBuilder(
               builder.subobjStart( STP_FIELD_NAME_SYNC_SOURCE ) ) ;
         sourceBuilder.append( STP_FIELD_NAME_HOST,
                               localNode.getHostName() ) ;
         sourceBuilder.append( STP_FIELD_NAME_SERVICE,
                               localNode.getServiceName() ) ;
         sourceBuilder.doneFast() ;

         // build BSON array for clients
         BSONArrayBuilder arrayBuilder(
                     builder.subarrayStart( STP_FIELD_NAME_SYNC_CLIENTS ) ) ;
         for ( STP_CLIENT_MAP::iterator iter = clients.begin() ;
               iter != clients.end() ;
               ++ iter )
         {
            stpClientNode &client = iter->second ;

            BSONObjBuilder clientBuilder( arrayBuilder.subobjStart() ) ;

            // build BSON for client
            rc = client.toBSON( clientBuilder, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for client %s, "
                         "rc: %d", client.toString().c_str(), rc ) ;

            clientBuilder.doneFast() ;
         }

         arrayBuilder.doneFast() ;

         // copy to output
         result = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETSYNCCLIENTSCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   /*
      _stpGetSyncStatusCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetSyncStatusCMD )

   _stpGetSyncStatusCMD::_stpGetSyncStatusCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpGetSyncStatusCMD::~_stpGetSyncStatusCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETSYNCSTATUSCMD_DOIT, "_stpGetSyncStatusCMD::doit" )
   INT32 _stpGetSyncStatusCMD::doit( stpSession *session,
                                     MsgHeader *message,
                                     BSONObj &result,
                                     BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSYNCSTATUSCMD_DOIT ) ;

      try
      {
         BSONObjBuilder builder ;

         // append role
         builder.append( STP_FIELD_NAME_ROLE,
                         stpGetRoleName( _stpCB->getOptions()->getRole() ) ) ;
         // append primary status
         builder.appendBool( STP_FIELD_NAME_IS_PRIMARY, _stpCB->isPrimary() ) ;

         if ( !_stpCB->isPrimary() )
         {
            // if local node is not primary, get synchronize status
            stpSourceNode source ;
            MsgRouteID sourceRID ;

            sourceRID.value =
                  _stpCB->getSyncClientManager()->getRegSourceRIDValue() ;

            // append synchronize status
            builder.append( STP_FIELD_NAME_SYNC_STATUS,
                            stpGetSyncStatusName(
                                  _stpCB->getSyncClientManager()->getStatus() ) ) ;

            BSONObjBuilder sourceBuilder(
                        builder.subobjStart( STP_FIELD_NAME_SYNC_SOURCE ) ) ;

            // append source as BSON format
            if ( MSG_INVALID_ROUTEID != sourceRID.value &&
                 SDB_OK == _stpCB->getSyncClientManager()->getSource( sourceRID,
                                                                      source ) )
            {
               rc = source.toBSON( sourceBuilder, TRUE, TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for source %s, "
                            "rc: %d", source.toString().c_str(), rc ) ;
            }

            sourceBuilder.doneFast() ;
         }

         // copy to output
         result = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETSYNCSTATUSCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpGetSyncHistoryCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetSyncHistoryCMD )

   _stpGetSyncHistoryCMD::_stpGetSyncHistoryCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpGetSyncHistoryCMD::~_stpGetSyncHistoryCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETSYNCHISTORYCMD_DOIT, "_stpGetSyncHistoryCMD::doit" )
   INT32 _stpGetSyncHistoryCMD::doit( stpSession *session,
                                      MsgHeader *message,
                                      BSONObj &result,
                                      BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSYNCHISTORYCMD_DOIT ) ;

      STP_SOURCE_MAP sources ;
      STP_SOURCE_LIST sourceList ;

      // get all synchronize sources
      rc = _stpCB->getSyncClientManager()->dumpSources( sources ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump sources, rc: %d", rc ) ;

      // sort sources by last synchronize time
      rc = _sortSources( sources, sourceList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sort sources, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;

         // append sources into BSON array
         BSONArrayBuilder arrayBuilder(
                     builder.subarrayStart( STP_FIELD_NAME_SYNC_SOURCES ) ) ;
         for ( STP_SOURCE_LIST::reverse_iterator iter = sourceList.rbegin() ;
               iter != sourceList.rend() ;
               ++ iter )
         {
            stpSourceNode &source = ( *iter ) ;

            BSONObjBuilder sourceBuilder( arrayBuilder.subobjStart() ) ;

            // output source into BSON format
            rc = source.toBSON( sourceBuilder, FALSE, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for source %s, "
                         "rc: %d", source.toString().c_str(), rc ) ;

            sourceBuilder.doneFast() ;
         }

         arrayBuilder.doneFast() ;

         // copy to output
         result = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETSYNCHISTORYCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETSYNCHISTORYCMD__SORTSOURCES, "_stpGetSyncHistoryCMD::_sortSources" )
   INT32 _stpGetSyncHistoryCMD::_sortSources( const STP_SOURCE_MAP &sources,
                                              STP_SOURCE_LIST &sourceList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSYNCHISTORYCMD__SORTSOURCES ) ;

      try
      {
         // merge statistics before sort
         for ( STP_SOURCE_MAP::const_iterator iter = sources.begin() ;
               iter != sources.end() ;
               ++ iter )
         {
            stpSourceNode source = iter->second ;

            // merge current statistics with history statistics
            source.mergeStats() ;

            // add into list
            sourceList.push_back( source ) ;
         }

         // sort sources by last synchronize time
         sort( sourceList.begin(), sourceList.end() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPGETSYNCHISTORYCMD__SORTSOURCES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpGetConfigCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetConfigCMD )

   _stpGetConfigCMD::_stpGetConfigCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpGetConfigCMD::~_stpGetConfigCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETCONFIGCMD_DOIT, "_stpGetConfigCMD::doit" )
   INT32 _stpGetConfigCMD::doit( stpSession *session,
                                 MsgHeader *message,
                                 BSONObj &result,
                                 BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSYNCHISTORYCMD_DOIT ) ;

      try
      {
         BSONObj configObject ;

         // get configs in BSON format from config file
         rc = _stpCB->getOptions()->toBSON( configObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get config options, "
                      "rc: %d", rc ) ;

         // copy to output
         result = configObject ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETSYNCHISTORYCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpUpdateConfigCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpUpdateConfigCMD )

   _stpUpdateConfigCMD::_stpUpdateConfigCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpUpdateConfigCMD::~_stpUpdateConfigCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPUPDATECONFIGCMD_INITIALIZE, "_stpUpdateConfigCMD::initialize" )
   INT32 _stpUpdateConfigCMD::initialize( const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPUPDATECONFIGCMD_INITIALIZE ) ;

      try
      {
         // parse new configs
         _configs = BSONObj( option ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse option for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPUPDATECONFIGCMD_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPUPDATECONFIGCMD_DOIT, "_stpUpdateConfigCMD::doit" )
   INT32 _stpUpdateConfigCMD::doit( stpSession *session,
                                    MsgHeader *message,
                                    BSONObj &result,
                                    BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPUPDATECONFIGCMD_DOIT ) ;

      try
      {
         string returnStr;
         BOOLEAN hasError = FALSE ;
         BSONObj errorObject ;
         pmdCfgRecord::controlParams cp( TRUE ) ;
         // update options ( will notify config changed to all STPCB modules
         // internally )
         rc = _stpCB->getOptions()->update( _configs, FALSE, cp, errorObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update config options, "
                      "rc: %d", rc ) ;

         // save to config file
         rc = _stpCB->getOptions()->save() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save config options, "
                      "rc: %d", rc ) ;

         // build error report
         if ( SDB_OK != optBuildErrorReport( errorObject,
                                             hasError,
                                             returnStr ) )
         {
            // ignore error
            goto error ;
         }

         if ( hasError )
         {
            rc = SDB_RTN_CONF_NOT_TAKE_EFFECT ;
            PD_LOG_MSG( PDERROR, returnStr.c_str() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPUPDATECONFIGCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpStopCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpStopCMD )

   _stpStopCMD::_stpStopCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpStopCMD::~_stpStopCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSTOPCMD_FINALIZE, "_stpStopCMD::finalize" )
   INT32 _stpStopCMD::finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSTOPCMD_FINALIZE ) ;

      // shutdown STP
      PMD_SHUTDOWN_DB( SDB_OK ) ;

      PD_TRACE_EXITRC( SDB__STPSTOPCMD_FINALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSTOPCMD_DOIT, "_stpStopCMD::doit" )
   INT32 _stpStopCMD::doit( stpSession *session,
                            MsgHeader *message,
                            BSONObj &result,
                            BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSTOPCMD_DOIT ) ;

      // do nothing, do the shutdown in finalize phase ( since we need to
      // reply to client after doing phase )
      PD_LOG( PDEVENT, "Got stop command" ) ;

      finished = TRUE ;

      PD_TRACE_EXITRC( SDB__STPSTOPCMD_DOIT, rc ) ;

      return rc ;
   }

   /*
      _stpReelectCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpReelectCMD )

   _stpReelectCMD::_stpReelectCMD( STPCB *stpCB )
   : stpCommand( stpCB ),
     _timeout( STP_REELECT_DFT_TIMEOUT ),
     _targetHostName( NULL )
   {
   }

   _stpReelectCMD::~_stpReelectCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREELECTCMD_INITIALIZE, "_stpReelectCMD::initialize" )
   INT32 _stpReelectCMD::initialize( const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREELECTCMD_INITIALIZE ) ;

      try
      {
         BSONObj temp = BSONObj( option ) ;
         BSONElement element ;

         _options = temp.getOwned() ;

         element = _options.getField( STP_FIELD_NAME_REELECT_TIMEOUT ) ;
         if ( EOO != element.type() )
         {
            PD_CHECK( element.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s] from option, "
                      "it is not a number", STP_FIELD_NAME_REELECT_TIMEOUT ) ;
            _timeout = (UINT32)( element.numberInt() ) ;
         }
         else
         {
            _timeout = STP_REELECT_DFT_TIMEOUT ;
         }

         element = _options.getField( STP_FIELD_NAME_HOST ) ;
         if ( EOO != element.type() )
         {
            PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s] from option, "
                      "it is not a string", STP_FIELD_NAME_HOST ) ;
            _targetHostName = element.valuestr() ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse option for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( NULL != _targetHostName &&
           0 == ossStrcmp( _targetHostName, pmdGetKRCB()->getHostName() ) )
      {
         // target primary is current node
         if ( _stpCB->isPrimaryServer() )
         {
            // do nothing
         }
         else if ( _stpCB->isSecondaryServer() )
         {
            PD_LOG( PDDEBUG, "Reelect target, set max weight" ) ;

            // it is not primary yet, set shadow weight to maximum
            _stpCB->getReplManager()->getVoteMachine()->
                  setShadowWeight( CLS_ELECTION_WEIGHT_MAX ) ;
         }
         else
         {
            PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                      "Failed to reelect primary to current node [%s], "
                      "current node is not server", _targetHostName ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPREELECTCMD_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREELECTCMD_DOIT, "_stpReelectCMD::doit" )
   INT32 _stpReelectCMD::doit( stpSession *session,
                               MsgHeader *message,
                               BSONObj &result,
                               BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREELECTCMD_DOIT ) ;

      BOOLEAN tempFinished = TRUE ;
      pmdEDUCB *cb = session->eduCB() ;

      if ( NULL == _targetHostName )
      {
         MsgRouteID targetRID ;
         targetRID.value = MSG_INVALID_ROUTEID ;

         // notify secondary to synchronize
         _stpCB->getMetaManager()->broadcastMetaNotify() ;

         // no target primary is given
         rc = _stpCB->getReplManager()->reelect( CLS_REELECTION_LEVEL_3,
                                                 _timeout, cb, targetRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to run reelect, rc: %d", rc ) ;
      }
      else if ( 0 != ossStrcmp( _targetHostName,
                                pmdGetKRCB()->getHostName() ) )
      {
         stpServerNode server ;
         rc = _stpCB->getNodeManager()->getServer( _targetHostName, server ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get server [%s], rc: %d",
                      _targetHostName, rc ) ;

         if ( server.getRouteIDValue() == message->routeID.value )
         {
            // the message is redirected from target node, no need to redirect
            // back

            // notify secondary to synchronize
            _stpCB->getMetaManager()->broadcastMetaNotify() ;

            rc = _stpCB->getReplManager()->reelect( CLS_REELECTION_LEVEL_3,
                                                    _timeout,
                                                    cb,
                                                    server.getRouteID() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to run reelect, rc: %d", rc ) ;
         }
         else
         {
            // redirect message to target node
            rc = _stpCB->getServiceManager()->redirectNode( session,
                                                            server.getRouteID(),
                                                            message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to redirect message to "
                         "server [%s], rc: %d", _targetHostName, rc ) ;

            // unset target node
            _targetHostName = NULL ;

            // not finished yet
            tempFinished = FALSE ;
         }
      }

   done:
      finished = tempFinished ;
      PD_TRACE_EXITRC( SDB__STPREELECTCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpConvTimeCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpConvTimeCMD )

   _stpConvTimeCMD::_stpConvTimeCMD( STPCB *stpCB )
   : stpCommand( stpCB ),
     _fromRTimeToLTime( FALSE ),
     _simpleMode( FALSE ),
     _logicalTime(),
     _realTime()
   {
   }

   _stpConvTimeCMD::~_stpConvTimeCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCONVTIMECMD_INITIALIZE, "_stpConvTimeCMD::initialize" )
   INT32 _stpConvTimeCMD::initialize( const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCONVTIMECMD_INITIALIZE ) ;

      BOOLEAN hasRealTime = FALSE, hasLogicalTime = FALSE ;

      try
      {
         BSONObj temp = BSONObj( option ) ;
         BSONElement element ;

         // parse logical time
         element = temp.getField( STP_FIELD_NAME_LOGICAL_TIME ) ;
         if ( EOO != element.type() )
         {
            rc = _logicalTime.fromBSONElement( element, FALSE, _simpleMode ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse logical time, rc: %d",
                         rc ) ;

            hasLogicalTime = TRUE ;
         }

         // parse real time
         element = temp.getField( STP_FIELD_NAME_REAL_TIME ) ;
         if ( EOO != element.type() )
         {
            rc = _realTime.fromBSONElement( element, TRUE, _simpleMode ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse real time, rc: %d",
                         rc ) ;

            hasRealTime = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse option for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // check conflicts
      if ( hasRealTime && !hasLogicalTime )
      {
         _fromRTimeToLTime = TRUE ;
      }
      else if ( !hasRealTime && hasLogicalTime )
      {
         _fromRTimeToLTime = FALSE ;
      }
      else if ( hasRealTime && hasLogicalTime )
      {
         PD_LOG( PDERROR, "Failed to parse options for command [%s], "
                 "can not have both real time and logical time",
                 getName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to parse options for command [%s], "
                 "no real time or logical time is found", getName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCONVTIMECMD_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCONVTIMECMD_DOIT, "_stpConvTimeCMD::doit" )
   INT32 _stpConvTimeCMD::doit( stpSession *session,
                                MsgHeader *message,
                                BSONObj &result,
                                BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCONVTIMECMD_DOIT ) ;

      stpMetaManager *metaManager = _stpCB->getMetaManager() ;

      try
      {
         BSONObjBuilder builder ;

         if ( _fromRTimeToLTime )
         {
            rc = metaManager->convRTimeToLTime( _realTime, _logicalTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to convert real time to "
                         "logical time, rc: %d", rc ) ;

            rc = _buildLTime( builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build result for logical "
                         "time, rc: %d", rc ) ;
         }
         else
         {
            rc = metaManager->convLTimeToRTime( _logicalTime, _realTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to convert real time to "
                         "logical time, rc: %d", rc ) ;

            rc = _buildRTime( builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build result for real "
                         "time, rc: %d", rc ) ;
         }

         result = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPCONVTIMECMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCONVTIMECMD__BUILDLTIME, "_stpConvTimeCMD::_buildLTime" )
   INT32 _stpConvTimeCMD::_buildLTime( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCONVTIMECMD__BUILDLTIME ) ;

      if ( _simpleMode )
      {
         // simple mode, build as number long
         builder.append( STP_FIELD_NAME_LOGICAL_TIME,
                         (INT64)( _logicalTime.toMicroSecond() ) ) ;
      }
      else
      {
         // otherwise, build as HP time
         BSONObjBuilder subBuilder(
               builder.subobjStart( STP_FIELD_NAME_LOGICAL_TIME ) ) ;
         rc = _logicalTime.toBSON( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build result with "
                      "logical time, rc: %d", rc ) ;
         subBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCONVTIMECMD__BUILDLTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCONVTIMECMD__BUILDRTIME, "_stpConvTimeCMD::_buildRTime" )
   INT32 _stpConvTimeCMD::_buildRTime( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCONVTIMECMD__BUILDRTIME ) ;

      if ( _simpleMode )
      {
         // simple mode, build as $timestamp
         rc = _realTime.toBSONTimestamp( builder,
                                         STP_FIELD_NAME_REAL_TIME ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build result with "
                      "real time, rc: %d", rc ) ;
      }
      else
      {
         // otherwise, build as HP time
         BSONObjBuilder subBuilder(
               builder.subobjStart( STP_FIELD_NAME_REAL_TIME ) ) ;
         rc = _realTime.toBSON( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build result with "
                      "real time, rc: %d", rc ) ;
         subBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCONVTIMECMD__BUILDRTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpGetTimeMapCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpGetTimeMapCMD )

   _stpGetTimeMapCMD::_stpGetTimeMapCMD( STPCB *stpCB )
   : stpCommand( stpCB ),
     _hasLogicalTime( FALSE ),
     _hasRealTime( FALSE ),
     _recordCount( 20 )
   {
   }

   _stpGetTimeMapCMD::~_stpGetTimeMapCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETTIMEMAPCMD_INITIALIZE, "_stpGetTimeMapCMD::initialize" )
   INT32 _stpGetTimeMapCMD::initialize( const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETTIMEMAPCMD_INITIALIZE ) ;

      try
      {
         BOOLEAN simpleMode = FALSE ;
         BSONObj temp = BSONObj( option ) ;
         BSONElement element ;

         // parse logical time
         element = temp.getField( STP_FIELD_NAME_LOGICAL_TIME ) ;
         if ( EOO != element.type() )
         {
            rc = _beginLogicalTime.fromBSONElement( element,
                                                    FALSE,
                                                    simpleMode ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse logical time, rc: %d",
                         rc ) ;

            _hasLogicalTime = TRUE ;
         }

         // parse real time
         element = temp.getField( STP_FIELD_NAME_REAL_TIME ) ;
         if ( EOO != element.type() )
         {
            rc = _beginRealTime.fromBSONElement( element,
                                                 TRUE,
                                                 simpleMode ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse real time, rc: %d",
                         rc ) ;

            _hasRealTime = TRUE ;
         }

         // parse count
         element = temp.getField( STP_FIELD_NAME_COUNT ) ;
         if ( EOO != element.type() )
         {
            _recordCount = (UINT32)( element.numberInt() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse options, occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( _hasLogicalTime && _hasRealTime )
      {
         PD_LOG( PDERROR, "Failed to parse options, could not have both "
                 "logical time and real time" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPGETTIMEMAPCMD_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETTIMEMAPCMD_DOIT, "_stpGetTimeMapCMD::doit" )
   INT32 _stpGetTimeMapCMD::doit( stpSession *session,
                                  MsgHeader *message,
                                  BSONObj &result,
                                  BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETTIMEMAPCMD_DOIT ) ;

      STP_TIMEMAP_RECLIST recordList ;
      stpTimeMapManager *timeMapManager =
            _stpCB->getMetaManager()->getTimeMapManager() ;

      if ( _hasLogicalTime )
      {
         rc = timeMapManager->getRecordsAfterLTime( _beginLogicalTime,
                                                    TRUE,
                                                    _recordCount,
                                                    recordList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get records after "
                      "logical time [%llu], rc: %d",
                      _beginLogicalTime.toMicroSecond(), rc ) ;
      }
      else if ( _hasRealTime )
      {
         rc = timeMapManager->getRecordsAfterRTime( _beginRealTime,
                                                    TRUE,
                                                    _recordCount,
                                                    recordList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get records after "
                      "real time [%llu], rc: %d",
                      _beginRealTime.toMicroSecond(), rc ) ;
      }
      else
      {
         rc = timeMapManager->getLastRecords( _recordCount, recordList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get latest records, rc: %d",
                      rc ) ;
      }

      try
      {
         BSONObjBuilder builder ;
         BSONArrayBuilder subBuilder(
               builder.subarrayStart( STP_FIELD_NAME_TIMEMAP ) ) ;

         for ( STP_TIMEMAP_RECLIST::reverse_iterator iter =
                                                      recordList.rbegin() ;
               iter != recordList.rend() ;
               ++ iter )
         {
            const stpTimeMapRecord &record = *iter ;
            stpHPTime logicalTime, realTime ;

            logicalTime.fromMicroSecond( record.getLogicalTime() ) ;
            realTime.fromMicroSecond( record.getRealTime() ) ;

            BSONObjBuilder objBuilder( subBuilder.subobjStart() ) ;

            // build logical time
            objBuilder.append( STP_FIELD_NAME_LOGICAL_TIME,
                               (INT64)( logicalTime.toMicroSecond() ) ) ;
            // build real time
            rc = realTime.toBSONTimestamp( objBuilder,
                                           STP_FIELD_NAME_REAL_TIME ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build result with "
                         "real time, rc: %d", rc ) ;

            objBuilder.doneFast() ;
         }

         subBuilder.doneFast() ;
         result = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      finished = TRUE ;
      PD_TRACE_EXITRC( SDB__STPGETTIMEMAPCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpMsgCMD implement
    */
   IMPLEMENT_STP_CMD_AUTO_REGISTER( _stpMsgCMD )

   _stpMsgCMD::_stpMsgCMD( STPCB *stpCB )
   : stpCommand( stpCB )
   {
   }

   _stpMsgCMD::~_stpMsgCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMSGCMD_INITIALIZE, "_stpMsgCMD::initialize" )
   INT32 _stpMsgCMD::initialize( const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMSGCMD_INITIALIZE ) ;

      try
      {
         BSONObj boOption( option ) ;
         BSONElement element = boOption.getField( FIELD_NAME_MESSAGE ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not a string",
                   FIELD_NAME_MESSAGE ) ;
         _message.assign( element.valuestr() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse option for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMSGCMD_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMSGCMD_DOIT, "_stpMsgCMD::doit" )
   INT32 _stpMsgCMD::doit( stpSession *session,
                           MsgHeader *message,
                           BSONObj &result,
                           BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMSGCMD_DOIT ) ;

      PD_LOG( getPDLevel(), "%s", _message.c_str() ) ;

      finished = TRUE ;

      PD_TRACE_EXITRC( SDB__STPMSGCMD_DOIT, rc ) ;

      return rc ;
   }

}
