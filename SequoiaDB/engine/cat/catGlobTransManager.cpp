/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = catGlobTransManager.cpp

   Descriptive Name = Catalog Global Transaction Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for global
   transaction manager in Catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catGlobTransManager.hpp"
#include "catGTSDef.hpp"
#include "catCommon.hpp"
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "rtn.hpp"
#include "rtnContextBuff.hpp"
#include "msgMessageFormat.hpp"
#include "pmd.hpp"
#include "dpsUtil.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{
   // if node had not reported for 2 minutes, it will be kicked out
   // from lowTran calculation ( consider it is down or disconnected )
   #define CAT_LOWTRAN_TIMEOUT   ( 2 * 60 * OSS_ONE_SEC )

   /*
      _catLowTranRecord implement
    */
   _catLowTranRecord::_catLowTranRecord()
   : _role( SDB_ROLE_DATA ),
     _lowTran( DPS_INVALID_TRANSID_SN ),
     _expireTran( DPS_INVALID_TRANSID_SN ),
     _globTransEnabled( FALSE ),
     _transOn( FALSE ),
     _globTransOn( FALSE ),
     _mvccOn( FALSE ),
     _stpAvailable( FALSE ),
     _updateTick( 0LL )
   {
      _routeID.value = MSG_INVALID_ROUTEID ;
   }

   _catLowTranRecord::_catLowTranRecord( const _catLowTranRecord &record )
   : _role( record._role ),
     _lowTran( record._lowTran ),
     _expireTran( record._expireTran ),
     _globTransEnabled( record._globTransEnabled ),
     _transOn( record._transOn ),
     _globTransOn( record._globTransOn ),
     _mvccOn( record._mvccOn ),
     _stpAvailable( record._stpAvailable ),
     _updateTick( record._updateTick )
   {
      _routeID.value = record._routeID.value ;
   }

   _catLowTranRecord::~_catLowTranRecord()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBLOWTRANRECORD_INITNODE, "_catLowTranRecord::initNode" )
   void _catLowTranRecord::initNode( const MsgRouteID &routeID )
   {
      PD_TRACE_ENTRY( SDB__CATGLOBLOWTRANRECORD_INITNODE ) ;

      // NOTE: only COORD and DATA nodes are needed
      _role = ( COORD_GROUPID == routeID.columns.groupID ) ?
              SDB_ROLE_COORD :
              SDB_ROLE_DATA ;

      _routeID.value = routeID.value ;
      _updateTick = pmdGetDBTick() ;

      PD_TRACE_EXIT( SDB__CATGLOBLOWTRANRECORD_INITNODE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBLOWTRANRECORD_UPDATELOWTRAN, "_catLowTranRecord::updateLowTran" )
   void _catLowTranRecord::updateLowTran( DPS_TRANSID_SN lowTran,
                                          DPS_TRANSID_SN expireTran,
                                          BOOLEAN transOn,
                                          BOOLEAN globTransOn,
                                          BOOLEAN mvccOn,
                                          BOOLEAN stpAvailable )
   {
      PD_TRACE_ENTRY( SDB__CATGLOBLOWTRANRECORD_UPDATELOWTRAN ) ;

      _lowTran = lowTran ;
      _expireTran = expireTran ;
      _transOn = transOn ;
      _globTransOn = globTransOn ;
      _mvccOn = mvccOn ;
      _stpAvailable = stpAvailable ;
      _updateTick = pmdGetDBTick() ;

      PD_TRACE_EXIT( SDB__CATGLOBLOWTRANRECORD_UPDATELOWTRAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBLOWTRANRECORD_GETLOWTRAN, "_catLowTranRecord::getLowTran" )
   void _catLowTranRecord::getLowTran( DPS_TRANSID_SN &lowTran,
                                       DPS_TRANSID_SN &expireTran )
   {
      PD_TRACE_ENTRY( SDB__CATGLOBLOWTRANRECORD_GETLOWTRAN ) ;

      UINT64 updatePassed = pmdGetTickSpanTime( _updateTick ) ;

      // set to invalid value
      lowTran = DPS_INVALID_TRANSID_SN ;
      expireTran = DPS_INVALID_TRANSID_SN ;

      if ( updatePassed > CAT_LOWTRAN_TIMEOUT )
      {
         // if this node had not reported for a while, kick it out from
         // global lowTran calculation, return it's lowTran as max value
         // which is not counted in global lowTran calculation
         PD_LOG( PDDEBUG, "Node %s had not been reported lowTran "
                 "for %llu ms, kick it out",
                 routeID2String( _routeID ).c_str(), updatePassed ) ;
         lowTran = DPS_MAX_TRANSID_SN ;
         expireTran = DPS_MAX_TRANSID_SN ;
      }
      else
      {
         // if this node is normal, fetch node lowTran directly
         lowTran = _lowTran ;
         expireTran = _expireTran ;
      }

      PD_TRACE_EXIT( SDB__CATGLOBLOWTRANRECORD_GETLOWTRAN ) ;
   }

   /*
      _catGlobTransManager implement
    */
   _catGlobTransManager::_catGlobTransManager()
   : _globLowTran( DPS_INVALID_TRANSID_SN ),
     _globExpireTran( DPS_INVALID_TRANSID_SN ),
     _lowTranMapLoaded( FALSE )
   {
   }

   _catGlobTransManager::~_catGlobTransManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER_CLEARGLOBLOWTRAN, "_catGlobTransManager::clearGlobLowTran" )
   void _catGlobTransManager::clearGlobLowTran()
   {
      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER_CLEARGLOBLOWTRAN ) ;

      ossScopedRWLock lock( &_lowTranMutex, EXCLUSIVE ) ;

      _globLowTran = DPS_INVALID_TRANSID_SN ;
      _globExpireTran = DPS_INVALID_TRANSID_SN ;
      _lowTranMapLoaded = FALSE ;
      _lowTranMap.clear() ;

      PD_TRACE_EXIT( SDB__CATGLOBTRANSMANAGER_CLEARGLOBLOWTRAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER_GETGLOBLOWTRAN, "_catGlobTransManager::getGlobLowTran" )
   DPS_TRANSID_SN _catGlobTransManager::getGlobLowTran()
   {
      DPS_TRANSID_SN globLowTran = DPS_INVALID_TRANSID_SN ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER_GETGLOBLOWTRAN ) ;

      ossScopedRWLock lock( &_lowTranMutex, SHARED ) ;
      globLowTran = _globLowTran ;

      PD_TRACE_EXIT( SDB__CATGLOBTRANSMANAGER_GETGLOBLOWTRAN ) ;

      return globLowTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER_UPDATEGLOBLOWTRAN, "_catGlobTransManager::updateGlobLowTran" )
   INT32 _catGlobTransManager::updateGlobLowTran( const MsgRouteID &nodeRID,
                                                  DPS_TRANSID_SN lowTran,
                                                  DPS_TRANSID_SN expireTran,
                                                  BOOLEAN transOn,
                                                  BOOLEAN globTransOn,
                                                  BOOLEAN mvccOn,
                                                  BOOLEAN stpAvailable,
                                                  DPS_TRANSID_SN &globLowTran,
                                                  DPS_TRANSID_SN &globExpireTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER_UPDATEGLOBLOWTRAN ) ;

      globLowTran = DPS_MAX_TRANSID_SN ;
      BOOLEAN hasUpdated = FALSE ;
      GTS_NODE_SET transNodes ;

      PD_LOG( PDINFO, "Got node lowTran [%s] "
              "expireTran [%s] transOn [%s] globTransOn [%s] "
              "mvccOn [%s] stpAvailable [%s] from route ID %s",
              dpsTransSNToString( lowTran ).c_str(),
              dpsTransSNToString( expireTran ).c_str(),
              transOn ? "TRUE" : "FALSE",
              globTransOn ? "TRUE" : "FALSE",
              mvccOn ? "TRUE" : "FALSE",
              stpAvailable ? "TRUE" : "FALSE",
              routeID2String( nodeRID ).c_str() ) ;

      ossScopedRWLock lock( &_lowTranMutex, EXCLUSIVE ) ;

      if ( !_lowTranMapLoaded )
      {
         PD_LOG( PDDEBUG, "Transaction nodes are expired, need reload from "
                 "system collection" ) ;

         // load all transaction nodes ( including COORD and DATA nodes )
         rc = _loadTransNodes( transNodes ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load transaction nodes, "
                      "rc: %d", rc ) ;

         // merge nodes ( check new or removed nodes )
         rc = _mergeTransNodes( transNodes ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to merge transaction nodes, "
                      "rc: %d", rc ) ;

         _lowTranMapLoaded = TRUE ;
      }

      // update lowTran of specified node
      rc = _updateNodeLowTran( nodeRID,
                               lowTran,
                               expireTran,
                               transOn,
                               globTransOn,
                               mvccOn,
                               stpAvailable ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update lowTran for node %s, "
                   "rc: %d", routeID2String( nodeRID ).c_str(), rc ) ;

      // re-calculate global lowTran
      rc = _calcGlobLowTran( globLowTran, globExpireTran ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to calculate global lowTran, "
                   "rc: %d", rc ) ;

      if ( DPS_INVALID_TRANSID_SN != globLowTran &&
           DPS_MAX_TRANSID_SN != globLowTran )
      {
         // update valid global lowTran
         if ( _globLowTran != globLowTran )
         {
            hasUpdated = TRUE ;
         }
         _globLowTran = globLowTran ;
      }

      if ( DPS_INVALID_TRANSID_SN != globExpireTran &&
           DPS_MAX_TRANSID_SN != globExpireTran )
      {
         // update valid global expireTran
         if ( _globExpireTran != globExpireTran )
         {
            hasUpdated = TRUE ;
         }
         _globExpireTran = globExpireTran ;
      }

      if ( hasUpdated )
      {
         PD_LOG( PDDEBUG, "Update global lowTran to [%s], "
                 "global expireTran to [%s]",
                 dpsTransSNToString( globLowTran ).c_str(),
                 dpsTransSNToString( globExpireTran ).c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATGLOBTRANSMANAGER_UPDATEGLOBLOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER__UPDATENODELOWTRAN, "_catGlobTransManager::_updateNodeLowTran" )
   INT32 _catGlobTransManager::_updateNodeLowTran( const MsgRouteID &nodeRID,
                                                   DPS_TRANSID_SN lowTran,
                                                   DPS_TRANSID_SN expireTran,
                                                   BOOLEAN transOn,
                                                   BOOLEAN globTransOn,
                                                   BOOLEAN mvccOn,
                                                   BOOLEAN stpAvailable )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER__UPDATENODELOWTRAN ) ;

      // find node
      GTS_LOWTRAN_MAP::iterator iter = _lowTranMap.find( nodeRID ) ;
      PD_CHECK( iter != _lowTranMap.end(),
                SDBCM_NODE_NOTEXISTED, error, PDERROR,
                "Failed to find node %s to update lowTran",
                routeID2String( nodeRID ).c_str() ) ;

      // update node
      iter->second.updateLowTran( lowTran,
                                  expireTran,
                                  transOn,
                                  globTransOn,
                                  mvccOn,
                                  stpAvailable ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATGLOBTRANSMANAGER__UPDATENODELOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER__CALCGLOBLOWTRAN, "_catGlobTransManager::_calcGlobLowTran" )
   INT32 _catGlobTransManager::_calcGlobLowTran( DPS_TRANSID_SN &globLowTran,
                                                 DPS_TRANSID_SN &globExpireTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER__CALCGLOBLOWTRAN ) ;

      UINT32 numInvalidExpireTran = 0 ;

      globLowTran = DPS_MAX_TRANSID_SN ;
      globExpireTran = DPS_MAX_TRANSID_SN ;

      // global lowTran can be calculated in one round between all nodes
      // global expireTran need two rounds:
      // - the first round to calculate global lowTran
      // - the second round to use global lowTran to calculate global expire
      //   lowTran

      for ( GTS_LOWTRAN_MAP::iterator iter = _lowTranMap.begin() ;
            iter != _lowTranMap.end() ;
            ++ iter )
      {
         DPS_TRANSID_SN nodeLowTran = DPS_INVALID_TRANSID_SN ;
         DPS_TRANSID_SN nodeExpireTran = DPS_INVALID_TRANSID_SN ;

         iter->second.getLowTran( nodeLowTran, nodeExpireTran ) ;

         // if node lowTran is invalid, it means the node had not report
         // lowTran yet
         // in this case, the global lowTran is not completed
         PD_CHECK( DPS_INVALID_TRANSID_SN != nodeLowTran,
                   SDB_GLOB_LOWTRAN_UNKNOWN, error, PDWARNING,
                   "Failed to calculate global lowTran, lowTran of node %s "
                   "had not been reported yet",
                   routeID2String( iter->first ).c_str() ) ;

         // global lowTran is the minimum running transaction among all nodes
         globLowTran = OSS_MIN( nodeLowTran, globLowTran ) ;

         // global expireTran is the maximum expired transaction among
         // all nodes
         if ( DPS_INVALID_TRANSID_SN != nodeExpireTran )
         {
            globExpireTran = OSS_MIN( nodeExpireTran, globExpireTran ) ;
         }
         else
         {
            // if node expireTran is invalid, it means the node had not report
            // expireTran yet, in this case, global expireTran is not completed
            PD_LOG( PDWARNING, "expireTran of node %s had not been "
                    "reported yet", routeID2String( iter->first ).c_str() ) ;
            ++ numInvalidExpireTran ;
         }
      }

      if ( numInvalidExpireTran > 0 )
      {
         // if node expireTran is invalid, it means the node had not report
         // expireTran yet, in this case, global expireTran is not completed
         globExpireTran = DPS_INVALID_TRANSID_SN ;
      }

      // in consideration of network delay, adjust global lowTran and
      // expireTran by maximum time error
      if ( DPS_INVALID_TRANSID_SN != globLowTran &&
           DPS_MAX_TRANSID_SN != globLowTran &&
           globLowTran > STP_MAX_TIME_ERROR_US )
      {
         globLowTran -= STP_MAX_TIME_ERROR_US ;
      }
      if ( DPS_INVALID_TRANSID_SN != globExpireTran &&
           DPS_MAX_TRANSID_SN != globExpireTran &&
           globExpireTran > STP_MAX_TIME_ERROR_US )
      {
         globExpireTran -= STP_MAX_TIME_ERROR_US ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATGLOBTRANSMANAGER__CALCGLOBLOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER__LOADTRANSNODES, "_catGlobTransManager::_loadTransNodes" )
   INT32 _catGlobTransManager::_loadTransNodes( GTS_NODE_SET &transNodes )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER__LOADTRANSNODES ) ;

      rtnQueryOptions options ;
      INT64 contextID = -1 ;
      rtnContextBuf buffer ;
      pmdEDUCB *eduCB = pmdGetThreadEDUCB() ;
      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      // read all records from SYSCAT.SYSNODES
      options.setCLFullName( CAT_NODE_INFO_COLLECTION ) ;
      rc = rtnQuery( options, eduCB, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run query on collection [%s], "
                   "rc: %d", rc ) ;

      // loop all records
      while ( TRUE )
      {
         rc = rtnGetMore( contextID, 1, buffer, eduCB, rtnCB ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               // end of query context
               rc = SDB_OK ;
               contextID = -1 ;
               break ;
            }
            PD_LOG ( PDERROR, "Failed to fetch from collection [%s], rc: %d",
                     CAT_NODE_INFO_COLLECTION, rc ) ;
            goto error ;
         }
         // parse record
         if ( buffer.data() != NULL )
         {
            try
            {
               UINT32 groupID = INVALID_GROUPID ;
               BSONObj groupInfo( buffer.data() ) ;
               BSONElement element ;

               // get group ID field
               element = groupInfo.getField( CAT_GROUPID_NAME ) ;
               PD_CHECK( element.isNumber(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to parse group info, "
                         "field [%s] should be a number",
                         CAT_GROUPID_NAME ) ;
               groupID = (UINT32)( element.numberInt() ) ;

               // only check COORD and DATA nodes
               if ( COORD_GROUPID != groupID &&
                    ( CAT_DATA_GROUP_ID_BEGIN > groupID ||
                      DATA_GROUP_ID_END < groupID ) )
               {
                  continue ;
               }

               // check group array
               // NOTE: if element is not array, means the group is empty
               element = groupInfo.getField( CAT_GROUP_NAME ) ;
               if ( Array == element.type() )
               {
                  BSONObjIterator iter( element.embeddedObject() ) ;
                  while ( iter.more() )
                  {
                     UINT16 nodeID = INVALID_NODEID ;
                     MsgRouteID routeID ;
                     BSONElement subElement ;
                     BSONObj nodeInfo ;

                     routeID.value = MSG_INVALID_ROUTEID ;

                     // get node info object
                     subElement = iter.next() ;
                     PD_CHECK( Object == subElement.type(),
                               SDB_CAT_CORRUPTION, error, PDERROR,
                               "Failed to parse node info, "
                               "node info should be an object" ) ;

                     nodeInfo = subElement.embeddedObject() ;

                     // get node ID field
                     subElement = nodeInfo.getField( CAT_NODEID_NAME ) ;
                     PD_CHECK( subElement.isNumber(),
                               SDB_CAT_CORRUPTION, error, PDERROR,
                               "Failed to parse node info, "
                               "field [%s] should be a number",
                               CAT_NODEID_NAME ) ;

                     nodeID = (UINT16)( subElement.numberInt() ) ;

                     // generate route ID with group ID and node ID
                     routeID.columns.groupID = groupID ;
                     routeID.columns.nodeID = nodeID ;
                     routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

                     // insert to transaction node list
                     transNodes.insert( routeID ) ;
                  }
               }
            }
            catch ( exception &e )
            {
               PD_LOG ( PDERROR, "Failed to parse group info, error: %s",
                        e.what() ) ;
               rc = SDB_CAT_CORRUPTION ;
               goto error ;
            }
         }
      }

   done:
      // kill context if needed
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__CATGLOBTRANSMANAGER__LOADTRANSNODES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER__MERGETRANSNODES, "_catGlobTransManager::_mergeTransNodes" )
   INT32 _catGlobTransManager::_mergeTransNodes( const GTS_NODE_SET &transNodes )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER__MERGETRANSNODES ) ;

      // merge transaction nodes, remove deleted nodes and add new nodes
      // WARNING: should be accessed under _lowTranMutex
      try
      {
         GTS_LOWTRAN_MAP::iterator lowTranIter ;
         GTS_NODE_SET::const_iterator nodeIter ;

         // check deleted nodes
         lowTranIter = _lowTranMap.begin() ;
         while ( lowTranIter != _lowTranMap.end() )
         {
            if ( transNodes.end() == transNodes.find( lowTranIter->first ) )
            {
               // node is not found in new transaction list, remove it from
               // lowTran map
               _lowTranMap.erase( lowTranIter ++ ) ;
            }
            else
            {
               ++ lowTranIter ;
            }
         }

         // check new nodes
         for ( nodeIter = transNodes.begin() ;
               nodeIter != transNodes.end() ;
               ++ nodeIter )
         {
            const MsgRouteID &routeID = *nodeIter ;
            lowTranIter = _lowTranMap.find( routeID ) ;
            if ( _lowTranMap.end() == lowTranIter )
            {
               // node is not found in lowTran map, it is newly added,
               // add to transMap, and mark it's lowTran invalid
               // ( means lowTran has not been reported yet )
               catLowTranRecord record ;
               record.initNode( routeID ) ;
               _lowTranMap.insert( make_pair( routeID, record ) ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to merge transaction nodes, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATGLOBTRANSMANAGER__MERGETRANSNODES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
