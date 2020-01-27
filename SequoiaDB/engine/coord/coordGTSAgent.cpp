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
   : _resource( NULL )
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
