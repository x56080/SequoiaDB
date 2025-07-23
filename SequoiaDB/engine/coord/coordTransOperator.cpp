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

   Source File Name = coordTransOperator.cpp

   Descriptive Name = Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/
#include "coordTransOperator.hpp"
#include "msgMessageFormat.hpp"
#include "coordUtil.hpp"
#include "rtnCommandDef.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordTransOperator implement
   */
   _coordTransOperator::_coordTransOperator()
   {
      _recvNum = 0 ;
   }

   _coordTransOperator::~_coordTransOperator()
   {
   }

   BOOLEAN _coordTransOperator::needRollback() const
   {
      return TRUE ;
   }

   INT32 _coordTransOperator::doOnGroups( coordSendMsgIn &inMsg,
                                          coordSendOptions &options,
                                          pmdEDUCB *cb,
                                          coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;

      if ( _canPrepareTrans( cb, inMsg.msg() ) )
      {
         if ( !cb->isTransaction() &&
              cb->getTransExecutor()->isTransAutoCommit() )
         {
            _prepareForTrans( cb, inMsg.msg() ) ;

            /// when group is one, push down autocommit to data node,
            /// otherwise begin transaction
            if ( options._groupLst.size() > 1 )
            {
               _groupSession.getPropSite()->beginTrans( cb, TRUE ) ;
            }

            /// transaction should access with primary
            options._primary = TRUE ;
         }
         else if ( cb->isTransaction() )
         {
            // need add transaction info for the send msg
            _prepareForTrans( cb, inMsg.msg() ) ;
         }
      }

      /// when data is old version
      if ( _remoteHandler.isVersion0() && _isTrans( cb, inMsg.msg() ) )
      {
         ROUTE_RC_MAP newNodeMap ;
         // build trans session on new data groups
         rc = buildTransSession( options._groupLst, cb, newNodeMap ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to build transaction session on "
                    "data nodes, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = coordOperator::doOnGroups( inMsg, options, cb, result ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Do command[%d] on data groups failed, rc: %d",
                 inMsg.opCode(), rc ) ;
         goto error ;
      }
      else if ( result.nokSize() > 0 )
      {
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordTransOperator::_isTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      if ( MSG_BS_TRANS_BEGIN_REQ == GET_REQUEST_TYPE( pMsg->opCode ) )
      {
         return FALSE ;
      }
      return cb->isTransaction() ;
   }

   BOOLEAN _coordTransOperator::_canPrepareTrans( pmdEDUCB *cb,
                                                  const MsgHeader *pMsg ) const
   {
      return TRUE ;
   }

   void _coordTransOperator::_onNodeReply( INT32 processType,
                                           MsgOpReply *pReply,
                                           pmdEDUCB *cb,
                                           coordSendMsgIn &inMsg )
   {
      if ( pReply->contextID > 0 )
      {
         _recvNum += pReply->contextID ;
      }
   }

   INT32 _coordTransOperator::releaseTransSession( SET_NODEID &nodes,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdRemoteSession *pSession = NULL ;
      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itrSub ;
      MsgOpTransRollback msgReq ;

      msgReq.header.messageLength = sizeof( MsgOpTransRollback ) ;
      msgReq.header.opCode = MSG_BS_TRANS_ROLLBACK_REQ ;
      msgReq.header.routeID.value = 0 ;
      msgReq.header.TID = cb->getTID() ;

      pSession = _groupSession.getSession() ;
      pSession->resetAllSubSession() ;

      SET_NODEID::iterator itSet = nodes.begin() ;
      while( itSet != nodes.end() )
      {
         pSub = pSession->addSubSession( *itSet ) ;
         pSub->setReqMsg( ( MsgHeader* )&msgReq, PMD_EDU_MEM_NONE ) ;

         /// delete trans node
         _groupSession.getPropSite()->delTransNode( pSub->getNodeID() ) ;

         rc = pSession->sendMsg( pSub ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Release node[%s] failed, rc: %d",
                    routeID2String( *itSet ).c_str(), rc ) ;
            /// remove the sub session
            pSession->resetSubSession( *itSet ) ;
         }
         ++itSet ;
      }

      /// get reply
      rc = pSession->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Wait reply for release transaction failed, "
                 "rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      itrSub = pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while ( itrSub.more() )
      {
         pSub = itrSub.next() ;
         MsgOpReply *pReply = ( MsgOpReply* )pSub->getRspMsg( FALSE ) ;

         if ( pReply->flags )
         {
            PD_LOG( PDWARNING, "Release node[%s]'s transaction failed, "
                    "rc: %d", routeID2String( pReply->header.routeID ).c_str(),
                    pReply->flags ) ;
         }
      }

      /// clear all sub session
      pSession->resetAllSubSession() ;

      return rc ;
   }

   INT32 _coordTransOperator::rollback( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SET_NODEID nodes ;
      DPS_TRANS_ID transID = cb->getTransID() ;

      PD_LOG ( PDEVENT, "Begin to rollback transaction[ID:%04x%010x]...",
               DPS_TRANS_GET_NODEID( transID ),
               DPS_TRANS_GET_SN( transID ) ) ;

      _groupSession.getPropSite()->dumpTransNode( nodes ) ;

      if ( 0 == nodes.size() )
      {
         goto done ;
      }

      cb->startRollback() ;
      rc = releaseTransSession( nodes, cb ) ;

   done:
      _groupSession.getPropSite()->endTrans( cb ) ;
      cb->stopRollback() ;
      return rc ;
   }

   INT32 _coordTransOperator::buildTransSession( const CoordGroupList &groupLst,
                                                 pmdEDUCB *cb,
                                                 ROUTE_RC_MAP &newNodeMap )
   {
      INT32 rc = SDB_OK ;

      if ( !cb->isTransaction() )
      {
         goto done ;
      }
      else
      {
         coordSendOptions options( TRUE ) ;
         coordProcessResult result ;
         coordSessionPropSite *pPropSite = _groupSession.getPropSite() ;

         CoordGroupList::const_iterator iterGroup ;
         MsgOpTransBegin msgReq ;
         coordSendMsgIn inMsg( (MsgHeader*)&msgReq ) ;

         msgReq.header.messageLength = sizeof( MsgOpTransBegin ) ;
         msgReq.header.opCode = MSG_BS_TRANS_BEGIN_REQ ;
         msgReq.header.routeID.value = 0 ;
         msgReq.header.TID = cb->getTID() ;
         msgReq.transID = DPS_TRANS_GET_ID( cb->getTransID() ) ;
         ossMemset( msgReq.reserved, 0, sizeof( msgReq.reserved ) ) ;

         iterGroup = groupLst.begin() ;
         while(  iterGroup != groupLst.end() )
         {
            if ( !pPropSite->hasTransNode( iterGroup->first ) )
            {
               /// not found, need to being the trans
               options._groupLst[ iterGroup->first ] = iterGroup->second ;
            }
            ++iterGroup ;
         }

         newNodeMap.clear() ;
         result._pOkRC = &newNodeMap ;
         rc = coordOperator::doOnGroups( inMsg, options, cb, result ) ;
         // add ok route id to trans node id
         if ( newNodeMap.size() > 0 )
         {
            MsgRouteID nodeID ;
            ROUTE_RC_MAP::iterator itRC = newNodeMap.begin() ;
            while( itRC != newNodeMap.end() )
            {
               nodeID.value = itRC->first ;
               pPropSite->addTransNode( nodeID ) ;
               ++itRC ;
            }
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Build transaction failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      return rc ;
   error :
      /// will rollback all suc node and the node before session
      goto done ;
   }

   /*
      _coordTransBegin implement
   */
   _coordTransBegin::_coordTransBegin()
   {
      const static string s_name( "TransBegin" ) ;
      setName( s_name ) ;
   }

   _coordTransBegin::~_coordTransBegin()
   {
   }

   INT32 _coordTransBegin::beginTrans( pmdEDUCB *cb, BOOLEAN isAutoCommit )
   {
      return _groupSession.getPropSite()->beginTrans( cb, isAutoCommit ) ;
   }

   INT32 _coordTransBegin::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      contextID   = -1 ;
      rc = beginTrans( cb ) ;
      if ( SDB_OK == rc )
      {
         /// unset all trans context
         rtnUnsetTransContext( cb, pmdGetKRCB()->getRTNCB() ) ;
      }
      return rc ;
   }

   /*
      _coord2PhaseCommit implement
   */
   _coord2PhaseCommit::_coord2PhaseCommit()
   {
   }

   _coord2PhaseCommit::~_coord2PhaseCommit()
   {
   }

   INT32 _coord2PhaseCommit::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needCancel = FALSE ;

      contextID                        = -1 ;

      if ( !cb->isTransaction() )
      {
         goto done ;
      }

      needCancel = TRUE ;
      rc = doPhase1( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute failed on phase1 in operator[%s], rc: %d",
                 getName(), rc ) ;
         goto error ;
      }

      needCancel = FALSE ;
      rc = doPhase2( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute failed on phase2 in operator[%s], rc: %d",
                 getName(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( needCancel )
      {
         INT32 rcTmp = cancelOp( pMsg, cb, contextID, buf ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Execute cancel phase in operator[%s], rc: %d",
                    getName(), rcTmp ) ;
         }
      }
      goto done ;
   }

   INT32 _coord2PhaseCommit::doPhase1( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      CHAR *pMsgReq                    = NULL ;
      INT32 msgSize                    = 0 ;

      rc = buildPhase1Msg( (const CHAR*)pMsg, &pMsgReq, &msgSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build message failed in operator[%s] phase1, "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

      // execute on data nodes
      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute on data group failed in operator[%s] "
                 "phase1, rc: %d", getName(), rc ) ;
         goto error ;
      }

   done:
      if ( pMsgReq )
      {
         releasePhase1Msg( pMsgReq, msgSize, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coord2PhaseCommit::doPhase2( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsgReq                    = NULL ;
      INT32 msgSize                    = 0 ;

      rc = buildPhase2Msg( (const CHAR*)pMsg, &pMsgReq, &msgSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build message failed in operator[%s] phase2, "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

      // execute on data nodes
      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute on data group failed in operator[%s] "
                 "phase2, rc: %d", getName(), rc ) ;
         goto error ;
      }

   done:
      if ( pMsgReq )
      {
         releasePhase2Msg( pMsgReq, msgSize, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coord2PhaseCommit::cancelOp( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      // do nothing, rollback will do in session
      return SDB_OK ;
   }

   /*
      _coordTransCommit implement
   */
   _coordTransCommit::_coordTransCommit()
   {
      const static string s_name( "TransCommit" ) ;
      setName( s_name ) ;
   }

   _coordTransCommit::~_coordTransCommit()
   {
   }

   INT32 _coordTransCommit::executeOnDataGroup( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                INT64 &contextID,
                                                rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itr ;

      ROUTE_RC_MAP nokRC ;
      _coordSessionPropSite::MAP_TRANS_NODES_CIT cit ;
      const _coordSessionPropSite::MAP_TRANS_NODES *pNodeMap = NULL ;

      pNodeMap = _groupSession.getPropSite()->getTransNodeMap() ;
      cit = pNodeMap->begin() ;

      /// clear
      _groupSession.resetSubSession() ;

      while( cit != pNodeMap->end() )
      {
         pSub = pSession->addSubSession( cit->second.value ) ;
         pSub->setReqMsg( pMsg, PMD_EDU_MEM_NONE ) ;

         rcTmp = pSession->sendMsg( pSub ) ;
         if ( rcTmp )
         {
            rc = rc ? rc : rcTmp ;
            PD_LOG ( PDWARNING, "Failed to send commit request to the "
                     "node[%s], rc: %d",
                     routeID2String( cit->second ).c_str(),
                     rcTmp ) ;
            nokRC[ cit->second.value ] = rcTmp ;
         }
         ++cit ;
      }

      rcTmp = pSession->waitReply1( TRUE ) ;
      if ( rcTmp )
      {
         rc = rc ? rc : rcTmp ;
         PD_LOG( PDERROR, "Failed to get the reply, rc: %d", rcTmp ) ;
      }

      itr = pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while ( itr.more() )
      {
         MsgOpReply *pReply = NULL ;
         pSub = itr.next() ;

         pReply = (MsgOpReply *)pSub->getRspMsg( FALSE ) ;
         rcTmp = pReply->flags ;

         if ( rcTmp )
         {
            rc = rc ? rc : rcTmp ;
            PD_LOG( PDERROR, "Data node[%s] commit transaction failed, rc: %d",
                    routeID2String( pReply->header.routeID ).c_str(),
                    rcTmp ) ;
            nokRC[ pReply->header.routeID.value ] = coordErrorInfo( pReply ) ;
         }
      }

      if ( rc )
      {
         goto error ;
      }
   done:
      _groupSession.resetSubSession() ;
      return rc ;
   error:
      if ( ( rc && nokRC.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 _coordTransCommit::buildPhase1Msg( const CHAR *pReceiveBuffer,
                                            CHAR **pMsg,
                                            INT32 *pMsgSize,
                                            pmdEDUCB *cb )
   {
      _phase1Msg.header.messageLength = sizeof( _phase1Msg ) ;
      _phase1Msg.header.opCode = MSG_BS_TRANS_COMMITPRE_REQ ;
      _phase1Msg.header.routeID.value = MSG_INVALID_ROUTEID ;
      _phase1Msg.header.requestID = 0 ;
      _phase1Msg.header.TID = cb->getTID() ;

      *pMsg = ( CHAR* )&_phase1Msg ;
      *pMsgSize = _phase1Msg.header.messageLength ;

      return SDB_OK ;
   }

   void _coordTransCommit::releasePhase1Msg( CHAR *pMsg,
                                             INT32 msgSize,
                                             pmdEDUCB *cb )
   {
   }

   INT32 _coordTransCommit::buildPhase2Msg( const CHAR *pReceiveBuffer,
                                            CHAR **pMsg,
                                            INT32 *pMsgSize,
                                            pmdEDUCB *cb )
   {
      _phase2Msg.header.messageLength = sizeof( _phase1Msg ) ;
      _phase2Msg.header.opCode = MSG_BS_TRANS_COMMIT_REQ ;
      _phase2Msg.header.routeID.value = MSG_INVALID_ROUTEID ;
      _phase2Msg.header.requestID = 0 ;
      _phase2Msg.header.TID = cb->getTID() ;

      *pMsg = ( CHAR* )&_phase2Msg ;
      *pMsgSize = _phase2Msg.header.messageLength ;

      return SDB_OK ;
   }

   void _coordTransCommit::releasePhase2Msg( CHAR *pMsg,
                                             INT32 msgSize,
                                             pmdEDUCB *cb )
   {
   }

   INT32 _coordTransCommit::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      DPS_TRANS_ID curTransID = cb->getTransID() ;

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), MSG_BS_TRANS_COMMIT_REQ,
                          "TransactionID: 0x%016x(%llu)",
                          curTransID, curTransID ) ;

      rc = _coord2PhaseCommit::execute( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

      // complete, delete transaction
      _groupSession.getPropSite()->endTrans( cb ) ;

      PD_LOG( PDINFO, "Execute commit(ID:%04x%010x)",
              DPS_TRANS_GET_NODEID( curTransID ),
              DPS_TRANS_GET_SN( curTransID ) ) ;

   done:
      return rc ;
   error:
      // rollback in session
      goto done ;
   }

   BOOLEAN _coordTransCommit::needRollback() const
   {
      return TRUE ;
   }

   /*
      _coordTransRollback implement
   */
   _coordTransRollback::_coordTransRollback()
   {
      const static string s_name( "TransRollback" ) ;
      setName( s_name ) ;
   }

   _coordTransRollback::~_coordTransRollback()
   {
   }

   INT32 _coordTransRollback::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;

      contextID                        = -1 ;

      if ( !cb->isTransaction() )
      {
         goto done ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), MSG_BS_TRANS_ROLLBACK_REQ,
                          "TransactionID: 0x%016x(%llu)",
                          cb->getTransID(),
                          cb->getTransID() ) ;

      rc = _coordTransOperator::rollback( cb ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Rollback transaction failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done;
   }

   void _coordTransRollback::_prepareForTrans( pmdEDUCB *cb,
                                               MsgHeader *pMsg )
   {
   }

   BOOLEAN _coordTransRollback::needRollback() const
   {
      return FALSE ;
   }

}

