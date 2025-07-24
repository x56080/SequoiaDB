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

   Source File Name = rtnCoordTransaction.cpp

   Descriptive Name = Runtime Coord Transaction

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   transaction management on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCoordTransaction.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "msgMessage.hpp"
#include "rtnCommandDef.hpp"

namespace engine
{
   INT32 rtnCoordTransBegin::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc    = SDB_OK ;
      contextID   = -1 ;

      rc = cb->createTransaction() ;
      PD_RC_CHECK( rc, PDERROR, "create transaction failed(rc=%d)", rc ) ;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoord2PhaseCommit::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      INT32 rcTmp = SDB_OK;

      contextID                        = -1 ;

      if ( !cb->isTransaction() )
      {
         goto error;
      }

      rc = doPhase1( pMsg, cb, contextID, buf );
      PD_CHECK( SDB_OK == rc, rc, errorcancel, PDERROR,
                "Execute failed on phase1(rc=%d)", rc ) ;

      rc = doPhase2( pMsg, cb, contextID, buf );
      PD_RC_CHECK( rc, PDERROR,
                   "Execute failed on phase2(rc=%d)", rc ) ;

   done:
      return rc ;
   errorcancel:
      rcTmp = cancelOp( pMsg, cb, contextID, buf );
      if ( rcTmp )
      {
         PD_LOG ( PDERROR, "Failed to cancel the operate, rc: %d",
                  rcTmp );
      }
   error:
      goto done ;
   }

   INT32 rtnCoord2PhaseCommit::doPhase1( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      CHAR *pMsgReq                    = NULL;

      rc = buildPhase1Msg( (CHAR*)pMsg, &pMsgReq );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build the message on phase1(rc=%d)",
                   rc );

      // execute on data nodes
      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute on data-group on phase1(rc=%d)",
                   rc ) ;

   done:
      if ( pMsgReq )
      {
         SDB_OSS_FREE( pMsgReq );
         pMsgReq = NULL;
      }
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoord2PhaseCommit::doPhase2( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsgReq                    = NULL;

      rc = buildPhase2Msg( (CHAR*)pMsg, &pMsgReq );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build the message on phase1(rc=%d)",
                   rc ) ;

      // execute on data nodes
      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute on data-group on phase1(rc=%d)",
                   rc ) ;

   done:
      if ( pMsgReq )
      {
         SDB_OSS_FREE( pMsgReq );
         pMsgReq = NULL;
      }
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoord2PhaseCommit::cancelOp( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      // do nothing, rollback will do in session
      return SDB_OK ;
   }

   INT32 rtnCoordTransCommit::executeOnDataGroup( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  INT64 &contextID,
                                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      netMultiRouteAgent *pAgent = pmdGetKRCB()->getCoordCB()->getRouteAgent() ;
      REQUESTID_MAP requestIdMap ;
      REPLY_QUE replyQue;
      DpsTransNodeMap *pNodeMap = cb->getTransNodeLst();
      DpsTransNodeMap::iterator iterMap = pNodeMap->begin();
      ROUTE_RC_MAP nokRC ;

      while( iterMap != pNodeMap->end() )
      {
         rcTmp = rtnCoordSendRequestToNode( (void *)pMsg, iterMap->second,
                                            pAgent, cb, requestIdMap ) ;
         if ( rcTmp )
         {
            rc = rc ? rc : rcTmp ;
            PD_LOG ( PDWARNING, "Failed to send commit request to the "
                     "node[%s], rc: %d",
                     routeID2String( iterMap->second ).c_str(),
                     rcTmp ) ;
         }
         ++iterMap ;
      }

      rcTmp = rtnCoordGetReply( cb, requestIdMap, replyQue,
                                MAKE_REPLY_TYPE( pMsg->opCode ) ) ;
      if ( rcTmp )
      {
         rc = rc ? rc : rcTmp ;
         PD_LOG( PDERROR, "Failed to get the reply, rc: %d", rcTmp ) ;
      }

      while ( !replyQue.empty() )
      {
         MsgOpReply *pReply = NULL;
         pReply = (MsgOpReply *)(replyQue.front());
         replyQue.pop();
         rcTmp = pReply->flags ;

         if ( rcTmp != SDB_OK )
         {
            rc = rc ? rc : rcTmp ;
            PD_LOG( PDERROR, "Data node[%s] commit transaction failed, rc: %d",
                    routeID2String( pReply->header.routeID ).c_str(),
                    rcTmp ) ;
            nokRC[ pReply->header.routeID.value ] = coordErrorInfo( pReply ) ;
         }
         SDB_OSS_FREE( pReply ) ;
      }

      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      if ( ( rc && nokRC.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( rtnBuildErrorObj( rc, cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 rtnCoordTransCommit::buildPhase1Msg( CHAR * pReceiveBuffer, CHAR **pMsg )
   {
      SDB_ASSERT( pMsg, "pMsg can't be NULL" ) ;
      INT32 bufferSize = 0;
      if ( *pMsg != NULL )
      {
         SDB_OSS_FREE( pMsg );
         *pMsg = NULL;
      }
      return msgBuildTransCommitPreMsg( pMsg, &bufferSize );
   }

   INT32 rtnCoordTransCommit::buildPhase2Msg( CHAR * pReceiveBuffer, CHAR **pMsg )
   {
      SDB_ASSERT( pMsg, "pMsg can't be NULL" ) ;
      INT32 bufferSize = 0;
      if ( *pMsg != NULL )
      {
         SDB_OSS_FREE( pMsg );
         *pMsg = NULL;
      }
      return msgBuildTransCommitMsg( pMsg, &bufferSize );
   }

   INT32 rtnCoordTransCommit::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), MSG_BS_TRANS_COMMIT_REQ,
                          "TransactionID: 0x%016x(%llu)",
                          cb->getTransID(),
                          cb->getTransID() ) ;

      rc = rtnCoord2PhaseCommit::execute( pMsg, cb, contextID, buf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to commit the transaction(rc=%d)",
                   rc ) ;
      // complete, delete transaction
      cb->delTransaction() ;
   done:
      return rc;
   error:
      // rollback in session
      goto done;
   }

   BOOLEAN rtnCoordTransCommit::needRollback() const
   {
      return TRUE ;
   }

   INT32 rtnCoordTransRollback::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();

      contextID                        = -1 ;

      if ( !cb->isTransaction() )
      {
         goto done;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), MSG_BS_TRANS_ROLLBACK_REQ,
                          "TransactionID: 0x%016x(%llu)",
                          cb->getTransID(),
                          cb->getTransID() ) ;

      rc = rtnCoordTransOperator::rollBack( cb, pRouteAgent ) ;
      PD_RC_CHECK( rc, PDERROR, "Rollback transaction failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done;
   }

   void rtnCoordTransRollback::_prepareForTrans( pmdEDUCB *cb,
                                                 MsgHeader *pMsg )
   {
   }

   BOOLEAN rtnCoordTransRollback::needRollback() const
   {
      return FALSE ;
   }

}
