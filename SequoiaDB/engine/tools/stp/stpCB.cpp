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

   Source File Name = stpCB.cpp

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
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"

#if defined(_LINUX)
#include <sys/timex.h>
#endif

using namespace std ;

namespace engine
{

   #define STP_CB_CHECK_TIMEOUT ( 60 * OSS_ONE_SEC )

   /*
      _stpCB implement
    */
   _stpCB::_stpCB()
   : _options(),
     _configHandler( this ),
     _netManager( &_netMsgHandler ),
     _pipeManager(),
     _netMsgHandler( this ),
     _pipeMsgHandler( this ),
     _serviceManager( this ),
     _nodeManager( this ),
     _metaManager( this ),
     _syncSourceManager( this ),
     _syncClientManager( this ),
     _replManager( this ),
     _checkTimeout( 0 )
   {
   }

   _stpCB::~_stpCB()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_INIT, "_stpCB::init" )
   INT32 _stpCB::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB_INIT ) ;

      // check time extra info
      // NOTE: machine time might adjust by NTP, just check
      _checkTimeExInfo() ;

      // set config handler ( handles config change )
      _options.setConfigHandler( &_configHandler ) ;

      // initialize net agent
      rc = _initNetAgent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize net agent, "
                   "rc: %d", rc ) ;

      // initialize pipe manager
      rc = _initPipeManager() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize pipe manager, "
                   "rc: %d", rc ) ;

      // initialize modules
      rc = _initializeModules() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize modules, rc: %d", rc ) ;

      // initialize each module
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         rc = (*iter)->initialize() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize [%s], rc: %d",
                      (*iter)->getModuleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCB_INIT, rc ) ;
      return rc ;

   error:
      // to be safe, call finalize here
      // NOTE: krcb will call fini() when initialize failed
      fini() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_ACTIVE, "_stpCB::active" )
   INT32 _stpCB::active()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB_ACTIVE ) ;

      // get role mask of config role
      UINT32 roleMask = tpGetRoleMask( _options.isTestMode(),
                                       _options.getRole() ) ;

      // test role mask against each module, activate supported module
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         // test role mask
         if ( OSS_BIT_TEST( (*iter)->getModuleRoleMask(), roleMask ) )
         {
            // activate module
            rc = (*iter)->activate() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to activate [%s], rc: %d",
                         (*iter)->getModuleName(), rc ) ;
         }
      }

      // activate system clock
      rc = _activeSystemClock() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active system clock, rc: %d", rc ) ;

      // activate net agent
      rc = _activeNetAgent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active net agent, rc: %d", rc ) ;

      // activate pipe manager
      rc = _activePipeManager() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active STP pipe manager, rc: %d",
                   rc ) ;

      // set business OK
      pmdGetKRCB()->setBusinessOK( TRUE ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCB_ACTIVE, rc ) ;
      return rc ;

   error:
      // to be safe, when error happened, deactivate modules
      // NOTE: krcb will call deactive() when active failed
      deactive() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_DEACTIVE, "_stpCB::deactive" )
   INT32 _stpCB::deactive()
   {
      PD_TRACE_ENTRY( SDB__STPCB_DEACTIVE ) ;

      // close and stop net agent
      _netManager.deactiveNetAgent() ;

      // deactivate each module
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->deactivate() ;
      }

      PD_TRACE_EXITRC( SDB__STPCB_DEACTIVE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_FINI, "_stpCB::fini" )
   INT32 _stpCB::fini()
   {
      PD_TRACE_ENTRY( SDB__STPCB_FINI ) ;

      // finalize each module
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->finalize() ;
      }

      // finalize pipe manager
      _pipeManager.fini() ;

      // unregister all modules
      _unregisterAllModules() ;

      PD_TRACE_EXITRC( SDB__STPCB_FINI, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_ONCONFIGCHANGE, "_stpCB::onConfigChange" )
   void _stpCB::onConfigChange()
   {
      PD_TRACE_ENTRY( SDB__STPCB_ONCONFIGCHANGE ) ;

      // update diagnostic log level
      setPDLevel( _options.getDiagLevel() ) ;

      // update start shift time
      _replManager.setStartShiftTime( _options.getStartShiftTime() ) ;

      // update max size of time map
      _metaManager.getTimeMapManager()->setMaxTimeMapSize(
                                             _options.getMaxTimeMapSize(),
                                             FALSE ) ;

      // update configs to node manager
      _nodeManager.updateConfigs() ;

      PD_TRACE_EXIT( SDB__STPCB_ONCONFIGCHANGE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_BEFORECHANGEPRIMARY, "_stpCB::beforeChangePrimary" )
   INT32 _stpCB::beforeChangePrimary( BOOLEAN isLocalPrimary )
   {
      PD_TRACE_ENTRY( SDB__STPCB_BEFORECHANGEPRIMARY ) ;

      // notify each module on before change primary event
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->beforeChangePrimary( isLocalPrimary ) ;
      }

      PD_TRACE_EXITRC( SDB__STPCB_BEFORECHANGEPRIMARY, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_AFTERCHANGEPRIMARY, "_stpCB::afterChangePrimary" )
   INT32 _stpCB::afterChangePrimary( const MsgRouteID &primaryRID,
                                     BOOLEAN isLocalPrimary )
   {
      PD_TRACE_ENTRY( SDB__STPCB_ONCHANGEPRIMARY ) ;

      // notify each module on after change primary event
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->afterChangePrimary( primaryRID, isLocalPrimary ) ;
      }

      PD_TRACE_EXITRC( SDB__STPCB_ONCHANGEPRIMARY, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_ONCHANGESERVERS, "_stpCB::onChangeServers" )
   INT32 _stpCB::onChangeServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB_ONCHANGESERVERS ) ;

      UINT32 version = STP_GROUP_INVALID_VERSION ;
      STP_SERVER_LIST servers ;

      // get servers
      rc = _nodeManager.getServers( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get servers, rc: %d", rc ) ;

      // notify each module to change servers
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->onChangeServers( version, servers ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCB_ONCHANGESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _stpCB::onChangeRole( STP_ROLE role )
   {
      PD_TRACE_ENTRY( SDB__STPCB_ONCHANGEROLE ) ;

      // get role mask for modules
      UINT32 roleMask = tpGetRoleMask( _options.isTestMode(),
                                       _options.getRole() ) ;

      // for each module, check against role mask
      // if role mask supports given role, activate module,
      // otherwise, deactivate module
      for ( STP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         // test role mask of given role against role mask of module
         if ( OSS_BIT_TEST( (*iter)->getModuleRoleMask(), roleMask ) )
         {
            // activate module
            (*iter)->activate() ;
         }
         else
         {
            // deactivate module
            (*iter)->deactivate() ;
         }
      }

      PD_TRACE_EXITRC( SDB__STPCB_ONCHANGEROLE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB_ONTIMER, "_stpCB::onTimer" )
   void _stpCB::onTimer( UINT64 timerID, UINT32 interval )
   {
      _checkTimeout += interval ;
      if ( _checkTimeout > STP_CB_CHECK_TIMEOUT )
      {
#if defined ( _DEBUG )
         _checkTimeExInfo() ;
#endif
         _checkTimeout = 0 ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__REGMODULE, "_stpCB::_registerModule" )
   INT32 _stpCB::_registerModule( stpModule *module )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB__REGMODULE ) ;

      SDB_ASSERT( NULL != module, "module is invalid" ) ;
      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must register in main thread" ) ;

      // check if module is valid
      PD_CHECK( NULL != module, SDB_INVALIDARG, error, PDERROR,
                "Failed to register module, module is invalid" ) ;
      // should be register in main thread
      PD_CHECK( NULL != pmdGetThreadEDUCB() &&
                EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                SDB_SYS, error, PDERROR, "Failed to register module, "
                "should use main thread to register" ) ;

      try
      {
         // add to module list
         _moduleList.push_back( module ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register module [%s], "
                 "occurred unexpected error: %s", module->getModuleName(),
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCB__REGMODULE, rc ) ;
      return rc ;

   error:
      // when error happened, unregister all modules
      _unregisterAllModules() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__UNREGMODULES, "_stpCB::_unregisterAllModules" )
   void _stpCB::_unregisterAllModules()
   {
      PD_TRACE_ENTRY( SDB__STPCB__UNREGMODULES ) ;

      // should be register in main thread ( only assert here )
      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must unregister in main thread" ) ;

      _moduleList.clear() ;

      PD_TRACE_EXIT( SDB__STPCB__UNREGMODULES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__INITMODULES, "_stpCB::_initializeModules" )
   INT32 _stpCB::_initializeModules()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB__INITMODULES ) ;

      // register net message handler
      rc = _registerModule( &_netMsgHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register net message handler, "
                   "rc: %d", rc ) ;

      // register pipe message handler
      rc = _registerModule( &_pipeMsgHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register pipe message handler, "
                   "rc: %d", rc ) ;

      // register service manager
      rc = _registerModule( &_serviceManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register service manager, "
                   "rc: %d", rc ) ;

      // register node manager
      rc = _registerModule( &_nodeManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register node manager, "
                   "rc: %d", rc ) ;

      // register meta manager
      rc = _registerModule( &_metaManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register meta manager, "
                   "rc: %d", rc ) ;

      // register synchronize source manager
      rc = _registerModule( &_syncSourceManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register synchronize source "
                   "manager, rc: %d", rc ) ;

      // register synchronize client manager
      rc = _registerModule( &_syncClientManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register synchronize client "
                   "manager, rc: %d", rc ) ;

      rc = _registerModule( &_replManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register replica manager, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCB__INITMODULES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__CHECKTIMEEXINFO, "_stpCB::_checkTimeExInfo" )
   void _stpCB::_checkTimeExInfo()
   {
      PD_TRACE_ENTRY( SDB__STPCB__CHECKTIMEEXINFO ) ;

#if defined(_LINUX)
      // check time extra info, which might be adjust by NTP
      // NOTE: NTP will not affect logical time of STP
      struct timex ex ;
      ex.modes = 0 ;
      int res = adjtimex( &ex ) ;
      if ( 0 <= res )
      {
         PD_LOG( PDEVENT, "NTP synchronize time status: "
                 "offset [%lld], freq [%lld], max-error [%lld], "
                 "est-error [%lld], status [%d], constant [%llu], "
                 "precision [%lld], tolerance [%lld], tick [%lld], "
                 "timex return code [%d]", ex.offset, ex.freq, ex.maxerror,
                 ex.esterror, ex.status, ex.constant, ex.precision,
                 ex.tolerance, ex.tick, res ) ;
      }
      else
      {
         PD_LOG( PDWARNING, "Failed to read timex, err: %d", res ) ;
      }
#endif

      PD_TRACE_EXIT( SDB__STPCB__CHECKTIMEEXINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__INITNETAGENT, "_stpCB::_initNetAgent" )
   INT32 _stpCB::_initNetAgent()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB__INITNETAGENT ) ;

      // get host name
      const CHAR *hostName = pmdGetKRCB()->getHostName() ;
      // get service name
      const CHAR *serviceName = _options.getServiceName() ;

      rc = _netManager.initNetAgent( hostName,
                                     serviceName,
                                     ( NET_FRAME_MASK_TCP |
                                       NET_FRAME_MASK_UDP ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize net agent, rc: %d",
                   rc ) ;

      PD_LOG( PDEVENT, "Listening on TCP and UDP port [%s]", serviceName ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCB__INITNETAGENT, rc ) ;;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__INITPIPEMANAGER, "_stpCB::_initPipeManager" )
   INT32 _stpCB::_initPipeManager()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB__INITPIPEMANAGER ) ;

      // initialize STP pipe
      rc = _pipeManager.init( STP_PIPE_SERVICE_NAME, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize STP pipe manager, "
                   "rc: %d", rc ) ;

      // register pipe message handler
      _pipeManager.registerHandler( &( _pipeMsgHandler ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCB__INITPIPEMANAGER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__ACTIVESYSTEMCLOCK, "_stpCB::_activeSystemClock" )
   INT32 _stpCB::_activeSystemClock()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB__ACTIVESYSTEMCLOCK ) ;

      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // start EDU to synchronize system clock with CPU tick
      // NOTE: this is not logical time, it is used to calculate time interval
      //       with CPU ticks
      rc = eduMgr->startEDU( EDU_TYPE_SYNCCLOCK, NULL, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start system clock EDU, rc: %d",
                   rc ) ;

      // wait until EDU is running
      rc = eduMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait system clock to be running, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCB__ACTIVESYSTEMCLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__ACTIVENETAGENT, "_stpCB::_activeNetAgent" )
   INT32 _stpCB::_activeNetAgent()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB__ACTIVENETAGENT ) ;

      rc = _netManager.activeNetAgent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active net agent, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCB__ACTIVENETAGENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCB__ACTIVEPIPEMANAGER, "_stpCB::_activePipeManager" )
   INT32 _stpCB::_activePipeManager()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCB__ACTIVEPIPEMANAGER ) ;

      // start EDU for pipe manager
      rc = _pipeManager.startEDU() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start EDU for STP pipe manager, "
                   "rc :%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPCB__ACTIVEPIPEMANAGER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
