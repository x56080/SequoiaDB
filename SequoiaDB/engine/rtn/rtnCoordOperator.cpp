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

   Source File Name = rtnCoordOperator.cpp

   Descriptive Name = Runtime Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCoordOperator.hpp"
#include "ossErr.h"
#include "../bson/bson.h"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "msgMessage.hpp"
#include "msgCatalog.hpp"
#include "rtnCoordCommon.hpp"
#include "coordSession.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "coordDef.hpp"
#include <stdlib.h>

using namespace bson;

namespace engine
{

   void rtnClearReplyQue( REPLY_QUE *pReply )
   {
      if ( pReply )
      {
         CHAR *pMsg = NULL ;
         while( !pReply->empty() )
         {
            pMsg = pReply->front() ;
            pReply->pop() ;
            if ( pMsg )
            {
               SDB_OSS_FREE( pMsg ) ;
            }
         }
      }
   }

   void rtnClearReplyMap( ROUTE_REPLY_MAP *pReply )
   {
      if ( pReply )
      {
         ROUTE_REPLY_MAP::iterator it = pReply->begin() ;
         while ( it != pReply->end() )
         {
            SDB_OSS_FREE( it->second ) ;
            ++it ;
         }
         pReply->clear() ;
      }
   }

   void rtnProcessResult::clearError()
   {
      rtnClearReplyMap( _pNokReply ) ;
      rtnClearReplyMap( _pIgnoreReply ) ;

      if ( _pNokRC )
      {
         _pNokRC->clear() ;
      }
      if ( _pIgnoreRC )
      {
         _pIgnoreRC->clear() ;
      }
   }
   void rtnProcessResult::clear()
   {
      clearError() ;

      rtnClearReplyMap( _pOkReply ) ;
      if ( _pOkRC )
      {
         _pOkRC->clear() ;
      }
      _sucGroupLst.clear() ;
   }

   INT32 rtnCoordOperator::doOnGroups( rtnSendMsgIn &inMsg,
                                       rtnSendOptions &options,
                                       netMultiRouteAgent *pAgent,
                                       pmdEDUCB *cb,
                                       rtnProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      UINT32 groupID = 0 ;
      UINT32 primaryID = 0 ;
      MsgRouteID routeID ;
      INT32 processType = RTN_PROCESS_OK ;

      inMsg.resetHeader( cb ) ;

      do
      {
         REQUESTID_MAP sendNodes ;
         if ( inMsg.hasData() )
         {
            rcTmp = rtnCoordSendRequestToNodeGroups( inMsg.msg(),
                                                     options._groupLst,
                                                     options._mapGroupInfo,
                                                     options._primary, pAgent,
                                                     cb, *inMsg.data(),
                                                     sendNodes,
                                                     _isResend( options._retryTimes ),
                                                     options._svcType ) ;
         }
         else
         {
            rcTmp = rtnCoordSendRequestToNodeGroups( (CHAR*)inMsg.msg(),
                                                     options._groupLst,
                                                     options._mapGroupInfo,
                                                     options._primary, pAgent,
                                                     cb, sendNodes,
                                                     _isResend( options._retryTimes ),
                                                     options._svcType ) ;
         }

         if ( rcTmp != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to send request to groups, rc: %d",
                     rcTmp ) ;
            rc = rc ? rc : rcTmp ;
         }

         REPLY_QUE replyQue;
         rcTmp = rtnCoordGetReply( cb, sendNodes, replyQue,
                                   MAKE_REPLY_TYPE( inMsg.opCode() ),
                                   TRUE, FALSE ) ;
         if ( rcTmp != SDB_OK )
         {
            PD_LOG ( PDWARNING, "Failed to do on data-node,"
                     "get reply failed(rc=%d)", rcTmp ) ;
            rc = rc ? rc : rcTmp ;
         }

         BOOLEAN needRetry = FALSE ;
         // clear error info
         result.clearError() ;
         processType = RTN_PROCESS_OK ;

         while ( !replyQue.empty() )
         {
            MsgOpReply *pReply = NULL ;
            pReply = (MsgOpReply *)( replyQue.front() ) ;
            replyQue.pop() ;

            routeID.value = pReply->header.routeID.value ;
            groupID = routeID.columns.groupID ;
            primaryID = pReply->startFrom ;
            rcTmp = pReply->flags ;

            if ( rcTmp && !options.isIgnored( rcTmp ) )
            {
               /// if error is not 'SDB_CLS_COORD_NODE_CAT_VER_OLD', 
               /// in transaction report error
               if ( _isTrans( cb, (MsgHeader*)pReply ) )
               {
                  processType = RTN_PROCESS_NOK ;
                  if ( SDB_CLS_COORD_NODE_CAT_VER_OLD != rcTmp ||
                       !result.pushNokRC( routeID.value, pReply ) )
                  {
                     PD_LOG( PDERROR, "Do trans command[%d] on data node[%s] "
                             "failed, rc: %d", inMsg.opCode(),
                             routeID2String( routeID ).c_str(), rcTmp ) ;
                     rc = rc ? rc : rcTmp ;
                  }
               }
               else
               {
                  if ( rtnCoordGroupReplyCheck( cb, rcTmp,
                                                _canRetry( options._retryTimes ),
                                                routeID,
                                                options._mapGroupInfo[ groupID ],
                                                NULL, TRUE, primaryID,
                                                isReadonly() ) )
                  {
                     processType = RTN_PROCESS_IGNORE ;
                     result.pushIgnoreRC( routeID.value, rcTmp ) ;
                     rcTmp = SDB_OK ;
                     needRetry = TRUE ;
                  }
                  else
                  {
                     processType = RTN_PROCESS_NOK ;
                     if ( !result.pushNokRC( routeID.value, pReply ) )
                     {
                        rc = rc ? rc : rcTmp ;
                     }
                     PD_LOG( ( rc ? PDERROR : PDINFO ),
                             "Failed to execute command[%u] on "
                             "node[%s], rc: %d", inMsg.opCode(),
                             routeID2String( routeID ).c_str(), rcTmp ) ;
                  }
               }
            }
            else
            {
               processType = RTN_PROCESS_OK ;
               // process succeed
               result._sucGroupLst[ groupID ] = groupID ;
               result.pushOkRC( routeID.value, rcTmp ) ;
               options._groupLst.erase( groupID ) ;
               options._mapGroupInfo.erase( groupID ) ;
            }

            // callback for parse
            _onNodeReply( processType, pReply, cb, inMsg ) ;

            if ( !result.pushReply( (MsgHeader *)pReply, processType ) )
            {
               SDB_OSS_FREE( pReply ) ;
            }
         } // end while

         if ( result.nokSize() > 0 )
         {
            needRetry = FALSE ;
         }
         if ( SDB_OK == rc && needRetry )
         {
            ++options._retryTimes ;
            continue ;
         }
         break ;
      }while ( TRUE ) ;

