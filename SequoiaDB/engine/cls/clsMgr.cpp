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

   Source File Name = clsMgr.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsMgr.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "clsShardSession.hpp"
#include "clsReplSession.hpp"
#include "clsFSDstSession.hpp"
#include "clsFSSrcSession.hpp"
#include "clsStorageCheckJob.hpp"
#include "../bson/bson.h"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "dpsOp2Record.hpp"
#include "pmdStartup.hpp"
#include "utilCommon.hpp"

using namespace bson ;

namespace engine
{

   //The max del session deque size
   #define MAX_SHD_SESSION_CATCH_DEQ_SIZE          (1000)

   #define CLS_WAIT_CB_ATTACH_TIMEOUT              ( 300 * OSS_ONE_SEC )


   /*
      _clsShardSessionMgr implement
   */
   _clsShardSessionMgr::_clsShardSessionMgr( _clsMgr *pClsMgr )
   {
      _pClsMgr    = pClsMgr ;
      _unShardSessionTimer = NET_INVALID_TIMER_ID ;
   }

   _clsShardSessionMgr::~_clsShardSessionMgr()
   {
      _pClsMgr    = NULL ;
   }

   BOOLEAN _clsShardSessionMgr::isUnShardTimerStarted() const
   {
      return NET_INVALID_TIMER_ID == _unShardSessionTimer ?
             FALSE : TRUE ;
   }

   void _clsShardSessionMgr::startUnShardTimer( UINT32 interval )
   {
      if ( _pRTAgent && _pTimerHandle && !isUnShardTimerStarted() )
      {
         _pRTAgent->addTimer( interval, _pTimerHandle,
                              _unShardSessionTimer ) ;
      }
   }

   void _clsShardSessionMgr::stopUnShardTimer()
   {
      if ( _pRTAgent && _pTimerHandle && isUnShardTimerStarted() )
      {
         _pRTAgent->removeTimer( _unShardSessionTimer ) ;
         _unShardSessionTimer = NET_INVALID_TIMER_ID ;
      }
   }

   UINT64 _clsShardSessionMgr::makeSessionID( const NET_HANDLE &handle,
                                              const MsgHeader *header )
   {
      UINT64 sessionID = ossPack32To64( header->routeID.columns.nodeID,
                                        header->TID ) ;
      if ( header->routeID.columns.nodeID < DATA_NODE_ID_BEGIN ||
           header->routeID.columns.groupID < DATA_GROUP_ID_BEGIN )
      {
         sessionID = ossPack32To64( PMD_BASE_HANDLE_ID + handle, header->TID ) ;
      }

      return sessionID ;
   }

   SDB_SESSION_TYPE _clsShardSessionMgr::_prepareCreate( UINT64 sessionID,
                                                         INT32 startType,
                                                         INT32 opCode )
   {
      SDB_SESSION_TYPE sessionType = SDB_SESSION_MAX ;
      UINT32 nodeID = 0 ;
      UINT32 tid = 0 ;

      ossUnpack32From64( sessionID, nodeID, tid ) ;
      // if nodeID <= PMD_BASE_HANDLE_ID, that means the request come from
      // the request from other nodes within the shard ( currently only
      // split request uses this part )
      if ( PMD_BASE_HANDLE_ID >= nodeID )
      {
         if ( PMD_SESSION_ACTIVE == startType )
         {
            // If it's proactive request, that means it's split destination
            // During split the destination part "asks" for the data from source
            sessionType = SDB_SESSION_SPLIT_DST ;
         }
         else
         {
            // otherwise it's the source, which receives the split request
            sessionType = SDB_SESSION_SPLIT_SRC ;
         }
      }
      // if nodeID > PMD_BASE_HANDLE_ID, that means the request come from
      // coord
      else
      {
         sessionType = SDB_SESSION_SHARD ;
      }

      return sessionType ;
   }

   BOOLEAN _clsShardSessionMgr::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      if ( SDB_SESSION_SHARD == sessionType )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _clsShardSessionMgr::_maxCacheSize() const
   {
      UINT32 maxPool = pmdGetOptionCB()->getMaxPooledEDU() ;
      return maxPool < MAX_SHD_SESSION_CATCH_DEQ_SIZE ?
             maxPool : MAX_SHD_SESSION_CATCH_DEQ_SIZE ;
   }

   // create session request for the manager
   // there are 3 types of sessions for shardsessions
   // 1) split destination
   // 2) split source
   // 3) regular shard session
   pmdAsyncSession* _clsShardSessionMgr::_createSession(
         SDB_SESSION_TYPE sessionType,
         INT32 startType,
         UINT64 sessionID,
         void *data )
   {
      pmdAsyncSession *pSession = NULL ;

      // Based on session type, let's create Async session
      if ( SDB_SESSION_SPLIT_DST == sessionType )
      {
         pSession = SDB_OSS_NEW _clsSplitDstSession ( sessionID, _pRTAgent,
                                                      data ) ;
      }
      else if ( SDB_SESSION_SPLIT_SRC == sessionType )
      {
         pSession = SDB_OSS_NEW _clsSplitSrcSession ( sessionID, _pRTAgent ) ;
      }
      else if ( SDB_SESSION_SHARD == sessionType )
      {
         pSession = SDB_OSS_NEW _clsShdSession ( sessionID ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Unknow session type[%d]", sessionType ) ;
      }

      return pSession ;
   }

   void _clsShardSessionMgr::_onSessionNew( pmdAsyncSession *pSession )
   {
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         map< UINT64, clsIdentifyInfo >::iterator it ;
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         it = _mapIdentifys.find( pSession->sessionID() ) ;
         if ( it != _mapIdentifys.end() )
         {
            pShdSession->setDelayLogin( it->second ) ;
            _mapIdentifys.erase( it ) ;
         }
      }
   }

   INT32 _clsShardSessionMgr::handleSessionTimeout( UINT32 timerID,
                                                    UINT32 interval )
   {
      INT32 rc = SDB_OK ;

      if ( _unShardSessionTimer == timerID )
      {
         _checkUnShardSessions( interval ) ;

         // start split task
         _pClsMgr->_startInnerSession( CLS_SHARD, this ) ;

         goto done ;
      }
      else if ( _sessionTimerID == timerID )
      {
         if ( _mapSession.size() <= MAX_SHD_SESSION_CATCH_DEQ_SIZE / 2 )
         {
            goto done ;
         }
      }

      rc = _pmdAsycSessionMgr::handleSessionTimeout( timerID, interval ) ;

   done:
      return rc ;
   }

