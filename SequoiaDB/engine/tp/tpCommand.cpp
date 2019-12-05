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

   Source File Name = tpCommand.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

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

#include "tpCommand.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "dpsLogDef.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{


   /*
      _tpCommand implement
    */
   _tpCommand::_tpCommand( SDB_TPCB *tpCB )
   : _tpCB( tpCB )
   {
   }

   _tpCommand::~_tpCommand()
   {
   }

   INT32 _tpCommand::initialize( const CHAR *option )
   {
      return SDB_OK ;
   }

   INT32 _tpCommand::finalize()
   {
      return SDB_OK ;
   }

   /*
      _tpCommandAssit implement
    */
   _tpCommandAssit::_tpCommandAssit( TP_CMD_NEW_FUNC func )
   {
      if ( NULL != func )
      {
         tpCommand *command = (*func)() ;
         if ( NULL != command )
         {
            const CHAR *name = command->getName() ;
            tpGetCommandBuilder()->_registerCommand( name, func ) ;
         }
         SAFE_OSS_DELETE( command ) ;
      }
   }

   _tpCommandAssit::~_tpCommandAssit()
   {
   }

   /*
      _tpCommandBuilder implement
    */
   _tpCommandBuilder::_tpCommandBuilder()
   {
   }

   _tpCommandBuilder::~_tpCommandBuilder()
   {
   }

   tpCommand *_tpCommandBuilder::createCommand( const CHAR *name )
   {
      TP_CMD_NEW_FUNC func = _findCommand( name ) ;
      if ( NULL != func )
      {
         return (*func)() ;
      }
      return NULL ;
   }

   void _tpCommandBuilder::releaseCommand( tpCommand *command )
   {
      SAFE_OSS_DELETE( command ) ;
   }

   void _tpCommandBuilder::_registerCommand( const CHAR *name,
                                             TP_CMD_NEW_FUNC func )
   {
      _commandMap.insert( make_pair( name, func ) ) ;
   }

   TP_CMD_NEW_FUNC _tpCommandBuilder::_findCommand( const CHAR *name )
   {
      if ( NULL != name )
      {
         TP_CMD_MAP::iterator iter = _commandMap.find( name ) ;
         if ( iter != _commandMap.end() )
         {
            return iter->second ;
         }
      }
      return NULL ;
   }

   /*
      tp command helper functions
    */

   tpCommandBuilder *tpGetCommandBuilder()
   {
      static tpCommandBuilder commandBuilder ;
      return ( &commandBuilder ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETCOMMAND, "tpGetCommand" )
   INT32 tpGetCommand( const CHAR *name, tpCommand **command )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETCOMMAND ) ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      tpCommand *tmpCommand = NULL ;

      PD_CHECK( NULL != name && '$' == name[ 0 ] && '\0' != name[ 1 ],
                SDB_INVALIDARG, error, PDERROR, "Failed to get command, "
                "name is invalid" ) ;

      tmpCommand = tpGetCommandBuilder()->createCommand( name + 1 ) ;
      PD_CHECK( NULL != tmpCommand, SDB_INVALIDARG, error, PDERROR,
                "Failed to get command with name [%s]", name ) ;

      *command = tmpCommand ;

   done:
      PD_TRACE_EXITRC( SDB__TPGETCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPINITCOMMAND, "tpInitCommand" )
   INT32 tpInitCommand( tpCommand *command, const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPINITCOMMAND ) ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize command, command is invalid" ) ;

      try
      {
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
      PD_TRACE_EXITRC( SDB__TPINITCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPRUNCOMMAND, "tpRunCommand" )
   INT32 tpRunCommand( tpCommand *command, BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPRUNCOMMAND ) ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to run command, command is invalid" ) ;

      if ( command->needCheckBusiness() )
      {
         PD_CHECK( pmdGetKRCB()->isBusinessOK(),
                   SDB_SYS, error, PDERROR,
                   "Failed to check business" ) ;
      }

      try
      {
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
      PD_TRACE_EXITRC( SDB__TPRUNCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPRELEASECOMMAND, "tpReleaseCommand" )
   INT32 tpReleaseCommand( tpCommand *command )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPRELEASECOMMAND ) ;

      if ( NULL != command )
      {
         command->finalize() ;
         tpGetCommandBuilder()->releaseCommand( command ) ;
      }

      PD_TRACE_EXITRC( SDB__TPRELEASECOMMAND, rc ) ;

      return rc ;
   }

   /*
      _tpGetTimeCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpGetTimeCMD )

   _tpGetTimeCMD::_tpGetTimeCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpGetTimeCMD::~_tpGetTimeCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETTIMECMD_DOIT, "_tpGetTimeCMD::doit" )
   INT32 _tpGetTimeCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETTIMECMD_DOIT ) ;

      tpLogicalTimeNS time ;

      rc = _tpCB->getMetaData()->getLogicalTimeNS( time, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;

         rc = time.getTime().toBSON( TP_FIELD_NAME_TIMESTAMP, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      TP_FIELD_NAME_TIMESTAMP, rc ) ;

         builder.append( TP_FIELD_NAME_TIME_ERROR,
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
      PD_TRACE_EXITRC( SDB__TPGETTIMECMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpGetMetaCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpGetMetaCMD )

   _tpGetMetaCMD::_tpGetMetaCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpGetMetaCMD::~_tpGetMetaCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETMETACMD_DOIT, "_tpGetMetaCMD::doit" )
   INT32 _tpGetMetaCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETMETACMD_DOIT ) ;

      const CHAR *metaSHMKey = NULL ;
      tpMetaData *metaData = NULL ;
      BOOLEAN gotLSN = FALSE ;
      DPS_LSN metaLSN ;

      metaSHMKey = _tpCB->getMetaManager()->getSHMKey() ;
      PD_CHECK( NULL != metaSHMKey, SDB_SYS, error, PDERROR,
                "Failed to get meta shared memory key, "
                "meta shared memory key is invalid" ) ;

      metaData = _tpCB->getMetaData() ;
      PD_CHECK( NULL != metaData, SDB_SYS, error, PDERROR,
                "Failed to get meta data, meta data is invalid" ) ;

      if ( _tpCB->getMetaManager()->isActivated() )
      {
         _tpCB->getMetaManager()->getMetaLSN( metaLSN ) ;
         gotLSN = TRUE ;
      }

      try
      {
         BSONObjBuilder builder ;

         builder.append( TP_FIELD_NAME_META_SHMKEY, metaSHMKey ) ;

         rc = metaData->toBSON( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for meta data, "
                      "rc: %d", rc ) ;

         if ( gotLSN )
         {
            BSONObjBuilder lsnBuilder(
                           builder.subobjStart( TP_FIELD_NAME_META_LSN ) ) ;
            lsnBuilder.append( TP_FIELD_NAME_META_OFFSET,
                               (INT64)metaLSN.offset ) ;
            lsnBuilder.append( TP_FIELD_NAME_META_VERSION,
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
      PD_TRACE_EXITRC( SDB__TPGETMETACMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpGetServersCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpGetServersCMD )

   _tpGetServersCMD::_tpGetServersCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpGetServersCMD::~_tpGetServersCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETSERVERSCMD_DOIT, "_tpGetServersCMD::doit" )
   INT32 _tpGetServersCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETSERVERSCMD_DOIT ) ;

      rc = _tpCB->getCatalogManager()->getServers( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPGETSERVERSCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpGetSyncClientsCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpGetSyncClientsCMD )

   _tpGetSyncClientsCMD::_tpGetSyncClientsCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpGetSyncClientsCMD::~_tpGetSyncClientsCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETSYNCCLIENTSCMD_DOIT, "_tpGetSyncClientsCMD::doit" )
   INT32 _tpGetSyncClientsCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETSYNCCLIENTSCMD_DOIT ) ;

      TP_CLIENT_MAP clients ;

      rc = _tpCB->getSourceManager()->dumpClients( clients ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump clients, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;

         BSONArrayBuilder arrayBuilder(
                     builder.subarrayStart( TP_FIELD_NAME_SYNC_CLIENTS ) ) ;
         for ( TP_CLIENT_MAP::iterator iter = clients.begin() ;
               iter != clients.end() ;
               ++ iter )
         {
            tpClientNode &client = iter->second ;

            BSONObjBuilder clientBuilder( arrayBuilder.subobjStart() ) ;

            rc = client.toBSON( clientBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for client %s, "
                         "rc: %d", client.toString().c_str(), rc ) ;

            clientBuilder.doneFast() ;
         }

         arrayBuilder.doneFast() ;

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
      PD_TRACE_EXITRC( SDB__TPGETSYNCCLIENTSCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   /*
      _tpGetSyncStatusCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpGetSyncStatusCMD )

   _tpGetSyncStatusCMD::_tpGetSyncStatusCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpGetSyncStatusCMD::~_tpGetSyncStatusCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETSYNCSTATUSCMD_DOIT, "_tpGetSyncStatusCMD::doit" )
   INT32 _tpGetSyncStatusCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETSYNCSTATUSCMD_DOIT ) ;

      try
      {
         BSONObjBuilder builder ;

         builder.append( TP_FIELD_NAME_ROLE,
                         tpGetRoleName( _tpCB->getOptions()->getRole() ) ) ;
         builder.appendBool( TP_FIELD_NAME_IS_PRIMARY, _tpCB->isPrimary() ) ;

         if ( !_tpCB->isPrimary() )
         {
            tpSourceNode source ;
            MsgRouteID primaryRID =
                              _tpCB->getCatalogManager()->getPrimaryRID() ;

            builder.append( TP_FIELD_NAME_SYNC_STATUS,
                            tpGetSyncStatusName(
                                  _tpCB->getSyncManager()->getStatus() ) ) ;

            if ( MSG_INVALID_ROUTEID != primaryRID.value &&
                 SDB_OK == _tpCB->getSyncManager()->getSource( primaryRID,
                                                                source ) )
            {
               rc = source.toBSON( builder, TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for source %s, "
                            "rc: %d", source.toString().c_str(), rc ) ;
            }
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
      PD_TRACE_EXITRC( SDB__TPGETSYNCSTATUSCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpGetSyncHistoryCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpGetSyncHistoryCMD )

   _tpGetSyncHistoryCMD::_tpGetSyncHistoryCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpGetSyncHistoryCMD::~_tpGetSyncHistoryCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETSYNCHISTORYCMD_DOIT, "_tpGetSyncHistoryCMD::doit" )
   INT32 _tpGetSyncHistoryCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETSYNCHISTORYCMD_DOIT ) ;

      TP_SOURCE_MAP sources ;
      TP_SOURCE_LIST sourceList ;

      rc = _tpCB->getSyncManager()->dumpSources( sources ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump sources, rc: %d", rc ) ;

      rc = _sortSources( sources, sourceList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sort sources, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;

         BSONArrayBuilder arrayBuilder(
                     builder.subarrayStart( TP_FIELD_NAME_SYNC_SOURCES ) ) ;
         for ( TP_SOURCE_LIST::reverse_iterator iter = sourceList.rbegin() ;
               iter != sourceList.rend() ;
               ++ iter )
         {
            tpSourceNode &source = ( *iter ) ;

            BSONObjBuilder sourceBuilder( arrayBuilder.subobjStart() ) ;

            rc = source.toBSON( sourceBuilder, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for source %s, "
                         "rc: %d", source.toString().c_str(), rc ) ;

            sourceBuilder.doneFast() ;
         }

         arrayBuilder.doneFast() ;

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
      PD_TRACE_EXITRC( SDB__TPGETSYNCHISTORYCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETSYNCHISTORYCMD__SORTSOURCES, "_tpGetSyncHistoryCMD::_sortSources" )
   INT32 _tpGetSyncHistoryCMD::_sortSources( const TP_SOURCE_MAP &sources,
                                              TP_SOURCE_LIST &sourceList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETSYNCHISTORYCMD__SORTSOURCES ) ;

      try
      {
         for ( TP_SOURCE_MAP::const_iterator iter = sources.begin() ;
               iter != sources.end() ;
               ++ iter )
         {
            tpSourceNode source = iter->second ;
            source.mergeStats() ;
            sourceList.push_back( source ) ;
         }

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
      PD_TRACE_EXITRC( SDB__TPGETSYNCHISTORYCMD__SORTSOURCES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpGetConfigCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpGetConfigCMD )

   _tpGetConfigCMD::_tpGetConfigCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpGetConfigCMD::~_tpGetConfigCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPGETCONFIGCMD_DOIT, "_tpGetConfigCMD::doit" )
   INT32 _tpGetConfigCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPGETSYNCHISTORYCMD_DOIT ) ;

      try
      {
         BSONObj tmpObj ;

         rc = _tpCB->getOptions()->toBSON( tmpObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get config options, "
                      "rc: %d", rc ) ;

         result = tmpObj ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result for command [%s], "
                 "occurred unexpected error: %s", getName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPGETSYNCHISTORYCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpUpdateConfigCMD implement
    */
   IMPLEMENT_TP_CMD_AUTO_REGISTER( _tpUpdateConfigCMD )

   _tpUpdateConfigCMD::_tpUpdateConfigCMD( SDB_TPCB *tpCB )
   : tpCommand( tpCB )
   {
   }

   _tpUpdateConfigCMD::~_tpUpdateConfigCMD()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPUPDATECONFIGCMD_INITIALIZE, "_tpUpdateConfigCMD::initialize" )
   INT32 _tpUpdateConfigCMD::initialize( const CHAR *option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPUPDATECONFIGCMD_INITIALIZE ) ;

      try
      {
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
      PD_TRACE_EXITRC( SDB__TPUPDATECONFIGCMD_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPUPDATECONFIGCMD_DOIT, "_tpUpdateConfigCMD::doit" )
   INT32 _tpUpdateConfigCMD::doit( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPUPDATECONFIGCMD_DOIT ) ;

      try
      {
         BSONObj errorObject ;

         rc = _tpCB->getOptions()->update( _configs, FALSE, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update config options, "
                      "rc: %d", rc ) ;

         rc = _tpCB->getOptions()->save() ;
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
      PD_TRACE_EXITRC( SDB__TPUPDATECONFIGCMD_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