      return rc ;
   }

   INT32 rtnCoordOperator::doOpOnMainCL( CoordCataInfoPtr &cataInfo,
                                         const BSONObj &objMatch,
                                         rtnSendMsgIn &inMsg,
                                         rtnSendOptions &options,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         rtnProcessResult &result )
   {
      INT32 rc = SDB_OK ;

      CoordGroupSubCLMap groupSubCLMap ;
      CoordGroupSubCLMap::iterator iterGroup ;
      ossValuePtr outPtr = ( ossValuePtr )NULL ;

      if ( FALSE == options._useSpecialGrp )
      {
         CoordSubCLlist subCLList ;
         // build group list
         options._groupLst.clear() ;

         // get sub cl list
         cataInfo->getMatchSubCLs( objMatch, subCLList ) ;
         if ( 0 == subCLList.size() )
         {
            CHAR clName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
            ossStrncpy( clName, cataInfo->getName(), DMS_COLLECTION_FULL_NAME_SZ ) ;

            rc = rtnCoordGetRemoteCata( cb, clName, cataInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update collection[%s]'s "
                         "catalog info, rc: %d", clName,
                         rc ) ;
            ++options._retryTimes ;
            rc = cataInfo->getMatchSubCLs( objMatch, subCLList ) ;
            if ( SDB_CAT_NO_MATCH_CATALOG == rc )
            {
               rc = SDB_OK ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get match sub-collection, "
                      "rc: %d", rc ) ;

         rc = rtnCoordGetSubCLsByGroups( subCLList, result._sucGroupLst, cb,
                                         groupSubCLMap, &objMatch ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get sub-collection info, rc: %d", rc );

         iterGroup = groupSubCLMap.begin() ;
         while( iterGroup != groupSubCLMap.end() )
         {
            options._groupLst[ iterGroup->first ] = iterGroup->first ;
            ++iterGroup ;
         }
      }
      else
      {
         PD_LOG( PDDEBUG, "Using specified group" ) ;
      }

      // construct msg
      rc = _prepareMainCLOp( cataInfo, groupSubCLMap, inMsg, options,
                             pRouteAgent, cb, result, outPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare main collection operation failed, "
                   "rc: %d", rc ) ;

      // do
      rc = doOnGroups( inMsg, options, pRouteAgent, cb, result ) ;

      _doneMainCLOp( outPtr, cataInfo, groupSubCLMap, inMsg, options,
                     pRouteAgent, cb, result ) ;

      PD_RC_CHECK( rc, PDERROR, "Do command[%d] on groups failed, rc: %d",
                   inMsg.opCode(), rc ) ;

      if ( result.nokSize() > 0 )
      {
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordOperator::_prepareMainCLOp( CoordCataInfoPtr &cataInfo,
                                             CoordGroupSubCLMap &grpSubCl,
                                             rtnSendMsgIn &inMsg,
                                             rtnSendOptions &options,
                                             netMultiRouteAgent *pRouteAgent,
                                             pmdEDUCB *cb,
                                             rtnProcessResult &result,
                                             ossValuePtr &outPtr )
   {
      return SDB_OK ;
   }

   void rtnCoordOperator::_doneMainCLOp( ossValuePtr itPtr,
                                         CoordCataInfoPtr &cataInfo,
                                         CoordGroupSubCLMap &grpSubCl,
                                         rtnSendMsgIn &inMsg,
                                         rtnSendOptions &options,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         rtnProcessResult &result )
   {
   }

   INT32 rtnCoordOperator::doOpOnCL( CoordCataInfoPtr &cataInfo,
                                     const BSONObj &objMatch,
                                     rtnSendMsgIn &inMsg,
                                     rtnSendOptions &options,
                                     netMultiRouteAgent *pRouteAgent,
                                     pmdEDUCB *cb,
                                     rtnProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      ossValuePtr outPtr ;

      if ( FALSE == options._useSpecialGrp )
      {
         options._groupLst.clear() ;

         rc = rtnCoordGetGroupsByCataInfo( cataInfo, result._sucGroupLst,
                                           options._groupLst, cb,
                                           NULL, &objMatch ) ;
         PD_RC_CHECK( rc, PDERROR, "Get the groups by catalog info failed, "
                      "matcher: %s, rc: %d", objMatch.toString().c_str(),
                      rc ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Using specified group" ) ;
      }

      // construct msg
      rc = _prepareCLOp( cataInfo, inMsg, options, pRouteAgent,
                         cb, result, outPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare collection operation failed, "
                   "rc: %d", rc ) ;

      // do
      rc = doOnGroups( inMsg, options, pRouteAgent, cb, result ) ;

      _doneCLOp( outPtr, cataInfo, inMsg, options, pRouteAgent, cb, result ) ;

      PD_RC_CHECK( rc, PDERROR, "Do command[%d] on groups failed, rc: %d",
                   inMsg.opCode(), rc ) ;

      if ( result.nokSize() > 0 )
      {
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordOperator::_prepareCLOp( CoordCataInfoPtr &cataInfo,
                                         rtnSendMsgIn &inMsg,
                                         rtnSendOptions &options,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         rtnProcessResult &result,
                                         ossValuePtr &outPtr )
   {
      return SDB_OK ;
   }

   void rtnCoordOperator::_doneCLOp( ossValuePtr itPtr,
                                     CoordCataInfoPtr &cataInfo,
                                     rtnSendMsgIn &inMsg,
                                     rtnSendOptions &options,
                                     netMultiRouteAgent *pRouteAgent,
                                     pmdEDUCB *cb,
                                     rtnProcessResult &result )
   {
   }

   BOOLEAN rtnCoordOperator::checkRetryForCLOpr( INT32 rc,
                                                 ROUTE_RC_MAP *pRC,
                                                 MsgHeader *pSrcMsg,
                                                 UINT32 times,
                                                 CoordCataInfoPtr &cataInfo,
                                                 pmdEDUCB *cb,
                                                 INT32 &errRC,
                                                 MsgRouteID *pNodeID,
                                                 BOOLEAN canUpdate )
   {
      BOOLEAN retry = FALSE ;

      errRC = SDB_OK ;
      if ( pNodeID )
      {
         pNodeID->value = 0 ;
      }

      if ( SDB_OK == rc && ( !pRC || pRC->empty() ) )
      {
         // is succeed, don't need to retry
         retry = FALSE ;
         goto done ;
      }

      if ( ( SDB_CLS_GRP_NOT_EXIST == rc ||
             SDB_CAT_NO_MATCH_CATALOG == rc ||
             SDB_CLS_COORD_NODE_CAT_VER_OLD == rc ||
             SDB_CLS_NO_CATALOG_INFO == rc ||
             SDB_CLS_NODE_NOT_EXIST == rc ) &&
           rtnCoordCataReplyCheck( cb, rc, _canRetry( times ), cataInfo,
                                   NULL, canUpdate ) )
      {
         retry = TRUE ;
      }
      else if ( rc )
      {
         errRC = rc ;
         retry = FALSE ;
      }
      else if ( pRC )
      {
         retry = TRUE ;
         BOOLEAN hasUpdate = FALSE ;
         ROUTE_RC_MAP::iterator it = pRC->begin() ;
         while( it != pRC->end() )
         {
            retry = rtnCoordCataReplyCheck( cb, it->second._rc, _canRetry( times ),
                                            cataInfo, &hasUpdate, canUpdate ) ;
            if ( !retry )
            {
               errRC = it->second._rc ;
               if ( pNodeID )
               {
                  pNodeID->value = it->first ;
               }
               break ;
            }
            if ( canUpdate && hasUpdate )
            {
               canUpdate = FALSE ;
            }
            ++it ;
         }
      }

      if ( retry && _isTrans( cb, pSrcMsg ) && SDB_OK != cb->getTransRC() )
      {
         retry = FALSE ;
         errRC = cb->getTransRC() ;
      }

   done:
      return retry ;
   }

   BOOLEAN rtnCoordOperator::needRollback() const
   {
      return FALSE ;
   }

   BOOLEAN rtnCoordOperator::_isResend( UINT32 times )
   {
      return times > 0 ? TRUE : FALSE ;
   }

   BOOLEAN rtnCoordOperator::_canRetry( UINT32 times )
   {
      return rtnCoordCanRetry( times ) ;
   }

   BOOLEAN rtnCoordOperator::_isTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      return FALSE ;
   }

   void rtnCoordOperator::_onNodeReply( INT32 processType,
                                        MsgOpReply *pReply,
                                        pmdEDUCB *cb,
                                        rtnSendMsgIn &inMsg )
   {
      // do nothing
   }

   /*
      rtnCoordOperatorDefault define
   */

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOOPDEFAULT_EXECUTE, "rtnCoordOperatorDefault::execute" )
   INT32 rtnCoordOperatorDefault::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY ( SDB_RTNCOOPDEFAULT_EXECUTE ) ;
      contextID          = -1 ;
      PD_TRACE_EXIT ( SDB_RTNCOOPDEFAULT_EXECUTE ) ;
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   /*
      rtnCoordTransOperator define
   */
   INT32 rtnCoordTransOperator::doOnGroups( rtnSendMsgIn &inMsg,
                                            rtnSendOptions &options,
                                            netMultiRouteAgent *pAgent,
                                            pmdEDUCB *cb,
                                            rtnProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      ROUTE_RC_MAP newNodeMap ;

      /// first to build trans session on new data groups
      rc = buildTransSession( options._groupLst, pAgent, cb, newNodeMap ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build transaction session on data node, "
                 "rc: %d", rc ) ;
         goto error ;
      }

      if ( cb->isTransaction() )
      {
         // need add transaction info for the send msg
         _prepareForTrans( cb, inMsg.msg() ) ;
      }

      rc = rtnCoordOperator::doOnGroups( inMsg, options,
                                         pAgent, cb, result ) ;
      // release the nodes transaction session( node in newNodeMap, but not
      // in result._sucGroupLst )
      if ( cb->isTransaction() && ( rc || result.nokSize() > 0 ) )
      {
         ROUTE_SET nodes ;
         MsgRouteID nodeID ;
         ROUTE_RC_MAP::iterator itRCMap = newNodeMap.begin() ;
         while( itRCMap != newNodeMap.end() )
         {
            nodeID.value = itRCMap->first ;
            if ( result._sucGroupLst.find( nodeID.columns.groupID ) ==
                 result._sucGroupLst.end() )
            {
               nodes.insert( nodeID.value ) ;
            }
            ++itRCMap ;
         }
         releaseTransSession( nodes, pAgent, cb ) ;
      }

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

   BOOLEAN rtnCoordTransOperator::needRollback() const
   {
      return TRUE ;
   }

   BOOLEAN rtnCoordTransOperator::_isTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      if ( MSG_BS_TRANS_BEGIN_REQ == GET_REQUEST_TYPE( pMsg->opCode ) )
      {
         return FALSE ;
      }
      return cb->isTransaction() ;
   }

   void rtnCoordTransOperator::_onNodeReply( INT32 processType,
                                             MsgOpReply *pReply,
                                             pmdEDUCB *cb,
                                             rtnSendMsgIn &inMsg )
   {
      if ( inMsg._pvtType == PRIVATE_DATA_NUMBERLONG &&
           inMsg._pvtData )
      {
         UINT64 *number = ( UINT64* )inMsg._pvtData ;

         if ( pReply->contextID > 0 )
         {
            *number += pReply->contextID ;
         }
      }
   }

   INT32 rtnCoordTransOperator::releaseTransSession( ROUTE_SET &nodes,
                                                     netMultiRouteAgent *pRouteAgent,
                                                     pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      MsgOpTransRollback msgReq ;

      msgReq.header.messageLength = sizeof( MsgOpTransRollback ) ;
      msgReq.header.opCode = MSG_BS_TRANS_ROLLBACK_REQ ;
      msgReq.header.routeID.value = 0 ;
      msgReq.header.TID = cb->getTID() ;

      REQUESTID_MAP sendNodes ;
      ROUTE_RC_MAP failedNodes ;
      REPLY_QUE replyQue ;
      ROUTE_SET::iterator itNode ;
      MsgRouteID nodeID ;

      rtnCoordSendRequestToNodes( ( void *)&msgReq, nodes, pRouteAgent, cb,
                                  sendNodes, failedNodes ) ;

      // failed node, send failed, but the node has already rollbacked
      if ( failedNodes.size() > 0 )
      {
         ROUTE_RC_MAP::iterator it = failedNodes.begin() ;
         while( it != failedNodes.end() )
         {
            nodeID.value = it->first ;
            PD_LOG( PDINFO, "Release node[%s] failed, because send msg "
                    "failed, rc: %d", routeID2String( nodeID ).c_str(),
                    it->second._rc ) ;
            ++it ;
         }
      }

      rc = rtnCoordGetReply( cb, sendNodes, replyQue,
                             MAKE_REPLY_TYPE(msgReq.header.opCode),
                             TRUE, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Get reply failed in release transaction session, "
                 "rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      while( !replyQue.empty() )
      {
         MsgOpReply *pReply = ( MsgOpReply* )replyQue.front() ;
         replyQue.pop() ;

         if ( SDB_OK != pReply->flags )
         {
            PD_LOG( PDINFO, "Release node[%s]'s transaction failed, flag: %d",
                    routeID2String( pReply->header.routeID ).c_str(),
                    pReply->flags ) ;
         }
         SDB_OSS_FREE( pReply ) ;
      }

      // delete trans node
      itNode = nodes.begin() ;
      while( itNode != nodes.end() )
      {
         nodeID.value = ( *itNode ) ;
         cb->delTransNode( nodeID ) ;
         ++itNode ;
      }

      return rc ;
   }

   INT32 rtnCoordTransOperator::rollBack( pmdEDUCB *cb,
                                          netMultiRouteAgent *pAgent )
   {
      INT32 rc = SDB_OK ;
      ROUTE_SET nodes ;
      DpsTransNodeMap *pTransMap = cb->getTransNodeLst() ;
      DpsTransNodeMap::iterator it ;

      if ( NULL == pTransMap )
      {
         goto done ;
      }

      it = pTransMap->begin() ;
      while( it != pTransMap->end() )
      {
         nodes.insert( it->second.value ) ;
         ++it ;
      }

      cb->startRollback() ;

      rc = releaseTransSession( nodes, pAgent, cb ) ;

   done:
      cb->delTransaction() ;
      cb->stopRollback() ;
      return rc ;
   }

   INT32 rtnCoordTransOperator::buildTransSession( const CoordGroupList &groupLst,
                                                   netMultiRouteAgent *pRouteAgent,
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
         rtnSendOptions options( TRUE ) ;
         rtnProcessResult result ;

         DpsTransNodeMap *pTransNodeLst = cb->getTransNodeLst() ;
         DpsTransNodeMap::iterator iterTrans ;
         CoordGroupList::const_iterator iterGroup ;
         MsgOpTransBegin msgReq ;
         rtnSendMsgIn inMsg( (MsgHeader*)&msgReq ) ;

         msgReq.header.messageLength = sizeof( MsgOpTransBegin ) ;
         msgReq.header.opCode = MSG_BS_TRANS_BEGIN_REQ ;
         msgReq.header.routeID.value = 0 ;
         msgReq.header.TID = cb->getTID() ;

         iterGroup = groupLst.begin() ;
         while(  iterGroup != groupLst.end() )
         {
            iterTrans = pTransNodeLst->find( iterGroup->first );
            if ( pTransNodeLst->end() == iterTrans )
            {
               /// not found, need to being the trans
               options._groupLst[ iterGroup->first ] = iterGroup->second ;
            }
            ++iterGroup ;
         }

         newNodeMap.clear() ;
         result._pOkRC = &newNodeMap ;
         rc = rtnCoordOperator::doOnGroups( inMsg, options, pRouteAgent,
                                            cb, result ) ;
         // add ok route id to trans node id
         if ( newNodeMap.size() > 0 )
         {
            MsgRouteID nodeID ;
            ROUTE_RC_MAP::iterator itRC = newNodeMap.begin() ;
            while( itRC != newNodeMap.end() )
            {
               nodeID.value = itRC->first ;
               cb->addTransNode( nodeID ) ;
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
      /// will rollback all suc node and the node before at session
      goto done ;
   }

   /*
      rtnCoordMsg implement
   */
   INT32 rtnCoordMsg::execute( MsgHeader *pMsg,
                               pmdEDUCB *cb,
                               INT64 &contextID,
                               rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb = pmdGetKRCB() ;
      CoordCB *pCoordcb = pKrcb->getCoordCB() ;
      netMultiRouteAgent *pRouteAgent = pCoordcb->getRouteAgent() ;

      REPLY_QUE replyQue ;

      // fill default-reply
      contextID    = -1 ;
      // set tid
      pMsg->TID = cb->getTID() ;

      CoordGroupList groupLst ;

      ROUTE_SET sendNodes ;
      REQUESTID_MAP successNodes ;
      ROUTE_RC_MAP failedNodes ;

      // run msg
      rtnMsg( (MsgOpMsg *)pMsg ) ;

      // list all groups
      rc = rtnCoordGetAllGroupList( cb, groupLst, NULL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all group list, rc: %d", rc ) ;

      // get nodes
      rc = rtnCoordGetGroupNodes( cb, BSONObj(), NODE_SEL_ALL,
                                  groupLst, sendNodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc ) ;
      if ( sendNodes.size() == 0 )
      {
         PD_LOG( PDWARNING, "Not found any node" ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }

      // send msg
      rtnCoordSendRequestToNodes( (void*)pMsg, sendNodes, 
                                  pRouteAgent, cb, successNodes,
                                  failedNodes ) ;
      rc = rtnCoordGetReply( cb, successNodes, replyQue,
                             MSG_BS_MSG_RES, TRUE, FALSE ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "Failed to get the reply, rc", rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || failedNodes.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( rtnBuildErrorObj( rc, cb, &failedNodes ) ) ;
      }
      rtnClearReplyQue( &replyQue ) ;
      return rc ;
   error:
      rtnCoordClearRequest( cb, successNodes ) ;
      goto done ;
   }

   rtnCoordShardKicker::rtnCoordShardKicker()
   {
   }

   rtnCoordShardKicker::~rtnCoordShardKicker()
   {
   }

   BOOLEAN rtnCoordShardKicker::_isUpdateReplace( const BSONObj &updator )
   {
      //INT32 rc = SDB_OK ;
      BSONObjIterator iter( updator ) ;
      while ( iter.more() )
      {
         BSONElement beTmp = iter.next() ;
         if ( 0 == ossStrcmp( beTmp.fieldName(),
                              CMD_ADMIN_PREFIX FIELD_OP_VALUE_REPLACE ) )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   UINT32 rtnCoordShardKicker::_addKeys( const BSONObj &objKey )
   {
      UINT32 count = 0 ;
      BSONObjIterator itr( objKey ) ;
      while( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( _setKeys.count( e.fieldName() ) > 0 )
         {
            continue ;
         }
         ++count ;
         _setKeys.insert( e.fieldName() ) ;
      }
      return count ;
   }

   INT32 rtnCoordShardKicker::kickShardingKey( const CoordCataInfoPtr &cataInfo,
                                               const BSONObj &updator,
                                               BSONObj &newUpdator,
                                               BOOLEAN &hasShardingKey )
   {
      INT32 rc = SDB_OK ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;

      if ( skSiteID > 0 )
      {
         /// if is the same sharding key
         if ( _skSiteIDs.count( skSiteID ) > 0 )
         {
            newUpdator = updator ;
            goto done ;
         }
         _skSiteIDs.insert( skSiteID ) ;
      }

      try
      {
         BSONObjBuilder bobNewUpdator( updator.objsize() ) ;
         BSONObj boShardingKey ;
         BSONObj subObj ;

         BOOLEAN isReplace = _isUpdateReplace( updator ) ;
         cataInfo->getShardingKey( boShardingKey ) ;
         BSONObjIterator iter( updator ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            if ( beTmp.type() != Object )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "updator's element must be an Object type:"
                       "updator=%s", updator.toString().c_str() ) ;
               goto error;
            }

            subObj = beTmp.embeddedObject() ;
            //if replace. leave the keep
            if ( isReplace &&
                 0 == ossStrcmp( beTmp.fieldName(),
                                 CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) )
            {
               _addKeys( subObj ) ;
               continue ;
            }

            BSONObjBuilder subBuilder( bobNewUpdator.subobjStart(
                                       beTmp.fieldName() ) ) ;
            BSONObjIterator iterField( subObj ) ;
            while( iterField.more() )
            {
               BSONElement beField = iterField.next() ;
               BSONObjIterator iterKey( boShardingKey ) ;
               BOOLEAN isKey = FALSE ;
               while( iterKey.more() )
               {
                  BSONElement beKey = iterKey.next();
                  const CHAR *pKey = beKey.fieldName();
                  const CHAR *pField = beField.fieldName();
                  while( *pKey == *pField && *pKey != '\0' )
                  {
                     ++pKey;
                     ++pField;
                  }

                  // shardingkey_fieldName == updator_fieldName
                  if ( *pKey == *pField
                     || ( '\0' == *pKey && '.' == *pField )
                     || ( '\0' == *pField && '.' == *pKey ) )
                  {
                     isKey = TRUE;
                     break;
                  }
               }
               if ( isKey )
               {
                  hasShardingKey = TRUE;
               }
               else
               {
                  subBuilder.append( beField ) ;
               }
            } // while( iterField.more() )

            subBuilder.done() ;
         } // while ( iter.more() )

         if ( isReplace )
         {
            //generate new $keep by combining boUpdator.$keep & boShardingKey.
            UINT32 count = _addKeys( boShardingKey ) ;
            if ( count > 0 )
            {
               hasShardingKey = TRUE ;
            }

            if ( !_setKeys.empty() )
            {
               BSONObjBuilder keepBuilder( bobNewUpdator.subobjStart(
                                           CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) ) ;
               SET_SHARDINGKEY::iterator itKey = _setKeys.begin() ;
               while( itKey != _setKeys.end() )
               {
                  keepBuilder.append( itKey->_pStr, (INT32)1 ) ;
                  ++itKey ;
               }
               keepBuilder.done() ;
            }
         } // if ( isReplace )
         newUpdator = bobNewUpdator.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR,"Failed to check the record is include sharding-key,"
                  "occured unexpected error: %s", e.what() ) ;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordShardKicker::kickShardingKeyForSubCL( 
                                                const CoordSubCLlist &subCLList,
                                                const BSONObj &updator,
                                                BSONObj &newUpdator,
                                                BOOLEAN &hasShardingKey,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      CoordSubCLlist::const_iterator iterCL = subCLList.begin();
      BSONObj boCur = updator;
      BSONObj boNew = updator;

      while( iterCL != subCLList.end() )
      {
         CoordCataInfoPtr subCataInfo;
         rc = rtnCoordGetCataInfo( cb, (*iterCL).c_str(), FALSE,
                                   subCataInfo ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "get catalog of sub-collection(%s) failed(rc=%d)",
                      (*iterCL).c_str(), rc ) ;
         rc = kickShardingKey( subCataInfo, boCur, boNew, hasShardingKey ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to kick sharding-key for "
                      "sub-collection(rc=%d)", rc ) ;
         boCur = boNew ;
         ++iterCL ;
      }

      newUpdator = boNew ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