   // check timeout for the irregular shard sessions ( like split sessions )
   // usually those types of sessions are for communication within between shard
   // like one shard directly send msg to another shard
   void _clsShardSessionMgr::_checkUnShardSessions( UINT32 interval )
   {
      pmdAsyncSession *pSession = NULL ;
      MAPSESSION_IT it = _mapSession.begin() ;
      while ( it != _mapSession.end() )
      {
         pSession = it->second ;
         // skip regular shard sessions
         if ( SDB_SESSION_SHARD == pSession->sessionType() )
         {
            ++it ;
            continue ;
         }
         // get rid of timeout sessions
         if ( !pSession->isProcess() && pSession->timeout( interval ) )
         {
            PD_LOG ( PDEVENT, "Session[%s] timeout", pSession->sessionName() ) ;
            _releaseSession_i ( pSession, TRUE, TRUE ) ;
            _mapSession.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }
   }

   void _clsShardSessionMgr::onSessionDestoryed( pmdAsyncSession *pSession )
   {
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         if( !pShdSession->isSetLogout() )
         {
            /// save identify info
            clsIdentifyInfo info ;
            info._id = pSession->identifyID() ;
            info._eduid = pSession->identifyEDUID() ;
            info._tid = pSession->identifyTID() ;
            info._username = pSession->getClient()->getUsername() ;
            pSession->getAuditConfig( info._auditMask,
                                      info._auditConfigMask ) ;
            if ( !info._username.empty() )
            {
               info._passwd = pSession->getClient()->getPassword() ;
            }
            _mapIdentifys[ pSession->sessionID() ] = info ;
         }
      }
   }

