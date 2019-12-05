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

   Source File Name = tpMetaManager.cpp

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

#include "tpMetaManager.hpp"
#include "tpCB.hpp"
#include "tpNode.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"

namespace engine
{

   #define TP_META_UPDATE_INTERVAL ( TP_SEC_TO_NANOSEC( 60 ) )

   /*
      _tpMetaManager implement
    */
   BEGIN_OBJ_MSG_MAP( _tpMetaManager, _tpManagerBase )
      // ON_MSG
      ON_MSG( MSG_TP_META_NOTIFY, processMessage )
      ON_MSG( MSG_TP_META_SYNC_REQ, processMessage )
      ON_MSG( MSG_TP_META_SYNC_RSP, processMessage )
   END_OBJ_MSG_MAP()

   _tpMetaManager::_tpMetaManager( SDB_TPCB *tpCB )
   : tpManagerBase( tpCB ),
     _metaSyncTimeout( 0LL )
   {
   }

   _tpMetaManager::~_tpMetaManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR_ONTIMER, "_tpMetaManager::onTimer" )
   void _tpMetaManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__TPMETAMGR_ONTIMER ) ;

      INT32 rc = SDB_OK ;

      if ( timerID == _timerID )
      {
         _metaSyncTimeout += interval ;
         if ( _metaSyncTimeout >= _getMetaSyncInterval() )
         {
            if ( _tpCB->isPrimary() )
            {
               rc = updateMetaLSN() ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to update meta LSN time, "
                          "rc: %d", rc ) ;
               }
            }
            else if ( _tpCB->isSecondaryServer() )
            {
               rc = launchMetaSync() ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to launch synchronize meta, "
                          "rc: %d", rc ) ;
               }
            }
            _metaSyncTimeout = 0LL ;
         }
      }

      PD_TRACE_EXIT( SDB__TPMETAMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR_PROCESSMESSAGE, "_tpMetaManager::processMessage" )
   INT32 _tpMetaManager::processMessage( NET_HANDLE handle,
                                         MsgHeader * message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_TP_META_NOTIFY :
         {
            rc = _handleMetaNotify(
                              handle, (const MsgTpMetaNotify *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize meta "
                         "notify, rc: %d", rc ) ;
            break ;
         }
         case MSG_TP_META_SYNC_REQ :
         {
            rc = _handleMetaSyncReq(
                              handle, (const MsgTpMetaSyncReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize meta "
                         "request, rc: %d", rc ) ;
            break ;
         }
         case MSG_TP_META_SYNC_RSP :
         {
            rc = _handleMetaSyncRsp(
                              handle, (const MsgTpMetaSyncRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize meta "
                         "response, rc: %d", rc ) ;
            break ;
         }
         default :
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown synchronize message [%d]",
                      message->opCode ) ;
            break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__INITIALIZE, "_tpMetaManager::_initialize" )
   INT32 _tpMetaManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__INITIALIZE ) ;

      UINT32 syncInterval = _options->getSyncInterval() ;
      tpMetaData *metaData = NULL ;
      BOOLEAN created = FALSE ;

      if ( !_buffer.attach( _options->getServiceName(),
                            tpMetaData::getBufferSize() ) )
      {
         PD_CHECK( _buffer.allocate( _options->getServiceName(),
                                     tpMetaData::getBufferSize() ),
                   SDB_OOM, error, PDERROR, "Failed to allocate shared memory "
                   "buffer [key: %s, size: %d]", _options->getServiceName(),
                   tpMetaData::getBufferSize() ) ;
         created = TRUE ;
      }

      PD_CHECK( NULL != _buffer.getBuffer(), SDB_OOM, error, PDERROR,
                "Failed to allocate shared memory buffer [key: %s, size: %d], "
                "it is empty", _options->getServiceName(),
                tpMetaData::getBufferSize() ) ;

      PD_LOG( PDEVENT, "Allocate shared memory [key: %s, size: %d, id: %llu]",
              _buffer.getKeyString(), _buffer.getSize(), _buffer.getID() ) ;

      // if not created, try attach first
      if ( !created )
      {
         metaData = tpMetaData::getBuffer( _buffer.getBuffer() ) ;
      }
      // new buffer or attach failed, create a new one
      if ( NULL == metaData )
      {
         metaData = tpMetaData::newBuffer( _buffer.getBuffer() ) ;
      }

      PD_CHECK( NULL != metaData, SDB_OOM, error, PDERROR,
                "Failed to allocate meta data" ) ;

      if ( TP_ROLE_SERVER == _options->getRole() ||
           TP_ROLE_STANDALONE == _options->getRole() )
      {
         rc = _store.initialize( _options->getLocalCfgPath() ) ;
         if ( SDB_FNE == rc )
         {
            PD_LOG( PDINFO, "meta file [%s] does not exist",
                    _store.getMetaFileName() ) ;
            tpHPTime curHWTime( TP_TIME_SAMPLE_REAL ) ;
            metaData->initialize( curHWTime.toMicroSecond(), syncInterval ) ;
            rc = _setMetaLSN( metaData->getLTValueUS(), 0 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set meta LSN, rc: %d",
                         rc ) ;
         }
         else
         {
            metaData->initialize( _store.getTime(), syncInterval ) ;
         }
      }
      else
      {
         tpHPTime curHWTime( TP_TIME_SAMPLE_REAL ) ;
         metaData->initialize( curHWTime.toMicroSecond(), syncInterval ) ;
      }

      setMetaData( metaData ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__FINALIZE, "_tpMetaManager::_finalize" )
   INT32 _tpMetaManager::_finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__FINALIZE ) ;

      setMetaData( NULL ) ;
      _buffer.release() ;

      PD_TRACE_EXITRC( SDB__TPMETAMGR__FINALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__POSTACTIVATE, "_tpMetaManager::_postActivate" )
   INT32 _tpMetaManager::_postActivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__POSTACTIVATE ) ;

      if ( _tpCB->isPrimary() )
      {
         getMetaData()->updateSyncTime() ;
      }

      PD_TRACE_EXITRC( SDB__TPMETAMGR__POSTACTIVATE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__ONCHANGEPRIMARY, "_tpMetaManager::_onChangePrimary" )
   INT32 _tpMetaManager::_onChangePrimary( const MsgRouteID &primaryRID,
                                           BOOLEAN isLocalPrimary )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__ONCHANGEPRIMARY ) ;

      if ( isLocalPrimary )
      {
         // primary is me now, update logical time to 60 second later
         // or current real time ( if current real time is larger )
         UINT64 metaTime = getMetaData()->getLTValueUS() ;
         tpHPTime curHWTime( TP_TIME_SAMPLE_REAL ) ;
         UINT64 curTime = curHWTime.toMicroSecond() ;
         BOOLEAN updated = FALSE ;
         if ( curTime > metaTime + TP_META_UPDATE_INTERVAL )
         {
            getMetaData()->reset( curTime ) ;
         }
         else
         {
            getMetaData()->reset( metaTime + TP_META_UPDATE_INTERVAL ) ;
         }
         rc = _updateMetaLSN( metaTime, TRUE, updated ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update meta data LSN, "
                      "rc: %d", rc ) ;
         if ( !updated )
         {
            PD_LOG( PDWARNING, "Failed to update meta data LSN, "
                    "it might be expired and ignored" ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__ONCHANGEPRIMARY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__HANDLEMETANOTIFY, "_tpMetaManager::_handleMetaNotify" )
   INT32 _tpMetaManager::_handleMetaNotify( NET_HANDLE handle,
                                            const MsgTpMetaNotify *notify )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__HANDLEMETANOTIFY ) ;

      if ( _tpCB->isSecondaryServer() )
      {
         launchMetaSync() ;
      }
      else
      {
         PD_LOG( PDWARNING, "Ignore handle synchronize meta notify, "
                 "primary is me" ) ;
      }

      PD_TRACE_EXITRC( SDB__TPMETAMGR__HANDLEMETANOTIFY, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__HANDLEMETASYNCREQ, "_tpMetaManager::_handleMetaSyncReq" )
   INT32 _tpMetaManager::_handleMetaSyncReq( NET_HANDLE handle,
                                             const MsgTpMetaSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__HANDLEMETASYNCREQ ) ;

      UINT64 time = 0LL ;
      UINT32 version = 0 ;

      PD_CHECK( _tpCB->isPrimaryServer(),
                SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to handle synchronize meta request, "
                "primary server is not me" ) ;

      rc = getMetaLSN( time, version ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get meta LSN, rc: %d", rc ) ;

      rc = _sendMetaSyncRsp( handle, request, time, version, SDB_OK ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send synchronize metadata response, "
                 "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__HANDLEMETASYNCREQ, rc ) ;
      return rc ;

   error:
      _sendMetaSyncRsp( handle, request, time, version, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__HANDLEMETASYNCRSP, "_tpMetaManager::_handleMetaSyncRsp" )
   INT32 _tpMetaManager::_handleMetaSyncRsp( NET_HANDLE handle,
                                             const MsgTpMetaSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__HANDLEMETASYNCRSP ) ;

      rc = response->reply.res ;
      if ( SDB_OK == rc )
      {
         DPS_LSN metaDataLSN ;
         metaDataLSN.set( response->time, response->version ) ;
         rc = updateMetaLSN( metaDataLSN ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update meta LSN, "
                      "rc: %d", rc ) ;
      }
      else
      {
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            _catalogManager->resetPrimary() ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to synchronize meta, "
                      "received response with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__HANDLEMETASYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__SENDMETANOTIFY, "_tpMetaManager::_sendMetaNotify" )
   INT32 _tpMetaManager::_sendMetaNotify( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__SENDMETANOTIFY ) ;

      MsgTpMetaNotify notify ;

      _fillRequestHeader( notify.header, sizeof( MsgTpMetaNotify ),
                          MSG_TP_META_NOTIFY ) ;

      rc = _netAgent->syncSend( routeID, &notify ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta "
                   "notify to %s, rc: %d", routeID2String( routeID ).c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__SENDMETANOTIFY, rc ) ;
      return rc ;

   error:
      goto done ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__SENDMETASYNCREQ, "_tpMetaManager::_sendMetaSyncReq" )
   INT32 _tpMetaManager::_sendMetaSyncReq( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__SENDMETASYNCREQ ) ;

      MsgTpMetaSyncReq request ;

      _fillRequestHeader( request.header, sizeof( MsgTpMetaSyncReq ),
                          MSG_TP_META_SYNC_REQ ) ;

      rc = _netAgent->syncSend( routeID, &request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta "
                   "request to %s, rc: %d", routeID2String( routeID ).c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__SENDMETASYNCREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__SENDMETASYNCRSP, "_tpMetaManager::_sendMetaSyncRsp" )
   INT32 _tpMetaManager::_sendMetaSyncRsp( NET_HANDLE handle,
                                           const MsgTpMetaSyncReq *request,
                                           UINT64 time,
                                           UINT32 version,
                                           INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__SENDMETASYNCRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      MsgTpMetaSyncRsp response ;

      _fillReplyHeader( request->header, response.reply,
                        sizeof( MsgTpMetaSyncRsp ), returnCode ) ;

      response.time = time ;
      response.version = version ;

      rc = _netAgent->syncSend( handle, &response ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta result, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__SENDMETASYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__BROADCASTMETANOTIFY, "_tpMetaManager::_broadcastMetaNotify" )
   INT32 _tpMetaManager::_broadcastMetaNotify()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__BROADCASTMETANOTIFY ) ;

      TP_SERVER_LIST servers ;

      rc = _catalogManager->dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      for ( TP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         _sendMetaNotify( iter->getRouteID() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__BROADCASTMETANOTIFY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR_LAUNCHMETASYNC, "_tpMetaManager::launchMetaSync" )
   INT32 _tpMetaManager::launchMetaSync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR_LAUNCHMETASYNC ) ;

      MsgRouteID primaryRID ;

      rc = _session.getPrimaryRID( primaryRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get primary route ID, "
                   "rc: %d", rc ) ;

      rc = _sendMetaSyncReq( primaryRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta "
                   "request, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR_LAUNCHMETASYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR_GETMETALSN, "_tpMetaManager::getMetaLSN" )
   INT32 _tpMetaManager::getMetaLSN( UINT64 &time, UINT32 &version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR_GETMETALSN ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;
      _store.getLSN( time, version ) ;

      PD_TRACE_EXITRC( SDB__TPMETAMGR_GETMETALSN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR_GETMETALSN_LSN, "_tpMetaManager::getMetaLSN" )
   INT32 _tpMetaManager::getMetaLSN( DPS_LSN &lsn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR_GETMETALSN_LSN ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;
      _store.getLSN( lsn ) ;

      PD_TRACE_EXITRC( SDB__TPMETAMGR_GETMETALSN_LSN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR_UPDATEMETALSN_LSN, "_tpMetaManager::updateMetaLSN" )
   INT32 _tpMetaManager::updateMetaLSN( const DPS_LSN &metaLSN )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR_UPDATEMETALSN_LSN ) ;

      BOOLEAN updated = FALSE ;

      if ( _tpCB->isPrimary() )
      {
         PD_LOG( PDWARNING, "Ignore update meta LSN, primary is me" ) ;
         goto done ;
      }

      PD_CHECK( !metaLSN.invalid(), SDB_CLS_SYNC_FAILED, error,
                PDWARNING, "Failed to update meta LSN, "
                "given meta LSN is invalid" ) ;

      rc = _updateMetaLSN( metaLSN, updated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update meta LSN, rc: %d",
                   rc ) ;
      PD_CHECK( updated, SDB_CLS_SYNC_FAILED, error, PDERROR,
                "Failed to update meta LSN, given LSN "
                "[time %llu, version %u] is ignored", metaLSN.offset,
                metaLSN.version ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR_UPDATEMETALSN_LSN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR_UPDATEMETALSN, "_tpMetaManager::updateMetaLSN" )
   INT32 _tpMetaManager::updateMetaLSN()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR_UPDATEMETALSN ) ;

      BOOLEAN updated = FALSE ;
      UINT64 time = 0LL ;

      if ( !_tpCB->isPrimary() )
      {
         PD_LOG( PDWARNING, "Ignore update meta LSN, primary is not me" ) ;
         goto done ;
      }

      time = getMetaData()->getLTValueUS() ;

      rc = _updateMetaLSN( time, FALSE, updated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update meta LSN time, "
                   "rc: %d", rc ) ;
      PD_CHECK( updated, SDB_SYS, error, PDERROR,
                "Failed to update meta LSN, given time [%llu] is ignored",
                time ) ;

      getMetaData()->updateSyncTime() ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR_UPDATEMETALSN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__SETMETALSN, "_tpMetaManager::_setMetaLSN" )
   INT32 _tpMetaManager::_setMetaLSN( UINT64 time, UINT32 version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__SETMETALSN ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      _store.setLSN( time, version ) ;

      rc = _store.save() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save meta, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Set meta LSN [ version %u time %llu ]",
              _store.getVersion(), _store.getTime() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__SETMETALSN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__UPDATEMETALSN, "_tpMetaManager::_updateMetaLSN" )
   INT32 _tpMetaManager::_updateMetaLSN( UINT64 time,
                                         BOOLEAN increaseVersion,
                                         BOOLEAN &updated )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__UPDATEMETALSN ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      UINT64 currentTime = _store.getTime() ;
      UINT32 currentVersion = _store.getVersion () ;

      if ( increaseVersion )
      {
         _store.setTime( time ) ;
         _store.increaseVersion() ;
         updated = TRUE ;
      }
      else if ( currentTime <= time )
      {
         _store.setTime( time ) ;
         updated = TRUE ;
      }
      else
      {
         PD_LOG( PDWARNING, "Ignore update meta LSN, "
                 "given time [%llu] is expired, current [%llu]", time,
                 currentTime ) ;
         updated = FALSE ;
      }

      if ( updated )
      {
         rc = _store.save() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save meta, rc: %d", rc ) ;

         PD_LOG( PDEVENT, "Update meta LSN [ version %u time %llu ]",
                 _store.getVersion(), _store.getTime() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__UPDATEMETALSN, rc ) ;
      return rc ;

   error:
      // need rollback
      updated = FALSE ;
      _store.setLSN( currentTime, currentVersion ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETAMGR__UPDATEMETALSN_LSN, "_tpMetaManager::_updateMetaLSN" )
   INT32 _tpMetaManager::_updateMetaLSN( const DPS_LSN &metaLSN,
                                         BOOLEAN &updated )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETAMGR__UPDATEMETALSN_LSN ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      DPS_LSN currentLSN ;

      _store.getLSN( currentLSN ) ;

      if ( currentLSN.compare( metaLSN ) <= 0 )
      {
         _store.setLSN( metaLSN ) ;
         updated = TRUE ;
      }
      else
      {
         PD_LOG( PDWARNING, "Ignore update meta LSN, given LSN "
                 "[ version %u, time %llu ] is expired, "
                 "current [ version %u, time %llu ]",
                 metaLSN.version, metaLSN.offset,
                 currentLSN.version, currentLSN.offset ) ;
         updated = FALSE ;
      }

      if ( updated )
      {
         rc = _store.save() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save meta, rc: %d", rc ) ;

         PD_LOG( PDEVENT, "Update meta LSN [ version %u time %llu ]",
                 _store.getVersion(), _store.getTime() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETAMGR__UPDATEMETALSN_LSN, rc ) ;
      return rc ;

   error:
      // need rollback
      updated = FALSE ;
      _store.setLSN( currentLSN ) ;
      goto done ;
   }

   UINT64 _tpMetaManager::_getMetaSyncInterval()
   {
      return (UINT64)( _options->getSyncInterval() ) * OSS_ONE_SEC ;
   }

}
