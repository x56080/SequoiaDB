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

   Source File Name = tpReplManager.cpp

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

#include "tpReplManager.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "msgMessageFormat.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _tpReplManager implement
    */
   BEGIN_OBJ_MSG_MAP( _tpReplManager, _tpManagerBase )
      // ON_MSG
      ON_MSG( MSG_CLS_BEAT, processMessage )
      ON_MSG( MSG_CLS_BEAT_RES, processMessage )
      ON_MSG( MSG_CLS_BALLOT, processMessage )
      ON_MSG( MSG_CLS_BALLOT_RES, processMessage )
   END_OBJ_MSG_MAP()

   _tpReplManager::_tpReplManager( SDB_TPCB *tpCB )
   : tpManagerBase( tpCB ),
     ICLSReplAgent( _netAgent )
   {
   }

   _tpReplManager::~_tpReplManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR_ONTIMER, "_tpReplManager::onTimer" )
   void _tpReplManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      if ( _timerID == timerID )
      {
         _handleTimeout( interval ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR_PROCESSMESSAGE, "_tpReplManager::processMessage" )
   INT32 _tpReplManager::processMessage( NET_HANDLE handle,
                                         MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR_PROCESSMESSAGE ) ;

      switch ( message->opCode )
      {
         case MSG_CLS_BEAT :
         case MSG_CLS_BEAT_RES :
         case MSG_CLS_BALLOT :
         case MSG_CLS_BALLOT_RES :
         {
            rc = _handleMsg( handle, message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle replica message [%d], "
                         "rc: %d", message->opCode, rc ) ;
            break ;
         }
         default :
         {
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown replica message [%d]", message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPREPLMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR__INITIALIZE, "_tpReplManager::_initialize" )
   INT32 _tpReplManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR__INITIALIZE ) ;

      setStartShiftTime( _options->getStartShiftTime() ) ;

      PD_TRACE_EXITRC( SDB__TPREPLMGR__INITIALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR__POSTACTIVE, "_tpReplManager::_postActivate" )
   INT32 _tpReplManager::_postActivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR__POSTACTIVE ) ;

      setMainEDUID( _eduID ) ;
      _activateReplGroup() ;

      PD_TRACE_EXITRC( SDB__TPREPLMGR__POSTACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR__PREDEACTIVE, "_tpReplManager::_preDeactivate" )
   INT32 _tpReplManager::_preDeactivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR__PREDEACTIVE ) ;

      _deactivate() ;

      PD_TRACE_EXITRC( SDB__TPREPLMGR__PREDEACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR__ONCHANGESERVERS, "_tpReplManager::_onChangeServers" )
   INT32 _tpReplManager::_onChangeServers( UINT32 version,
                                           const TP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR__ONCHANGESERVERS ) ;

      rc = _updateReplGroup( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update replica group, rc: %d", rc ) ;

      _activate() ;

   done:
      PD_TRACE_EXITRC( SDB__TPREPLMGR__ONCHANGESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   BOOLEAN _tpReplManager::checkVoteLaunch()
   {
      return TRUE ;
   }

   DPS_LSN _tpReplManager::getLocalExpectLSN()
   {
      DPS_LSN metaLSN ;
      _tpCB->getMetaManager()->getMetaLSN( metaLSN ) ;
      return metaLSN ;
   }

   DPS_LSN _tpReplManager::getLocalCurrentLSN()
   {
      return getLocalExpectLSN() ;
   }

   void _tpReplManager::getLSNWindow( DPS_LSN &fileBeginLSN,
                                      DPS_LSN &memBeginLSN,
                                      DPS_LSN &endLSN,
                                      DPS_LSN &expectLSN )
   {
      expectLSN = getLocalExpectLSN() ;
      fileBeginLSN.set( 0, 0 ) ;
      memBeginLSN.set( 0, 0 ) ;
      endLSN = expectLSN ;
   }

   BOOLEAN _tpReplManager::isLocalOK()
   {
      return TRUE ;
   }

   BOOLEAN _tpReplManager::isLocalSpare()
   {
      return FALSE ;
   }

   UINT8 _tpReplManager::getVoteWeight()
   {
      return (UINT8)( _options->getWeight() ) ;
   }

   UINT32 _tpReplManager::getSharingBreakTime()
   {
      return _options->getSharingBreakTime() ;
   }

   INT32 _tpReplManager::getSyncStrategy()
   {
      return CLS_SYNC_DTF_STRATEGY ;
   }

   INT32 _tpReplManager::onLocalNotFoundInGroup()
   {
      return SDB_SYS ;
   }

   void _tpReplManager::beforePrimaryActive()
   {
   }

   void _tpReplManager::onPrimaryActive( const MsgRouteID &newPrimaryRID,
                                         const MsgRouteID &oldPrimaryRID )
   {
      pmdSetPrimary( TRUE ) ;
   }

   void _tpReplManager::afterPrimaryActive( const MsgRouteID &newPrimaryRID,
                                            const MsgRouteID &oldPrimaryRID )
   {
      _tpCB->onChangePrimary( newPrimaryRID, TRUE ) ;
   }

   void _tpReplManager::beforePrimaryDeactive()
   {
   }

   void _tpReplManager::onPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                           const MsgRouteID &oldPrimaryRID )
   {
      pmdSetPrimary( FALSE ) ;
   }

   void _tpReplManager::afterPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                              const MsgRouteID &oldPrimaryRID )
   {
      _tpCB->onChangePrimary( newPrimaryRID, FALSE ) ;
   }

   void _tpReplManager::onLocalGroupExpired()
   {
   }

   void _tpReplManager::onNotifiedPrimaryChange()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR__ACTIVATEREPLGROUP, "_tpReplManager::_activateReplGroup" )
   INT32 _tpReplManager::_activateReplGroup()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR__ACTIVATEREPLGROUP ) ;

      UINT32 version = TP_GROUP_INVALID_VERSION ;
      TP_SERVER_LIST servers ;

      setLocalID( _catalogManager->getLocalRID() ) ;

      rc = _catalogManager->getServers( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get servers, rc: %d", rc ) ;

      rc = _updateReplGroup( version, servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update replica group, rc: %d", rc ) ;

      _activate() ;

   done:
      PD_TRACE_EXITRC( SDB__TPREPLMGR__ACTIVATEREPLGROUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR__UPDATEREPLGROUP, "_tpReplManager::_updateReplGroup" )
   INT32 _tpReplManager::_updateReplGroup( UINT32 version,
                                           const TP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR__UPDATEREPLGROUP ) ;

      NET_ROUTE_MAP nodes ;
      BOOLEAN changeStatus = FALSE ;
      UINT32 groupHashCode = 0 ;

      rc = _buildReplGroup( servers, nodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build replica group, rc: %d", rc ) ;

      rc = _setGroupSet( version, nodes, groupHashCode, FALSE, changeStatus ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set group, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPREPLMGR__UPDATEREPLGROUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPREPLMGR__BUILDREPLGROUP, "_tpReplManager::_buildReplGroup" )
   INT32 _tpReplManager::_buildReplGroup( const TP_SERVER_LIST &servers,
                                          NET_ROUTE_MAP &nodes )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPREPLMGR__BUILDREPLGROUP ) ;

      nodes.clear() ;

      // only fetch first 7 nodes
      for ( TP_SERVER_LIST::const_iterator iter = servers.begin() ;
            iter != servers.end() && nodes.size() < CLS_REPLSET_MAX_NODE_SIZE ;
            ++ iter )
      {
         netRouteNode node ;

         rc = iter->toRouteNode( node ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build route node for server %s, "
                      "rc: %d", iter->toString().c_str(), rc ) ;

         nodes.insert( make_pair( node._id.value, node ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPREPLMGR__BUILDREPLGROUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
