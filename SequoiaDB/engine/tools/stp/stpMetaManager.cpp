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

   Source File Name = stpMetaManager.cpp

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
#include "stpMetaManager.hpp"
#include "stpCB.hpp"
#include "stpNode.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"

namespace engine
{

   // interval ( in nanoseconds ) to update meta file ( 60 seconds )
   #define STP_META_UPDATE_INTERVAL_NS ( STP_SEC_TO_NANOSEC( 60 ) )

   /*
      _stpMetaManager implement
    */
   BEGIN_OBJ_MSG_MAP( _stpMetaManager, _stpManagerBase )
      // ON_MSG
      ON_MSG( MSG_STP_META_NOTIFY, processMessage )
      ON_MSG( MSG_STP_META_SYNC_REQ, processMessage )
      ON_MSG( MSG_STP_META_SYNC_RSP, processMessage )
   END_OBJ_MSG_MAP()

   _stpMetaManager::_stpMetaManager( STPCB *stpCB )
   : stpManagerBase( stpCB ),
     _metaSyncTimeout( 0LL ),
     _timeMapMgr()
   {
   }

   _stpMetaManager::~_stpMetaManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_ONTIMER, "_stpMetaManager::onTimer" )
   void _stpMetaManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__STPMETAMGR_ONTIMER ) ;

      INT32 rc = SDB_OK ;

      // For each synchronize interval ( configured by --syncinterval )
      // - for primary server, update the LSN of meta data and broadcast the
      //   the meta data updating notification to secondary servers
      //   and it will check and save time mapping record between logical
      //   time and real time
      // - for secondary server, will send meta data synchronize request to
      //   primary server
      // For each second
      // - for primary server, double check if we need to save time mapping
      //   record

      if ( timerID == _timerID )
      {
         // check timeout to synchronize meta LSN
         _metaSyncTimeout += interval ;
         if ( _metaSyncTimeout >= _getMetaSyncInterval() )
         {
            if ( _stpCB->isPrimary() )
            {
               // if this is primary server, update meta LSN by
               // logical time
               rc = updateMetaLSN() ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to update meta LSN time, "
                          "rc: %d", rc ) ;
               }

               // broadcast the notify
               broadcastMetaNotify() ;
            }
            else if ( _stpCB->isSecondaryServer() )
            {
               // if this is secondary server, launch synchronize meta LSN
               rc = launchMetaSync() ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to launch synchronize meta, "
                          "rc: %d", rc ) ;
               }
            }

            // reset timeout
            _metaSyncTimeout = 0LL ;
         }
         else if ( _stpCB->isPrimary() &&
                   _timeMapMgr.isNeedSaveTimeMapping() )
         {
            // the time manager need save time mapping, which means it is
            // failed to save by last time, so we need to retry here
            _timeMapMgr.saveTimeMapping( getMetaData() ) ;
         }
      }

      PD_TRACE_EXIT( SDB__STPMETAMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_PROCESSMESSAGE, "_stpMetaManager::processMessage" )
   INT32 _stpMetaManager::processMessage( NET_HANDLE handle,
                                          MsgHeader * message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_STP_META_NOTIFY :
         {
            // handle meta notify
            rc = _handleMetaNotify( handle,
                                    (const stpMetaNotify *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize meta "
                         "notify, rc: %d", rc ) ;
            break ;
         }
         case MSG_STP_META_SYNC_REQ :
         {
            // handle meta synchronize request
            rc = _handleMetaSyncReq( handle,
                                     (const stpMetaSyncReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize meta "
                         "request, rc: %d", rc ) ;
            break ;
         }
         case MSG_STP_META_SYNC_RSP :
         {
            // handle meta synchronize response
            rc = _handleMetaSyncRsp( handle,
                                     (const stpMetaSyncRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize meta "
                         "response, rc: %d", rc ) ;
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown synchronize message [%d]",
                      message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__INITIALIZE, "_stpMetaManager::_initialize" )
   INT32 _stpMetaManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__INITIALIZE ) ;

      UINT32 syncInterval = _options->getSyncInterval() ;
      stpMetaData *metaData = NULL ;
      BOOLEAN created = FALSE ;

      // try to attach shared memory
      if ( !_buffer.attach( _options->getServiceName(),
                            stpMetaData::getBufferSize() ) )
      {
         // failed to attach, allocate new one
         PD_CHECK( _buffer.allocate( _options->getServiceName(),
                                     stpMetaData::getBufferSize() ),
                   SDB_OOM, error, PDERROR, "Failed to allocate shared memory "
                   "buffer [key: %s, size: %d]", _options->getServiceName(),
                   stpMetaData::getBufferSize() ) ;
         created = TRUE ;
      }

      // check if buffer is valid
      PD_CHECK( NULL != _buffer.getBuffer(), SDB_OOM, error, PDERROR,
                "Failed to allocate shared memory buffer [key: %s, size: %d], "
                "it is empty", _options->getServiceName(),
                stpMetaData::getBufferSize() ) ;

      PD_LOG( PDEVENT, "Allocate shared memory [key: %s, size: %d, id: %llu]",
              _buffer.getKeyString(), _buffer.getSize(), _buffer.getID() ) ;

      // if buffer is attached, try reuse the buffer first
      if ( !created )
      {
         // check if we could reuse the buffer ( check header and tailer )
         metaData = stpMetaData::getBuffer( _buffer.getBuffer() ) ;
      }
      // new buffer or reuse failed, reconstruct the meta data
      if ( NULL == metaData )
      {
         metaData = stpMetaData::newBuffer( _buffer.getBuffer() ) ;
      }

      // check if meta data is valid
      PD_CHECK( NULL != metaData, SDB_OOM, error, PDERROR,
                "Failed to allocate meta data" ) ;

      if ( STP_ROLE_SERVER == _options->getRole() )
      {
         // for testmode or server, try read meta file
         // initialize meta file
         rc = _store.initialize( _options->getStpPath() ) ;
         if ( SDB_FNE == rc )
         {
            // meta file is not found
            PD_LOG( PDINFO, "meta file [%s] does not exist",
                    _store.getMetaFileName() ) ;

            // initialize meta data with real time
            stpHPTime curHWTime( STP_SAMPLE_TIME_REAL ) ;
            metaData->initialize( curHWTime.toMicroSecond(), syncInterval ) ;

            // initialize meta LSN with default version
            rc = _setMetaLSN( metaData->getLTValueUS(), 0 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set meta LSN, rc: %d",
                         rc ) ;
         }
         else
         {
            // meta file exists, initialize meta data with meta file
            metaData->initialize( _store.getTime(), syncInterval ) ;
         }
      }
      else
      {
         // for client role, initialize meta data with real time
         stpHPTime curHWTime( STP_SAMPLE_TIME_REAL ) ;
         metaData->initialize( curHWTime.toMicroSecond(), syncInterval ) ;
      }

      // set meta data
      setMetaData( metaData ) ;

      // initialize time mapping manager
      rc = _timeMapMgr.initialize( _options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize time mapping "
                   "manager, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__FINALIZE, "_stpMetaManager::_finalize" )
   INT32 _stpMetaManager::_finalize()
   {
      PD_TRACE_ENTRY( SDB__STPMETAMGR__FINALIZE ) ;

      _timeMapMgr.finalize() ;

      // reset meta data
      setMetaData( NULL ) ;

      // release shared memory
      _buffer.release() ;

      PD_TRACE_EXITRC( SDB__STPMETAMGR__FINALIZE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__POSTACTIVATE, "_stpMetaManager::_postActivate" )
   INT32 _stpMetaManager::_postActivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__POSTACTIVATE ) ;

      // if this is primary ( only one server or test mode ),
      // update synchronize time to meta data
      // if this is not primary, it needs time to vote, so we don't launch meta
      // synchronize here, and let the time to launch meta synchronize
      if ( _stpCB->isPrimary() )
      {
         getMetaData()->updateSyncTime() ;
         _timeMapMgr.saveTimeMapping( getMetaData() ) ;
      }

      PD_TRACE_EXITRC( SDB__STPMETAMGR__POSTACTIVATE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__BEFORECHANGEPRIMARY, "_stpMetaManager::_beforeChangePrimary" )
   INT32 _stpMetaManager::_beforeChangePrimary( BOOLEAN primaryIsMe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__BEFORECHANGEPRIMARY ) ;

      if ( primaryIsMe )
      {
         // this node is primary now, update the meta to maximum of below times
         // - logical time push 60 second buffer
         // - time stored in meta file plus 60 second buffer
         // - current real time ( if current real time is larger )

         // get logical time
         UINT64 metaTime = getMetaData()->getLTValueUS() ;

         // get current real time
         stpHPTime curHWTime( STP_SAMPLE_TIME_REAL ) ;
         UINT64 curTime = curHWTime.toMicroSecond() ;

         UINT64 metaLSNTime = 0LL ;
         UINT32 metaLSNVersion = 0LL ;

         BOOLEAN updated = FALSE ;
         UINT64 maxTime = 0LL ;

         // get time from meta LSN
         rc = getMetaLSN( metaLSNTime, metaLSNVersion ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get meta LSN, rc: %d", rc ) ;

         // move forward with 60 seconds
         metaTime += STP_META_UPDATE_INTERVAL_NS ;
         metaLSNTime += STP_META_UPDATE_INTERVAL_NS ;

         // get maximum time
         maxTime = OSS_MAX( curTime, OSS_MAX( metaTime, metaLSNTime ) ) ;

         // reset time to meta data
         getMetaData()->reset( maxTime ) ;

         // update meta LSN ( increase version )
         rc = _updateMetaLSN( maxTime, TRUE, updated ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update meta data LSN, "
                      "rc: %d", rc ) ;
         if ( !updated )
         {
            PD_LOG( PDWARNING, "Failed to update meta data LSN, "
                    "it might be expired and ignored" ) ;
         }

         // save time mapping
         _timeMapMgr.saveTimeMapping( getMetaData() ) ;
      }

      // otherwise the node is becoming secondary, do nothing

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__BEFORECHANGEPRIMARY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__HANDLEMETANOTIFY, "_stpMetaManager::_handleMetaNotify" )
   INT32 _stpMetaManager::_handleMetaNotify( NET_HANDLE handle,
                                             const stpMetaNotify *notify )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__HANDLEMETANOTIFY ) ;

      SDB_ASSERT( NULL != notify, "notify message is invalid" ) ;
      SDB_ASSERT( MSG_STP_META_NOTIFY == notify->header.opCode,
                  "opcode of message is invalid" ) ;

      // handle meta notify only in secondary server
      if ( _stpCB->isSecondaryServer() )
      {
         // launch meta synchronize
         rc = launchMetaSync() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to launch meta data synchronize, "
                      "rc: %d", rc ) ;

         // reset timeout
         _metaSyncTimeout = 0LL ;
      }
      else
      {
         PD_LOG( PDWARNING, "Ignore handle synchronize meta notify, "
                 "I am primary" ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__HANDLEMETANOTIFY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__HANDLEMETASYNCREQ, "_stpMetaManager::_handleMetaSyncReq" )
   INT32 _stpMetaManager::_handleMetaSyncReq( NET_HANDLE handle,
                                              const stpMetaSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__HANDLEMETASYNCREQ ) ;

      SDB_ASSERT( MSG_STP_META_SYNC_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      UINT64 time = 0LL ;
      UINT32 version = 0 ;

      // only handle meta synchronize request in primary server
      PD_CHECK( _stpCB->isPrimaryServer(),
                SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to handle synchronize meta request, "
                "primary server is not me" ) ;

      // get meta LSN
      rc = getMetaLSN( time, version ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get meta LSN, rc: %d", rc ) ;

      // send meta synchronize response
      rc = _sendMetaSyncRsp( handle, request, time, version, SDB_OK ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send synchronize metadata response, "
                 "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__HANDLEMETASYNCREQ, rc ) ;
      return rc ;

   error:
      // send response with error
      _sendMetaSyncRsp( handle, request, time, version, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__HANDLEMETASYNCRSP, "_stpMetaManager::_handleMetaSyncRsp" )
   INT32 _stpMetaManager::_handleMetaSyncRsp( NET_HANDLE handle,
                                              const stpMetaSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__HANDLEMETASYNCRSP ) ;

      SDB_ASSERT( MSG_STP_META_SYNC_RSP == response->reply.header.opCode,
                  "opcode of message is invalid" ) ;

      // get return code from response
      rc = response->reply.res ;

      if ( SDB_OK == rc )
      {
         // if response is OK, set meta LSN with response
         DPS_LSN metaLSN ;
         // fill meta LSN
         metaLSN.set( response->time, response->version ) ;
         // update meta LSN
         rc = updateMetaLSN( metaLSN ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update meta LSN, "
                      "rc: %d", rc ) ;
      }
      else
      {
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            // remote is not primary, reset primary to node manager
            getNodeManager()->resetPrimaryOnError(
                                    response->reply.header.routeID ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to synchronize meta, "
                      "received response with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__HANDLEMETASYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__SENDMETANOTIFY, "_stpMetaManager::_sendMetaNotify" )
   INT32 _stpMetaManager::_sendMetaNotify( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__SENDMETANOTIFY ) ;

      stpMetaNotify notify ;

      // fill request header
      _netMsgHandler->fillRequestHeader( notify.header,
                                         sizeof( stpMetaNotify ),
                                         MSG_STP_META_NOTIFY ) ;

      // send notify by net agent
      rc = _netAgent->syncSend( routeID, (MsgHeader *)&notify ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta "
                   "notify to %s, rc: %d", routeID2String( routeID ).c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__SENDMETANOTIFY, rc ) ;
      return rc ;

   error:
      goto done ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__SENDMETASYNCREQ, "_stpMetaManager::_sendMetaSyncReq" )
   INT32 _stpMetaManager::_sendMetaSyncReq( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__SENDMETASYNCREQ ) ;

      stpMetaSyncReq request ;

      // fill request header
      _netMsgHandler->fillRequestHeader( request.header,
                                         sizeof( stpMetaSyncReq ),
                                         MSG_STP_META_SYNC_REQ ) ;

      // send request by net agent
      rc = _netAgent->syncSend( routeID, (MsgHeader *)&request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta "
                   "request to %s, rc: %d", routeID2String( routeID ).c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__SENDMETASYNCREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__SENDMETASYNCRSP, "_stpMetaManager::_sendMetaSyncRsp" )
   INT32 _stpMetaManager::_sendMetaSyncRsp( NET_HANDLE handle,
                                            const stpMetaSyncReq *request,
                                            UINT64 time,
                                            UINT32 version,
                                            INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__SENDMETASYNCRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      stpMetaSyncRsp response ;

      // fill reply header
      _netMsgHandler->fillReplyHeader( request->header,
                                       response.reply,
                                       sizeof( stpMetaSyncRsp ),
                                       returnCode ) ;

      // fill meta LSN fields
      response.time = time ;
      response.version = version ;

      // send reply by net agent
      rc = _netAgent->syncSend( handle, (MsgHeader *)&response ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta result, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__SENDMETASYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_LAUNCHMETASYNC, "_stpMetaManager::launchMetaSync" )
   INT32 _stpMetaManager::launchMetaSync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_LAUNCHMETASYNC ) ;

      MsgRouteID primaryRID ;

      // get route ID of primary
      rc = _session.getPrimaryRID( primaryRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get primary route ID, "
                   "rc: %d", rc ) ;

      // send meta synchronize to primary
      rc = _sendMetaSyncReq( primaryRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize meta "
                   "request, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR_LAUNCHMETASYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_BROADCASTMETANOTIFY, "_stpMetaManager::broadcastMetaNotify" )
   INT32 _stpMetaManager::broadcastMetaNotify()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_BROADCASTMETANOTIFY ) ;

      STP_SERVER_LIST servers ;

      // get route ID of local node
      MsgRouteID localRID = getNodeManager()->getLocalRID() ;

      // get all servers
      rc = getNodeManager()->dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      // send notify to each server except for local node
      for ( STP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         // check route ID with local route ID
         if ( iter->getRouteIDValue() != localRID.value )
         {
            _sendMetaNotify( iter->getRouteID() ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR_BROADCASTMETANOTIFY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_GETMETALSN, "_stpMetaManager::getMetaLSN" )
   INT32 _stpMetaManager::getMetaLSN( UINT64 &time, UINT32 &version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_GETMETALSN ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      // get meta LSN from meta file
      _store.getLSN( time, version ) ;

      PD_TRACE_EXITRC( SDB__STPMETAMGR_GETMETALSN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_GETMETALSN_LSN, "_stpMetaManager::getMetaLSN" )
   INT32 _stpMetaManager::getMetaLSN( DPS_LSN &lsn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_GETMETALSN_LSN ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      // get meta LSN from meta file
      _store.getLSN( lsn ) ;

      PD_TRACE_EXITRC( SDB__STPMETAMGR_GETMETALSN_LSN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_UPDATEMETALSN_LSN, "_stpMetaManager::updateMetaLSN" )
   INT32 _stpMetaManager::updateMetaLSN( const DPS_LSN &metaLSN )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_UPDATEMETALSN_LSN ) ;

      BOOLEAN updated = FALSE ;

      // only secondary could update meta file in LSN format
      // LSN is sent by primary
      if ( _stpCB->isPrimary() )
      {
         PD_LOG( PDWARNING, "Ignore update meta LSN, I am primary" ) ;
         goto done ;
      }

      // check if meta LSN is valid
      PD_CHECK( !metaLSN.invalid(), SDB_CLS_SYNC_FAILED, error,
                PDWARNING, "Failed to update meta LSN, "
                "given meta LSN is invalid" ) ;

      // update meta LSN to meta file
      rc = _updateMetaLSN( metaLSN, updated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update meta LSN, rc: %d",
                   rc ) ;
      // check if meta file is updated
      PD_CHECK( updated, SDB_CLS_SYNC_FAILED, error, PDERROR,
                "Failed to update meta LSN, given LSN "
                "[time %llu, version %u] is ignored", metaLSN.offset,
                metaLSN.version ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR_UPDATEMETALSN_LSN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_UPDATEMETALSN, "_stpMetaManager::updateMetaLSN" )
   INT32 _stpMetaManager::updateMetaLSN()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_UPDATEMETALSN ) ;

      BOOLEAN updated = FALSE ;
      UINT64 time = 0LL ;

      // only primary could update meta itself
      if ( !_stpCB->isPrimary() )
      {
         PD_LOG( PDWARNING, "Ignore update meta LSN, this is not primary" ) ;
         goto done ;
      }

      // set time map manager need save time mapping to avoid failure to
      // save LSN, and if failed unfortunately, the next timeout event will
      // retry the saving
      _timeMapMgr.setNeedSaveTimeMapping( TRUE ) ;

      // get logical time as meta LSN offset
      time = getMetaData()->getLTValueUS() ;

      // update meta LSN ( no need to increase version )
      rc = _updateMetaLSN( time, FALSE, updated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update meta LSN time, "
                   "rc: %d", rc ) ;
      // check if meta LSN is updated
      PD_CHECK( updated, SDB_SYS, error, PDERROR,
                "Failed to update meta LSN, given time [%llu] is ignored",
                time ) ;

      // save time mapping, ignore errors, since we already set need save
      // flag to true, the next timeout event will retry the saving
      _timeMapMgr.saveTimeMapping( getMetaData() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR_UPDATEMETALSN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__SETMETALSN, "_stpMetaManager::_setMetaLSN" )
   INT32 _stpMetaManager::_setMetaLSN( UINT64 time, UINT32 version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__SETMETALSN ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      // set meta LSN to meta file
      _store.setLSN( time, version ) ;

      // save meta file
      rc = _store.save() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save meta, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Set meta LSN [ version %u time %llu ]",
              _store.getVersion(), _store.getTime() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__SETMETALSN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__UPDATEMETALSN, "_stpMetaManager::_updateMetaLSN" )
   INT32 _stpMetaManager::_updateMetaLSN( UINT64 time,
                                          BOOLEAN increaseVersion,
                                          BOOLEAN &updated )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__UPDATEMETALSN ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      // get current time and version of meta file
      UINT64 currentTime = _store.getTime() ;
      UINT32 currentVersion = _store.getVersion () ;

      // NOTE: when update meta LSN, we must updated with either larger time
      //       or larger version
      if ( increaseVersion )
      {
         // need increase version, so no need to compare times
         // NOTE: version has higher priority than time ( offset )
         _store.setTime( time ) ;
         _store.increaseVersion() ;
         updated = TRUE ;
      }
      else if ( currentTime < time )
      {
         // update time if given time is larger
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
         // no time synchronize on primary server, so update synchronize
         // time of meta data when meta LSN updated
         // NOTE:
         //    - update the synchronize time to tell STP agents, this STP node
         //      is alive and available to acquire logical time
         //    - saving into file may take a while, so update synchronize
         //      time before flushing to file to avoid blocking the STP agents
         //      to acquire logical time
         getMetaData()->updateSyncTime() ;

         // if updated, save to meta file
         rc = _store.save() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save meta, rc: %d", rc ) ;

         PD_LOG( PDEVENT, "Update meta LSN [ version %u time %llu ]",
                 _store.getVersion(), _store.getTime() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__UPDATEMETALSN, rc ) ;
      return rc ;

   error:
      // rollback to previous version
      updated = FALSE ;
      _store.setLSN( currentTime, currentVersion ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR__UPDATEMETALSN_LSN, "_stpMetaManager::_updateMetaLSN" )
   INT32 _stpMetaManager::_updateMetaLSN( const DPS_LSN &metaLSN,
                                          BOOLEAN &updated )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR__UPDATEMETALSN_LSN ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      DPS_LSN currentLSN ;

      // get current meta LSN
      _store.getLSN( currentLSN ) ;

      // we only update larger LSN
      if ( currentLSN.compare( metaLSN ) < 0 )
      {
         // set LSN is given is larger
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
         // if updated, save to meta file
         rc = _store.save() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save meta, rc: %d", rc ) ;

         PD_LOG( PDEVENT, "Update meta LSN [ version %u time %llu ]",
                 _store.getVersion(), _store.getTime() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR__UPDATEMETALSN_LSN, rc ) ;
      return rc ;

   error:
      // rollback to previous LSN
      updated = FALSE ;
      _store.setLSN( currentLSN ) ;
      goto done ;
   }

   UINT64 _stpMetaManager::_getMetaSyncInterval()
   {
      // return synchronize interval as meta synchronize interval
      return (UINT64)( _options->getSyncInterval() ) * OSS_ONE_SEC ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_CONVRTTOLT, "_stpMetaManager::convRTimeToLTime" )
   INT32 _stpMetaManager::convRTimeToLTime( const stpHPTime &realTime,
                                            stpHPTime &logicalTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_CONVRTTOLT ) ;

      rc = _timeMapMgr.convRTimeToLTime( realTime, logicalTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to convert real time [%llu] to "
                   "logical time, rc: %d", realTime.toMicroSecond(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR_CONVRTTOLT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETAMGR_CONVLTTORT, "_stpMetaManager::convLTimeToRTime" )
   INT32 _stpMetaManager::convLTimeToRTime( const stpHPTime &logicalTime,
                                            stpHPTime &realTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETAMGR_CONVLTTORT ) ;

      rc = _timeMapMgr.convLTimeToRTime( logicalTime, realTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to convert logical time [%llu] to "
                   "real time, rc: %d", logicalTime.toMicroSecond(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETAMGR_CONVLTTORT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
