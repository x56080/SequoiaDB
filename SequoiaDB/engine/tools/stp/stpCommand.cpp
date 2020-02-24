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
         stpCommand *command = (*func)() ;
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

   stpCommand *_stpCommandBuilder::createCommand( const CHAR *name )
   {
      // find command by name
      STP_CMD_NEW_FUNC func = _findCommand( name ) ;
      if ( NULL != func )
      {
         // call new function to create command
         return (*func)() ;
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
   INT32 stpGetCommand( const CHAR *name, stpCommand **command )
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
      tmpCommand = stpGetCommandBuilder()->createCommand( name + 1 ) ;
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
   INT32 stpRunCommand( stpCommand *command, BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPRUNCOMMAND ) ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      // check if command is valid
      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to run command, command is invalid" ) ;

      // check business if needed
      if ( command->needCheckBusiness() )
      {
         PD_CHECK( pmdGetKRCB()->isBusinessOK(),
                   SDB_SYS, error, PDERROR,
                   "Failed to check business" ) ;
      }

      try
      {
         // run command and get result in BSON format
         rc = command->doit( result ) ;
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

         // reelase command
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
   : stpCommand( stpCB )
   {
   }

   _stpGetTimeCMD::~_stpGetTimeCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPGETTIMECMD_DOIT, "_stpGetTimeCMD::doit" )
   INT32 _stpGetTimeCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETTIMECMD_DOIT ) ;

      stpLogicalTimeNS time ;

      // get logical time in nanoseconds
      rc = _stpCB->getMetaData()->getLogicalTimeNS( time, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;

         // build time into BSON format
         rc = time.getTime().toBSON( STP_FIELD_NAME_TIMESTAMP, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      STP_FIELD_NAME_TIMESTAMP, rc ) ;

         // append time error
         builder.append( STP_FIELD_NAME_TIME_ERROR,
                         (INT64)time.getTimeError() ) ;

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
   INT32 _stpGetMetaCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETMETACMD_DOIT ) ;

      const CHAR *metaSHMKey = NULL ;
      stpMetaData *metaData = NULL ;
      BOOLEAN gotLSN = FALSE ;
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
         gotLSN = TRUE ;
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

         if ( gotLSN )
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
   INT32 _stpGetServersCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSERVERSCMD_DOIT ) ;

      // get servers into BSON format
      rc = _stpCB->getNodeManager()->getServers( result, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
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
   INT32 _stpGetSyncClientsCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSYNCCLIENTSCMD_DOIT ) ;

      STP_CLIENT_MAP clients ;

      // get all clients
      rc = _stpCB->getSyncSourceManager()->dumpClients( clients ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump clients, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;

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
   INT32 _stpGetSyncStatusCMD::doit( BSONObj &result )
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
            MsgRouteID primaryRID =
                              _stpCB->getNodeManager()->getPrimaryRID() ;

            // append synchronize status
            builder.append( STP_FIELD_NAME_SYNC_STATUS,
                            stpGetSyncStatusName(
                                  _stpCB->getSyncManager()->getStatus() ) ) ;

            // append source as BSON format
            if ( MSG_INVALID_ROUTEID != primaryRID.value &&
                 SDB_OK == _stpCB->getSyncManager()->getSource( primaryRID,
                                                                source ) )
            {
               rc = source.toBSON( builder, TRUE, TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for source %s, "
                            "rc: %d", source.toString().c_str(), rc ) ;
            }
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
   INT32 _stpGetSyncHistoryCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPGETSYNCHISTORYCMD_DOIT ) ;

      STP_SOURCE_MAP sources ;
      STP_SOURCE_LIST sourceList ;

      // get all synchronize sources
      rc = _stpCB->getSyncManager()->dumpSources( sources ) ;
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
   INT32 _stpGetConfigCMD::doit( BSONObj &result )
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
   INT32 _stpUpdateConfigCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPUPDATECONFIGCMD_DOIT ) ;

      try
      {
         BSONObj errorObject ;

         // update options ( will notify config changed to all STPCB modules
         // internally )
         rc = _stpCB->getOptions()->update( _configs, FALSE, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update config options, "
                      "rc: %d", rc ) ;

         // save to config file
         rc = _stpCB->getOptions()->save() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save config options, "
                      "rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPUPDATECONFIGCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