   void _clsShardSessionMgr::onSessionDisconnect( pmdAsyncSession *pSession )
   {
      /// recv the disconnect msg, so need to logout
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         pShdSession->setLogout() ;
         _mapIdentifys.erase( pSession->sessionID() ) ;
      }
   }

   void _clsShardSessionMgr::onNoneSessionDisconnect( UINT64 sessionID )
   {
      _mapIdentifys.erase( sessionID ) ;
   }

   void _clsShardSessionMgr::onSessionHandleClose( pmdAsyncSession *pSession )
   {
      /// when net handle closed, need to logout
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         pShdSession->setLogout() ;
         _mapIdentifys.erase( pSession->sessionID() ) ;
      }
   }

   INT32 _clsShardSessionMgr::onErrorHanding( INT32 rc,
                                              const MsgHeader *pReq,
                                              const NET_HANDLE &handle,
                                              UINT64 sessionID,
                                              pmdAsyncSession *pSession )
   {
      INT32 ret = SDB_OK ;

      UINT32 nodeID = 0 ;
      UINT32 tid = 0 ;
      ossUnpack32From64( sessionID, nodeID, tid ) ;

      if ( nodeID > PMD_BASE_HANDLE_ID )
      {
         /// shard session
         ret = _reply( handle, rc, pReq ) ;
      }
      else if ( 0 == sessionID )
      {
         ret = rc ;
      }

      return ret ;
   }

   /*
      _clsReplSessionMgr implement
   */
   _clsReplSessionMgr::_clsReplSessionMgr( _clsMgr *pClsMgr )
   {
      _pClsMgr = pClsMgr ;
   }

   _clsReplSessionMgr::~_clsReplSessionMgr()
   {
      _pClsMgr = NULL ;
   }

   INT32 _clsReplSessionMgr::handleSessionTimeout( UINT32 timerID,
                                                   UINT32 interval )
   {
      INT32 rc = SDB_OK ;

      rc = _pmdAsycSessionMgr::handleSessionTimeout( timerID, interval ) ;
      if ( SDB_OK == rc )
      {
         // start repl/fs sessions
         _pClsMgr->_startInnerSession( CLS_REPL, this ) ;
      }

      return rc ;
   }

   UINT64 _clsReplSessionMgr::makeSessionID( const NET_HANDLE & handle,
                                             const MsgHeader * header )
   {
      return ossPack32To64( header->routeID.columns.nodeID,
                            header->TID ) ;
   }

   SDB_SESSION_TYPE _clsReplSessionMgr::_prepareCreate( UINT64 sessionID,
                                                        INT32 startType,
                                                        INT32 opCode )
   {
      SDB_SESSION_TYPE sessionType = SDB_SESSION_MAX ;
      UINT32 nodeID = 0 ;
      UINT32 tid = 0 ;

      ossUnpack32From64( sessionID, nodeID, tid ) ;

      if ( CLS_TID_REPL_SYC == tid )
      {
         sessionType = PMD_SESSION_ACTIVE == startType ?
                       SDB_SESSION_REPL_DST :
                       SDB_SESSION_REPL_SRC ;
      }
      else if ( CLS_TID_REPL_FS_SYC == tid )
      {
         if ( PMD_SESSION_ACTIVE == startType )
         {
            sessionType = SDB_SESSION_FS_DST ;
         }
         else
         {
            sessionType = SDB_SESSION_FS_SRC ;
         }
      }

      return sessionType ;
   }

   BOOLEAN _clsReplSessionMgr::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      return FALSE ;
   }

   UINT32 _clsReplSessionMgr::_maxCacheSize() const
   {
      return 0 ;
   }

   // create replication sessions manager
   // include:
   // 1) replication destination
   // 2) replication source
   // 3) full sync destination
   // 4) full sync source
   pmdAsyncSession* _clsReplSessionMgr::_createSession(
         SDB_SESSION_TYPE sessionType,
         INT32 startType,
         UINT64 sessionID,
         void *data )
   {
      pmdAsyncSession *pSession = NULL ;
      // check session type for replication sessions
      if ( SDB_SESSION_REPL_DST == sessionType )
      {
         // slave node uses dest
         pSession = SDB_OSS_NEW clsReplDstSession( sessionID ) ;
      }
      else if ( SDB_SESSION_REPL_SRC == sessionType )
      {
         // primary node uses src
         UINT32 nodeID = 0 ;
         UINT32 tid = 0 ;
         ossUnpack32From64( sessionID, nodeID, tid ) ;

         // if we find the requested nodeID is not the nodeID for the current
         // node, that means we get something from another node and we are
         // going to create a new replsrc session
         if ( pmdGetNodeID().columns.nodeID != 0 &&
              pmdGetNodeID().columns.nodeID != nodeID )
         {
            pSession = SDB_OSS_NEW clsReplSrcSession( sessionID ) ;
         }
      }
      // FS means full sync
      else if ( SDB_SESSION_FS_DST == sessionType )
      {
         pSession = SDB_OSS_NEW _clsFSDstSession ( sessionID,
                                                   _pRTAgent ) ;
      }
      else if ( SDB_SESSION_FS_SRC == sessionType )
      {
         pSession = SDB_OSS_NEW _clsFSSrcSession ( sessionID,
                                                   _pRTAgent ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Unknow session type[%d]", sessionType ) ;
      }

      return pSession ;
   }

   INT32 _clsReplSessionMgr::onErrorHanding( INT32 rc,
                                             const MsgHeader *pReq,
                                             const NET_HANDLE &handle,
                                             UINT64 sessionID,
                                             pmdAsyncSession *pSession )
   {
      INT32 ret = SDB_OK ;

      if ( 0 == sessionID )
      {
         ret = rc ;
      }

      return ret ;
   }

   /*
      _clsMgr implement
   */
   BEGIN_OBJ_MSG_MAP( _clsMgr, _pmdObjBase )
      ON_MSG ( MSG_CAT_REG_RES, _onCatRegisterRes )
      ON_MSG ( MSG_CAT_QUERY_TASK_RSP, _onCatQueryTaskRes )
      ON_EVENT( PMD_EDU_EVENT_STEP_DOWN, _onStepDown )
      ON_EVENT( PMD_EDU_EVENT_STEP_UP, _onStepUp )
      //ON_EVENT FUCTION MAP
   END_OBJ_MSG_MAP()

   _clsMgr::_clsMgr ()
   :_shdMsgHandlerObj ( &_shardSessionMgr ),
    _replMsgHandlerObj ( &_replSessionMgr ),
    _shdTimerHandler ( &_shardSessionMgr ),
    _replTimerHandler ( &_replSessionMgr ),
    _replNetRtAgent ( &_replMsgHandlerObj ),
    _shardNetRtAgent ( &_shdMsgHandlerObj ),
    _shdObj ( &_shardNetRtAgent ),
    _replObj ( &_replNetRtAgent ),
    _shardSessionMgr( this ),
    _replSessionMgr( this ),
    _shardServiceID ( MSG_ROUTE_SHARD_SERVCIE ),
    _replServiceID ( MSG_ROUTE_REPL_SERVICE ),
    _taskMgr( 0x7FFFFFFF ),
    _taskID ( 0 ),
    _regTimerID ( CLS_INVALID_TIMERID ),
    _regFailedTimes( 0 ),
    _oneSecTimerID ( CLS_INVALID_TIMERID )
   {
      _replServiceName[0] = 0 ;
      _shdServiceName[0]  = 0 ;
      _selfNodeID.value   = MSG_INVALID_ROUTEID ;
   }

   _clsMgr::~_clsMgr ()
   {
   }

   SDB_CB_TYPE _clsMgr::cbType () const
   {
      return SDB_CB_CLS ;
   }

   const CHAR* _clsMgr::cbName () const
   {
      return "CLSCB" ;
   }

   INT32 _clsMgr::init ()
   {
      INT32 rc = SDB_OK ;
      NodeID nodeID = _selfNodeID ;
      const CHAR* hostName = pmdGetKRCB()->getHostName() ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;

      // 1. init param
      ossStrncpy( _shdServiceName, optCB->shardService(),
                  OSS_MAX_SERVICENAME ) ;
      ossStrncpy( _replServiceName, optCB->replService(),
                  OSS_MAX_SERVICENAME ) ;

      INIT_OBJ_GOTO_ERROR ( getShardCB() ) ;
      INIT_OBJ_GOTO_ERROR ( getReplCB() ) ;

      // 2. create listen socket
      nodeID.columns.serviceID = _replServiceID ;
      _replNetRtAgent.updateRoute( nodeID, hostName, _replServiceName ) ;
      rc = _replNetRtAgent.listen( nodeID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Create listen[Hostname:%s, ServiceName:%s] failed",
                  hostName, _replServiceName ) ;
         goto error ;
      }
      PD_LOG ( PDEVENT, "Create replicate group listen[ServiceName:%s] succeed",
               _replServiceName ) ;

      nodeID.columns.serviceID = _shardServiceID ;
      _shardNetRtAgent.updateRoute( nodeID, hostName, _shdServiceName ) ;
      rc = _shardNetRtAgent.listen( nodeID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Create listen[Hostname:%s, ServiceName:%s] failed",
                  hostName, _shdServiceName ) ;
         goto error ;
      }
      PD_LOG ( PDEVENT, "Create sharding listen[ServiceName:%s] succeed",
               _shdServiceName ) ;

      // 3. init session manager
      rc = _shardSessionMgr.init( &_shardNetRtAgent, &_shdTimerHandler,
                                  60 * OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init shard session manager, rc: %d",
                   rc ) ;

      rc = _replSessionMgr.init( &_replNetRtAgent, &_replTimerHandler,
                                 OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init repl session manager, rc: %d",
                   rc ) ;

      // 4. set bussiness not ok( need wait register to change )
      pmdGetKRCB()->setBusinessOK( FALSE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::_getMaxDMSLSN( SDB_DMSCB *dmsCB, DPS_LSN_OFFSET &maxLsn )
   {
      INT32 rc = SDB_OK ;
      set< monCSSimple >  csList ;
      set< monCSSimple >::iterator it ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      dmsCB->dumpInfo( csList, TRUE ) ;

      for ( it = csList.begin() ; it != csList.end() ; ++it )
      {
         const monCSSimple &csInfo = *it ;

         if ( 0 == ossStrcmp( csInfo._name, SDB_DMSTEMP_NAME ) )
         {
            continue ;
         }

         dmsStorageUnit *su = NULL ;
         suID = DMS_INVALID_SUID ;
         rc = dmsCB->nameToSUAndLock( csInfo._name, suID, &su ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to lock collectionspace[%s], rc: %d",
                    csInfo._name, rc ) ;
            goto error ;
         }

         rtnRecoverUnit recoverUnit ;
         rc = recoverUnit.init( su ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init recover unit:rc=%d", rc ) ;

         if ( DPS_INVALID_LSN_OFFSET == maxLsn ||
              maxLsn < recoverUnit.getMaxValidLsn() )
         {
            maxLsn = recoverUnit.getMaxValidLsn() ;
         }

         if ( DMS_INVALID_SUID != suID )
         {
            dmsCB->suUnlock( suID ) ;
            suID = DMS_INVALID_SUID ;
         }
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_ACTIVE, "_clsMgr::active" )
   INT32 _clsMgr::active ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_ACTIVE ) ;

      if ( pmdGetStartup().isOK() )
      {
         SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         if ( NULL != dmsCB && NULL != dpsCB )
         {
            DPS_LSN_OFFSET maxLSN = DPS_INVALID_LSN_OFFSET ;
            DPS_LSN expectLSN = dpsCB->expectLsn() ;
            if ( 0 == expectLSN.version && 0 == expectLSN.offset )
            {
               rc = _getMaxDMSLSN( dmsCB, maxLSN ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get max dms lsn:rc=%d",
                            rc ) ;

               if ( DPS_INVALID_LSN_OFFSET != maxLSN
                    && expectLSN.offset < maxLSN )
               {
                  DPS_LSN newDPSLSN = expectLSN ;
                  newDPSLSN.offset = maxLSN
                           + ossAlign4( (UINT32)sizeof( dpsLogRecordHeader ) ) ;
                  if ( DPS_INVALID_LSN_VERSION == newDPSLSN.version )
                  {
                     newDPSLSN.version = DPS_INVALID_LSN_VERSION + 1 ;
                  }

                  /// clear transinfo
                  sdbGetTransCB()->clearTransInfo() ;
                  /// then move to new dps lsn
                  rc = dpsCB->move( newDPSLSN.offset, newDPSLSN.version ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to move(%lld:%lld)",
                               newDPSLSN.version, newDPSLSN.offset ) ;

                  PD_LOG( PDEVENT, "Move new lsn(%lld:%lld) succeed",
                          newDPSLSN.version, newDPSLSN.offset ) ;
               }
            }
         }
      }

      // 1. start cls edu and shard edu
      _attachEvent.reset() ;
      rc = _startEDU ( EDU_TYPE_CLUSTER, PMD_EDU_UNKNOW,
                       (_pmdObjBase*)this, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _attachEvent.wait( CLS_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait cluster edu attach failed, rc: %d", rc ) ;

      _attachEvent.reset() ;
      rc = _startEDU ( EDU_TYPE_CLUSTERSHARD, PMD_EDU_UNKNOW,
                       (_pmdObjBase*)getShardCB(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _attachEvent.wait( CLS_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait cluster-shard attach failed, rc: %d",
                   rc ) ;

      // Start log notify
      rc = _startEDU( EDU_TYPE_CLSLOGNTY, PMD_EDU_UNKNOW,
                      (_pmdObjBase*)getReplCB(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      // 2. start network daemons for shard/repl reader
      rc = _startEDU ( EDU_TYPE_SHARDR, PMD_EDU_RUNNING,
                       (netRouteAgent*)getShardRouteAgent(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _startEDU ( EDU_TYPE_REPR, PMD_EDU_RUNNING,
                       (netRouteAgent*)getReplRouteAgent(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      // 3. set timer
      _oneSecTimerID = setTimer ( CLS_REPL, OSS_ONE_SEC ) ;

      if ( CLS_INVALID_TIMERID == _oneSecTimerID )
      {
         PD_LOG ( PDERROR, "Register repl/shard/one seccond timer failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _regTimerID = setTimer( CLS_SHARD, OSS_ONE_SEC ) ;

      if ( CLS_INVALID_TIMERID == _regTimerID )
      {
         PD_LOG ( PDERROR, "Register timer failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // 4. send register msg
      _sendRegisterMsg () ;

      // Start storage check job only for data nodes
      if ( SDB_ROLE_DATA == pmdGetKRCB()->getDBRole() )
      {
         rc = startStorageCheckJob( NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Start storage checking job thread failed, rc: %d",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR_ACTIVE, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::deactive ()
   {
      // 1. members to deactive
      _replObj.deactive() ;
      _shdObj.deactive() ;

      // 2. stop listen
      _replNetRtAgent.closeListen() ;
      _shardNetRtAgent.closeListen() ;

      // 3. stop io
      _replNetRtAgent.stop() ;
      _shardNetRtAgent.stop() ;

      _shardSessionMgr.setForced() ;
      _replSessionMgr.setForced() ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_FINAL, "_clsMgr::fini" )
   INT32 _clsMgr::fini ()
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR_FINAL ) ;

      _shardSessionMgr.fini() ;
      _replSessionMgr.fini() ;

      _shdObj.final () ;
      _replObj.final () ;

      PD_TRACE_EXIT ( SDB__CLSMGR_FINAL );
      return SDB_OK ;
   }

   void _clsMgr::onConfigChange ()
   {
      _shdObj.onConfigChange() ;
      _replObj.onConfigChange() ;
   }

   void* _clsMgr::queryInterface( SDB_INTERFACE_TYPE type )
   {
      if ( SDB_IF_CLS == type )
      {
         return dynamic_cast<ICluster*>( &_replObj ) ;
      }
      return IControlBlock::queryInterface( type ) ;
   }

   void _clsMgr::attachCB ( pmdEDUCB *pMainCB )
   {
      if ( EDU_TYPE_CLUSTER == pMainCB->getType() )
      {
         //Set MsgHandler EDU
         _shdMsgHandlerObj.attach ( pMainCB ) ;
         _replMsgHandlerObj.attach ( pMainCB ) ;

         //Set TimerHandler EDU
         _shdTimerHandler.attach ( pMainCB ) ;
         _replTimerHandler.attach ( pMainCB ) ;
      }
      else if ( EDU_TYPE_CLUSTERSHARD == pMainCB->getType() )
      {
         _shdMsgHandlerObj.attachShardCB( pMainCB ) ;
      }

      _attachEvent.signalAll() ;
   }

   void _clsMgr::detachCB( pmdEDUCB *pMainCB )
   {
      if ( EDU_TYPE_CLUSTER == pMainCB->getType() )
      {
         //Set MsgHandler EDU
         _shdMsgHandlerObj.detach() ;
         _replMsgHandlerObj.detach () ;

         //Set TimerHandler EDU
         _shdTimerHandler.detach () ;
         _replTimerHandler.detach () ;
      }
      else if ( EDU_TYPE_CLUSTERSHARD == pMainCB->getType() )
      {
         _shdMsgHandlerObj.detachShardCB() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__STRATEDU, "_clsMgr::_startEDU" )
   INT32 _clsMgr::_startEDU ( INT32 type, EDU_STATUS waitStatus,
                              void *agrs, BOOLEAN regSys )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__STRATEDU );
      EDUID eduID = PMD_INVALID_EDUID ;
      pmdKRCB *pKRCB = pmdGetKRCB () ;
      pmdEDUMgr *pEDUMgr = pKRCB->getEDUMgr () ;

      //Start EDU
      rc = pEDUMgr->startEDU( (EDU_TYPES)type, (void *)agrs, &eduID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to create EDU[type:%d(%s)], rc = %d",
                  type, getEDUName( (EDU_TYPES)type ), rc );
         goto error ;
      }

      //Resiter EDU Type
      if ( regSys )
      {
         pEDUMgr->regSystemEDU( (EDU_TYPES)type, eduID ) ;
      }

      //Wait edu running
      if ( PMD_EDU_UNKNOW != waitStatus )
      {
         rc = pEDUMgr->waitUntil( (EDU_TYPES)type, waitStatus ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to wait EDU[type:%d(%s)] to "
                    "status[%d(%s)], rc: %d", type,
                    getEDUName( (EDU_TYPES)type ), waitStatus,
                    getEDUStatusDesp( waitStatus ), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR__STRATEDU, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__ONPRMCHG, "_clsMgr::ntyPrimaryChange" )
   void _clsMgr::ntyPrimaryChange( BOOLEAN primary,
                                   SDB_EVENT_OCCUR_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR__ONPRMCHG );

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         PD_LOG ( PDEVENT, "Node change to [%s]",
                  primary ? "Primary" : "Secondary" ) ;
      }

      // let's ignore the event if the node is still starting up
      if ( !pmdGetStartup().isOK() ||
           _shdObj.getDCMgr()->getDCBaseInfo()->isReadonly() ||
           !_shdObj.getDCMgr()->getDCBaseInfo()->isActivated() )
      {
         return ;
      }

      // if we are switching to primary, let's increase log version BEFORE
      // it actually happen
      if ( primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         // inc dps log version
         sdbGetDPSCB()->incVersion() ;
      }
      // if we are switching to slave, let's interrupt all EDUs that doing write
      else if ( !primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         sdbGetDPSCB()->cancelIncVersion() ;
         // interrupt writing edus
         pmdGetKRCB()->getEDUMgr()->interruptWritingEDUS() ;
      }

      // notify sub members
      getShardCB()->ntyPrimaryChange( primary, type ) ;
      getReplCB()->ntyPrimaryChange( primary, type ) ;

      // for "post trigger" event
      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         // if change to primary, need to start query task
         if ( primary )
         {
            BSONObj match = BSON ( CAT_TARGETID_NAME <<
                                   _selfNodeID.columns.groupID ) ;
            startTaskCheck( match ) ;
         }
         // if change to secondary, need to clean up all query task
         else
         {
            ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
            _mapTaskQuery.clear () ;
         }
      }

      // call other handler
      pmdGetKRCB()->callPrimaryChangeHandler( primary, type ) ;

      PD_TRACE_EXIT ( SDB__CLSMGR__ONPRMCHG );
   }

   const CHAR *_clsMgr::getShardServiceName () const
   {
      return _shdServiceName ;
   }
   const CHAR *_clsMgr::getReplServiceName () const
   {
      return _replServiceName ;
   }
   NodeID _clsMgr::getNodeID () const
   {
      return _selfNodeID ;
   }
   UINT16 _clsMgr::getShardServiceID () const
   {
      return _shardServiceID ;
   }
   UINT16 _clsMgr::getReplServiceID () const
   {
      return _replServiceID ;
   }

   _netRouteAgent *_clsMgr::getShardRouteAgent ()
   {
      return &_shardNetRtAgent ;
   }
   _netRouteAgent *_clsMgr::getReplRouteAgent ()
   {
      return &_replNetRtAgent ;
   }
   shardCB *_clsMgr::getShardCB ()
   {
      return &_shdObj ;
   }
   replCB *_clsMgr::getReplCB ()
   {
      return &_replObj ;
   }
   catAgent *_clsMgr::getCatAgent ()
   {
      return _shdObj.getCataAgent() ;
   }
   nodeMgrAgent* _clsMgr::getNodeMgrAgent ()
   {
      return _shdObj.getNodeMgrAgent() ;
   }
   shdMsgHandler* _clsMgr::getShardMsgHandle()
   {
      return &_shdMsgHandlerObj ;
   }
   _clsTaskMgr* _clsMgr::getTaskMgr()
   {
      return &_taskMgr ;
   }
   BOOLEAN _clsMgr::isPrimary ()
   {
      return _replObj.primaryIsMe () ;
   }
   INT32 _clsMgr::clearAllData ()
   {
      return _shdObj.clearAllData () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_INVDATACAT, "_clsMgr::invalidateCata" )
   INT32 _clsMgr::invalidateCata( const CHAR * name )
   {
      INT32 rc = SDB_CLS_NOT_PRIMARY ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_INVDATACAT );

      if ( isPrimary() )
      {
         /// write sync cata info log
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         dpsMergeInfo info ;
         info.setInfoEx( ~0, ~0, DMS_INVALID_EXTENT, NULL ) ;
         dpsLogRecord &record = info.getMergeBlock().record() ;
         rc = dpsInvalidCata2Record( name, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build invalid-cata log:%d",rc ) ;
            goto error ;
         }
         rc = dpsCB->prepare(info ) ;
         if ( SDB_OK == rc )
         {
            dpsCB->writeData( info ) ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR_INVDATACAT, rc );
      return rc ;
   error:
      goto done ;
   }

   // Register async internal sessions
   // The function itself doesn't start session. Instead the function place
   // a request in _vecInnerSessionParam vector so that another daemon will
   // create a background inernal sessions afterwards
   // By default the daemon will be triggered every single seconds to detect
   // if the queue is empty or not
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_STARTINSN, "_clsMgr::startInnerSession" )
   INT32 _clsMgr::startInnerSession ( INT32 type, INT32 innerTID, void *data )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR_STARTINSN );
      ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;

      _innerSessionInfo info ;
      info.type = type ;
      info.startType = PMD_SESSION_ACTIVE ;
      info.innerTid = innerTID ;
      info.data = data ;
      info.sessionID = ossPack32To64 ( _selfNodeID.columns.nodeID, innerTID ) ;

      _vecInnerSessionParam.push_back ( info ) ;

      PD_TRACE_EXIT ( SDB__CLSMGR_STARTINSN );
      return SDB_OK ;
   }

   // Start a background task check request
   // Another daemon will be triggered every second, it will send the check
   // request to CATALOG to check for task collection
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_STARTTSKCHK, "_clsMgr::startTaskCheck" )
   INT32 _clsMgr::startTaskCheck ( const BSONObj & match )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_STARTTSKCHK );
      if ( !isPrimary() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
      }
      else
      {
         ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
         _mapTaskQuery[++_taskID] = match.copy() ;
      }
      PD_TRACE_EXIT ( SDB__CLSMGR_STARTTSKCHK );
      return rc ;
   }

   INT32 _clsMgr::stopTask( UINT64 taskID )
   {
      ossScopedLock lock ( &_clsLatch, SHARED ) ;
      map< UINT64, UINT64 >::iterator it = _mapTaskID.find( taskID ) ;
      if ( it != _mapTaskID.end() )
      {
         _taskMgr.stopTask( it->second ) ;
      }
      return SDB_OK ;
   }

   // remove the task from local
   INT32 _clsMgr::removeTask( UINT64 taskID )
   {
      ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
      map< UINT64, UINT64 >::iterator it = _mapTaskID.find( taskID ) ;
      if ( it != _mapTaskID.end() )
      {
         _mapTaskID.erase( it ) ;
      }
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__DFTMSGFUNC, "_clsMgr::_defaultMsgFunc" )
   INT32 _clsMgr::_defaultMsgFunc( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__DFTMSGFUNC );
      // the msg is not mine, dispatch to sub object
      // restore the type
      INT32 type = (INT32) msg->TID ;
      INT32 opCode = msg->opCode ;
      msg->TID = 0 ;

      if ( CLS_REPL == type ||
           MSG_CAT_GRP_RES == opCode ||
           MSG_CAT_PAIMARY_CHANGE_RES == opCode ||
           MSG_CLS_GINFO_UPDATED == opCode )
      {
         rc = _replObj.dispatchMsg( handle, msg ) ;
      }
      else
      {
         rc = _shdObj.dispatchMsg ( handle, msg ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__DFTMSGFUNC, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_ONTMR, "_clsMgr::onTimer" )
   void _clsMgr::onTimer ( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR_ONTMR );
      //Judge the timer is myself, if not my self will dispatch to sub object
      if ( timerID == _regTimerID )
      {
         _sendRegisterMsg () ;
      }
      // if we hit one second
      else if ( timerID == _oneSecTimerID )
      {
         //Check _deqShdDeletingSessions
         _shardSessionMgr.onTimer( interval ) ;
         _replSessionMgr.onTimer( interval ) ;

         //prepare task
         _prepareTask () ;

         // if we have one or more pending tasks, and if the unshard timer
         // not started yet, let's start one
         if ( _taskMgr.taskCount() > 0 &&
              !_shardSessionMgr.isUnShardTimerStarted() )
         {
            _shardSessionMgr.startUnShardTimer( OSS_ONE_SEC ) ;
         }
         // if unshard time is already started but pending tasks are 0, let's
         // stop it
         else if ( _shardSessionMgr.isUnShardTimerStarted() &&
                   0 == _taskMgr.taskCount() )
         {
            _shardSessionMgr.stopUnShardTimer() ;
         }
      }
      else
      {
         // otherwise let's extract the type from timerID, and call onTimer
         // call back functions based on the request type
         // For now we only have 2 possible types, shard or repl
         UINT32 type = 0 ;
         UINT32 netTimerID = 0 ;
         ossUnpack32From64 ( timerID, type, netTimerID ) ;
         _pmdObjBase *pSubObj = &_shdObj ;
         if ( CLS_REPL == (INT32)type )
         {
            pSubObj = &_replObj ;
         }

         pSubObj->onTimer ( timerID, interval ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSMGR_ONTMR );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__STARTINSN, "_clsMgr::_startInnerSession" )
   INT32 _clsMgr::_startInnerSession ( INT32 type,
                                       pmdAsycSessionMgr *pSessionMgr )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__STARTINSN );
      _pmdAsyncSession *pSession = NULL ;
      ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;

      VECINNERPARAM::iterator it = _vecInnerSessionParam.begin() ;
      // iterate for all pending internal session requests
      while ( it != _vecInnerSessionParam.end() )
      {
         _innerSessionInfo &info = *it ;
         // skip for any unmatch types or existing sessions
         // if the session already started, we simply ignore the request in
         // the list and wait for start next time
         if ( info.type != type ||
              SDB_OK == pSessionMgr->getSession( info.sessionID,
                                                 info.startType,
                                                 NET_INVALID_HANDLE,
                                                 FALSE, 0, NULL,
                                                 NULL ) )
         {
            ++it ;
            continue ;
         }
         // let's start the session
         rc = pSessionMgr->getSession ( info.sessionID, info.startType,
                                        NET_INVALID_HANDLE, TRUE, 0,
                                        info.data,
                                        &pSession ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG ( PDEVENT, "Create inner session[%s] succeed",
                     pSession->sessionName() ) ;
            it = _vecInnerSessionParam.erase ( it ) ;
            continue ;
         }
         // if we get here, that means something wrong and we can't start
         // the session
         PD_LOG ( PDERROR, "Create inner session[TID:%d] failed, rc: %d",
                  info.innerTid, rc ) ;
         ++it ;
      }

      PD_TRACE_EXITRC ( SDB__CLSMGR__STARTINSN, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__PREPTASK, "_clsMgr::_prepareTask" )
   INT32 _clsMgr::_prepareTask ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__PREPTASK );
      ossScopedLock lock ( &_clsLatch, SHARED ) ;
      MAPTASKQUERY::iterator it = _mapTaskQuery.begin () ;
      while ( it != _mapTaskQuery.end() )
      {
         // send query msg to catalog
         rc = _sendQueryTaskReq ( it->first, "CAT", &(it->second) ) ;
         if ( SDB_OK != rc )
         {
            break ;
         }
         ++it ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__PREPTASK, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__ADDTSKINSN, "_clsMgr::_addTaskInnerSession" )
   INT32 _clsMgr::_addTaskInnerSession ( const CHAR * objdata )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__ADDTSKINSN );
      INT32 jobType = CLS_TASK_UNKNOW ;
      _clsSplitTask *pTask = NULL ;
      UINT32 tid = 0 ;
      INT32 type = CLS_SHARD ;
      UINT64 taskID = CLS_INVALID_TASKID ;

      try
      {
         BSONObj resultObj ( objdata ) ;
         BSONElement ele = resultObj.getField( CAT_TASKTYPE_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in task[%s]", CAT_TASKTYPE_NAME,
                    resultObj.toString().c_str() ) ;
         jobType = ele.numberInt () ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "addTaskInnerSession exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      switch ( jobType )
      {
         case CLS_TASK_SPLIT :
            taskID = _taskMgr.getTaskID() ;
            // memory will be freed in clsTaskMgr destructor
            pTask = SDB_OSS_NEW _clsSplitTask ( taskID ) ;
            type = CLS_SHARD ;
            break ;
         default :
            PD_LOG ( PDERROR, "Unknow job type[%d]", jobType ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }

      if ( SDB_OK == rc && !pTask )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for task[type:%d]",
                  jobType ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      else if ( !pTask )
      {
         goto error ;
      }

      rc = pTask->init( objdata ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Init task failed[rc:%d]", rc ) ;
         goto error ;
      }

      //add to taskMgr, the task will delete in taskMgr whether suc or failed
      rc = _taskMgr.addTask( pTask, taskID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to add task, rc = %d", rc ) ;
         pTask = NULL ;
         goto error ;
      }

      _clsLatch.get() ;
      _mapTaskID[ pTask->taskID() ] = taskID ;
      _clsLatch.release() ;

      //start inner session
      tid = (UINT32)taskID ;
      rc = startInnerSession ( type, tid, (void *)pTask ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to start inner session, rc = %d",
                  rc ) ;
         pTask = NULL ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR__ADDTSKINSN, rc );
      return rc ;
   error:
      if ( pTask )
      {
         SDB_OSS_DEL pTask ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_SETTMR, "_clsMgr::setTimer" )
   UINT64 _clsMgr::setTimer ( CLS_MEMBER_TYPE type, UINT32 milliSec )
   {
      UINT64 rc;
      PD_TRACE_ENTRY ( SDB__CLSMGR_SETTMR );
      UINT32 timeID = 0 ;
      _netTimeoutHandler * pHandler = &_shdTimerHandler ;
      _netRouteAgent * pRtAgent = &_shardNetRtAgent ;

      if ( CLS_REPL == type )
      {
         pHandler = &_replTimerHandler ;
         pRtAgent = &_replNetRtAgent ;
      }

      if ( pRtAgent->addTimer( milliSec, pHandler, timeID ) == SDB_OK )
      {
         rc = ossPack32To64( (UINT32)type, timeID ) ;
      }
      else
      {
         rc = CLS_INVALID_TIMERID ;
      }
      PD_TRACE1( SDB__CLSMGR_SETTMR, PD_PACK_ULONG(rc) );
      PD_TRACE_EXIT ( SDB__CLSMGR_SETTMR );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_KILLTMR, "_clsMgr::killTimer" )
   void _clsMgr::killTimer( UINT64 timerID )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR_KILLTMR );
      UINT32 type = 0 ;
      UINT32 netTimerID = 0 ;

      ossUnpack32From64 ( timerID, type, netTimerID ) ;

      _netRouteAgent * pRtAgent = &_shardNetRtAgent ;

      if ( CLS_REPL == (INT32)type )
      {
         pRtAgent = &_replNetRtAgent ;
      }

      pRtAgent->removeTimer( netTimerID ) ;
      PD_TRACE_EXIT ( SDB__CLSMGR_KILLTMR );
   }

   INT32 _clsMgr::sendToCatlog ( MsgHeader * msg )
   {
      return _shdObj.sendToCatlog ( msg ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__SNDREGMSG, "_clsMgr::_sendRegisterMsg" )
   INT32 _clsMgr::_sendRegisterMsg ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__SNDREGMSG );
      pmdKRCB *pKRCB = pmdGetKRCB () ;
      BSONObjBuilder bsonBuilder ;
      const CHAR* hostName = pmdGetKRCB()->getHostName() ;

      bsonBuilder.append ( CAT_TYPE_FIELD_NAME,  (INT32)(pKRCB->getDBRole()) ) ;
      bsonBuilder.append ( CAT_HOST_FIELD_NAME, hostName ) ;
      bsonBuilder.append ( PMD_OPTION_DBPATH, pKRCB->getDBPath() ) ;

      if ( utilCheckInstanceID( pKRCB->getOptionCB()->getInstanceID(), FALSE ) )
      {
         bsonBuilder.append ( PMD_OPTION_INSTANCE_ID,
                              pKRCB->getOptionCB()->getInstanceID() ) ;
      }

      BSONArrayBuilder subServiceBuild( bsonBuilder.subarrayStart(
         CAT_SERVICE_FIELD_NAME ) ) ;

      /// local
      BSONObjBuilder subLocalBuild( subServiceBuild.subobjStart() ) ;
      subLocalBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                            (INT32)MSG_ROUTE_LOCAL_SERVICE ) ;
      subLocalBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                            pKRCB->getSvcname() ) ;
      subLocalBuild.done() ;

      /// repl
      BSONObjBuilder subReplBuild( subServiceBuild.subobjStart() ) ;
      subReplBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                            (INT32)_replServiceID ) ;
      subReplBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                            _replServiceName ) ;
      subReplBuild.done() ;

      /// shard
      BSONObjBuilder subShdBuild( subServiceBuild.subobjStart() ) ;
      subShdBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                           (INT32)_shardServiceID) ;
      subShdBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                           _shdServiceName) ;
      subShdBuild.done() ;

      /// cata
      BSONObjBuilder subCataBuild( subServiceBuild.subobjStart() ) ;
      subCataBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                            (INT32)MSG_ROUTE_CAT_SERVICE ) ;
      subCataBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                            pKRCB->getOptionCB()->catService() ) ;
      subCataBuild.done() ;

      subServiceBuild.done() ;

      // append IP address
      ossIPInfo ipInfo ;
      if ( ipInfo.getIPNum() > 0 )
      {
         BSONArrayBuilder subIPBuild( bsonBuilder.subarrayStart(
            CAT_IP_FIELD_NAME ) ) ;

         ossIP* ip = ipInfo.getIPs() ;
         for ( INT32 i = ipInfo.getIPNum(); i > 0; i-- )
         {
            // skip loopback IP
            if (0 != ossStrncmp( ip->ipAddr, OSS_LOOPBACK_IP,
                                 ossStrlen(OSS_LOOPBACK_IP)) )
            {
               subIPBuild.append( ip->ipAddr ) ;
            }
            ip++ ;
         }

         // support 'localhost' and '127.0.0.1' for node's hostname
         subIPBuild.append( OSS_LOOPBACK_IP ) ;
         subIPBuild.append( OSS_LOCALHOST ) ;

         subIPBuild.done() ;
      }

      BSONObj regObj = bsonBuilder.obj () ;
      UINT32 length = regObj.objsize () + sizeof ( MsgCatRegisterReq ) ;
      // free by end of the function
      CHAR * buff = (CHAR *)SDB_OSS_MALLOC ( length ) ;
      MsgCatRegisterReq *pReq = NULL ;

      if ( buff == NULL )
      {
         PD_LOG ( PDERROR, "Failed to allocate memroy for register req" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      pReq = (MsgCatRegisterReq*)buff ;
      pReq->header.messageLength = length ;
      pReq->header.opCode = MSG_CAT_REG_REQ ;
      pReq->header.requestID = 0 ;
      pReq->header.TID = 0 ;
      pReq->header.routeID.value = 0 ;
      ossMemcpy( pReq->data, regObj.objdata(), regObj.objsize() ) ;

      rc = sendToCatlog( (MsgHeader *) pReq ) ;
      PD_LOG ( PDDEBUG, "Send node register[rc: %d]", rc ) ;

   done:
      if ( buff )
      {
         SDB_OSS_FREE ( buff ) ;
         buff = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__SNDREGMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__SNDQTSKREQ, "_clsMgr::_sendQueryTaskReq" )
   INT32 _clsMgr::_sendQueryTaskReq ( UINT64 requestID, const CHAR * clFullName,
                                      const BSONObj* match )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR__SNDQTSKREQ );
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgHeader *msg = NULL ;
      INT32 rc = SDB_OK ;

      rc = msgBuildQueryMsg ( &pBuff, &buffSize, clFullName, 0, requestID,
                              0, -1, match, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      msg = ( MsgHeader* )pBuff ;
      msg->opCode = MSG_CAT_QUERY_TASK_REQ ;
      msg->TID = 0 ;
      msg->routeID.value = 0 ;

      // send msg
      rc = sendToCatlog( msg ) ;
      PD_LOG ( PDDEBUG, "Send MSG_CAT_QUERY_TASK_REQ[%s] to catalog[rc:%d]",
               match->toString().c_str(), rc ) ;
   done:
      if ( pBuff )
      {
         SDB_OSS_FREE ( pBuff ) ;
         pBuff = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__SNDQTSKREQ, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::updateCatGroup ( INT64 millisec )
   {
      return _shdObj.updateCatGroup ( millisec ) ;
   }

   //message function
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__ONCATREGRES, "_clsMgr::_onCatRegisterRes" )
   INT32 _clsMgr::_onCatRegisterRes ( NET_HANDLE handle, MsgHeader* msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__ONCATREGRES );
      NodeID nodeID ;

      // have register succeed
      if ( _regTimerID == CLS_INVALID_TIMERID )
      {
         goto done ;
      }

      rc = MSG_GET_INNER_REPLY_RC( msg ) ;
      if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         updateCatGroup ( 100 ) ;
         goto error ;
      }
      else if ( rc != SDB_OK )
      {
         PD_LOG ( PDSEVERE, "Node register failed[Respone:%d]", rc ) ;
         goto error ;
      }

      {
         //get nodeid
         BSONObj object ( MSG_GET_INNER_REPLY_DATA(msg) ) ;
         BSONElement gidEl = object.getField ( CAT_GROUPID_NAME ) ;
         BSONElement nidEl = object.getField ( CAT_NODEID_NAME ) ;

         if ( gidEl.type() != NumberInt || nidEl.type() != NumberInt )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Node register response error" ) ;
            goto error ;
         }

         //Kill register timer
         killTimer ( _regTimerID ) ;
         _regTimerID = CLS_INVALID_TIMERID ;
         _regFailedTimes = 0 ;

         //Update the net route agent the local id
         _selfNodeID.columns.groupID = (UINT32)gidEl.Int () ;
         _selfNodeID.columns.nodeID = (UINT32)nidEl.Int () ;
         _shdObj.setNodeID( _selfNodeID ) ;
         PD_LOG ( PDEVENT, "Register succeed, groupID:%u, nodeID:%u",
                  _selfNodeID.columns.groupID,
                  _selfNodeID.columns.nodeID ) ;

         BSONElement hostEle = object.getField ( CAT_HOST_FIELD_NAME ) ;
         if ( hostEle.type() == String )
         {
            /*
             * The node can be created by hostname or ip,
             * so the actual 'HostName' maybe current host's name or ip address.
             * Here we ensure the KRCB's HostName is consistent with catalog.
             */
            pmdGetKRCB()->setHostName( hostEle.String().c_str() ) ;
         }

         /// update dc base info
         if ( msgIsInnerOpReply( msg ) &&
              msg->messageLength > (INT32)sizeof( MsgOpReply ) +
              object.objsize() + 5 )
         {
            MsgOpReply *pReply = ( MsgOpReply* )msg ;
            if ( pReply->numReturned > 1 )
            {
               clsDCBaseInfo *pInfo = _shdObj.getDCMgr()->getDCBaseInfo() ;
               BSONObj objDCInfo( ( const CHAR* )msg + sizeof( MsgOpReply ) +
                                  ossAlign4( (UINT32)object.objsize() ) ) ;
               _shdObj.getDCMgr()->updateDCBaseInfo( objDCInfo ) ;

               pmdGetKRCB()->setDBReadonly( pInfo->isReadonly() ) ;
               pmdGetKRCB()->setDBDeactivated( !pInfo->isActivated() ) ;
            }
         }
      }

      nodeID.value = _selfNodeID.value ;
      nodeID.columns.serviceID = _replServiceID ;
      _replNetRtAgent.setLocalID ( nodeID ) ;
      nodeID.columns.serviceID = _shardServiceID ;
      _shardNetRtAgent.setLocalID ( nodeID ) ;

      // set global id
      pmdSetNodeID( _selfNodeID ) ;

      pmdGetKRCB()->callRegisterEventHandler( _selfNodeID ) ;
      pmdGetKRCB()->setBusinessOK( TRUE ) ;

      //Update the primary catlog node
      if ( SDB_OK != _shdObj.updatePrimary( msg->routeID, TRUE ) )
      {
         _shdObj.updateCatGroup () ;
      }

      //Active the shard and repl CBs
      rc = _shdObj.active () ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "active shardCB failed[rc:%d]", rc ) ;
         goto error ;
      }

      rc = _replObj.active () ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "active replCB failed[rc:%d]", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC (SDB__CLSMGR__ONCATREGRES, rc );
      return rc ;
   error:
      //Need to shutdown
      if ( rc == SDB_CAT_AUTH_FAILED )
      {
         ++_regFailedTimes ;
         if ( SDB_ROLE_CATALOG != pmdGetDBRole() ||
              _regFailedTimes >= CLS_REPLSET_MAX_NODE_SIZE )
         {
            PD_LOG ( PDSEVERE, "Catlog auth the db node failed, shutdown..." ) ;
            PMD_SHUTDOWN_DB( SDB_CAT_AUTH_FAILED ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__ONCATQTSKRES, "_clsMgr::_onCatQueryTaskRes" )
   INT32 _clsMgr::_onCatQueryTaskRes ( NET_HANDLE handle, MsgHeader * msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR__ONCATQTSKRES );
      MsgCatQueryTaskRes *res = ( MsgCatQueryTaskRes* )msg ;
      PD_LOG ( PDDEBUG, "Recieve catalog query task response[requestID:%lld, "
               "flag: %d]", msg->requestID, res->flags ) ;

      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> objList ;
      MAPTASKQUERY::iterator it ;

      // need to update catalog group
      if ( SDB_CLS_NOT_PRIMARY == res->flags )
      {
         if ( SDB_OK != _shdObj.updatePrimaryByReply( msg ) )
         {
            updateCatGroup() ;
         }
      }
      // need to clear the query task
      else if ( SDB_DMS_EOC == res->flags ||
                SDB_CAT_TASK_NOTFOUND == res->flags )
      {
         _clsLatch.get() ;
         /// if is the last query and not { TargetID : groupID }, need to
         /// query all( by { TargetID : groupID } )
         it = _mapTaskQuery.find ( msg->requestID ) ;
         if ( it != _mapTaskQuery.end() )
         {
            if ( 1 == _mapTaskQuery.size() )
            {
               BSONObj queryAll = BSON( CAT_TARGETID_NAME <<
                                        _selfNodeID.columns.groupID ) ;
               if ( 0 != queryAll.woCompare( it->second ) )
               {
                  _mapTaskQuery[ ++_taskID ] = queryAll ;
               }
            }
         }
         _mapTaskQuery.erase ( msg->requestID ) ;
         _clsLatch.release() ;
         PD_LOG ( PDINFO, "The query task[%lld] has 0 jobs", msg->requestID ) ;
      }
      else if ( SDB_OK != res->flags )
      {
         PD_LOG ( PDERROR, "Query task[%lld] failed[rc=%d]",
                  msg->requestID, res->flags ) ;
         goto error ;
      }
      else
      {
         rc = msgExtractReply ( (CHAR *)msg, &flag, &contextID, &startFrom,
                                &numReturned, objList ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         // find the task query map, and remove it
         {
            ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
            it = _mapTaskQuery.find ( msg->requestID ) ;
            if ( it == _mapTaskQuery.end() )
            {
               PD_LOG ( PDWARNING, "The query task response[%lld] is not exist",
                        msg->requestID ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            //remove the query task
            _mapTaskQuery.erase ( it ) ;
         }

         PD_LOG ( PDINFO, "The query task[%lld] has %d jobs", msg->requestID,
                  numReturned ) ;

         // add task inner session
         {
            UINT32 index = 0 ;
            while ( index < objList.size() )
            {
               rc = _addTaskInnerSession ( objList[index].objdata() ) ;
               if ( rc && SDB_CLS_MUTEX_TASK_EXIST != rc )
               {
                  startTaskCheck( objList[index] ) ;
               }
               ++index ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR__ONCATQTSKRES, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::_onStepDown( pmdEDUEvent *event )
   {
      return sdbGetReplCB()->dispatchEvent( event ) ;
   }

   INT32 _clsMgr::_onStepUp( pmdEDUEvent *event )
   {
      return sdbGetReplCB()->dispatchEvent( event ) ;
   }

   /*
      get global cls cb
   */
   clsCB* sdbGetClsCB ()
   {
      static clsCB s_clsCB ;
      return &s_clsCB ;
   }
   shardCB* sdbGetShardCB ()
   {
      return sdbGetClsCB()->getShardCB() ;
   }
   replCB* sdbGetReplCB ()
   {
      return sdbGetClsCB()->getReplCB() ;
   }

}

