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

   Source File Name = tpModule.cpp

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

#include "tpModule.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"

namespace engine
{

   #define TP_MODULE_ATTACH_TIMEOUT ( 60 * OSS_ONE_SEC )

   /*
      _tpModule implement
    */
   _tpModule::_tpModule( SDB_TPCB *tpCB )
   : _tpCB( tpCB ),
     _options( tpCB->getOptions() ),
     _netAgent( tpCB->getNetAgent() ),
     _pipeManager( tpCB->getPipeManager() ),
     _initialized( FALSE ),
     _activated( FALSE )
   {
      SDB_ASSERT( NULL != tpCB, "tpCB is invalid" ) ;
   }

   _tpModule::~_tpModule()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMODULE_INITIALIZE, "_tpModule::initialize" )
   INT32 _tpModule::initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMODULE_INITIALIZE ) ;

      if ( _initialized )
      {
         PD_LOG( PDEVENT, "[%s] is already initialized", getModuleName() ) ;
         goto done ;
      }

      rc = _initialize() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] initialize, "
                   "rc: %d", getModuleName(), rc ) ;

      _initialized = TRUE ;

      PD_LOG( PDEVENT, "[%s] is initialized", getModuleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMODULE_INITIALIZE, rc ) ;
      return rc ;

   error:
      _initialized = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMODULE_FINALIZE, "_tpModule::finalize" )
   INT32 _tpModule::finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMODULE_FINALIZE ) ;

      _finalize() ;
      _initialized = FALSE ;

      PD_LOG( PDEVENT, "[%s] is finalized", getModuleName() ) ;

      PD_TRACE_EXITRC( SDB__TPMODULE_FINALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMODULE_ACTIVATE, "_tpModule::activate" )
   INT32 _tpModule::activate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMODULE_ACTIVATE ) ;

      if ( _activated )
      {
         PD_LOG( PDEVENT, "[%s] is already activated", getModuleName() ) ;
         goto done ;
      }

      rc = _preActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] prepare activate, "
                   "rc: %d", getModuleName(), rc ) ;

      _activated = TRUE ;

      rc = _postActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] post activate, "
                   "rc: %d", getModuleName(), rc ) ;

      PD_LOG( PDEVENT, "[%s] is activated", getModuleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMODULE_ACTIVATE, rc ) ;
      return rc ;

   error:
      _activated = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMODULE_DEACTIVE, "_tpModule::deactivate" )
   INT32 _tpModule::deactivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMODULE_DEACTIVE ) ;

      _preDeactivate() ;
      _activated = FALSE ;
      _postDeactivate() ;

      PD_LOG( PDEVENT, "[%s] is deactivated", getModuleName() ) ;

      PD_TRACE_EXITRC( SDB__TPMODULE_DEACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMODULE_ONCHANGEPRIMARY, "_tpModule::onChangePrimary" )
   INT32 _tpModule::onChangePrimary( const MsgRouteID &primaryRID,
                                     BOOLEAN isLocalPrimary )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMODULE_ONCHANGEPRIMARY ) ;

      if ( _activated )
      {
         rc = _onChangePrimary( primaryRID, isLocalPrimary ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle change primary event "
                      "in [%s], rc: %d", getModuleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMODULE_ONCHANGEPRIMARY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMODULE_ONCHANGESERVERS, "_tpModule::onChangeServers" )
   INT32 _tpModule::onChangeServers( UINT32 version,
                                     const TP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMODULE_ONCHANGESERVERS ) ;

      if ( _activated )
      {
         rc = _onChangeServers( version, servers ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle change servers event "
                      "in [%s], rc: %d", getModuleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMODULE_ONCHANGESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpHandlerBase implement
    */
   _tpHandlerBase::_tpHandlerBase( SDB_TPCB *tpCB )
   : tpModule( tpCB ),
     _serviceManager( tpCB->getServiceManager() ),
     _catalogManager( tpCB->getCatalogManager() ),
     _metaManager( tpCB->getMetaManager() ),
     _sourceManager( tpCB->getSourceManager() ),
     _syncManager( tpCB->getSyncManager() ),
     _replManager( tpCB->getReplManager() )
   {
   }

   _tpHandlerBase::~_tpHandlerBase()
   {
   }

   /*
      _tpManagerBase implement
    */
   _tpManagerBase::_tpManagerBase( SDB_TPCB *tpCB )
   : tpHandlerBase( tpCB ),
     tpMetaHolder(),
     netTimeoutHandler(),
     _eduID( PMD_INVALID_EDUID ),
     _eduCB( NULL ),
     _session( tpCB ),
     _netMsgHandler( tpCB->getNetMsgHandler() ),
     _pipeMsgHandler( tpCB->getPipeMsgHandler() ),
     _timerID( TP_INVALID_TIMERID )
   {
   }

   _tpManagerBase::~_tpManagerBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE_ACTIVE, "_tpManagerBase::activate" )
   INT32 _tpManagerBase::activate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE_ACTIVE ) ;

      if ( _activated )
      {
         PD_LOG( PDEVENT, "[%s] is already activated", getModuleName() ) ;
         goto done ;
      }

      if ( NULL == getMetaData() )
      {
         setMetaData( _tpCB->getMetaData() ) ;
      }

      rc = _preActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] prepare activate, rc: %d",
                   getModuleName(), rc ) ;

      if ( activeEDU() )
      {
         rc = _startEDU() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start EDU for [%s], rc: %d",
                      getModuleName(), rc ) ;
      }

      if ( activeTimer() )
      {
         rc = _registerTimer() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register timer for [%s], rc: %d",
                      getModuleName(), rc ) ;
      }

      _activated = TRUE ;

      rc = _postActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] post activate, rc: %d",
                   getModuleName(), rc ) ;

      PD_LOG( PDEVENT, "[%s] is activated", getModuleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMGRBASE_ACTIVE, rc ) ;
      return rc ;

   error:
      _activated = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE_DEACTIVE, "_tpManagerBase::deactivate" )
   INT32 _tpManagerBase::deactivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE_DEACTIVE ) ;

      setMetaData( NULL ) ;
      _unregisterTimer() ;
      _stopEDU() ;

      _preDeactivate() ;
      _activated = FALSE ;
      _postDeactivate() ;

      PD_LOG( PDEVENT, "[%s] is deactivated", getModuleName() ) ;

      PD_TRACE_EXITRC( SDB__TPMGRBASE_DEACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE_ATTACHCB, "_tpManagerBase::attachCB" )
   void _tpManagerBase::attachCB( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__TPMGRBASE_ATTACHCB ) ;

      ossScopedLock lock( &_eduLatch, EXCLUSIVE ) ;
      _eduCB = cb ;

      _attachEvent.signalAll() ;

      PD_TRACE_EXIT( SDB__TPMGRBASE_ATTACHCB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE_DETACHCB, "_tpManagerBase::detachCB" )
   void _tpManagerBase::detachCB( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__TPMGRBASE_DETACHCB ) ;

      ossScopedLock lock( &_eduLatch, EXCLUSIVE ) ;
      _eduCB = NULL ;

      PD_TRACE_EXIT( SDB__TPMGRBASE_DETACHCB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE_HANDLETIMEOUT, "_tpManagerBase::handleTimeout" )
   void _tpManagerBase::handleTimeout( const UINT32 &millisec,
                                       const UINT32 &timerID )
   {
      PD_TRACE_ENTRY( SDB__TPMGRBASE_HANDLETIMEOUT ) ;

      BOOLEAN handled = FALSE ;
      if ( PMD_INVALID_EDUID != _eduID )
      {
         handled = _asyncHandleTimeout( timerID, millisec ) ;
      }
      if ( !handled )
      {
         onTimer( timerID, millisec ) ;
      }

      PD_TRACE_EXIT( SDB__TPMGRBASE_HANDLETIMEOUT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE_HANDLEMESSAGE, "_tpManagerBase::handleMessage" )
   INT32 _tpManagerBase::handleMessage( NET_HANDLE handle,
                                         const MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE_HANDLEMESSAGE ) ;

      BOOLEAN handled = FALSE ;
      if ( PMD_INVALID_EDUID != _eduID )
      {
         handled = _asyncHandleMessage( handle, message ) ;
      }
      if ( !handled )
      {
         rc = processMessage( handle, const_cast< MsgHeader * >( message ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process message %s, rc: %d",
                      msg2String( message ).c_str(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMGRBASE_HANDLEMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE_PROCESSMESSAGE, "_tpManagerBase::processMessage" )
   INT32 _tpManagerBase::processMessage( NET_HANDLE handle,
                                         MsgHeader *message )
   {
      INT32 rc = SDB_UNKNOWN_MESSAGE ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE_PROCESSMESSAGE ) ;

      PD_TRACE_EXITRC( SDB__TPMGRBASE_PROCESSMESSAGE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__STARTEDU, "_tpManagerBase::_startEDU" )
   INT32 _tpManagerBase::_startEDU()
   {
      INT32 rc = SDB_OK ;


      PD_TRACE_ENTRY( SDB__TPMGRBASE__STARTEDU ) ;

      EDUID eduID = PMD_INVALID_EDUID ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;

      _stopEDU() ;

      rc = eduMgr->startEDU( EDU_TYPE_TP_MODULE, (void *)this, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start EDU for [%s], rc: %d",
                   getModuleName(), rc ) ;

      rc = _attachEvent.wait( TP_MODULE_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait EDU attached for [%s], "
                   "rc: %d", getModuleName(), rc ) ;

      _eduID = eduID ;

   done:
      PD_TRACE_EXITRC( SDB__TPMGRBASE__STARTEDU, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__STOPEDU, "_tpManagerBase::_stopEDU" )
   INT32 _tpManagerBase::_stopEDU()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE__STOPEDU ) ;

      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      if ( !eduMgr->isDestroyed() && PMD_INVALID_EDUID != _eduID )
      {
         EDUID eduID = _eduID ;
         _eduID = PMD_INVALID_EDUID ;
         eduMgr->forceUserEDU( eduID ) ;
      }

      PD_TRACE_EXITRC( SDB__TPMGRBASE__STOPEDU, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__REGTIMER, "_tpManagerBase::_registerTimer" )
   INT32 _tpManagerBase::_registerTimer()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE__REGTIMER ) ;

      _unregisterTimer() ;

      UINT32 timerID = TP_INVALID_TIMERID ;
      rc = _netAgent->addTimer( OSS_ONE_SEC, this, timerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add synchronize timer, "
                   "rc: %d", rc ) ;

      _timerID = timerID ;

      PD_LOG( PDEVENT, "Register [%s] synchronize timer [%llu]",
              getModuleName(), _timerID ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMGRBASE__REGTIMER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__UNREGTIMER, "_tpManagerBase::_unregisterTimer" )
   INT32 _tpManagerBase::_unregisterTimer()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE__UNREGTIMER ) ;

      if ( TP_INVALID_TIMERID != _timerID )
      {
         _netAgent->removeTimer( _timerID ) ;
         _timerID = TP_INVALID_TIMERID ;
      }

      PD_TRACE_EXITRC( SDB__TPMGRBASE__UNREGTIMER, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__ASYNCHANDLETIMEOUT, "_tpManagerBase::_asyncHandleTimeout" )
   BOOLEAN _tpManagerBase::_asyncHandleTimeout( const UINT32 &timerID,
                                                const UINT32 &millisec )
   {
      BOOLEAN handled = FALSE ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE__ASYNCHANDLETIMEOUT ) ;

      PMD_EVENT_MESSAGES *event = NULL ;
      ossTimestamp ts ;

      ossScopedLock lock( &_eduLatch, SHARED ) ;

      if ( NULL == _eduCB || _eduCB->isInterrupted() )
      {
         goto done ;
      }

      event = (PMD_EVENT_MESSAGES *)SDB_THREAD_ALLOC(
                                             sizeof( PMD_EVENT_MESSAGES ) ) ;
      if ( NULL == event )
      {
         PD_LOG ( PDWARNING, "Failed to allocate memory for PDM "
                  "timeout Event for %d bytes",
                  sizeof( PMD_EVENT_MESSAGES ) ) ;
         goto done ;
      }

      ossGetCurrentTime( ts ) ;

      event->timeoutMsg.timerID = timerID ;
      event->timeoutMsg.interval = millisec ;
      event->timeoutMsg.occurTime = ts.time ;

      // post timeout
      _eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_TIMEOUT,
                                      PMD_EDU_MEM_THREAD,
                                      (void *)event ) ) ;
      handled = TRUE ;

   done:
      PD_TRACE_EXIT( SDB__TPMGRBASE__ASYNCHANDLETIMEOUT ) ;
      return handled ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__ASYNCHANDLEMESSAGE, "_tpManagerBase::_asyncHandleMessage" )
   BOOLEAN _tpManagerBase::_asyncHandleMessage( NET_HANDLE handle,
                                                const MsgHeader *message )
   {
      BOOLEAN handled = FALSE ;

      PD_TRACE_ENTRY( SDB__TPMGRBASE__ASYNCHANDLEMESSAGE ) ;

      CHAR *buffer = NULL ;

      ossScopedLock lock( &_eduLatch, SHARED ) ;
      if ( NULL == _eduCB || _eduCB->isInterrupted() )
      {
         goto done ;
      }

      buffer = (CHAR *)SDB_THREAD_ALLOC( message->messageLength + 1 ) ;
      if ( NULL == buffer )
      {
         PD_LOG ( PDWARNING, "Failed to allocate memory for PDM "
                  "message Event for %d bytes",
                  message->messageLength + 1 ) ;
         goto done ;
      }

      // copy data
      ossMemcpy( buffer, message, message->messageLength ) ;
      buffer[ message->messageLength ] = 0 ;

      // post message
      _eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                      PMD_EDU_MEM_THREAD,
                                      (void *)buffer,
                                      (UINT64)handle ) ) ;
      handled = TRUE ;

   done:
      PD_TRACE_EXIT( SDB__TPMGRBASE__ASYNCHANDLEMESSAGE ) ;
      return handled ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__FILLREQHEADER, "_tpManagerBase::_fillRequestHeader" )
   void _tpManagerBase::_fillRequestHeader( MsgHeader &request,
                                            UINT32 requestSize,
                                            INT32 opCode )
   {
      PD_TRACE_ENTRY( SDB__TPMGRBASE__FILLREQHEADER ) ;

      MsgRouteID localRID = _catalogManager->getLocalRID() ;

      request.messageLength = requestSize ;
      request.opCode = opCode ;
      request.TID = 0 ;
      request.routeID.value = localRID.value ;
      request.requestID = _netMsgHandler->allocateRequestID() ;

      PD_TRACE_EXIT( SDB__TPMGRBASE__FILLREQHEADER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__FILLREPHEADER, "_tpManagerBase::_fillReplyHeader" )
   void _tpManagerBase::_fillReplyHeader( const MsgHeader &request,
                                          MsgOpReply &reply,
                                          UINT32 replySize,
                                          INT32 returnCode )
   {
      PD_TRACE_ENTRY( SDB__TPMGRBASE__FILLREPHEADER ) ;

      MsgRouteID localRID = _catalogManager->getLocalRID() ;

      reply.header.messageLength = replySize ;
      reply.header.opCode = MAKE_REPLY_TYPE( request.opCode ) ;
      reply.header.TID = 0 ;
      reply.header.routeID.value = localRID.value ;
      reply.header.requestID = request.requestID ;
      reply.contextID = -1 ;
      reply.flags = returnCode ;
      reply.startFrom = 0 ;
      reply.numReturned = 1 ;

      PD_TRACE_EXIT( SDB__TPMGRBASE__FILLREPHEADER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMGRBASE__FILLREPHEADER_INT, "_tpManagerBase::_fillReplyHeader" )
   void _tpManagerBase::_fillReplyHeader( const MsgHeader &request,
                                          MsgInternalReplyHeader &reply,
                                          UINT32 replySize,
                                          INT32 returnCode )
   {
      PD_TRACE_ENTRY( SDB__TPMGRBASE__FILLREPHEADER_INT ) ;

      MsgRouteID localRID = _catalogManager->getLocalRID() ;

      reply.header.messageLength = replySize ;
      reply.header.opCode = MAKE_REPLY_TYPE( request.opCode ) ;
      reply.header.TID = 0 ;
      reply.header.routeID.value = localRID.value ;
      reply.header.requestID = request.requestID ;
      reply.res = returnCode ;

      PD_TRACE_EXIT( SDB__TPMGRBASE__FILLREPHEADER_INT ) ;
   }

}
