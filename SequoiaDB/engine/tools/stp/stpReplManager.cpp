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

   Source File Name = stpReplManager.cpp

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
#include "stpReplManager.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgMessageFormat.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _stpReplManager implement
    */
   BEGIN_OBJ_MSG_MAP( _stpReplManager, _stpManagerBase )
      // ON_MSG
      ON_MSG( MSG_CLS_BEAT, processMessage )
      ON_MSG( MSG_CLS_BEAT_RES, processMessage )
      ON_MSG( MSG_CLS_BALLOT, processMessage )
      ON_MSG( MSG_CLS_BALLOT_RES, processMessage )

      ON_EVENT( PMD_EDU_EVENT_STEP_DOWN, handleEvent )
      ON_EVENT( PMD_EDU_EVENT_STEP_UP, handleEvent )
   END_OBJ_MSG_MAP()

   _stpReplManager::_stpReplManager( STPCB *stpCB )
   : stpManagerBase( stpCB ),
     ICLSReplAgent( _netAgent )
   {
   }

   _stpReplManager::~_stpReplManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR_ONTIMER, "_stpReplManager::onTimer" )
   void _stpReplManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      if ( _timerID == timerID )
      {
         // handle timeout
         _handleTimeout( interval ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR_PROCESSMESSAGE, "_stpReplManager::processMessage" )
   INT32 _stpReplManager::processMessage( NET_HANDLE handle,
                                          MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREPLMGR_PROCESSMESSAGE ) ;

      switch ( message->opCode )
      {
         case MSG_CLS_BEAT :
         case MSG_CLS_BEAT_RES :
         case MSG_CLS_BALLOT :
         case MSG_CLS_BALLOT_RES :
         {
            // handle replica messages
            rc = _handleMsg( handle, message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle replica message [%d], "
                         "rc: %d", message->opCode, rc ) ;
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown replica message [%d]", message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPREPLMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR_HNDEVENT, "_stpReplManager::handleEvent" )
   INT32 _stpReplManager::handleEvent( pmdEDUEvent *event )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREPLMGR_HNDEVENT ) ;

      if ( PMD_EDU_EVENT_STEP_UP == event->_eventType ||
           PMD_EDU_EVENT_STEP_DOWN == event->_eventType )
      {
         rc = _handleEvent( event ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle event [%d], rc: %d",
                      event->_eventType, rc ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Unknown event type [%d]", event->_eventType ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPREPLMGR_HNDEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR__INITIALIZE, "_stpReplManager::_initialize" )
   INT32 _stpReplManager::_initialize()
   {
      PD_TRACE_ENTRY( SDB__STPREPLMGR__INITIALIZE ) ;

      // set start shift time
      setStartShiftTime( _options->getStartShiftTime() ) ;

      PD_TRACE_EXITRC( SDB__STPREPLMGR__INITIALIZE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR__POSTACTIVE, "_stpReplManager::_postActivate" )
   INT32 _stpReplManager::_postActivate()
   {
      PD_TRACE_ENTRY( SDB__STPREPLMGR__POSTACTIVE ) ;

      // set ID of main EDU
      setMainEDUID( _eduID ) ;

      // activate to vote
      _activateReplGroup() ;

      PD_TRACE_EXITRC( SDB__STPREPLMGR__POSTACTIVE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR__PREDEACTIVE, "_stpReplManager::_preDeactivate" )
   INT32 _stpReplManager::_preDeactivate()
   {
      PD_TRACE_ENTRY( SDB__STPREPLMGR__PREDEACTIVE ) ;

      // deactivate
      _deactivate() ;

      PD_TRACE_EXITRC( SDB__STPREPLMGR__PREDEACTIVE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR__ONCHANGESERVERS, "_stpReplManager::_onChangeServers" )
   INT32 _stpReplManager::_onChangeServers( UINT32 version,
                                            const STP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREPLMGR__ONCHANGESERVERS ) ;

      // update replica group by given servers
      rc = _updateReplGroup( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update replica group, rc: %d", rc ) ;

      // activate to vote
      _activate() ;

   done:
      PD_TRACE_EXITRC( SDB__STPREPLMGR__ONCHANGESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   BOOLEAN _stpReplManager::checkVoteLaunch()
   {
      // always valid
      return TRUE ;
   }

   DPS_LSN _stpReplManager::getLocalExpectLSN()
   {
      // get LSN from meta
      // NOTE: time of meta as LSN offset
      DPS_LSN metaLSN ;
      _stpCB->getMetaManager()->getMetaLSN( metaLSN ) ;
      // plus 1 for expect LSN
      if ( DPS_INVALID_LSN_OFFSET != metaLSN.offset )
      {
         ++ metaLSN.offset ;
      }
      return metaLSN ;
   }

   DPS_LSN _stpReplManager::getLocalCurrentLSN()
   {
      // get LSN from meta
      // NOTE: time of meta as LSN offset
      DPS_LSN metaLSN ;
      _stpCB->getMetaManager()->getMetaLSN( metaLSN ) ;
      return metaLSN ;
   }

   void _stpReplManager::getLSNWindow( DPS_LSN &fileBeginLSN,
                                       DPS_LSN &memBeginLSN,
                                       DPS_LSN &endLSN,
                                       DPS_LSN &expectLSN )
   {
      // get current LSN
      endLSN = getLocalCurrentLSN() ;
      // get expected LSN
      expectLSN = getLocalExpectLSN() ;
      // no file or memory concept in STP, cover all range from 0 to
      // expected LSN
      fileBeginLSN.set( 0, 0 ) ;
      memBeginLSN.set( 0, 0 ) ;
   }

   BOOLEAN _stpReplManager::isLocalOK()
   {
      // always OK
      return TRUE ;
   }

   BOOLEAN _stpReplManager::isLocalSpare()
   {
      // always no spare
      // NOTE: spare is concept of SequoiaDB, used as spare of DATA node
      return FALSE ;
   }

   UINT8 _stpReplManager::getVoteWeight()
   {
      // get weight from option
      return (UINT8)( _options->getWeight() ) ;
   }

   UINT32 _stpReplManager::getSharingBreakTime()
   {
      // get sharing break time from option
      return _options->getSharingBreakTime() ;
   }

   INT32 _stpReplManager::getSyncStrategy()
   {
      // default strategy ( no used in STP )
      return CLS_SYNC_DTF_STRATEGY ;
   }

   BOOLEAN _stpReplManager::getDetectDisk()
   {
      return FALSE ;
   }

   INT32 _stpReplManager::onLocalNotFoundInGroup()
   {
      // return SDB_SYS if local is not found in replica group
      return SDB_SYS ;
   }

   void _stpReplManager::beforePrimaryActive()
   {
      // notify other modules on before change primary event
      _stpCB->beforeChangePrimary( TRUE ) ;
   }

   void _stpReplManager::onPrimaryActive( const MsgRouteID &newPrimaryRID,
                                          const MsgRouteID &oldPrimaryRID )
   {
      // set this primary
      pmdSetPrimary( TRUE ) ;
   }

   void _stpReplManager::afterPrimaryActive( const MsgRouteID &newPrimaryRID,
                                             const MsgRouteID &oldPrimaryRID )
   {
      // notify other modules on after change primary event
      _stpCB->afterChangePrimary( newPrimaryRID, TRUE ) ;
   }

   void _stpReplManager::beforePrimaryDeactive()
   {
      // do nothing
   }

   void _stpReplManager::onPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                           const MsgRouteID &oldPrimaryRID )
   {
      // set this not primary
      pmdSetPrimary( FALSE ) ;
   }

   void _stpReplManager::afterPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                              const MsgRouteID &oldPrimaryRID )
   {
      // notify other modules to change primary
      _stpCB->afterChangePrimary( newPrimaryRID, FALSE ) ;
   }

   void _stpReplManager::onLocalGroupExpired()
   {
      // do nothing
   }

   void _stpReplManager::beforeFoundNewPrimary()
   {
      // do nothing
   }

   void _stpReplManager::afterFoundNewPrimary( const MsgRouteID &newPrimaryRID )
   {
      _stpCB->afterChangePrimary( newPrimaryRID, FALSE ) ;
   }

   void _stpReplManager::processBeatLSN( const MsgRouteID &remote,
                                         const DPS_LSN &lsn )
   {
      _sync.complete( remote, lsn, 0 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR__ACTIVATEREPLGROUP, "_stpReplManager::_activateReplGroup" )
   INT32 _stpReplManager::_activateReplGroup()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREPLMGR__ACTIVATEREPLGROUP ) ;

      UINT32 version = STP_GROUP_INVALID_VERSION ;
      STP_SERVER_LIST servers ;

      // set route ID of local
      setLocalID( _nodeManager->getLocalRID() ) ;

      // get group version and servers
      rc = _nodeManager->getServers( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get servers, rc: %d", rc ) ;

      // update replica group for version and servers
      // NOTE: use first 7 server in server list to initialize replica group
      rc = _updateReplGroup( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update replica group, rc: %d", rc ) ;

      // activate to vote
      _activate() ;

   done:
      PD_TRACE_EXITRC( SDB__STPREPLMGR__ACTIVATEREPLGROUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR__UPDATEREPLGROUP, "_stpReplManager::_updateReplGroup" )
   INT32 _stpReplManager::_updateReplGroup( UINT32 version,
                                            const STP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREPLMGR__UPDATEREPLGROUP ) ;

      NET_ROUTE_MAP nodes ;
      BOOLEAN changeStatus = FALSE ;
      UINT32 groupHashCode = 0 ;

      // build replica nodes from servers
      rc = _buildReplGroup( servers, nodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build replica group, rc: %d", rc ) ;

      // update replica group
      rc = _setGroupSet( version, nodes, groupHashCode, FALSE, changeStatus ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set group, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPREPLMGR__UPDATEREPLGROUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPREPLMGR__BUILDREPLGROUP, "_stpReplManager::_buildReplGroup" )
   INT32 _stpReplManager::_buildReplGroup( const STP_SERVER_LIST &servers,
                                           NET_ROUTE_MAP &nodes )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPREPLMGR__BUILDREPLGROUP ) ;

      nodes.clear() ;

      // only fetch first 7 nodes
      for ( STP_SERVER_LIST::const_iterator iter = servers.begin() ;
            iter != servers.end() && nodes.size() < CLS_REPLSET_MAX_NODE_SIZE ;
            ++ iter )
      {
         netRouteNode node ;

         // build replica node from server
         rc = iter->toRouteNode( node ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build route node for server %s, "
                      "rc: %d", iter->toString().c_str(), rc ) ;

         // save to output
         nodes.insert( make_pair( node._id.value, node ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPREPLMGR__BUILDREPLGROUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
