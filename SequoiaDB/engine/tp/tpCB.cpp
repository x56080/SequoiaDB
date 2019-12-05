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

   Source File Name = tpCB.cpp

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

#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"

#if defined(_LINUX)
#include <sys/timex.h>
#endif

using namespace std ;

namespace engine
{

   /*
      _tpCB implement
    */
   _tpCB::_tpCB()
   : _options(),
     _netAgent( &_netMsgHandler ),
     _pipeManager(),
     _netMsgHandler( this ),
     _pipeMsgHandler( this ),
     _serviceManager( this ),
     _catalogManager( this ),
     _metaManager( this ),
     _sourceManager( this ),
     _syncManager( this ),
     _replManager( this )
   {
   }

   _tpCB::~_tpCB()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB_INIT, "_tpCB::init" )
   INT32 _tpCB::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB_INIT ) ;

      _checkTimeExInfo() ;

      _options.setConfigHandler( pmdGetKRCB() ) ;

      rc = _initializeModules() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize modules, rc: %d", rc ) ;

      for ( TP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         rc = (*iter)->initialize() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize [%s], rc: %d",
                      (*iter)->getModuleName(), rc ) ;
      }

      rc = _initNetAgent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize net agent, "
                   "rc: %d", rc ) ;

      rc = _initPipeManager() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize pipe manager, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB_ACTIVE, "_tpCB::active" )
   INT32 _tpCB::active()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB_ACTIVE ) ;

      UINT32 roleMask = tpGetRoleMask( _options.getRole() ) ;

      for ( TP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         if ( OSS_BIT_TEST( (*iter)->getModuleRoleMask(), roleMask ) )
         {
            rc = (*iter)->activate() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to activate [%s], rc: %d",
                         (*iter)->getModuleName(), rc ) ;
         }
      }

      rc = _activeSystemClock() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active system clock, rc: %d", rc ) ;

      rc = _activeNetAgent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active net agent, rc: %d", rc ) ;

      rc = _activePipeManager() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active TP pipe manager, rc: %d",
                   rc ) ;

      pmdGetKRCB()->setBusinessOK( TRUE ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB_ACTIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB_DEACTIVE, "_tpCB::deactive" )
   INT32 _tpCB::deactive()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB_DEACTIVE ) ;

      _netAgent.closeListen() ;
      _netAgent.stop() ;

      for ( TP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->deactivate() ;
      }

      PD_TRACE_EXITRC( SDB__TPCB_DEACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB_FINI, "_tpCB::fini" )
   INT32 _tpCB::fini()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB_FINI ) ;

      for ( TP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->finalize() ;
      }

      _pipeManager.fini() ;

      PD_TRACE_EXITRC( SDB__TPCB_FINI, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB_ONCONFIGCHANGE, "_tpCB::onConfigChange" )
   void _tpCB::onConfigChange()
   {
      PD_TRACE_ENTRY( SDB__TPCB_ONCONFIGCHANGE ) ;

      setPDLevel( _options.getDiagLevel() ) ;
      _replManager.setStartShiftTime( _options.getStartShiftTime() ) ;
      _catalogManager.updateConfigs() ;

      PD_TRACE_EXIT( SDB__TPCB_ONCONFIGCHANGE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB_ONCHANGEPRIMARY, "_tpCB::onChangePrimary" )
   INT32 _tpCB::onChangePrimary( const MsgRouteID &primaryRID,
                                 BOOLEAN isLocalPrimary )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB_ONCHANGEPRIMARY ) ;

      for ( TP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->onChangePrimary( primaryRID, isLocalPrimary ) ;
      }

      PD_TRACE_EXITRC( SDB__TPCB_ONCHANGEPRIMARY, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB_ONCHANGESERVERS, "_tpCB::onChangeServers" )
   INT32 _tpCB::onChangeServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB_ONCHANGESERVERS ) ;

      UINT32 version = TP_GROUP_INVALID_VERSION ;
      TP_SERVER_LIST servers ;

      rc = _catalogManager.getServers( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get servers, rc: %d", rc ) ;

      for ( TP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         (*iter)->onChangeServers( version, servers ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCB_ONCHANGESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _tpCB::onChangeRole( TP_ROLE role )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB_ONCHANGEROLE ) ;

      UINT32 roleMask = tpGetRoleMask( _options.getRole() ) ;

      for ( TP_MODULE_LIST::iterator iter = _moduleList.begin() ;
            iter != _moduleList.end() ;
            ++ iter )
      {
         if ( OSS_BIT_TEST( (*iter)->getModuleRoleMask(), roleMask ) )
         {
            (*iter)->activate() ;
         }
         else
         {
            (*iter)->deactivate() ;
         }
      }

      PD_TRACE_EXITRC( SDB__TPCB_ONCHANGEROLE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__REGMODULE, "_tpCB::_registerModule" )
   INT32 _tpCB::_registerModule( tpModule *module )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB__REGMODULE ) ;

      SDB_ASSERT( NULL != module, "module is invalid" ) ;

      try
      {
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
      PD_TRACE_EXITRC( SDB__TPCB__REGMODULE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__INITMODULES, "_tpCB::_initializeModules" )
   INT32 _tpCB::_initializeModules()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB__INITMODULES ) ;

      rc = _registerModule( &_netMsgHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register net message handler, "
                   "rc: %d", rc ) ;

      rc = _registerModule( &_pipeMsgHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register pipe message handler, "
                   "rc: %d", rc ) ;

      rc = _registerModule( &_serviceManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register service manager, "
                   "rc: %d", rc ) ;

      rc = _registerModule( &_catalogManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register catalog manager, "
                   "rc: %d", rc ) ;

      rc = _registerModule( &_metaManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register meta manager, "
                   "rc: %d", rc ) ;

      rc = _registerModule( &_sourceManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register source manager, "
                   "rc: %d", rc ) ;

      rc = _registerModule( &_syncManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register synchronize manager, "
                   "rc: %d", rc ) ;

      rc = _registerModule( &_replManager ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register replica manager, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB__INITMODULES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__CHECKTIMEEXINFO, "_tpCB::_checkTimeExInfo" )
   void _tpCB::_checkTimeExInfo()
   {
      PD_TRACE_ENTRY( SDB__TPCB__CHECKTIMEEXINFO ) ;

#if defined(_LINUX)
      struct timex ex ;
      ex.modes = 0 ;
      int res = adjtimex( &ex ) ;
      if ( 0 <= res )
      {
         PD_LOG( PDEVENT, "NTP synchroinze time status: "
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

      PD_TRACE_EXIT( SDB__TPCB__CHECKTIMEEXINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__INITNETAGENT, "_tpCB::_initNetAgent" )
   INT32 _tpCB::_initNetAgent()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB__INITNETAGENT ) ;

      tpClientNode local = _catalogManager.getLocal() ;

      // listen on both TCP and UDP
      rc = _netAgent.listen( local.getRouteID(),
                             ( NET_FRAME_MASK_TCP | NET_FRAME_MASK_UDP ),
                             &_netMsgHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to listen on port [%u], rc: %d",
                   _options.getPort(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB__INITNETAGENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__INITPIPEMANAGER, "_tpCB::_initPipeManager" )
   INT32 _tpCB::_initPipeManager()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB__INITPIPEMANAGER ) ;

      // initialize TP pipe
      rc = _pipeManager.init( TP_PIPE_SERVICE_NAME, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize TP pipe manager, "
                   "rc: %d", rc ) ;

      _pipeManager.registerHandler( &( _pipeMsgHandler ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB__INITPIPEMANAGER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__ACTIVESYSTEMCLOCK, "_tpCB::_activeSystemClock" )
   INT32 _tpCB::_activeSystemClock()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB__ACTIVESYSTEMCLOCK ) ;

      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      rc = eduMgr->startEDU( EDU_TYPE_SYNCCLOCK, NULL, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start system clock EDU, rc: %d",
                   rc ) ;

      rc = eduMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait system clock to be running, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB__ACTIVESYSTEMCLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__ACTIVENETAGENT, "_tpCB::_activeNetAgent" )
   INT32 _tpCB::_activeNetAgent()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB__ACTIVENETAGENT ) ;

      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      rc = eduMgr->startEDU( EDU_TYPE_TP_NET_AGENT, (void *)( &_netAgent ),
                             &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start TP network EDU, rc: %d",
                   rc ) ;

      rc = eduMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait TP network to be running, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB__ACTIVENETAGENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCB__ACTIVEPIPEMANAGER, "_tpCB::_activePipeManager" )
   INT32 _tpCB::_activePipeManager()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCB__ACTIVEPIPEMANAGER ) ;

      rc = _pipeManager.startEDU() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start EDU for TP pipe manager, "
                   "rc :%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCB__ACTIVEPIPEMANAGER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   SDB_TPCB *sdbGetTPCB()
   {
      static SDB_TPCB s_tpCB ;
      return &s_tpCB ;
   }

}
