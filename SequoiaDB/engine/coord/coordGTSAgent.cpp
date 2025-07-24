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

   Source File Name = coordGTSAgent.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   global transaction control in COORD.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordGTSAgent.hpp"
#include "pmdEnv.hpp"
#include "pmd.hpp"
#include "dpsTransCB.hpp"
#include "dpsTransID.hpp"
#include "coordRemoteSession.hpp"
#include "coordCB.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordGTSAgent implement
    */
   _coordGTSAgent::_coordGTSAgent()
   : _dpsGTSAgent(),
     _resource( NULL )
   {
   }

   _coordGTSAgent::~_coordGTSAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_INIT, "_coordGTSAgent::init" )
   INT32 _coordGTSAgent::init( _coordResource *resource )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_INIT ) ;

      _resource = resource ;

      PD_TRACE_EXITRC( SDB__COORDGTSAGENT_INIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_FINI, "_coordGTSAgent::fini" )
   void _coordGTSAgent::fini()
   {
      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_FINI ) ;

      _resource = NULL ;

      PD_TRACE_EXIT( SDB__COORDGTSAGENT_FINI ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_UPDATEGLOBLOWTRAN, "_coordGTSAgent::updateGlobLowTran" )
   INT32 _coordGTSAgent::updateGlobLowTran()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_UPDATEGLOBLOWTRAN ) ;

      coordGroupSession session ;
      pmdSubSession *subSession = NULL ;

      MsgGTSLowTranReq request ;
      MsgHeader *receiveMessage = NULL ;

      DPS_TRANSID_SN localLowTran = DPS_INVALID_TRANSID_SN ;
      DPS_TRANSID_SN localExpireTran = DPS_INVALID_TRANSID_SN ;
      BSONObj requestObject ;
      netIOVec iov ;

      // only COORD and DATA need report
      // NOTE: here is COORD node
      if ( SDB_ROLE_COORD != pmdGetDBRole() )
      {
         goto done ;
      }

      // initialize group session
      rc = session.init( _resource, pmdGetThreadEDUCB() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize session, rc: %d", rc ) ;

      // send message to CATALOG primary
      session.getGroupSel()->setPrimary( TRUE ) ;
      session.getGroupSel()->setServiceType( MSG_ROUTE_CAT_SERVICE ) ;

      // get local lowTran
      rc = _getLocalLowTran( localLowTran, localExpireTran ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get local lowTran, rc: %d", rc ) ;

      // fill lowTran request
      rc = _fillLowTranReq( &request, localLowTran, localExpireTran,
                            requestObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to fill lowTran request, rc: %d", rc ) ;

      // request only have header, so we could push request object right after
      // message header
      iov.push_back( netIOV( requestObject.objdata(),
                             requestObject.objsize() ) ) ;

      while( TRUE )
      {
         session.getSession()->clearSubSession() ;

         MsgOpReply *reply = NULL ;

         // send request to CATALOG
         rc = session.sendMsg( (MsgHeader *)( &request ),
                               CATALOG_GROUPID,
                               &iov,
                               &subSession ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to send lowTran request, rc: %d",
                      rc ) ;

         // wait for reply
         rc = session.getSession()->waitReply1( TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to wait lowTran response, rc: %d",
                      rc ) ;

         // extract received message
         receiveMessage = subSession->getRspMsg() ;

         SDB_ASSERT( NULL != receiveMessage, "receive message is invalid" ) ;
         SDB_ASSERT( MSG_GTS_LOWTRAN_RSP == receiveMessage->opCode,
                     "receive message is not global lowTran response" ) ;

         // extract reply
         reply = (MsgOpReply *)receiveMessage ;
         rc = reply->flags ;
         SDB_ASSERT( reply->contextID == -1, "Context id must be -1" ) ;

         if ( SDB_OK != rc )
         {
            // response is not OK, check if we could retry
            coordGroupSessionCtrl *groupCtrl = session.getGroupCtrl() ;
            UINT32 primaryID = reply->startFrom ;

            if ( groupCtrl->canRetry( rc, receiveMessage->routeID,
                                      primaryID, TRUE, TRUE ) )
            {
               groupCtrl->incRetry() ;
               continue ;
            }

            PD_RC_CHECK( rc, PDERROR, "Failed to process global lowTran "
                         "response, rc: %d", rc ) ;
         }
         else
         {
            // extract global lowTran from response, and update
            MsgGTSLowTranRsp *response = (MsgGTSLowTranRsp *)receiveMessage ;
            DPS_TRANSID_SN globLowTran = DPS_INVALID_TRANSID_SN ;
            DPS_TRANSID_SN globExpireTran = DPS_INVALID_TRANSID_SN ;

            rc = _parseLowTranRsp( response, globLowTran, globExpireTran ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse lowTran response, "
                         "rc: %d", rc ) ;

            _setGlobLowTran( globLowTran, globExpireTran ) ;

            // quit
            break ;
         }

         SDB_ASSERT( FALSE, "Should not go here" ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDGTSAGENT_UPDATEGLOBLOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_ARBITGLOBTRANS, "_coordGTSAgent::arbitGlobTrans" )
   INT32 _coordGTSAgent::arbitGlobTrans( pmdEDUCB *eduCB,
                                         const DPS_TRANS_ID &readTransID,
                                         const DPS_TRANS_ID &writeTransID,
                                         DPS_TRANS_STATUS writeTransStatus,
                                         BOOLEAN forceLocal,
                                         BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_ARBITGLOBTRANS ) ;

      visible = FALSE ;

      // COORD shouldn't arbitrate global transaction
      SDB_ASSERT( FALSE, "COORD should not arbitrate global transaction" ) ;
      PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                "COORD should not arbitrate global transaction" ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORDGTSAGENT_ARBITGLOBTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_WAITARBITCOMMIT, "_coordGTSAgent::waitArbitCommit" )
   INT32 _coordGTSAgent::waitArbitCommit( pmdEDUCB *eduCB,
                                          const DPS_TRANS_ID &arbitTransID,
                                          INT32 timeout,
                                          BOOLEAN &committed,
                                          BOOLEAN &multiGroups,
                                          stpLogicalTimeUS &commitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_WAITARBITCOMMIT ) ;

      // COORD shouldn't wait for transaction commit
      SDB_ASSERT( FALSE, "COORD should not wait for transaction commit" ) ;
      PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                "COORD should not wait for transaction commit" ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORDGTSAGENT_WAITARBITCOMMIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_WAITARBITCHANGE, "_coordGTSAgent::waitArbitChange" )
   INT32 _coordGTSAgent::waitArbitChange( pmdEDUCB *eduCB,
                                          const DPS_TRANS_ID &arbitTransID,
                                          DPS_TRANS_STATUS currentStatus,
                                          INT32 timeout,
                                          dpsTransBackInfo &newInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_WAITARBITCHANGE ) ;

      // COORD shouldn't wait for transaction status
      SDB_ASSERT( FALSE, "COORD should not wait for transaction status" ) ;
      PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                "COORD should not wait for transaction status" ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORDGTSAGENT_WAITARBITCHANGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_ONATTACH, "_coordGTSAgent::onAttach" )
   void _coordGTSAgent::onAttach( pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_ONATTACH ) ;

      CoordCB *coord = pmdGetKRCB()->getCoordCB() ;
      coord->getRSManager()->registerEDU( eduCB ) ;

      PD_TRACE_EXIT( SDB__COORDGTSAGENT_ONATTACH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDGTSAGENT_ONDETACH, "_coordGTSAgent::onDetach" )
   void _coordGTSAgent::onDetach( pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__COORDGTSAGENT_ONDETACH ) ;

      CoordCB *coord = pmdGetKRCB()->getCoordCB() ;
      coord->getRSManager()->unregEUD( eduCB ) ;

      PD_TRACE_EXIT( SDB__COORDGTSAGENT_ONDETACH ) ;
   }

}
