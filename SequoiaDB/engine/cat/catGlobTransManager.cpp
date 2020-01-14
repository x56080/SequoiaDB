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
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _catGlobTransManager implement
    */
   _catGlobTransManager::_catGlobTransManager()
   : _globLowTran( (UINT64)( DPS_MAX_TRANSID_SN ) ),
     _lowTranMapLoaded( FALSE )
   {
   }

   _catGlobTransManager::~_catGlobTransManager()
   {
   }

   DPS_TRANSID_SN _catGlobTransManager::getGlobLowTran()
   {
      ossScopedRWLock lock( &_lowTranMutex, SHARED ) ;
      return _globLowTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER_UPDATEGLOBLOWTRAN, "_catGlobTransManager::updateGlobLowTran" )
   INT32 _catGlobTransManager::updateGlobLowTran( const MsgRouteID &nodeRID,
                                                  DPS_TRANSID_SN nodeLowTran,
                                                  DPS_TRANSID_SN &globLowTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER_UPDATEGLOBLOWTRAN ) ;

      globLowTran = DPS_MAX_TRANSID_SN ;
      GTS_NODE_SET transNodes ;

      PD_LOG( PDINFO, "Got node lowTran [%llu(0x%llX)] from route ID %s",
              nodeLowTran, nodeLowTran, routeID2String( nodeRID ).c_str() ) ;

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
      rc = _updateNodeLowTran( nodeRID, nodeLowTran ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update lowTran for node %s, "
                   "rc: %d", routeID2String( nodeRID ).c_str(), rc ) ;

      // re-calculate global lowTran
      rc = _calcGlobLowTran( globLowTran ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to calculate global lowTran, "
                   "rc: %d", rc ) ;

      // update global lowTran
      _globLowTran = globLowTran ;

      PD_LOG( PDEVENT, "Update global lowTran to [%llu(0x%llX)]",
              globLowTran, globLowTran ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATGLOBTRANSMANAGER_UPDATEGLOBLOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER__UPDATENODELOWTRAN, "_catGlobTransManager::_updateNodeLowTran" )
   INT32 _catGlobTransManager::_updateNodeLowTran( const MsgRouteID &nodeRID,
                                                   DPS_TRANSID_SN nodeLowTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER__UPDATENODELOWTRAN ) ;

      try
      {
         // replace with specified node route ID
         _lowTranMap[ nodeRID.value ] = nodeLowTran ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update low transaction for node %s, "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATGLOBTRANSMANAGER__UPDATENODELOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGLOBTRANSMANAGER__CALCGLOBLOWTRAN, "_catGlobTransManager::_calcGlobLowTran" )
   INT32 _catGlobTransManager::_calcGlobLowTran( DPS_TRANSID_SN &globLowTran )
   {
      INT32 rc = SDB_OK ;

      globLowTran = DPS_MAX_TRANSID_SN ;

      PD_TRACE_ENTRY( SDB__CATGLOBTRANSMANAGER__CALCGLOBLOWTRAN ) ;

      for ( GTS_LOWTRAN_MAP::iterator iter = _lowTranMap.begin() ;
            iter != _lowTranMap.end() ;
            ++ iter )
      {
         DPS_TRANSID_SN nodeLowTran = iter->second ;

         // if node lowTran is invalid, it means the node had not report
         // lowTran yet
         // in this case, the global lowTran is not completed
         PD_CHECK( DPS_INVALID_TRANSID_SN != nodeLowTran,
                   SDB_GLOB_LOWTRAN_UNKNOWN, error, PDERROR,
                   "Failed to calculate global lowTran, lowTran of node %s "
                   "had not reported yet",
                   routeID2String( iter->first ).c_str() ) ;

         // global lowTran is the earliest running transaction among nodes
         globLowTran = OSS_MIN( nodeLowTran, globLowTran ) ;
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
                     transNodes.insert( routeID.value ) ;
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
            UINT64 routeIDValue = ( *nodeIter ) ;
            lowTranIter = _lowTranMap.find( routeIDValue ) ;
            if ( _lowTranMap.end() == lowTranIter )
            {
               // node is not found in lowTran map, it is newly added,
               // add to transMap, and mark it's lowTran invalid
               // ( means lowTran has not been reported yet )
               _lowTranMap[ routeIDValue ] = DPS_INVALID_TRANSID_SN ;
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
