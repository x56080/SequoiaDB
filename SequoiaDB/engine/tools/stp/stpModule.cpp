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

   Source File Name = stpModule.cpp

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
#include "stpModule.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"

namespace engine
{

   // timeout for attaching EDU for module
   #define STP_MODULE_ATTACH_TIMEOUT ( 60 * OSS_ONE_SEC )

   /*
      _stpModule implement
    */
   _stpModule::_stpModule( STPCB *stpCB )
   : _stpCB( stpCB ),
     _options( stpCB->getOptions() ),
     _netAgent( stpCB->getNetAgent() ),
     _pipeManager( stpCB->getPipeManager() ),
     _initialized( FALSE ),
     _activated( FALSE )
   {
      SDB_ASSERT( NULL != stpCB, "stpCB is invalid" ) ;
   }

   _stpModule::~_stpModule()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMODULE_INITIALIZE, "_stpModule::initialize" )
   INT32 _stpModule::initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMODULE_INITIALIZE ) ;

      // ignore re-initialize
      if ( _initialized )
      {
         PD_LOG( PDEVENT, "[%s] is already initialized", getModuleName() ) ;
         goto done ;
      }

      // set net agent
      _netAgent = _stpCB->getNetAgent() ;
      PD_CHECK( NULL != _netAgent, SDB_SYS, error, PDERROR,
                "Failed to initialize [%s], net agent is invalid",
                getModuleName() ) ;

      // call internal initialize
      rc = _initialize() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] initialize, "
                   "rc: %d", getModuleName(), rc ) ;

      // set module initialized
      _initialized = TRUE ;

      PD_LOG( PDEVENT, "[%s] is initialized", getModuleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMODULE_INITIALIZE, rc ) ;
      return rc ;

   error:
      // error happened, set module uninitialized
      _initialized = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMODULE_FINALIZE, "_stpModule::finalize" )
   INT32 _stpModule::finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMODULE_FINALIZE ) ;

      // call internal finalize
      _finalize() ;

      // set module uninitialized
      _initialized = FALSE ;

      PD_LOG( PDEVENT, "[%s] is finalized", getModuleName() ) ;

      PD_TRACE_EXITRC( SDB__STPMODULE_FINALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMODULE_ACTIVATE, "_stpModule::activate" )
   INT32 _stpModule::activate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMODULE_ACTIVATE ) ;

      // ignore re-activate
      if ( _activated )
      {
         PD_LOG( PDEVENT, "[%s] is already activated", getModuleName() ) ;
         goto done ;
      }

      // call internal event before activate
      rc = _preActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] prepare activate, "
                   "rc: %d", getModuleName(), rc ) ;

      // set module activated
      _activated = TRUE ;

      // call internal event after activate
      rc = _postActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] post activate, "
                   "rc: %d", getModuleName(), rc ) ;

      PD_LOG( PDEVENT, "[%s] is activated", getModuleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMODULE_ACTIVATE, rc ) ;
      return rc ;

   error:
      // on error happened, set module deactivated
      _activated = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMODULE_DEACTIVE, "_stpModule::deactivate" )
   INT32 _stpModule::deactivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMODULE_DEACTIVE ) ;

      // call internal event before deactivate
      _preDeactivate() ;

      // set module deactivated
      _activated = FALSE ;

      // call internal event after deactivate
      _postDeactivate() ;

      PD_LOG( PDEVENT, "[%s] is deactivated", getModuleName() ) ;

      PD_TRACE_EXITRC( SDB__STPMODULE_DEACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMODULE_BEFORECHANGEPRIMARY, "_stpModule::beforeChangePrimary" )
   INT32 _stpModule::beforeChangePrimary( BOOLEAN primaryIsMe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMODULE_BEFORECHANGEPRIMARY ) ;

      // only process when module is activated
      if ( _activated )
      {
         // call internal event of primary change
         rc = _beforeChangePrimary( primaryIsMe ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle before change primary "
                      "event in [%s], rc: %d", getModuleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMODULE_BEFORECHANGEPRIMARY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMODULE_AFTERCHANGEPRIMARY, "_stpModule::afterChangePrimary" )
   INT32 _stpModule::afterChangePrimary( const MsgRouteID &primaryRID,
                                         BOOLEAN primaryIsMe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMODULE_AFTERCHANGEPRIMARY ) ;

      // only process when module is activated
      if ( _activated )
      {
         // call internal event of primary change
         rc = _afterChangePrimary( primaryRID, primaryIsMe ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle after change primary "
                      "event in [%s], rc: %d", getModuleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMODULE_AFTERCHANGEPRIMARY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMODULE_ONCHANGESERVERS, "_stpModule::onChangeServers" )
   INT32 _stpModule::onChangeServers( UINT32 version,
                                      const STP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMODULE_ONCHANGESERVERS ) ;

      // only process when module is activated
      if ( _activated )
      {
         // call internal event of servers change
         rc = _onChangeServers( version, servers ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle change servers event "
                      "in [%s], rc: %d", getModuleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMODULE_ONCHANGESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpHandlerBase implement
    */
   _stpHandlerBase::_stpHandlerBase( STPCB *stpCB )
   : stpModule( stpCB ),
     _serviceManager( stpCB->getServiceManager() ),
     _nodeManager( stpCB->getNodeManager() ),
     _metaManager( stpCB->getMetaManager() ),
     _syncSourceManager( stpCB->getSyncSourceManager() ),
     _syncClientManager( stpCB->getSyncClientManager() ),
     _replManager( stpCB->getReplManager() )
   {
   }

   _stpHandlerBase::~_stpHandlerBase()
   {
   }

   /*
      _stpManagerBase implement
    */
   _stpManagerBase::_stpManagerBase( STPCB *stpCB )
   : stpHandlerBase( stpCB ),
     stpMetaHolder(),
     netTimeoutHandler(),
     _eduID( PMD_INVALID_EDUID ),
     _eduCB( NULL ),
     _session( stpCB ),
     _netManager( stpCB->getNetManager() ),
     _netMsgHandler( stpCB->getNetMsgHandler() ),
     _pipeMsgHandler( stpCB->getPipeMsgHandler() ),
     _timerID( STP_INVALID_TIMERID )
   {
   }

   _stpManagerBase::~_stpManagerBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE_ACTIVE, "_stpManagerBase::activate" )
   INT32 _stpManagerBase::activate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE_ACTIVE ) ;

      // to avoid re-activated
      if ( _activated )
      {
         PD_LOG( PDEVENT, "[%s] is already activated", getModuleName() ) ;
         goto done ;
      }

      // if meta data is not set, set meta data
      if ( NULL == getMetaData() )
      {
         setMetaData( _stpCB->getMetaData() ) ;
      }

      // on event before activate
      rc = _preActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] prepare activate, rc: %d",
                   getModuleName(), rc ) ;

      // if EDU is needed, start the EDU
      if ( activeEDU() )
      {
         rc = _startEDU() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start EDU for [%s], rc: %d",
                      getModuleName(), rc ) ;
      }

      // if timer is needed, register timer
      if ( activeTimer() )
      {
         rc = _registerTimer() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register timer for [%s], rc: %d",
                      getModuleName(), rc ) ;
      }

      // set module is activated
      _activated = TRUE ;

      // on event after activate
      rc = _postActivate() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] post activate, rc: %d",
                   getModuleName(), rc ) ;

      PD_LOG( PDEVENT, "[%s] is activated", getModuleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMGRBASE_ACTIVE, rc ) ;
      return rc ;

   error:
      // on error happens, set module deactivated
      _activated = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE_DEACTIVE, "_stpManagerBase::deactivate" )
   INT32 _stpManagerBase::deactivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE_DEACTIVE ) ;

      // reset meta data
      setMetaData( NULL ) ;

      // unregister timer
      _unregisterTimer() ;

      // stop running EDU
      _stopEDU() ;

      // on event before deactivate
      _preDeactivate() ;

      // set module deactivated
      _activated = FALSE ;

      // on event after deactivate
      _postDeactivate() ;

      PD_LOG( PDEVENT, "[%s] is deactivated", getModuleName() ) ;

      PD_TRACE_EXITRC( SDB__STPMGRBASE_DEACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE_ATTACHCB, "_stpManagerBase::attachCB" )
   void _stpManagerBase::attachCB( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__STPMGRBASE_ATTACHCB ) ;

      ossScopedLock lock( &_eduLatch, EXCLUSIVE ) ;

      // set control block of EDU
      _eduCB = cb ;

      // signal the EDU is attached
      _attachEvent.signalAll() ;

      PD_TRACE_EXIT( SDB__STPMGRBASE_ATTACHCB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE_DETACHCB, "_stpManagerBase::detachCB" )
   void _stpManagerBase::detachCB( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__STPMGRBASE_DETACHCB ) ;

      ossScopedLock lock( &_eduLatch, EXCLUSIVE ) ;

      // reset the control block of EDU
      _eduCB = NULL ;

      PD_TRACE_EXIT( SDB__STPMGRBASE_DETACHCB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE_HANDLETIMEOUT, "_stpManagerBase::handleTimeout" )
   void _stpManagerBase::handleTimeout( const UINT32 &millisec,
                                        const UINT32 &timerID )
   {
      PD_TRACE_ENTRY( SDB__STPMGRBASE_HANDLETIMEOUT ) ;

      BOOLEAN handled = FALSE ;
      if ( PMD_INVALID_EDUID != _eduID )
      {
         // if EDU is running, redirect timer event
         handled = _asyncHandleTimeout( timerID, millisec ) ;
      }
      if ( !handled )
      {
         // if timer is not handled by asynchronous handler, handle directly
         onTimer( timerID, millisec ) ;
      }

      PD_TRACE_EXIT( SDB__STPMGRBASE_HANDLETIMEOUT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE_HANDLEMESSAGE, "_stpManagerBase::handleMessage" )
   INT32 _stpManagerBase::handleMessage( NET_HANDLE handle,
                                         const MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE_HANDLEMESSAGE ) ;

      BOOLEAN handled = FALSE ;
      if ( PMD_INVALID_EDUID != _eduID )
      {
         // if EDU is running, redirect message
         handled = _asyncHandleMessage( handle, message ) ;
      }
      if ( !handled )
      {
         // if message is not handled by asynchronous handler, handle directly
         rc = processMessage( handle, const_cast< MsgHeader * >( message ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process message %s, rc: %d",
                      msg2String( message ).c_str(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMGRBASE_HANDLEMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE_PROCESSMESSAGE, "_stpManagerBase::processMessage" )
   INT32 _stpManagerBase::processMessage( NET_HANDLE handle,
                                          MsgHeader *message )
   {
      INT32 rc = SDB_UNKNOWN_MESSAGE ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE_PROCESSMESSAGE ) ;

      // do nothing ( implemented by derived classes )

      PD_TRACE_EXITRC( SDB__STPMGRBASE_PROCESSMESSAGE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE__STARTEDU, "_stpManagerBase::_startEDU" )
   INT32 _stpManagerBase::_startEDU()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE__STARTEDU ) ;

      EDUID eduID = PMD_INVALID_EDUID ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;

      // stop remainning EDU
      _stopEDU() ;

      // start EDU by EDU manager
      rc = eduMgr->startEDU( EDU_TYPE_STP_MODULE, (void *)this, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start EDU for [%s], rc: %d",
                   getModuleName(), rc ) ;

      // wait EDU is attached
      rc = _attachEvent.wait( STP_MODULE_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait EDU attached for [%s], "
                   "rc: %d", getModuleName(), rc ) ;

      // set EDU ID
      _eduID = eduID ;

   done:
      PD_TRACE_EXITRC( SDB__STPMGRBASE__STARTEDU, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE__STOPEDU, "_stpManagerBase::_stopEDU" )
   INT32 _stpManagerBase::_stopEDU()
   {
      PD_TRACE_ENTRY( SDB__STPMGRBASE__STOPEDU ) ;

      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      // stop specified EDU if EDU manger is running
      if ( !eduMgr->isDestroyed() && PMD_INVALID_EDUID != _eduID )
      {
         // reset EDU ID
         EDUID eduID = _eduID ;
         _eduID = PMD_INVALID_EDUID ;

         // force EDU to stop ( ignore error )
         eduMgr->forceUserEDU( eduID ) ;
      }

      PD_TRACE_EXITRC( SDB__STPMGRBASE__STOPEDU, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE__REGTIMER, "_stpManagerBase::_registerTimer" )
   INT32 _stpManagerBase::_registerTimer()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE__REGTIMER ) ;

      UINT32 timerID = STP_INVALID_TIMERID ;

      // unregister remaining timer
      _unregisterTimer() ;

      // add timer to net agent
      // NOTE: 1 second for timeout interval, and derived classes should
      //       calculate their owned timeout
      rc = _netAgent->addTimer( OSS_ONE_SEC, this, timerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add synchronize timer, "
                   "rc: %d", rc ) ;

      // set timer ID
      _timerID = timerID ;

      PD_LOG( PDEVENT, "Register [%s] synchronize timer [%llu]",
              getModuleName(), _timerID ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMGRBASE__REGTIMER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE__UNREGTIMER, "_stpManagerBase::_unregisterTimer" )
   INT32 _stpManagerBase::_unregisterTimer()
   {
      PD_TRACE_ENTRY( SDB__STPMGRBASE__UNREGTIMER ) ;

      if ( STP_INVALID_TIMERID != _timerID )
      {
         _netAgent->removeTimer( _timerID ) ;
         _timerID = STP_INVALID_TIMERID ;
      }

      PD_TRACE_EXITRC( SDB__STPMGRBASE__UNREGTIMER, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE__ASYNCHANDLETIMEOUT, "_stpManagerBase::_asyncHandleTimeout" )
   BOOLEAN _stpManagerBase::_asyncHandleTimeout( const UINT32 &timerID,
                                                 const UINT32 &millisec )
   {
      BOOLEAN handled = FALSE ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE__ASYNCHANDLETIMEOUT ) ;

      PMD_EVENT_MESSAGES *event = NULL ;
      ossTimestamp ts ;

      ossScopedLock lock( &_eduLatch, SHARED ) ;

      // ignored if EDU is not running
      if ( NULL == _eduCB || _eduCB->isInterrupted() )
      {
         goto done ;
      }

      // allocate event
      event = (PMD_EVENT_MESSAGES *)SDB_THREAD_ALLOC(
                                             sizeof( PMD_EVENT_MESSAGES ) ) ;
      if ( NULL == event )
      {
         PD_LOG ( PDWARNING, "Failed to allocate memory for PDM "
                  "timeout Event for %d bytes",
                  sizeof( PMD_EVENT_MESSAGES ) ) ;
         goto done ;
      }

      // fill event
      ossGetCurrentTime( ts ) ;
      event->timeoutMsg.timerID = timerID ;
      event->timeoutMsg.interval = millisec ;
      event->timeoutMsg.occurTime = ts.time ;

      // post timeout event to running EDU
      _eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_TIMEOUT,
                                      PMD_EDU_MEM_THREAD,
                                      (void *)event ) ) ;

      // set message handled
      handled = TRUE ;

   done:
      PD_TRACE_EXIT( SDB__STPMGRBASE__ASYNCHANDLETIMEOUT ) ;
      return handled ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMGRBASE__ASYNCHANDLEMESSAGE, "_stpManagerBase::_asyncHandleMessage" )
   BOOLEAN _stpManagerBase::_asyncHandleMessage( NET_HANDLE handle,
                                                 const MsgHeader *message )
   {
      BOOLEAN handled = FALSE ;

      PD_TRACE_ENTRY( SDB__STPMGRBASE__ASYNCHANDLEMESSAGE ) ;

      CHAR *buffer = NULL ;

      ossScopedLock lock( &_eduLatch, SHARED ) ;

      // ignored if EDU is not running
      if ( NULL == _eduCB || _eduCB->isInterrupted() )
      {
         goto done ;
      }

      // allocate event
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

      // post message to running EDU
      _eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                      PMD_EDU_MEM_THREAD,
                                      (void *)buffer,
                                      (UINT64)handle ) ) ;

      // set message handled
      handled = TRUE ;

   done:
      PD_TRACE_EXIT( SDB__STPMGRBASE__ASYNCHANDLEMESSAGE ) ;
      return handled ;
   }

}
