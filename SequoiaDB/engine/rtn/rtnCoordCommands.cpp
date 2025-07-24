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

   Source File Name = rtnCoordCommands.cpp

   Descriptive Name = Runtime Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#include "ossErr.h"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtnContext.hpp"
#include "netMultiRouteAgent.hpp"
#include "msgCatalog.hpp"
#include "rtnCoordCommands.hpp"
#include "rtnCoordCommon.hpp"
#include "msgCatalog.hpp"
#include "catCommon.hpp"
#include "../bson/bson.h"
#include "pmdOptions.hpp"
#include "rtnRemoteExec.hpp"
#include "dms.hpp"
#include "omagentDef.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnCommand.hpp"
#include "dpsTransLockDef.hpp"
#include "coordSession.hpp"
#include "mthModifier.hpp"
#include "rtnCoordDef.hpp"
#include "aggrDef.hpp"
#include "spdSession.hpp"
#include "spdCoordDownloader.hpp"
#include "rtnDataSet.hpp"
#include "rtnAlterRunner.hpp"
#include "../include/authDef.hpp"

using namespace bson;
namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCOM_PROCCATREPLY, "rtnCoordCommand::_processCatReply" )
   INT32 rtnCoordCommand::_processCatReply( const BSONObj &obj,
                                            CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCOM_PROCCATREPLY ) ;

      do
      {
         try
         {
            BSONElement beGroupArr = obj.getField( CAT_GROUP_NAME ) ;
            if ( beGroupArr.eoo() || beGroupArr.type() != Array )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG ( PDERROR, "Failed to get the field(%s) from obj[%s]",
                        CAT_GROUP_NAME, obj.toString().c_str() ) ;
               break ;
            }
            BSONObjIterator i( beGroupArr.embeddedObject() ) ;
            while ( i.more() )
            {
               BSONElement beTmp = i.next();
               if ( Object != beTmp.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Group info in obj[%s] must be object",
                          obj.toString().c_str() ) ;
                  break ;
               }
               BSONObj boGroupInfo = beTmp.embeddedObject() ;
               BSONElement beGrpId = boGroupInfo.getField( CAT_GROUPID_NAME ) ;
               if ( beGrpId.eoo() || !beGrpId.isNumber() )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDERROR, "Failed to get the field(%s) from obj[%s]",
                           CAT_GROUPID_NAME, obj.toString().c_str() );
                  break ;
               }

               // add to group list
               groupLst[ beGrpId.number() ] = beGrpId.number() ;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Parse catalog reply object occur exception: %s",
                     e.what() ) ;
            break ;
         }
      }while( FALSE ) ;

      PD_TRACE_EXITRC ( SDB_RTNCOCOM_PROCCATREPLY, rc ) ;
      return rc ;
   }

   INT32 rtnCoordCommand::_processSucReply( ROUTE_REPLY_MAP &okReply,
                                            rtnContextCoord *pContext )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      BOOLEAN takeOver = FALSE ;
      MsgOpReply *pReply = NULL ;
      MsgRouteID nodeID ;
      ROUTE_REPLY_MAP::iterator it = okReply.begin() ;
      while( it != okReply.end() )
      {
         takeOver = FALSE ;
         pReply = (MsgOpReply*)(it->second) ;
         nodeID.value = pReply->header.routeID.value ;

         if ( SDB_OK == pReply->flags )
         {
            if ( pContext )
            {
               rcTmp = pContext->addSubContext( pReply, takeOver ) ;
               if ( rcTmp )
               {
                  PD_LOG( PDERROR, "Add sub data[node: %s, context: %lld] to "
                          "context[%s] failed, rc: %d",
                          routeID2String( nodeID ).c_str(), pReply->contextID,
                          pContext->toString().c_str(), rcTmp ) ;
                  rc = rcTmp ;
               }
            }
            else
            {
               SDB_ASSERT( pReply->contextID == -1, "Context leak" ) ;
            }
         }

         if ( !takeOver )
         {
            SDB_OSS_FREE( pReply ) ;
         }
         ++it ;
      }
      okReply.clear() ;

      return rc ;
   }

   INT32 rtnCoordCommand::_processNodesReply( REPLY_QUE &replyQue,
                                              ROUTE_RC_MAP &faileds,
                                              ROUTE_SET &retriedNodes,
                                              ROUTE_SET &needRetryNodes,
                                              rtnCoordCtrlParam &ctrlParam,
                                              rtnContextCoord *pContext,
                                              SET_RC *pIgnoreRC,
                                              ROUTE_SET *pSucNodes )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN takeOver = FALSE ;
      MsgOpReply *pReply = NULL ;
      MsgRouteID nodeID ;

      while( !replyQue.empty() )
      {
         pReply = ( MsgOpReply *)( replyQue.front() ) ;
         replyQue.pop() ;

         takeOver = FALSE ;
         nodeID.value = pReply->header.routeID.value ;

         if ( SDB_OK == pReply->flags )
         {
            if ( pSucNodes )
            {
               pSucNodes->insert( nodeID.value ) ;
            }

            if ( pContext )
            {
               rc = pContext->addSubContext( pReply, takeOver ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Add sub data[node: %s, context: %lld] to "
                          "context[%s] failed, rc: %d",
                          routeID2String( nodeID ).c_str(), pReply->contextID,
                          pContext->toString().c_str(), rc ) ;
               }
            }
            else
            {
               SDB_ASSERT( pReply->contextID == -1, "Context leak" ) ;
            }
         }
         else if ( pIgnoreRC && pIgnoreRC->end() !=
                   pIgnoreRC->find( pReply->flags ) )
         {
            /// ignored
         }
         else
         {
            if ( !_getRetryNodes(retriedNodes, needRetryNodes, ctrlParam, pReply ) )
            {
            PD_LOG( ( pContext ? PDINFO : PDWARNING ),
                    "Failed to process reply[node: %s, flag: %d]",
                    routeID2String( nodeID ).c_str(), pReply->flags ) ;
            faileds[ nodeID.value ] = pReply->flags ;
         }
         }

         if ( !takeOver )
         {
            SDB_OSS_FREE( pReply ) ;
            pReply = NULL ;
         }
      }

      return rc ;
   }

   BOOLEAN rtnCoordCommand::_getRetryNodes( ROUTE_SET &retriedNodes,
                                          ROUTE_SET &needRetryNodes,
                                          rtnCoordCtrlParam &ctrlParam,
                                          MsgOpReply *pReply )
   {
      // Must send to primary but replyed not primary!
      if ( SDB_CLS_NOT_PRIMARY == pReply->flags
           && pReply->startFrom != 0
           && NODE_SEL_PRIMARY == ctrlParam._emptyFilterSel
           && retriedNodes.find( pReply->startFrom ) == retriedNodes.end() )
      {
         needRetryNodes.insert( pReply->startFrom ) ;
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 rtnCoordCommand::_buildFailedNodeReply( ROUTE_RC_MAP &failedNodes,
                                                 rtnContextCoord *pContext )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( pContext != NULL, "pContext can't be NULL!" ) ;

      CoordCB *pCoordcb = pmdGetKRCB()->getCoordCB() ;
      ROUTE_RC_MAP::iterator iter ;
      CoordGroupInfoPtr groupInfo ;
      string strHostName ;
      string strServiceName ;
      string strNodeName ;
      string strGroupName ;
      MsgRouteID routeID ;
      BSONObj errObj ;
      BSONObjBuilder builder ;
      BSONArrayBuilder arrayBD( builder.subarrayStart(
                                FIELD_NAME_ERROR_NODES ) ) ;
      if ( 0 == failedNodes.size() )
      {
         goto done ;
      }

      iter = failedNodes.begin() ;
      while ( iter != failedNodes.end() )
      {
         routeID.value = iter->first ;
         rc = pCoordcb->getGroupInfo( routeID.columns.groupID, groupInfo ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to get group[%d] info, rc: %d",
                    routeID.columns.groupID, rc ) ;
            errObj = BSON( FIELD_NAME_NODEID <<
                           (INT32)routeID.columns.nodeID <<
                           FIELD_NAME_RCFLAG << iter->second ) ;
         }
         else
         {
            strGroupName = groupInfo->groupName() ;

            routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;
            rc = groupInfo->getNodeInfo( routeID, strHostName,
                                         strServiceName ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to get node[%d] info failed, rc: %d",
                       routeID.columns.nodeID, rc ) ;
               errObj = BSON( FIELD_NAME_NODEID <<
                              (INT32)routeID.columns.nodeID <<
                              FIELD_NAME_RCFLAG << iter->second ) ;
            }
            else
            {
               strNodeName = strHostName + ":" + strServiceName ;
               errObj = BSON( FIELD_NAME_NODE_NAME << strNodeName <<
                              FIELD_NAME_RCFLAG << iter->second ) ;
            }
         }

         try
         {
            errObj = BSON( FIELD_NAME_NODE_NAME << strNodeName <<
                           FIELD_NAME_HOST << strHostName <<
                           FIELD_NAME_SERVICE_NAME << strServiceName <<
                           FIELD_NAME_GROUPNAME << strGroupName <<
                           FIELD_NAME_NODEID << (INT32)routeID.columns.nodeID <<
                           FIELD_NAME_GROUPID << routeID.columns.groupID <<
                           FIELD_NAME_RCFLAG << iter->second ) ;
            arrayBD.append( errObj ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDWARNING, "Build error object occur exception: %s",
                    e.what() ) ;
            /// then ignored this record
         }
         ++iter ;
      }

      arrayBD.done() ;
      rc = pContext->append( builder.obj() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append obj, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordCommand::_executeOnGroups( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            const CoordGroupList &groupLst,
                                            MSG_ROUTE_SERVICE_TYPE type,
                                            BOOLEAN onPrimary,
                                            SET_RC *pIgnoreRC,
                                            CoordGroupList *pSucGrpLst,
                                            rtnContextCoord **ppContext )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      pmdKRCB *pKrcb                   = pmdGetKRCB();
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();

      rtnSendMsgIn inMsg( pMsg ) ;
      rtnSendOptions sendOpt ;
      rtnProcessResult result ;
      rtnContextCoord *pTmpContext = NULL ;
      INT64 contextID = -1 ;

      sendOpt._groupLst = groupLst ;
      sendOpt._svcType = type ;
      sendOpt._primary = onPrimary ;
      sendOpt._pIgnoreRC = pIgnoreRC ;

      ROUTE_REPLY_MAP okReply ;
      result._pOkReply = &okReply ;

      if ( ppContext )
      {
         if ( NULL == *ppContext )
         {
            // create context
            rc = pRtncb->contextNew( RTN_CONTEXT_COORD,
                                     (rtnContext **)ppContext,
                                     contextID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                         rc ) ;
         }
         else
         {
            contextID = (*ppContext)->contextID() ;
            // the context is create in out side, do nothing
         }
         pTmpContext = *ppContext ;

         // context for catalog: only primary, so query,sel,orderby...will
         // push to catalog
         // context for data: only for drop cs/cl execute command, not any
         // return obj
         rc = pTmpContext->open( BSONObj(), BSONObj(), -1, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Open context failed(rc=%d)", rc ) ;
      }

      rc = doOnGroups( inMsg, sendOpt, pRouteAgent, cb, result ) ;
      /// process succeed reply msg
      rcTmp = _processSucReply( okReply, pTmpContext ) ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Do command[%d] on groups failed, rc: %d",
                 pMsg->opCode, rc ) ;
         goto error ;
      }
      else if ( rcTmp )
      {
         rc = rcTmp ;
         goto error ;
      }

      if ( pTmpContext )
      {
         pTmpContext->addSubDone( cb ) ;
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID  )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
         *ppContext = NULL ;
      }
      goto done ;
   }

   INT32 rtnCoordCommand::executeOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               const CoordGroupList &groupLst,
                                               BOOLEAN onPrimary,
                                               SET_RC *pIgnoreRC,
                                               CoordGroupList *pSucGrpLst,
                                               rtnContextCoord **ppContext )
   {
      return _executeOnGroups( pMsg, cb, groupLst, MSG_ROUTE_SHARD_SERVCIE,
                               onPrimary, pIgnoreRC, pSucGrpLst, ppContext ) ;
   }

   INT32 rtnCoordCommand::executeOnCataGroup( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              BOOLEAN onPrimary,
                                              SET_RC *pIgnoreRC,
                                              rtnContextCoord **ppContext )
   {
      CoordGroupList grpList ;
      grpList[ CATALOG_GROUPID ] = CATALOG_GROUPID ;
      return _executeOnGroups( pMsg, cb, grpList, MSG_ROUTE_CAT_SERVICE,
                               onPrimary, pIgnoreRC, NULL, ppContext ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCOM_EXEONCATAGR, "rtnCoordCommand::executeOnCataGroup" )
   INT32 rtnCoordCommand::executeOnCataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               CoordGroupList *pGroupList,
                                               vector<BSONObj> *pReplyObjs,
                                               BOOLEAN onPrimary,
                                               SET_RC *pIgnoreRC )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCOM_EXEONCATAGR ) ;

      rtnContextBuf buffObj ;
      rtnContextCoord *pContext = NULL ;

      rc = executeOnCataGroup( pMsg, cb, onPrimary, pIgnoreRC, &pContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command[%d] on catalog, "
                   "rc: %d", pMsg->opCode, rc ) ;

      while( TRUE )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get more from context[%lld], rc: %d",
                    pContext->contextID(), rc ) ;
            goto error ;
         }

         try
         {
            BSONObj obj( buffObj.data() ) ;

            if ( pGroupList )
            {
               rc = _processCatReply( obj, *pGroupList ) ;
               PD_RC_CHECK( rc, PDERROR, "Get groups from catalog reply[%s] "
                            "failed, rc: %d", obj.toString().c_str(), rc ) ;
            }

            if ( pReplyObjs )
            {
               pReplyObjs->push_back( obj.getOwned() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Extrace catalog reply obj occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done :
      if ( pContext )
      {
         INT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         pContext = NULL ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCOM_EXEONCATAGR, rc ) ;
      return rc;
   error :
      goto done ;
   }

   INT32 rtnCoordCommand::executeOnCataCL( MsgOpQuery *pMsg,
                                           pmdEDUCB *cb,
                                           const CHAR *pCLName,
                                           BOOLEAN onPrimary,
                                           SET_RC *pIgnoreRC,
                                           rtnContextCoord **ppContext )
   {
      INT32 rc = SDB_OK ;
      CoordCataInfoPtr cataInfo ;
      UINT32 retryTimes = 0 ;

      rc = rtnCoordGetRemoteCata( cb, pCLName, cataInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Update collection[%s] catalog info failed, "
                   "rc: %d", pCLName, rc ) ;

   retry:
      pMsg->version = cataInfo->getVersion() ;

      rc = executeOnCataGroup( (MsgHeader*)pMsg, cb, onPrimary, pIgnoreRC,
                               ppContext ) ;
      if ( rc )
      {
         if ( rtnCoordCataReplyCheck( cb, rc, _canRetry( retryTimes ),
                                      cataInfo, NULL, TRUE ) )
         {
            ++retryTimes ;
            goto retry ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordCommand::executeOnCL( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       const CHAR *pCLName,
                                       BOOLEAN firstUpdateCata,
                                       const CoordGroupList *pSpecGrpLst,
                                       SET_RC *pIgnoreRC,
                                       CoordGroupList *pSucGrpLst,
                                       rtnContextCoord **ppContext )
   {
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();

      rtnCoordQuery queryOpr( isReadonly() ) ;
      rtnQueryConf queryConf ;
      rtnSendOptions sendOpt ;

      queryConf._allCataGroups = TRUE ;
      queryConf._realCLName = pCLName ;
      queryConf._updateAndGetCata = firstUpdateCata ;
      queryConf._openEmptyContext = TRUE ;

      sendOpt._primary = TRUE ;
      sendOpt._pIgnoreRC = pIgnoreRC ;
      if ( pSpecGrpLst )
      {
         sendOpt._groupLst = *pSpecGrpLst ;
         sendOpt._useSpecialGrp = TRUE ;
      }

      if ( !pSucGrpLst )
      {
         return queryOpr.queryOrDoOnCL( pMsg, pRouteAgent, cb, ppContext,
                                        sendOpt, &queryConf ) ;
      }
      else
      {
         return queryOpr.queryOrDoOnCL( pMsg, pRouteAgent, cb, ppContext,
                                        sendOpt, *pSucGrpLst, &queryConf ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCOM_QUERYONCATALOG, "rtnCoordCommand::queryOnCatalog" )
   INT32 rtnCoordCommand::queryOnCatalog( MsgHeader *pMsg,
                                          INT32 requestType,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCOM_QUERYONCATALOG ) ;
      rtnContextCoord *pContext        = NULL ;

      // fill default-reply(list success)
      contextID = -1 ;

      // forward source request to dest
      pMsg->opCode                     = requestType ;

      // execute query data group on catalog
      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, &pContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Query[%d] on catalog group failed, rc = %d",
                  requestType, rc ) ;
         goto error ;
      }

   done :
      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }
	  PD_TRACE_EXITRC ( SDB_RTNCOCOM_QUERYONCATALOG, rc );
      return rc ;
   error :
      // make sure to clear context whenever error happened
      if ( pContext )
      {
         INT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         pContext = NULL ;
      }
      //PD_TRACE_EXIT ( SDB_RTNCOCOM_QUERYONCATALOG ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCOM_QUERYONCATAANDPUSHTOCONTEXT, "rtnCoordCommand::queryOnCataAndPushToContext" )
   INT32 rtnCoordCommand::queryOnCatalog( const rtnQueryOptions &options,
                                          pmdEDUCB *cb,
                                          SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCOM_QUERYONCATAANDPUSHTOCONTEXT ) ;

      CHAR *msgBuf = NULL ;
      INT32 msgBufLen = 0 ;
      contextID = -1 ;

      rc = options.toQueryMsg( &msgBuf, msgBufLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build query msg:%d", rc ) ;
         goto error ;
      }

      rc = queryOnCatalog( (MsgHeader*)msgBuf, MSG_BS_QUERY_REQ, cb,
                           contextID, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on catalog group failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != msgBuf )
      {
         SDB_OSS_FREE( msgBuf ) ;
         msgBuf = NULL ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOCOM_QUERYONCATAANDPUSHTOCONTEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCOM_QUERYONCATAANDPUSHTOVEC, "rtnCoordCommand::queryOnCataAndPushToContext" )
   INT32 rtnCoordCommand::queryOnCataAndPushToVec( const rtnQueryOptions &options,
                                                   pmdEDUCB *cb,
                                                   vector< BSONObj > &objs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCOM_QUERYONCATAANDPUSHTOVEC ) ;
      SINT64 contextID = -1 ;
      rtnContextBuf bufObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = queryOnCatalog( options, cb, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to query on catalog:%d", rc ) ;
         goto error ;
      }

      do
      {
         rc = rtnGetMore( contextID, -1, bufObj, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            contextID = -1 ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            contextID = -1 ;
            PD_LOG( PDERROR, "failed to getmore from context:%d", rc ) ;
            goto error ;
         }
         else
         {
            while ( !bufObj.eof() )
            {
               BSONObj obj ;
               rc = bufObj.nextObj( obj ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to get obj from obj buf:%d", rc ) ;
                  goto error ;
               }

               objs.push_back( obj.getOwned() ) ;
            }
            continue ;
         }
      } while( TRUE ) ;

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOCOM_QUERYONCATAANDPUSHTOVEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCOM__PRINTDEBUG, "rtnCoordCommand::_printDebug" )
   void rtnCoordCommand::_printDebug ( CHAR *pReceiveBuffer,
                                       const CHAR *pFuncName )
   {
   #if defined (_DEBUG)
      INT32 rc         = SDB_OK ;
      INT32 flag       = 0 ;
      CHAR *collection = NULL ;
      SINT64 skip      =  0;
      SINT64 limit     = -1 ;
      CHAR *query      = NULL ;
      CHAR *selector   = NULL ;
      CHAR *orderby    = NULL ;
      CHAR *hint       = NULL ;
      rc = msgExtractQuery( pReceiveBuffer, &flag, &collection,
                            &skip, &limit, &query, &selector,
                            &orderby, &hint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract query msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj func( query ) ;
         PD_LOG( PDDEBUG, "%s: %s", pFuncName,
                 func.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done :
      return ;
   error :
      goto done ;
   #endif // _DEBUG
   }

   INT32 rtnCoordCommand::executeOnNodes( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          ROUTE_SET &nodes,
                                          ROUTE_RC_MAP &faileds,
                                          rtnCoordCtrlParam &ctrlParam,
                                          ROUTE_SET *pSucNodes,
                                          SET_RC *pIgnoreRC,
                                          rtnContextCoord *pContext )
   {
      INT32 rc                      = SDB_OK ;
      INT32 rcTmp                   = SDB_OK ;
      pmdKRCB *krcb                 = pmdGetKRCB() ;
      CoordCB *pCoordCB             = krcb->getCoordCB() ;
      netMultiRouteAgent *pAgent    = pCoordCB->getRouteAgent() ;
      REQUESTID_MAP sendNodes ;
      REPLY_QUE replyQue ;
      ROUTE_SET retriedNodes ;
      ROUTE_SET needRetryNodes  = nodes ;

      /// clear msg
      pMsg->TID = cb->getTID() ;
      pMsg->routeID.value = MSG_INVALID_ROUTEID ;

   retry:
      /// send msg
      rtnCoordSendRequestToNodes( (void *)pMsg, needRetryNodes, pAgent, cb,
                                  sendNodes, faileds ) ;
      retriedNodes.insert( needRetryNodes.begin(), needRetryNodes.end() ) ;
      needRetryNodes.clear() ;

      /// recv reply
      rcTmp = rtnCoordGetReply( cb, sendNodes, replyQue,
                                MAKE_REPLY_TYPE(pMsg->opCode),
                                TRUE, FALSE ) ;
      rc = rc ? rc : rcTmp ;

      /// process reply
      rcTmp = _processNodesReply( replyQue, faileds, retriedNodes,
                                  needRetryNodes, ctrlParam, pContext,
                                  pIgnoreRC, pSucNodes ) ;
      rc = rc ? rc : rcTmp ;

      if ( rc )
      {
         rtnClearReplyQue( &replyQue ) ;
         rtnCoordClearRequest( cb, sendNodes ) ;
      }
      else if ( needRetryNodes.size() != 0 )
      {
         goto retry ;
      }

      return rc ;
   }

   INT32 rtnCoordCommand::executeOnNodes( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnCoordCtrlParam &ctrlParam,
                                          UINT32 mask,
                                          ROUTE_RC_MAP &faileds,
                                          rtnContextCoord **ppContext,
                                          BOOLEAN openEmptyContext,
                                          SET_RC *pIgnoreRC,
                                          ROUTE_SET *pSucNodes )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb = pmdGetKRCB() ;
      SDB_RTNCB *pRtncb = pKrcb->getRTNCB() ;
      rtnQueryOptions queryOption ;
      const CHAR *pSrcFilterObjData = NULL ;
      BSONObj *pFilterObj = NULL ;
      BOOLEAN hasNodeOrGroupFilter = FALSE ;

      CoordGroupList allGroupLst ;
      CoordGroupList expectGrpLst ;
      CoordGroupList groupLst ;
      ROUTE_SET sendNodes ;
      BSONObj newFilterObj ;
      BOOLEAN hasParseRetry = FALSE ;

      CHAR *pNewMsg = NULL ;
      INT32 newMsgSize = 0 ;
      INT64 contextID = -1 ;
      rtnContextCoord *pTmpContext = NULL ;
      BOOLEAN needReset = FALSE ;

      /// 1. extrace msg
      rc = queryOption.fromQueryMsg( (CHAR*)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extrace query msg failed, rc: %d", rc ) ;
      /// 2. get filter obj
      pFilterObj = rtnCoordGetFilterByID( ctrlParam._filterID, queryOption ) ;
      pSrcFilterObjData = pFilterObj->objdata() ;
      /// 3. parse control param
      rc = rtnCoordParseControlParam( *pFilterObj, ctrlParam, mask,
                                      &newFilterObj ) ;
      PD_RC_CHECK( rc, PDERROR, "prase control param failed, rc: %d", rc ) ;
      *pFilterObj = newFilterObj ;

      /// 4. parse groups
      rc = rtnCoordGetAllGroupList( cb, allGroupLst, NULL, FALSE, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all group list, rc: %d",
                   rc ) ;
      {
         CoordGroupList::iterator itGrp = allGroupLst.begin() ;
         while( itGrp != allGroupLst.end() )
         {
            if ( ( !ctrlParam._role[ SDB_ROLE_DATA ] &&
                   itGrp->second >= DATA_GROUP_ID_BEGIN &&
                   itGrp->second <= DATA_GROUP_ID_END ) ||
                 ( !ctrlParam._role[ SDB_ROLE_CATALOG ] &&
                   CATALOG_GROUPID == itGrp->second ) ||
                 ( !ctrlParam._role[ SDB_ROLE_COORD ] &&
                   COORD_GROUPID == itGrp->second ) )
            {
               ++itGrp ;
               continue ;
            }
            expectGrpLst[ itGrp->first ] = itGrp->second ;
            ++itGrp ;
         }
      }

      if ( !pFilterObj->isEmpty() )
      {
         rc = rtnCoordParseGroupList( cb, *pFilterObj, groupLst,
                                      &newFilterObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse groups, rc: %d", rc  ) ;
         if ( pFilterObj->objdata() != newFilterObj.objdata() )
         {
            hasNodeOrGroupFilter = TRUE ;
         }
         *pFilterObj = newFilterObj ;
      }

   parseNode:
      /// 5. parse nodes
      rc = rtnCoordGetGroupNodes( cb, *pFilterObj, ctrlParam._emptyFilterSel,
                                  ( 0 == groupLst.size() ? ( hasParseRetry ?
                                  expectGrpLst : allGroupLst ) : groupLst ),
                                  sendNodes, &newFilterObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc ) ;
      if ( sendNodes.size() == 0 )
      {
         PD_LOG( PDWARNING, "Node specfic nodes[%s]",
                 pFilterObj->toString().c_str() ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }
      if ( pFilterObj->objdata() != newFilterObj.objdata() )
      {
         hasNodeOrGroupFilter = TRUE ;
      }
      else if ( !hasParseRetry && 0 == groupLst.size() &&
                allGroupLst.size() != expectGrpLst.size() )
      {
         hasParseRetry = TRUE ;
         goto parseNode ;
      }
      *pFilterObj = newFilterObj ;

      if ( !ctrlParam._isGlobal )
      {
         /// no group and node info
         if ( !hasNodeOrGroupFilter )
         {
            rc = SDB_RTN_CMD_IN_LOCAL_MODE ;
            goto error ;
         }
         ctrlParam._isGlobal = TRUE ;
      }

      ///6. open context
      if ( ppContext )
      {
         if ( NULL == *ppContext )
         {
            // create context
            rc = pRtncb->contextNew( RTN_CONTEXT_COORD,
                                     (rtnContext **)ppContext,
                                     contextID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                         rc ) ;
         }
         else
         {
            contextID = (*ppContext)->contextID() ;
            // the context is create in out side, do nothing
         }
         pTmpContext = *ppContext ;
      }
      if ( pTmpContext && !pTmpContext->isOpened() )
      {
         if ( openEmptyContext )
         {
            rc = pTmpContext->open( BSONObj(), BSONObj(), -1, 0 ) ;
         }
         else
         {
            BSONObj srcSelector = queryOption._selector ;
            INT64 srcLimit = queryOption._limit ;
            INT64 srcSkip = queryOption._skip ;

            if ( sendNodes.size() > 1 )
            {
               if ( srcLimit > 0 && srcSkip > 0 )
               {
                  queryOption._limit = srcLimit + srcSkip ;
               }
               queryOption._skip = 0 ;
            }
            else
            {
               srcLimit = -1 ;
               srcSkip = 0 ;
            }

            // build new selector
            rtnNeedResetSelector( srcSelector, queryOption._orderBy,
                                  needReset ) ;
            if ( needReset )
            {
               queryOption._selector = BSONObj() ;
            }
            // open context
            rc = pTmpContext->open( queryOption._orderBy,
                                    needReset ? srcSelector : BSONObj(),
                                    srcLimit, srcSkip ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Open context failed(rc=%d)", rc ) ;
      }

      /// 7. ensure new msg
      if ( pSrcFilterObjData == pFilterObj->objdata() && !needReset )
      {
         /// not change
         pNewMsg = (CHAR*)pMsg ;
         MsgOpQuery *pQueryMsg = ( MsgOpQuery* )pNewMsg ;
         pQueryMsg->numToReturn = queryOption._limit ;
         pQueryMsg->numToSkip = queryOption._skip ;
      }
      else
      {
         rc = queryOption.toQueryMsg( &pNewMsg, newMsgSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Build new query message failed, rc: %d",
                      rc ) ;
      }

      /// 8. execute
      rc = executeOnNodes( (MsgHeader*)pNewMsg, cb, sendNodes,
                           faileds, ctrlParam, pSucNodes, pIgnoreRC,
                           pTmpContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Execute on nodes failed, rc: %d", rc ) ;

      /// 9. build failed result
      if ( pTmpContext )
      {
         rc = _buildFailedNodeReply( faileds, pTmpContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Build failed node reply failed, rc: %d",
                      rc ) ;
      }

   done:
      if ( pNewMsg != (CHAR*)pMsg )
      {
         SDB_OSS_FREE( pNewMsg ) ;
         pNewMsg = NULL ;
         newMsgSize = 0 ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         *ppContext = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCODEFCOM_EXE, "rtnCoordDefaultCommand::execute" )
   INT32 rtnCoordDefaultCommand::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY ( SDB_RTNCODEFCOM_EXE ) ;
      contextID = -1 ;
      PD_TRACE_EXIT ( SDB_RTNCODEFCOM_EXE ) ;
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   INT32 rtnCoordBackupBase::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      rtnCoordCtrlParam ctrlParam ;
      rtnContextCoord *pContext = NULL ;
      ROUTE_RC_MAP failedNodes ;
      UINT32 mask = _getMask() ;

      contextID = -1 ;
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = _getGroupMatherIndex() ;
      ctrlParam._emptyFilterSel = _nodeSelWhenNoFilter() ;

      rc = executeOnNodes( pMsg, cb, ctrlParam, mask, failedNodes,
                           _useContext() ? &pContext : NULL,
                           FALSE, NULL, NULL ) ;
      if ( rc )
      {
         if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
         {
            rc = SDB_INVALIDARG ;
         }
         else
         {
            PD_LOG( PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
         }
         goto error ;
      }

      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }
      else if ( !_allowFailed() && failedNodes.size() > 0 )
      {
         rc = failedNodes.begin()->second ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   FILTER_BSON_ID rtnCoordListBackup::_getGroupMatherIndex ()
   {
      return FILTER_ID_HINT ;
   }

   NODE_SEL_STY rtnCoordListBackup::_nodeSelWhenNoFilter ()
   {
      return NODE_SEL_ALL ;
   }

   BOOLEAN rtnCoordListBackup::_allowFailed ()
   {
      return TRUE ;
   }

   BOOLEAN rtnCoordListBackup::_useContext ()
   {
      return TRUE ;
   }

   UINT32 rtnCoordListBackup::_getMask() const
   {
      return RTN_CTRL_MASK_ALL ;
   }

   FILTER_BSON_ID rtnCoordRemoveBackup::_getGroupMatherIndex ()
   {
      return FILTER_ID_MATCHER ;
   }

   NODE_SEL_STY rtnCoordRemoveBackup::_nodeSelWhenNoFilter ()
   {
      return NODE_SEL_ALL ;
   }

   BOOLEAN rtnCoordRemoveBackup::_allowFailed ()
   {
      return FALSE ;
   }

   BOOLEAN rtnCoordRemoveBackup::_useContext ()
   {
      return FALSE ;
   }

   UINT32 rtnCoordRemoveBackup::_getMask() const
   {
      return ~RTN_CTRL_MASK_NODE_SELECT ;
   }

   FILTER_BSON_ID rtnCoordBackupOffline::_getGroupMatherIndex ()
   {
      return FILTER_ID_MATCHER ;
   }

   NODE_SEL_STY rtnCoordBackupOffline::_nodeSelWhenNoFilter ()
   {
      return NODE_SEL_PRIMARY ;
   }

   BOOLEAN rtnCoordBackupOffline::_allowFailed ()
   {
      return FALSE ;
   }

   BOOLEAN rtnCoordBackupOffline::_useContext ()
   {
      return FALSE ;
   }

   UINT32 rtnCoordBackupOffline::_getMask() const
   {
      return ~RTN_CTRL_MASK_NODE_SELECT ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDLISTGRS_EXE, "rtnCoordCMDListGroups::execute" )
   INT32 rtnCoordCMDListGroups::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDLISTGRS_EXE ) ;
      rc = queryOnCatalog ( pMsg,
                            MSG_CAT_QUERY_DATA_GRP_REQ,
                            cb,
                            contextID,
                            buf ) ;
      PD_TRACE_EXITRC ( SDB_RTNCOCMDLISTGRS_EXE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCRCS_EXE, "rtnCoordCMDCreateCollectionSpace::execute" )
   INT32 rtnCoordCMDCreateCollectionSpace::execute( MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    INT64 &contextID,
                                                    rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCRCS_EXE ) ;

      CHAR *pCollectionName = NULL ;
      CHAR *pQuery = NULL ;
      BSONObj boQuery ;

      // fill default-reply
      contextID                        = -1 ;
      MsgOpQuery *pCreateReq           = (MsgOpQuery *)pMsg;

      try
      {
         rc = msgExtractQuery( (CHAR*)pMsg, NULL, &pCollectionName,
                               NULL, NULL, &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract message failed, rc: %d", rc ) ;

         boQuery = BSONObj( pQuery ) ;

         pCreateReq->header.opCode = MSG_CAT_CREATE_COLLECTION_SPACE_REQ ;
         // execute create collection on catalog
         rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, pCollectionName + 1, AUDIT_OBJ_CS,
                           boQuery.getField(FIELD_NAME_NAME).valuestrsafe(),
                           rc, "Option:%s", boQuery.toString().c_str() ) ;
         /// CHECK ERRORS
         if ( rc )
         {
            PD_LOG ( PDERROR, "create collectionspace failed, rc = %d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCRCS_EXE, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDALCL__EXEOLD, "rtnCoordCMDAlterCollection::_executeOld" )
   INT32 rtnCoordCMDAlterCollection::_executeOld( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  INT64 &contextID,
                                                  rtnContextBuf *buf,
                                                  string &clName )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDALCL__EXEOLD ) ;
      // fill default-reply
      contextID                        = -1 ;

      MsgOpQuery *pAlterReq            = (MsgOpQuery *)pMsg ;
      pAlterReq->header.opCode         = MSG_CAT_ALTER_COLLECTION_REQ ;

      CoordGroupList groupList ;
      const CHAR *fullName             = NULL ;
      CHAR *queryBuf                   = NULL ;
      SET_RC ignoreRC ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &queryBuf,
                            NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract query msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj query( queryBuf ) ;
         BSONElement ele = query.getField( FIELD_NAME_NAME ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid query object:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         fullName = ele.valuestr() ;
         clName = fullName ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // send request to catalog
      rc = executeOnCataGroup( pMsg, cb, &groupList ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "alter collection failed on catalog, rc = %d",
                  rc ) ;
         goto error ;
      }

      pAlterReq->header.opCode = MSG_BS_QUERY_REQ ;
      /// we only want to update data's catalog version.
      ignoreRC.insert( SDB_MAIN_CL_OP_ERR ) ;
      ignoreRC.insert( SDB_CLS_COORD_NODE_CAT_VER_OLD ) ;

      rc = executeOnCL( pMsg, cb, fullName, TRUE, &groupList,
                        &ignoreRC, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to alter collection on data group, rc: %d",
                 rc ) ;
         rc = SDB_BUT_FAILED_ON_DATA ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDALCL__EXEOLD, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDALCL__TESTCL, "rtnCoordCMDAlterCollection::_testCollection" )
   INT32 rtnCoordCMDAlterCollection::_testCollection( const CHAR *fullName,
                                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCMDALCL__TESTCL ) ;
      CHAR *msg = NULL ;
      INT32 msgSize = 0 ;
      BSONObj obj = BSON( FIELD_NAME_NAME << fullName ) ;
      rc = msgBuildQueryMsg( &msg, &msgSize,
                             CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION,
                             0, 0, 0, -1, &obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build msg:%d", rc ) ;
         goto error ;
      }

      rc = executeOnCL( ( MsgHeader * )msg, cb, fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to test collection:%d", rc ) ;
         goto error ;
      }
   done:
      SAFE_OSS_FREE( msg ) ;
      PD_TRACE_EXITRC( SDB_RTNCOCMDALCL__TESTCL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDALCL__EXE, "rtnCoordCMDAlterCollection::_execute" )
   INT32 rtnCoordCMDAlterCollection::_execute( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               INT64 &contextID,
                                               rtnContextBuf *buf,
                                               string &clName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCMDALCL__EXE ) ;
      _rtnAlterJob job ;
      CHAR *query = NULL ;
      INT32 opCode = pMsg->opCode ;
      SET_RC ignoreLst ;

      /// 1. extract msg
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &query,
                            NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract query msg:%d", rc ) ;
         goto error ;
      }

      /// 2. init job
      try
      {
         rc = job.init( BSONObj( query ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to init alter job:%d", rc ) ;
            goto error ;
         }
         clName = job.getName() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// 3. test collection
      rc = _testCollection( job.getName(), cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to test collection[%s], rc:%d",
                 job.getName(), rc ) ;
         goto error ;
      }

      /// 4. on catalog
      pMsg->opCode = MSG_CAT_ALTER_COLLECTION_REQ ;
      rc = executeOnCataGroup( pMsg, cb, TRUE ) ;
      pMsg->opCode = opCode ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute on catalog:%d", rc ) ;
         goto error ;
      }

      ignoreLst.insert( SDB_IXM_REDEF ) ;
      ignoreLst.insert( SDB_IXM_NOTEXIST ) ;

      /// 5. on data
      rc = executeOnCL( pMsg, cb,
                        job.getName(),
                        TRUE,
                        NULL,
                        &ignoreLst ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute on cl:%s, rc:%d",
                 job.getName(), rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOCMDALCL__EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDALCL_EXE, "rtnCoordCMDAlterCollection::execute" )
   INT32 rtnCoordCMDAlterCollection::execute( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              INT64 &contextID,
                                              rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCMDALCL_EXE ) ;
      string clName ;
      CHAR *query = NULL ;
      BOOLEAN isOld = FALSE ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &query,
                            NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract query msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         isOld = BSONObj( query ).getField( FIELD_NAME_VERSION ).eoo() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = isOld ?
           _executeOld( pMsg, cb, contextID, buf, clName ) :
           _execute( pMsg, cb, contextID, buf, clName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to alter collection:%d", rc ) ;
         goto error ;
      }
   done:
      if ( !clName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_ALTER_COLLECTION, AUDIT_OBJ_CL,
                           clName.c_str(), rc, "Option:%s",
                           BSONObj(query).toString().c_str() ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOCMDALCL_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCRCL_EXE, "rtnCoordCMDCreateCollection::execute" )
   INT32 rtnCoordCMDCreateCollection::execute( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               INT64 &contextID,
                                               rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCRCL_EXE ) ;

      // fill default-reply(delete success)
      contextID                        = -1 ;

      MsgOpQuery *pCreateReq           = (MsgOpQuery *)pMsg;
      pCreateReq->header.opCode        = MSG_CAT_CREATE_COLLECTION_REQ ;

      CoordGroupList groupLst ;
      vector<BSONObj> replyFromCata ;
      BOOLEAN isMainCL                 = FALSE ;
      const CHAR *pCollectionName      = NULL ;
      CHAR *pCommand                   = NULL ;
      BSONObj boQuery ;

      try
      {
         CHAR *pQuery = NULL ;
         BSONElement beIsMainCL ;
         BSONElement beShardingType ;
         BSONElement beShardingKey ;
         BSONElement eleName ;

         rc = msgExtractQuery( (CHAR*)pMsg, NULL, &pCommand,
                               NULL, NULL, &pQuery,
                               NULL, NULL, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to parse the create collection "
                    "message, rc:%d", rc ) ;
            goto error ;
         }

         boQuery = BSONObj( pQuery );
         beIsMainCL = boQuery.getField( FIELD_NAME_ISMAINCL );
         isMainCL = beIsMainCL.booleanSafe();
         if ( isMainCL )
         {
            beShardingKey = boQuery.getField( FIELD_NAME_SHARDINGKEY );
            PD_CHECK( beShardingKey.type() == Object, SDB_NO_SHARDINGKEY,
                      error, PDERROR, "There is no valid sharding-key field" ) ;
            beShardingType = boQuery.getField( FIELD_NAME_SHARDTYPE );
            PD_CHECK( 0 != beShardingType.str().compare(
                      FIELD_NAME_SHARDTYPE_HASH ),
                      SDB_INVALID_MAIN_CL_TYPE, error, PDERROR,
                      "The sharding-type of main-collection must be range" ) ;
         }
         eleName = boQuery.getField( FIELD_NAME_NAME ) ;
         // get collection name
         if ( eleName.type() != String )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] is error",
                    FIELD_NAME_NAME, eleName.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pCollectionName = eleName.valuestr() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to create collection, received "
                  "unexpected error: %s", e.what() ) ;
         goto error ;
      }

      // send request to catalog
      if ( !isMainCL )
      {
         rc = executeOnCataGroup ( pMsg, cb, &groupLst, &replyFromCata ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "create collection failed on catalog, rc = %d",
                     rc ) ;
            goto error ;
         }

         pCreateReq->header.opCode = MSG_BS_QUERY_REQ ;
         rc = executeOnCL( pMsg, cb, pCollectionName, TRUE, &groupLst,
                           NULL, NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDWARNING, "Create collection failed on data "
                     "node, rc: %d", rc ) ;
            rc = SDB_OK ;
         }
      }
      else
      {
         rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "create collection failed on catalog, rc = %d",
                     rc ) ;
            goto error ;
         }
      }

      /// check whether should notify data group to complete tasks.
      if ( !isMainCL && !replyFromCata.empty() )
      {
         BSONElement task = replyFromCata.at(0).getField( CAT_TASKID_NAME ) ;
         if ( Array == task.type() )
         {
            rc = _notifyDataGroupsToStartTask( pCollectionName, task, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to notify data groups to start "
                       "task: %d", rc ) ;
               /// meta data has already been modified.
               /// here we change a errno.
               rc = SDB_BUT_FAILED_ON_DATA ;
               goto error ;
            }
         }
      }

   done :
      if ( pCollectionName )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, pCommand + 1, AUDIT_OBJ_CL,
                           pCollectionName, rc, "Option:%s",
                           boQuery.toString().c_str() ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCRCL_EXE, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDSSONNODE__NOTIFYDATAGROUPS, "rtnCoordCMDCreateCollection::_notifyDataGroupsToStartTask" )
   INT32 rtnCoordCMDCreateCollection::_notifyDataGroupsToStartTask( const CHAR *pCLName,
                                                                    const BSONElement &task,
                                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCMDSSONNODE__NOTIFYDATAGROUPS ) ;
      CHAR *buffer = NULL ;
      INT32 bufferLen = 0 ;
      vector<BSONObj> reply ;

      BSONObjBuilder builder ;
      builder.appendAs( task, "$in" ) ;
      BSONObj condition = BSON( FIELD_NAME_TASKID << builder.obj() );

      BSONElement group ;
      CoordGroupList groupList ;
      MsgOpQuery *msgHeader = NULL ;
      INT32 everRc = SDB_OK ;
      INT64 contextID = -1  ;
      vector<BSONObj>::const_iterator itr ;

      CoordCB *coordCb = pmdGetKRCB()->getCoordCB () ;
      rtnCoordCommand *cmd = coordCb->getProcesserFactory(
                             )->getCommandProcesser( COORD_CMD_WAITTASK ) ;

      rc = msgBuildQueryMsg( &buffer, &bufferLen, CAT_TASK_INFO_COLLECTION,
                             0, 0, 0, -1, &condition, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build query msg:%d", rc ) ;
         goto error ;
      }

      msgHeader = ( MsgOpQuery * )buffer ;
      msgHeader->header.opCode = MSG_CAT_QUERY_TASK_REQ ;

      /// get task info from catalog.
      rc = executeOnCataGroup( (MsgHeader*)buffer, cb, NULL, &reply ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get task info from catalog, rc:%d",
                 rc ) ;
         goto error ;
      }

      /// notify all groups to start task.
      for ( itr = reply.begin() ; itr != reply.end(); itr++ )
      {
         groupList.clear() ;

         group = itr->getField( FIELD_NAME_TARGETID ) ;
         if ( NumberInt != group.type() )
         {
            PD_LOG( PDERROR, "target id is not a numberint.[%s]",
                    itr->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         groupList[ group.Int() ] = group.Int() ;

         rc = msgBuildQueryMsg( &buffer, &bufferLen,
                                CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                0, 0, 0, -1, &(*itr), NULL, NULL, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build split msg:%d", rc ) ;
            goto error ;
         }

         msgHeader = ( MsgOpQuery * )buffer ;

         rc = executeOnCL( (MsgHeader *)buffer, cb, pCLName, FALSE,
                           &groupList, NULL, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to notify group[%d] to split "
                    "collection[%s], rc: %d", group.Int(), pCLName, rc ) ;
            everRc = ( SDB_OK == everRc ) ? rc : everRc ;
            rc = SDB_OK ;
            continue ;
            /// here we try to send msg to all groups, do not goto error.
         }
      }

      rc = ( SDB_OK == everRc ) ? SDB_OK : everRc ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Some spliting are faled, check the catalog info" ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &buffer, &bufferLen, "CAT",
                             0, 0, 0, -1, &condition, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build split msg: %d", rc ) ;
         goto error ;
      }

      rc = cmd->execute( (MsgHeader*)buffer, cb, contextID, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to wait task done:%d", rc ) ;
         rc = SDB_OK ;
         ossSleep( 5000 ) ;
         /// do not return err. only sleep some time to wait task done.
      }

   done:
      if ( NULL != buffer )
      {
         SDB_OSS_FREE( buffer ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOCMDSSONNODE__NOTIFYDATAGROUPS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordCMDSnapshotIntrBase::execute( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               INT64 &contextID,
                                               rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      rtnCoordCtrlParam ctrlParam ;
      ROUTE_RC_MAP faileds ;
      rtnContextCoord *pContext = NULL ;

      contextID = -1 ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;

      rc = executeOnNodes( pMsg, cb, ctrlParam, RTN_CTRL_MASK_ALL,
                           faileds, &pContext, FALSE, NULL, NULL ) ;
      if ( rc )
      {
         if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
         {
            rc = SDB_COORD_UNKNOWN_OP_REQ ;
         }
         else
         {
            PD_LOG( PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
         }
         goto error ;
      }

      if ( pContext )
      {
         if ( _useContext() )
         {
            contextID = pContext->contextID() ;
         }
         else
         {
            pmdGetKRCB()->getRTNCB()->contextDelete( pContext->contextID(),
                                                     cb ) ;
            pContext = NULL ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // snapshot collection operation. Basically this operation from coord does
   // not broadcast to data node. Instead it works like ListGroups, which sends
   // request to catalog
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSSCLS_EXE, "rtnCoordCMDSnapshotCollections::execute" )
   INT32 rtnCoordCMDSnapshotCollectionsTmp::execute( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     INT64 &contextID,
                                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDSSCLS_EXE ) ;
      rc = queryOnCatalog ( pMsg,
                            MSG_CAT_QUERY_COLLECTIONS_REQ,
                            cb, contextID, buf ) ;
      PD_TRACE_EXITRC ( SDB_RTNCOCMDSSCLS_EXE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSSCSS_EXE, "rtnCoordCMDSnapshotCollectionSpaces::execute" )
   INT32 rtnCoordCMDSnapshotCollectionSpacesTmp::execute( MsgHeader *pMsg,
                                                          pmdEDUCB *cb,
                                                          INT64 &contextID,
                                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDSSCSS_EXE ) ;
      rc = queryOnCatalog ( pMsg,
                            MSG_CAT_QUERY_COLLECTIONSPACES_REQ,
                            cb, contextID, buf ) ;
      PD_TRACE_EXITRC ( SDB_RTNCOCMDSSCSS_EXE, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCOCMD2PC_EXE, "rtnCoordCMD2PhaseCommit::execute" )
   INT32 rtnCoordCMD2PhaseCommit::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMD2PC_EXE ) ;
      pmdKRCB *pKrcb = pmdGetKRCB();
      _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
      SET_RC ignoreRCList ;
      string strName ;

      contextID = -1 ;
      _getIgnoreRCList( ignoreRCList ) ;

      // phase 1
      rc = doP1OnDataGroup( (CHAR*)pMsg, cb, ignoreRCList,
                            contextID, strName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute phase1 on data group(rc=%d)",
                   rc );

      rc = doOnCataGroup( (CHAR*)pMsg, cb, strName );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute on cata group(rc=%d)",
                   rc );

      // phase 2
      rc = doP2OnDataGroup( (CHAR*)pMsg, cb, strName, contextID );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute phase2 on data group(rc=%d)",
                   rc ) ;

      rc = complete( (CHAR*)pMsg, cb, strName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to complete the operation(rc=%d)",
                   rc ) ;

   done:
      if ( -1 != contextID )
      {
         pRtncb->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( !strName.empty() )
      {
         BOOLEAN isCL = ossStrchr( strName.c_str(), '.' ) ? TRUE : FALSE ;
         PD_AUDIT_COMMAND( AUDIT_DDL, ( isCL ? CMD_NAME_DROP_COLLECTION :
                                               CMD_NAME_DROP_COLLECTIONSPACE ),
                           ( isCL ? AUDIT_OBJ_CL : AUDIT_OBJ_CS ),
                           strName.c_str(), rc, "" ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMD2PC_EXE, rc ) ;
      return rc;
   error:
      goto done ;
   }

   void rtnCoordCMD2PhaseCommit::_getIgnoreRCList( SET_RC &ignoreRCList )
   {
   }

   INT32 rtnCoordCMD2PhaseCommit::complete( CHAR *pReceiveBuffer,
                                            pmdEDUCB * cb,
                                            const string &strName )
   {
      return SDB_OK;
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCOCMD2PC_DOP1, "rtnCoordCMD2PhaseCommit::doP1OnDataGroup" )
   INT32 rtnCoordCMD2PhaseCommit::doP1OnDataGroup( CHAR *pReceiveBuffer,
                                                   pmdEDUCB *cb,
                                                   SET_RC &ignoreRCList,
                                                   SINT64 &contextID,
                                                   string &strName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMD2PC_DOP1 ) ;

      rtnContextCoord *pContext = NULL ;
      CoordGroupList groupLst ;

      rc = _getGroupList( pReceiveBuffer, cb, groupLst, strName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group-list, rc: %d", rc ) ;

      /// is collection
      if ( NULL != ossStrchr( strName.c_str(), '.' ) )
      {
         rc = executeOnCL( (MsgHeader*)pReceiveBuffer, cb, strName.c_str(),
                           FALSE, NULL, &ignoreRCList, NULL, &pContext ) ;
      }
      else
      {
         rc = executeOnDataGroup( (MsgHeader*)pReceiveBuffer, cb, groupLst,
                                  TRUE, &ignoreRCList, NULL, &pContext ) ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute phase-1 on data node, rc: %d",
                   rc ) ;

      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMD2PC_DOP1, rc ) ;
      return rc;
   error:
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         contextID = -1 ;
         pContext = NULL ;
      }
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCOCMD2PC_DOP2, "rtnCoordCMD2PhaseCommit::doP2OnDataGroup" )
   INT32 rtnCoordCMD2PhaseCommit::doP2OnDataGroup( CHAR *pReceiveBuffer,
                                                   pmdEDUCB * cb,
                                                   const string &strName,
                                                   SINT64 &contextID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMD2PC_DOP2 ) ;
      pmdKRCB *pKrcb = pmdGetKRCB();
      _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
      rtnContextBuf buffObj;
      rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
      if ( SDB_DMS_EOC == rc )
      {
         contextID = -1;
         rc = SDB_OK;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute phase-2 on data node(rc=%d)",
                   rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMD2PC_DOP2, rc ) ;
      return rc;
   error:
      if ( -1 != contextID )
      {
         pRtncb->contextDelete ( contextID, cb ) ;
         contextID = -1;
      }
      goto done;
   }

   void rtnCoordCMDDropCollection::_getIgnoreRCList( SET_RC &ignoreRCList )
   {
      ignoreRCList.insert( SDB_DMS_NOTEXIST );
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCODROPCL_GETCLNAME, "rtnCoordCMDDropCollection::_getCLName" )
   INT32 rtnCoordCMDDropCollection::_getCLName( CHAR *pReceiveBuffer,
                                                string &strCLName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCODROPCL_GETCLNAME ) ;

      CHAR *pQuery                     = NULL;
      BSONObj boQuery ;
      rc = msgExtractQuery( pReceiveBuffer, NULL, NULL,
                            NULL, NULL, &pQuery, NULL,
                            NULL, NULL );
      PD_RC_CHECK( rc, PDERROR, "Failed to parse the request(rc=%d)", rc ) ;
      try
      {
         boQuery = BSONObj( pQuery );
         BSONElement beCLName = boQuery.getField( CAT_COLLECTION_NAME );
         PD_CHECK( beCLName.type() == String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get collection name" );
         strCLName = beCLName.str() ;
      }
      catch( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Failed to drop collection, occured unexpected "
                  "error:%s", e.what() ) ;
         goto error;
      }
   done:
      PD_TRACE_EXITRC ( SDB_RTNCODROPCL_GETCLNAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCODROPCL_GETGPLST, "rtnCoordCMDDropCollection::_getGroupList" )
   INT32 rtnCoordCMDDropCollection::_getGroupList( CHAR *pReceiveBuffer,
                                                   pmdEDUCB *cb,
                                                   CoordGroupList &groupLst,
                                                   string &strName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCODROPCL_GETGPLST ) ;

      rc = _getCLName( pReceiveBuffer, strName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection name, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCODROPCL_GETGPLST, rc ) ;
      return rc;
   error:
      goto done;
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCODROPCL_CMPL, "rtnCoordCMDDropCollection::complete" )
   INT32 rtnCoordCMDDropCollection::complete( CHAR *pReceiveBuffer,
                                              pmdEDUCB * cb,
                                              const string &strName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCODROPCL_CMPL ) ;
      pmdKRCB *pKrcb = pmdGetKRCB();
      CoordCB *pCoordcb = pKrcb->getCoordCB();

      string strMainCLName ;
      CoordCataInfoPtr cataInfo ;

      rc = rtnCoordGetCataInfo( cb, strName.c_str(), FALSE, cataInfo ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get catalog, complete drop-CL failed(rc=%d)",
                   rc ) ;

      strMainCLName = cataInfo->getCatalogSet()->getMainCLName() ;
      pCoordcb->delCataInfo( strName ) ;
      if ( !strMainCLName.empty() )
      {
         pCoordcb->delCataInfo( strMainCLName ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCODROPCL_CMPL, rc ) ;
      return SDB_OK ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCODROPCL_DOONCATA, "rtnCoordCMDDropCollection::doOnCataGroup" )
   INT32 rtnCoordCMDDropCollection::doOnCataGroup( CHAR *pReceiveBuffer,
                                                   pmdEDUCB * cb,
                                                   const string &strName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCODROPCL_DOONCATA ) ;
      MsgOpQuery *pDropReq             = (MsgOpQuery *)pReceiveBuffer ;
      SINT32 opCode                    = pDropReq->header.opCode ;
      UINT32 TID                       = pDropReq->header.TID ;

      pDropReq->header.opCode = MSG_CAT_DROP_COLLECTION_REQ ;
      rc = executeOnCataCL( pDropReq, cb, strName.c_str(), TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to drop the catalog of cl(rc=%d)",
                   rc ) ;

   done:
      pDropReq->header.opCode = opCode;
      pDropReq->header.TID = TID;
      PD_TRACE_EXITRC ( SDB_RTNCODROPCL_DOONCATA, rc ) ;
      return rc;
   error:
      goto done;
   }

   void rtnCoordCMDDropCollectionSpace::_getIgnoreRCList( SET_RC &ignoreRCList )
   {
      ignoreRCList.insert( SDB_DMS_CS_NOTEXIST );
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCODROPCS_GETGPLST, "rtnCoordCMDDropCollectionSpace::_getGroupList" )
   INT32 rtnCoordCMDDropCollectionSpace::_getGroupList( CHAR *pReceiveBuffer,
                                                        pmdEDUCB *cb,
                                                        CoordGroupList &groupLst,
                                                        string &strName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCODROPCS_GETGPLST ) ;

      CHAR *pQuery                     = NULL;
      BSONObj boQuery;
      BSONObj boEmpty;

      CHAR *pBuffer                    = NULL;
      INT32 bufferSize                 = 0;

      CoordGroupList::const_iterator iter;
      groupLst.clear() ;

      rc = msgExtractQuery( pReceiveBuffer, NULL, NULL,
                            NULL, NULL, &pQuery, NULL,
                            NULL, NULL );
      PD_RC_CHECK( rc, PDERROR, "Failed to parse the request(rc=%d)",
                   rc ) ;

      try
      {
         boQuery = BSONObj( pQuery );
         BSONElement beCSName = boQuery.getField( CAT_COLLECTION_SPACE_NAME );
         PD_CHECK( beCSName.type() == String, SDB_INVALIDARG,
                   error, PDERROR, "failed to get cs name" );
         strName = beCSName.str() ;
      }
      catch( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Failed to drop cs, received unexpected error:%s",
                 e.what() );
         goto error ;
      }

      rc = msgBuildQuerySpaceReqMsg( &pBuffer, &bufferSize, 0, 0, 0, -1,
                                     cb->getTID(), &boQuery, &boEmpty,
                                     &boEmpty, &boEmpty );
      PD_RC_CHECK( rc, PDERROR, "Failed to build query request(rc=%d)",
                   rc );

      rc = executeOnCataGroup( (MsgHeader*)pBuffer, cb, &groupLst );
      PD_RC_CHECK( rc, PDERROR, "Failed to get cs info from catalog(rc=%d)",
                   rc ) ;

   done:
      SAFE_OSS_FREE( pBuffer ) ;
      PD_TRACE_EXITRC ( SDB_RTNCODROPCS_GETGPLST, rc ) ;
      return rc;
   error:
      goto done;
   }

   //PD_TRACE_DECLARE_FUNCTION (SDB_RTNCODROPCS_DOONCATA, "rtnCoordCMDDropCollectionSpace::doOnCataGroup" )
   INT32 rtnCoordCMDDropCollectionSpace::doOnCataGroup( CHAR *pReceiveBuffer,
                                                        pmdEDUCB *cb,
                                                        const string &strName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCODROPCS_DOONCATA ) ;

      MsgOpQuery *pDropReq             = (MsgOpQuery *)pReceiveBuffer ;
      SINT32 opCode                    = pDropReq->header.opCode ;
      UINT32 TID                       = pDropReq->header.TID ;

      pDropReq->header.opCode = MSG_CAT_DROP_SPACE_REQ ;

      rc = executeOnCataGroup( (MsgHeader*)pDropReq, cb, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop the catalog of cs, rc: %d",
                   rc ) ;

   done:
      pDropReq->header.opCode = opCode ;
      pDropReq->header.TID = TID ;
      PD_TRACE_EXITRC ( SDB_RTNCODROPCS_DOONCATA, rc ) ;
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordCMDDropCollectionSpace::complete( CHAR *pReceiveBuffer,
                                                   pmdEDUCB *cb,
                                                   const string &strName )
   {
      vector< string > subCLSet ;
      CoordCB *pCoordCB = pmdGetKRCB()->getCoordCB() ;
      pCoordCB->delCataInfoByCS( strName.c_str(), &subCLSet ) ;

      /// clear relate sub collection's catalog info
      vector< string >::iterator it = subCLSet.begin() ;
      while( it != subCLSet.end() )
      {
         pCoordCB->delCataInfo( *it ) ;
         ++it ;
      }
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDQUBASE_EXE, "rtnCoordCMDQueryBase::execute" )
   INT32 rtnCoordCMDQueryBase::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDQUBASE_EXE ) ;

      contextID                        = -1 ;
      string clName ;
      rtnQueryOptions queryOpt ;

      // parse msg
      rc = queryOpt.fromQueryMsg( (CHAR*)pMsg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract query message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = _preProcess( queryOpt, clName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "PreProcess[%s] failed, rc: %d",
                 queryOpt.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !clName.empty() )
      {
         queryOpt._fullName = clName.c_str() ;
      }
      queryOpt._flag |= FLG_QUERY_WITH_RETURNDATA ;

      // query on catalog
      rc = queryOnCatalog( queryOpt, cb, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on catalog[%s] failed, rc: %d",
                 queryOpt.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDQUBASE_EXE, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 rtnCoordCMDListCollectionSpace::_preProcess( rtnQueryOptions &queryOpt,
                                                      string &clName )
   {
      BSONObjBuilder builder ;
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      builder.appendNull( CAT_COLLECTION_SPACE_NAME ) ;
      queryOpt._selector = builder.obj() ;
      return SDB_OK ;
   }

   INT32 rtnCoordCMDListCollection::_preProcess( rtnQueryOptions &queryOpt,
                                                 string & clName )
   {
      BSONObjBuilder builder ;
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      builder.appendNull( CAT_COLLECTION_NAME ) ;
      queryOpt._selector = builder.obj() ;
      return SDB_OK ;
   }

   INT32 rtnCoordCMDListUser::_preProcess( rtnQueryOptions &queryOpt,
                                           string & clName )
   {
      BSONObjBuilder builder ;
      clName = AUTH_USR_COLLECTION ;
      if ( queryOpt._selector.isEmpty() )
      {
         builder.appendNull( FIELD_NAME_USER ) ;
      }
      queryOpt._selector = builder.obj() ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDTESTCS_EXE, "rtnCoordCMDTestCollectionSpace::execute" )
   INT32 rtnCoordCMDTestCollectionSpace::execute( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  INT64 &contextID,
                                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDTESTCS_EXE ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      contextID = -1 ;

      do
      {
         rtnCoordProcesserFactory *pProcesserFactory
               = pCoordcb->getProcesserFactory();
         rtnCoordOperator *pCmdProcesser = NULL ;
         pCmdProcesser = pProcesserFactory->getCommandProcesser(
            COORD_CMD_LISTCOLLECTIONSPACES ) ;
         SDB_ASSERT( pCmdProcesser , "pCmdProcesser can't be NULL" ) ;
         rc = pCmdProcesser->execute( pMsg, cb, contextID, buf ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to list collectionspaces(rc=%d)", rc ) ;
            break;
         }

         // get more
         rtnContextBuf buffObj ;
         rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;

         if ( rc )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_DMS_CS_NOTEXIST ;
            }
            else
            {
               PD_LOG ( PDERROR, "getmore failed(rc=%d)", rc ) ;
            }
         }
      }while ( FALSE ) ;

      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDTESTCS_EXE, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDTESTCL_EXE, "rtnCoordCMDTestCollection::execute" )
   INT32 rtnCoordCMDTestCollection::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDTESTCL_EXE ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB() ;
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB() ;
      CoordCB *pCoordcb                = pKrcb->getCoordCB() ;
      contextID                        = -1 ;

      do
      {
         rtnCoordProcesserFactory *pProcesserFactory
               = pCoordcb->getProcesserFactory() ;
         rtnCoordOperator *pCmdProcesser = NULL ;
         pCmdProcesser = pProcesserFactory->getCommandProcesser(
            COORD_CMD_LISTCOLLECTIONS ) ;
         SDB_ASSERT( pCmdProcesser , "pCmdProcesser can't be NULL" ) ;
         rc = pCmdProcesser->execute( pMsg, cb, contextID, buf ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to list collections(rc=%d)", rc ) ;
            break;
         }

         rtnContextBuf buffObj ;
         rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
         if ( rc )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_DMS_NOTEXIST;
            }
            else
            {
               PD_LOG ( PDERROR, "Getmore failed(rc=%d)", rc ) ;
            }
         }
      }while ( FALSE );

      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }

      PD_TRACE_EXITRC ( SDB_RTNCOCMDTESTCL_EXE, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTGR, "rtnCoordCMDCreateGroup::execute" )
   INT32 rtnCoordCMDCreateGroup::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTGR ) ;
      // fill default-reply(create group success)
      contextID                        = -1 ;

      MsgOpQuery *pCreateReq = (MsgOpQuery *)pMsg ;
      pCreateReq->header.opCode = MSG_CAT_CREATE_GROUP_REQ ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to execute on catalog, rc = %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTGR, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDRMR, "rtnCoordCMDRemoveGroup::execute" )
   INT32 rtnCoordCMDRemoveGroup::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDRMR ) ;
      const CHAR *groupName = NULL ;
      CHAR *pQuery = NULL;

      // fill default-reply(remove group success)
      contextID            = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
      forward->header.opCode = MSG_CAT_RM_GROUP_REQ;
      CoordGroupInfoPtr group;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL );
      try
      {
         BSONObj obj( pQuery ) ;
         BSONElement ele = obj.getField( FIELD_NAME_GROUPNAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            PD_LOG( PDERROR, "failed to get groupname from msg[%s]",
                    obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         groupName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// get group info, for cleanup.
      rc = rtnCoordGetGroupInfo( cb, groupName, TRUE, group ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get group info by name:%d", rc ) ;
         goto error ;
      }

      /// exec on catalog
      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to execute on catalog, rc = %d", rc ) ;
         goto error ;
      }

      if ( 0 == ossStrcmp( groupName, CATALOG_GROUPNAME ) )
      {
         // clean catalog info
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->clearCatNodeAddrList() ;
         sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;

         CoordGroupInfo *pEmptyGroupInfo = NULL ;
         pEmptyGroupInfo = SDB_OSS_NEW CoordGroupInfo( CAT_CATALOG_GROUPID ) ;
         if ( NULL != pEmptyGroupInfo )
         {
            CoordGroupInfoPtr groupInfo( pEmptyGroupInfo ) ;
            sdbGetCoordCB()->updateCatGroupInfo( groupInfo ) ;
         }
      }

      /// clean up
      {
      SINT32 ret = SDB_OK ;
      BSONObj execObj ;

      MsgRouteID routeID ;
      string hostName ;
      string serviceName ;
      UINT32 index = 0 ;

      while ( SDB_OK == group->getNodeInfo( index++, routeID, hostName,
                                            serviceName,
                                            MSG_ROUTE_LOCAL_SERVICE ) )
      {
         execObj = BSON( FIELD_NAME_HOST << hostName
                         << PMD_OPTION_SVCNAME << serviceName ) ;
         rc = rtnRemoteExec ( SDBSTOP, hostName.c_str() ,
                              &ret, &execObj ) ;
         rc = SDB_OK == rc ? ret : rc ;
         /// here we only return a err code. do not goto error.
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Stop node[GroupName: %s, HostName: %s, "
                    "SvcName: %s] failed, rc: %d, retObj: %s",
                    groupName, hostName.c_str(), serviceName.c_str(),
                    rc, execObj.toString().c_str() ) ;
         }

         rc = rtnRemoteExec ( SDBRM, hostName.c_str(), &ret, &execObj ) ;
         rc = SDB_OK == rc ? ret : rc ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Remove node[GroupName: %s, HostName: %s, "
                    "SvcName: %s] failed, rc: %d, retObj: %s",
                    groupName, hostName.c_str(), serviceName.c_str(),
                    rc, execObj.toString().c_str() ) ;
         }
      }

      rtnCoordRemoveGroup ( group->groupID() ) ;

      {
         CoordSession *session = cb->getCoordSession();
         if ( NULL != session )
         {
            session->removeLastNode( group->groupID()) ;
         }
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDRMR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCONFN_GETNCONF, "rtnCoordCMDConfigNode::getNodeConf" )
   INT32 rtnCoordCMDConfigNode::getNodeConf( char *pQuery,
                                             BSONObj &nodeConf,
                                             CoordGroupInfoPtr &catGroupInfo )
   {
      INT32 rc             = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCONFN_GETNCONF ) ;
      const CHAR *roleStr  = NULL ;
      SDB_ROLE role        = SDB_ROLE_DATA ;

      try
      {
         // role and catalogaddr will be added if user doesn't provide
         BSONObj boInput( pQuery ) ;
         BSONObjBuilder bobNodeConf ;
         BSONObjIterator iter( boInput ) ;
         BOOLEAN hasCatalogAddrKey = FALSE ;

         // loop through each input parameter
         while ( iter.more() )
         {
            BSONElement beField = iter.next();
            std::string strFieldName(beField.fieldName());
            // make sure to skip hostname and group name
            if ( strFieldName == FIELD_NAME_HOST ||
                 strFieldName == PMD_OPTION_ROLE )
            {
               continue;
            }
            if ( strFieldName == FIELD_NAME_GROUPNAME )
            {
               if ( 0 == ossStrcmp( CATALOG_GROUPNAME, beField.valuestr() ) )
               {
                  role = SDB_ROLE_CATALOG ;
               }
               else if ( 0 == ossStrcmp( COORD_GROUPNAME, beField.valuestr() ) )
               {
                  role = SDB_ROLE_COORD ;
               }
               continue ;
            }

            // append into beField
            bobNodeConf.append( beField );

            // for the ones we need to add default value
            if ( PMD_OPTION_CATALOG_ADDR == strFieldName )
            {
               hasCatalogAddrKey = TRUE ;
            }
         }
         // assign role if it doesn't include
         roleStr = utilDBRoleStr( role ) ;
         if ( *roleStr == 0 )
         {
            goto error ;
         }
         bobNodeConf.append ( PMD_OPTION_ROLE, roleStr ) ;

         // assign catalog address, make sure to include all catalog nodes
         // that configured in the system ( for HA ), each system should be
         // separated by "," and sit in a single key: PMD_OPTION_CATALOG_ADDR
         if ( !hasCatalogAddrKey )
         {
            MsgRouteID routeID ;
            std::string cataNodeLst = "";
            UINT32 i = 0;
            if ( catGroupInfo->nodeCount() == 0 )
            {
               rc = SDB_CLS_EMPTY_GROUP ;
               PD_LOG ( PDERROR, "Get catalog group info failed(rc=%d)", rc ) ;
               goto error ;
            }

            routeID.value = MSG_INVALID_ROUTEID ;
            string host ;
            string service ;

            while ( SDB_OK == catGroupInfo->getNodeInfo( i, routeID, host,
                                                         service,
                                                         MSG_ROUTE_CAT_SERVICE ) )
            {
               if ( i > 0 )
               {
                  cataNodeLst += "," ;
               }
               cataNodeLst += host + ":" + service ;
               ++i ;
            }
            bobNodeConf.append( PMD_OPTION_CATALOG_ADDR, cataNodeLst ) ;
         }
         nodeConf = bobNodeConf.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() );
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCONFN_GETNCONF, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCONFN_GETNINFO, "rtnCoordCMDConfigNode::getNodeInfo" )
   INT32 rtnCoordCMDConfigNode::getNodeInfo( char *pQuery,
                                             BSONObj &NodeInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCONFN_GETNINFO ) ;
      do
      {
         try
         {
            BSONObj boConfig( pQuery );
            BSONObjBuilder bobNodeInfo;
            BSONElement beGroupName = boConfig.getField( FIELD_NAME_GROUPNAME );
            if ( beGroupName.eoo() || beGroupName.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to get the field(%s)",
                        FIELD_NAME_GROUPNAME );
               break;
            }
            bobNodeInfo.append( beGroupName );

            BSONElement beHostName = boConfig.getField( FIELD_NAME_HOST );
            if ( beHostName.eoo() || beGroupName.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to get the field(%s)",
                        FIELD_NAME_HOST );
               break;
            }
            bobNodeInfo.append( beHostName );

            BSONElement beLocalSvc = boConfig.getField( PMD_OPTION_SVCNAME );
            if ( beLocalSvc.eoo() || beLocalSvc.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to get the field(%s)",
                        PMD_OPTION_SVCNAME );
               break;
            }
            bobNodeInfo.append( beLocalSvc );

            BSONElement beReplSvc = boConfig.getField( PMD_OPTION_REPLNAME );
            if ( !beReplSvc.eoo() && beReplSvc.type()==String )
            {
               bobNodeInfo.append( beReplSvc );
            }

            BSONElement beShardSvc = boConfig.getField( PMD_OPTION_SHARDNAME );
            if ( !beShardSvc.eoo() && beShardSvc.type()==String )
            {
               bobNodeInfo.append( beShardSvc );
            }
            BSONElement beCataSvc = boConfig.getField( PMD_OPTION_CATANAME );
            if ( !beCataSvc.eoo() && beCataSvc.type()==String )
            {
               bobNodeInfo.append( beCataSvc );
            }

            BSONElement beDBPath = boConfig.getField( PMD_OPTION_DBPATH );
            if ( beDBPath.eoo() || beDBPath.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to get the field(%s)",
                        PMD_OPTION_DBPATH );
               break;
            }
            bobNodeInfo.append( beDBPath );
            NodeInfo = bobNodeInfo.obj();
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() ) ;
            break;
         }
      }while ( FALSE );
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCONFN_GETNINFO, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTN_EXEC, "rtnCoordCMDCreateNode::execute" )
   INT32 rtnCoordCMDCreateNode::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCMDCTN_EXEC ) ;
      INT32 flag = 0 ;
      CHAR *pCMDName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      CHAR *pQuery = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;
      BOOLEAN onlyAttach = FALSE ;
      BSONObj query ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR,
                  "failed to parse create node request(rc=%d)",
                  rc );
         goto error ;
      }

      try
      {
         query = BSONObj( pQuery ) ;
         BSONElement e = query.getField( FIELD_NAME_ONLY_ATTACH ) ;
         if ( e.eoo() )
         {
            onlyAttach = FALSE ;
         }
         else if ( Bool == e.type() )
         {
            onlyAttach = e.Bool() ;
         }
         else
         {
            PD_LOG( PDERROR, "invalid type of \"OnlyAttach\" in msg:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !onlyAttach )
      {
         rc = _createNode( pMsg, cb, contextID, buf ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create node:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _attachNode( query, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to attach node:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOCMDCTN_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTN__ATTACHNODE, "rtnCoordCMDCreateNode::_attachNode" )
   INT32 rtnCoordCMDCreateNode::_attachNode( const BSONObj &info,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCMDCTN__ATTACHNODE ) ;
      BSONElement hostEle ;
      BSONElement gpEle ;
      BSONElement ele ;
      std::vector<BSONObj> objs ;
      BSONObj nodeConf ;
      SINT32 retCode = SDB_OK ;
      CHAR *buf = NULL ;
      MsgOpQuery *header = NULL ;
      BOOLEAN onCata = FALSE ;
      BOOLEAN keepData = FALSE ;

      try
      {
         hostEle = info.getField( FIELD_NAME_HOST ) ;
         if ( hostEle.eoo() || String != hostEle.type() )
         {
            PD_LOG( PDERROR, "invalid type of \"HostName\" in msg:%s",
                    info.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         gpEle = info.getField( FIELD_NAME_GROUPNAME ) ;
         if ( String != gpEle.type() )
         {
            PD_LOG( PDERROR, "invalid type of \"GroupName\" in msg:%s",
                    info.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         
         ele = info.getField( FIELD_NAME_KEEP_DATA ) ;
         if ( ele.eoo() || Bool != ele.type() )
         {
            PD_LOG( PDERROR, "Failed to get KeepData from msg[%s]",
                 info.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         keepData = ele.boolean() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = rtnRemoteExec( SDBGETCONF, hostEle.valuestr(),
                          &retCode, &info,
                          NULL, NULL, NULL, &objs ) ;
      rc = SDB_OK == rc ? retCode : rc ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get conf of node, "
                 "rc:%d", rc ) ;
         goto error ;
      }
      else if ( 1 != objs.size() )
      {
         PD_LOG( PDERROR, "invalid objs's size:%d", objs.size() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      nodeConf = objs.at( 0 ) ;

      /// node's role must be suit for group
      /// TODO: we can modify node's role here.
      if ( 0 == ossStrcmp( gpEle.valuestr(),
                           CATALOG_GROUPNAME ) ||
           0 == ossStrcmp( gpEle.valuestr(),
                           COORD_GROUPNAME ) )
      {
         PD_LOG( PDERROR, "only data-group surpports \"attachNode\" now" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _buildAttachMsg( nodeConf, gpEle.valuestr(),
                            hostEle.valuestr(), buf, header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build msg:%d", rc ) ;
         goto error ;
      }

      rc = executeOnCataGroup( &( header->header ), cb, TRUE ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "failed to execute on catalog:%d", rc ) ;
         goto error ;
      }
      onCata = TRUE ;

      if ( !keepData )
      {
         rc = rtnRemoteExec( SDBCLEARDATA, hostEle.valuestr(),
                             &retCode, &info ) ;
         rc = SDB_OK == rc ?
              retCode : rc ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to clear data on node, "
                    "rc:%d", rc ) ;
            goto error ;
         }
      }

      rc = rtnRemoteExec ( SDBSTART, hostEle.valuestr(),
                           &retCode, &info ) ;
      rc = SDB_OK == rc ?
           retCode : rc ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get conf of node, "
                 "rc:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOCMDCTN__ATTACHNODE, rc ) ;
      SAFE_OSS_FREE( buf ) ;
      return rc ;
   error:
      if ( onCata )
      {
         PD_LOG( PDEVENT, "begin to rollback info on catalog" ) ;
         header->header.opCode = MSG_CAT_DEL_NODE_REQ ;
         INT32 rrc = executeOnCataGroup( &( header->header ), cb, TRUE ) ;
         if ( SDB_OK != rrc )
         {
            PD_LOG( PDERROR, "failed to rollback on catalog:%d", rc ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "rollback done on catalog" ) ;
         }
      }
      goto done ;
   }

   INT32 rtnCoordCMDCreateNode::_buildAttachMsg( const BSONObj &conf,
                                                 const CHAR *gpName,
                                                 const CHAR *host,
                                                 CHAR *&buf,
                                                 MsgOpQuery *&header )
   {
      INT32 rc = SDB_OK ;
      INT32 bufSize = 0 ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      builder.append( FIELD_NAME_GROUPNAME, gpName ) ;
      builder.append( FIELD_NAME_HOST, host ) ;
      builder.appendElements( conf ) ;
      obj = builder.obj() ;
      rc = msgBuildQueryMsg( &buf, &bufSize,
                             CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE,
                             0, 0, 0, -1,
                             &obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build msg:%d", rc ) ;
         goto error ;
      }

      header = ( MsgOpQuery * )buf ;
      header->header.opCode = MSG_CAT_CREATE_NODE_REQ ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTN__CRTNODE, "rtnCoordCMDCreateNode::_createNode" )
   INT32 rtnCoordCMDCreateNode::_createNode( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTN__CRTNODE ) ;

      // fill default-reply(create group success)
      contextID                        = -1 ;

      do
      {
         INT32 flag = 0 ;
         CHAR *pCMDName = NULL ;
         SINT64 numToSkip = 0 ;
         SINT64 numToReturn = 0 ;
         CHAR *pQuery = NULL ;
         CHAR *pFieldSelector = NULL ;
         CHAR *pOrderBy = NULL ;
         CHAR *pHint = NULL ;
         rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                               &numToReturn, &pQuery, &pFieldSelector,
                               &pOrderBy, &pHint );
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR,
                     "failed to parse create node request(rc=%d)",
                     rc );
            break;
         }
         BSONObj boNodeInfo;
         rc = getNodeInfo( pQuery, boNodeInfo );
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Create node failed, failed to get "
                     "node info(rc=%d)", rc ) ;
            break;
         }
         CoordGroupInfoPtr catGroupInfo ;
         rc = rtnCoordGetCatGroupInfo( cb, TRUE, catGroupInfo );
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Create node failed, failed to get "
                     "cata-group-info(rc=%d)", rc ) ;
            break ;
         }
         CHAR *pBuffer = NULL;
         INT32 bufferSize = 0;
         BSONObj fieldSelector;
         BSONObj orderBy;
         BSONObj hint;
         BSONObjBuilder builder ;
         BSONObj newNodeInfo ;
         /// we enforced remove node in rollback.
         try
         {
            BSONObjIterator itrObj( boNodeInfo ) ;
            while ( itrObj.more() )
            {
               BSONElement nextEle = itrObj.next() ;
               if ( 0 == ossStrcmp( nextEle.fieldName(), FIELD_NAME_ENFORCED )
                 || 0 == ossStrcmp( nextEle.fieldName(), FIELD_NAME_ENFORCED1 ) )
               {
                  continue ;
               }
               builder.append( nextEle ) ;
            }

            builder.appendBool( FIELD_NAME_ENFORCED1, TRUE ) ;
            newNodeInfo = builder.obj() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexcepted err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            break ;
         }

         MsgOpQuery *pCatReq = (MsgOpQuery *)pMsg;
         pCatReq->header.opCode = MSG_CAT_CREATE_NODE_REQ;
         rc = executeOnCataGroup( pMsg, cb, TRUE ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to create node, execute on catalog-node "
                     "failed(rc=%d)", rc ) ;
            break;
         }
         string strHostName ;
         BSONObj boNodeConfig ;
         try
         {
            BSONElement beHostName = boNodeInfo.getField( FIELD_NAME_HOST );
            if ( beHostName.eoo() || beHostName.type() != String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to get the field(%s)",
                        FIELD_NAME_HOST ) ;
               break;
            }
            strHostName = beHostName.str() ;
            rc = getNodeConf( pQuery, boNodeConfig, catGroupInfo );
            if ( rc != SDB_OK )
            {
               PD_LOG( PDERROR, "Failed to get node config(rc=%d)",
                       rc ) ;
               break ;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() ) ;
         }
         SINT32 retCode;
         rc = rtnRemoteExec ( SDBADD, strHostName.c_str(),
                              &retCode, &boNodeConfig ) ;
         rc = rc ? rc : retCode;
         if ( SDB_OK == rc )
         {
            if ( 0 == ossStrcmp( newNodeInfo.getField(
                                 FIELD_NAME_GROUPNAME ).valuestr(),
                                 CATALOG_GROUPNAME ) )
            {
               // update catalog group
               rtnCoordGetCatGroupInfo( cb, TRUE, catGroupInfo ) ;
               // notify all nodes
               rtnCataChangeNtyToAllNodes( cb ) ;
            }
            break ;
         }

         PD_LOG( PDERROR, "Remote node execute(configure) failed(rc=%d)",
                 rc ) ;
         // Rollback: delete the node-info on catalog-node
         INT32 rcDel = SDB_OK ;
         rcDel = msgBuildQueryMsg( &pBuffer, &bufferSize, COORD_CMD_REMOVENODE,
                                   flag, 0, numToSkip, numToReturn,
                                   &newNodeInfo, &fieldSelector, &orderBy,
                                   &hint ) ;
         if ( rcDel != SDB_OK )
         {
            PD_LOG ( PDERROR, "failed to build the request for "
                     "catalog-node(rc=%d)", rcDel ) ;
            break ;
         }
         pCatReq = (MsgOpQuery *)pBuffer;
         pCatReq->header.opCode = MSG_CAT_DEL_NODE_REQ;
         rcDel = executeOnCataGroup( (MsgHeader*)pBuffer, cb, TRUE ) ;
         SDB_OSS_FREE(pBuffer);
         if ( rcDel!= SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to delete node, execute on catalog-node "
                     "failed(rc=%d)", rcDel ) ;
            break ;
         }
         break ;
      }while ( FALSE ) ;

      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTN__CRTNODE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDRMN_EXE, "rtnCoordCMDRemoveNode::execute" )
   INT32 rtnCoordCMDRemoveNode::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY ( SDB_RTNCOCMDRMN_EXE ) ;
      INT32 rc = SDB_OK ;
      netMultiRouteAgent *pAgent = pmdGetKRCB()->getCoordCB()->getRouteAgent() ;
      contextID                  = -1 ;

      MsgOpQuery *forward = NULL ;
      string groupName ;
      string host ;
      string srv ;
      CoordGroupInfoPtr groupInfo ;

      CHAR *pQuery = NULL ;
      BSONObj rInfo ;

      BOOLEAN onlyDetach = FALSE ;
      BOOLEAN keepData = FALSE ;

      forward = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_DEL_NODE_REQ ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to parse msg:%s",rc ) ;
         goto error ;
      }

      try
      {
         rInfo = BSONObj( pQuery ) ;
         BSONElement ele = rInfo.getField( FIELD_NAME_GROUPNAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            PD_LOG( PDERROR, "failed to get groupname from msg[%s]",
                    rInfo.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         groupName = ele.String() ;

         ele = rInfo.getField( FIELD_NAME_HOST ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            PD_LOG( PDERROR, "failed to get host from msg[%s]",
                    rInfo.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         host = ele.String() ;

         ele = rInfo.getField( PMD_OPTION_SVCNAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            PD_LOG( PDERROR, "Failed to get srv from msg[%s]",
                    rInfo.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         srv = ele.String() ;

         ele = rInfo.getField( FIELD_NAME_ONLY_DETACH ) ;
         if ( ele.eoo() )
         {
            onlyDetach = FALSE ;
         }
         else if ( Bool == ele.type() )
         {
            onlyDetach = ele.Bool() ;
         }
         else
         {
            PD_LOG( PDERROR, "unexpected type(%d) of \"OnlyDetach\""
                    "it should be bool in %s", ele.type(),
                    rInfo.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( onlyDetach )
         {
            if ( 0 == ossStrcmp( groupName.c_str(), CATALOG_GROUPNAME ) ||
                 0 == ossStrcmp( groupName.c_str(), COORD_GROUPNAME ) )
            {
               PD_LOG( PDERROR, "only data-group surpports \"detachNode\" now" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            ele = rInfo.getField( FIELD_NAME_KEEP_DATA ) ;
            if ( ele.eoo() || Bool != ele.type() )
            {
               PD_LOG( PDERROR, "Failed to get KeepData from msg[%s]",
                    rInfo.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            keepData = ele.boolean() ;  
         }
         
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happended:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// remove data node on catalog
      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to remove node[GroupName:%s, "
                  "HostName: %s, SvcName: %s] in catalog, rc: %d",
                  groupName.c_str(), host.c_str(), srv.c_str(), rc ) ;
         goto error ;
      }

      /// get group info from catalog
      rc = rtnCoordGetGroupInfo( cb, groupName.c_str(), TRUE, groupInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get groupinfo[%s] from catalog, rc: %d",
                 groupName.c_str(), rc ) ;
         goto error ;
      }

      /// notify the other nodes to update groupinfo.
      /// here we do not care whether they succeed.
      {
         _MsgClsGInfoUpdated updated ;
         updated.groupID = groupInfo->groupID() ;
         MsgRouteID routeID ;
         UINT32 index = 0 ;

         while ( SDB_OK == groupInfo->getNodeID( index++, routeID,
                                                 MSG_ROUTE_SHARD_SERVCIE ) )
         {
            rtnCoordSendRequestToNodeWithoutReply( (void *)(&updated),
                                                    routeID, pAgent );
         }
      }

      // notify cm to stop and remove node
      {
         SINT32 retCode;

         /// ignore the stop result, because remove or clear will
         /// retry to stop in sdbcm
         rc = rtnRemoteExec ( SDBSTOP, host.c_str(), &retCode, &rInfo ) ;
         rc = ( SDB_OK == rc ) ? retCode : rc ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Stop the node[GroupName: %s, HostName: %s "
                    "SvcName: %s] failed, rc: %d, retObj: %s",
                    groupName.c_str(), host.c_str(), srv.c_str(), rc,
                    rInfo.toString().c_str() ) ;
            /// ignored this error
         }

         if ( !onlyDetach )
         {
            rc = rtnRemoteExec ( SDBRM, host.c_str(), &retCode, &rInfo ) ;
            rc = SDB_OK == rc ? retCode : rc ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Remove the node[GroupName: %s, HostName: %s, "
                       "SvcName: %s] failed, rc: %d, retObj: %s",
                       groupName.c_str(), host.c_str(), srv.c_str(), rc,
                       rInfo.toString().c_str() ) ;
             }
         }
         else if ( !keepData )
         {
            rc = rtnRemoteExec( SDBCLEARDATA, host.c_str(), &retCode, &rInfo ) ;
            rc = SDB_OK == rc ? retCode : rc ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Clear data on node[GroupName: %s, HostName: "
                       "%s, SvcName: %s] failed, rc: %d, retObj: %s",
                       groupName.c_str(), host.c_str(), srv.c_str(),
                       rc, rInfo.toString().c_str() ) ;
            }
         }
      }

      /// do not care rc.
      if ( CATALOG_GROUPID == groupInfo->groupID() )
      {
         rtnCataChangeNtyToAllNodes( cb ) ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDRMN_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDUPN_EXE, "rtnCoordCMDUpdateNode::execute" )
   INT32 rtnCoordCMDUpdateNode::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDUPN_EXE ) ;

      // fill default-reply(create group success)
      contextID = -1 ;

      // TODO:
      // 1. first modify by the host's cm
      // 2. then send to catalog to update dbpath or other info

      rc = SDB_COORD_UNKNOWN_OP_REQ ;

      PD_TRACE_EXITRC ( SDB_RTNCOCMDUPN_EXE, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDATGR_STNS, "rtnCoordCMDActiveGroup::startNodes" )
   INT32 rtnCoordCMDActiveGroup::startNodes( bson::BSONObj &boGroupInfo,
                                             vector<BSONObj> &objList )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDATGR_STNS ) ;
      do
      {
         try
         {
            BSONElement beGroup = boGroupInfo.getField( FIELD_NAME_GROUP );
            if ( beGroup.eoo() || beGroup.type()!=Array )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR,
                        "failed to get the field(%s)",
                        FIELD_NAME_GROUP );
               break;
            }
            BSONObjIterator i( beGroup.embeddedObject() );
            while ( i.more() )
            {
               BSONElement beTmp = i.next();
               BSONObj boTmp = beTmp.embeddedObject();
               BSONElement beHostName = boTmp.getField( FIELD_NAME_HOST );
               if ( beHostName.eoo() || beHostName.type()!=String )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDERROR, "Failed to get the HostName" );
                  break;
               }
               string strHostName = beHostName.str();
               BSONElement beService = boTmp.getField( FIELD_NAME_SERVICE );
               if ( beService.eoo() || beService.type()!=Array )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDWARNING, "Failed to get the field(%s)",
                           FIELD_NAME_SERVICE );
                  break;
               }
               string strServiceName;
               rc = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE,
                                    strServiceName );
               if ( rc != SDB_OK )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDWARNING, "Failed to get local-service-name" ) ;
                  break ;
               }
               SINT32 retCode ;
               BSONObjBuilder bobLocalService ;
               bobLocalService.append( PMD_OPTION_SVCNAME, strServiceName );
               BSONObj boLocalService = bobLocalService.obj();
               rc = rtnRemoteExec ( SDBSTART, strHostName.c_str(),
                                    &retCode, &boLocalService ) ;
               if ( SDB_OK == rc && SDB_OK == retCode )
               {
                  continue ;
               }
               if ( rc != SDB_OK )
               {
                  PD_LOG( PDERROR, "start the node failed (HostName=%s, "
                          "LocalService=%s, rc=%d)", strHostName.c_str(),
                          strServiceName.c_str(), rc ) ;
               }
               else if ( retCode != SDB_OK )
               {
                  rc = retCode;
                  PD_LOG( PDERROR, "remote node execute(start) failed "
                          "(HostName=%s, LocalService=%s, rc=%d)",
                          strHostName.c_str(), strServiceName.c_str(), rc ) ;
               }
               BSONObjBuilder bobReply;
               bobReply.append( FIELD_NAME_HOST, strHostName );
               bobReply.append( PMD_OPTION_SVCNAME, strServiceName );
               bobReply.append( FIELD_NAME_ERROR_NO, retCode );
               objList.push_back( bobReply.obj() );
            }
            break ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "occured unexpected error:%s", e.what() ) ;
            break;
         }
      }while ( FALSE );

      if ( objList.size() != 0 )
      {
         rc = SDB_CM_RUN_NODE_FAILED ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDATGR_STNS, rc ) ;
      return rc ;
   }

   INT32 rtnCoordCMDActiveGroup::startNodes( clsGroupItem *pItem,
                                             vector<BSONObj> &objList )
   {
      INT32 rc = SDB_OK ;
      MsgRouteID id ;
      string hostName ;
      string svcName ;
      UINT32 pos = 0 ;
      SINT32 retCode = SDB_OK ;

      while ( SDB_OK == pItem->getNodeInfo( pos, id, hostName, svcName,
                                            MSG_ROUTE_LOCAL_SERVICE ) )
      {
         ++pos ;

         retCode = SDB_OK ;
         BSONObjBuilder bobLocalService ;
         bobLocalService.append( PMD_OPTION_SVCNAME, svcName ) ;
         BSONObj boLocalService = bobLocalService.obj() ;

         rc = rtnRemoteExec ( SDBSTART, hostName.c_str(),
                              &retCode, &boLocalService ) ;
         if ( SDB_OK == rc && SDB_OK == retCode )
         {
            continue ;
         }
         if ( rc != SDB_OK )
         {
            PD_LOG( PDERROR, "start the node failed (HostName=%s, "
                    "LocalService=%s, rc=%d)", hostName.c_str(),
                    svcName.c_str(), rc ) ;
         }
         else if ( retCode != SDB_OK )
         {
            rc = retCode ;
            PD_LOG( PDERROR, "remote node execute(start) failed "
                    "(HostName=%s, LocalService=%s, rc=%d)",
                    hostName.c_str(), svcName.c_str(), rc ) ;
         }
         BSONObjBuilder bobReply ;
         bobReply.append( FIELD_NAME_HOST, hostName ) ;
         bobReply.append( PMD_OPTION_SVCNAME, svcName ) ;
         bobReply.append( FIELD_NAME_ERROR_NO, retCode ) ;
         objList.push_back( bobReply.obj() ) ;
      }

      if ( objList.size() != 0 )
      {
         rc = SDB_CM_RUN_NODE_FAILED ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDATGR_EXE, "rtnCoordCMDActiveGroup::execute" )
   INT32 rtnCoordCMDActiveGroup::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDATGR_EXE ) ;

      // fill default-reply(active group success)
      contextID                        = -1 ;

      MsgOpQuery *pReq = (MsgOpQuery *)pMsg ;
      pReq->header.opCode = MSG_CAT_ACTIVE_GROUP_REQ ;

      const CHAR *pGroupName = NULL ;
      vector<BSONObj> objGrpLst ;
      vector<BSONObj> objList ;
      CoordGroupInfoPtr catGroupInfo ;

      do
      {
         CHAR *pQuery = NULL ;
         rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                               &pQuery, NULL, NULL, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to extract msg, rc: %d", rc ) ;
            break ;
         }
         try
         {
            BSONObj boQuery( pQuery ) ;
            BSONElement ele = boQuery.getField( CAT_GROUPNAME_NAME ) ;
            if ( ele.type() != String )
            {
               PD_LOG( PDERROR, "Get field[%s] type[%d] is not String",
                       CAT_GROUPNAME_NAME, ele.type() ) ;
               rc = SDB_INVALIDARG ;
               break ;
            }
            pGroupName = ele.valuestr() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_INVALIDARG ;
            break ;
         }

         rc = executeOnCataGroup( pMsg, cb, NULL, &objGrpLst ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to active group[%s], execute on "
                     "catalog-node failed(rc=%d)", pGroupName, rc ) ;

            if ( 0 != ossStrcmp( CATALOG_GROUPNAME, pGroupName ) ||
                 SDB_OK != rtnCoordGetLocalCatGroupInfo( catGroupInfo ) ||
                 NULL == catGroupInfo.get() ||
                 catGroupInfo->nodeCount() == 0 )
            {
               break ;
            }
         }

         if ( catGroupInfo.get() &&
              catGroupInfo->nodeCount() > 0 )
         {
            rc = startNodes( catGroupInfo.get(), objList ) ;
         }
         else if ( objGrpLst.size() > 0 )
         {
            rc = startNodes( objGrpLst[0], objList ) ;
         }
         else
         {
            rc = SDB_SYS ;
         }

         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Start node failed(rc=%d)", rc ) ;
            UINT32 i = 0;
            string strNodeList;
            for ( ; i < objList.size(); i++ )
            {
               strNodeList += objList[i].toString( false, false );
            }
            PD_LOG_MSG( PDERROR, "Strart failed nodes: %s",
                        strNodeList.c_str() ) ;
            break ;
         }
      }while ( FALSE ) ;

      PD_TRACE_EXITRC ( SDB_RTNCOCMDATGR_EXE, rc ) ;
      return rc;
   }

   INT32 rtnCoordCMDCreateIndex::checkIndexKey( const CoordCataInfoPtr &cataInfo,
                                                const BSONObj &indexObj,
                                                set< UINT32 > &haveSet,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj shardingKey ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;

      if ( skSiteID > 0 )
      {
         if ( haveSet.count( skSiteID ) > 0 )
         {
            /// already checked
            goto done ;
         }
         haveSet.insert( skSiteID ) ;
      }

      /// check the sharding key
      {
         cataInfo->getShardingKey ( shardingKey ) ;
         BSONObjIterator shardingItr ( shardingKey ) ;
         while ( shardingItr.more() )
         {
            BSONElement sk = shardingItr.next() ;
            if ( indexObj.getField( sk.fieldName() ).eoo() )
            {
               PD_LOG( PDWARNING, "All fields in sharding key must "
                       "be included in unique index, missing field: %s,"
                       "shardingKey: %s, indexKey: %s, collection: %s",
                       sk.fieldName(), shardingKey.toString().c_str(),
                       indexObj.toString().c_str(), cataInfo->getName() ) ;
               rc = SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY ;
               goto error ;
            }
         }
      }

      if ( cataInfo->isMainCL() )
      {
         CoordSubCLlist subCLList ;
         cataInfo->getSubCLList( subCLList ) ;

         CoordSubCLlist::iterator it = subCLList.begin() ;
         while( it != subCLList.end() )
         {
            CoordCataInfoPtr subCataInfo ;
            rc = rtnCoordGetCataInfo( cb, (*it).c_str(), FALSE,
                                      subCataInfo, NULL ) ;
            PD_RC_CHECK( rc, PDERROR, "Get sub collection[%s] catalog info "
                         "failed, rc: %d", (*it).c_str(), rc ) ;

            rc = checkIndexKey( subCataInfo, indexObj, haveSet, cb ) ;
            if ( rc )
            {
               goto error ;
            }
            ++it ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTIND_EXE, "rtnCoordCMDCreateIndex::execute" )
   INT32 rtnCoordCMDCreateIndex::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTIND_EXE ) ;
      INT32 tempRC                     = SDB_OK ;

      // fill default-reply(active group success)
      contextID                        = -1 ;

      CHAR *pQuery                     = NULL ;
      BOOLEAN emptyUpdateCata          = FALSE ;

      CoordGroupList sucGrpLst ;

      const CHAR *strCollectionName    = NULL ;
      const CHAR *strIndexName         = NULL ;
      CoordCataInfoPtr cataInfo;

      // rollback related
      CHAR *pDropMsg                   = NULL;
      INT32 bufferSize                 = 0;
      SET_RC ignoreRC ;

      // extract message
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL );
      PD_RC_CHECK ( rc, PDERROR,
                    "failed to parse create index request(rc=%d)",
                    rc ) ;
      try
      {
         BSONObj boQuery(pQuery);
         // get collection name
         BSONElement beCollectionName
                     = boQuery.getField( FIELD_NAME_COLLECTION );
         // get index name
         BSONElement beIndex = boQuery.getField( FIELD_NAME_INDEX );
         BSONElement beIndexName ;
         // make sure collection name exists
         PD_CHECK ( beCollectionName.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "create index failed, failed to get the field(%s)",
                    FIELD_NAME_COLLECTION ) ;
         strCollectionName = beCollectionName.valuestr() ;

         // make sure index object exist
         PD_CHECK ( beIndex.type() == Object,
                    SDB_INVALIDARG, error, PDERROR,
                    "create index failed, failed to get the field(%s)",
                    FIELD_NAME_INDEX ) ;
         // get embedded index name
         beIndexName = beIndex.embeddedObject().getField( IXM_FIELD_NAME_NAME );
         PD_CHECK ( beIndexName.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "create index failed, failed to get the field(%s)",
                    IXM_FIELD_NAME_NAME ) ;
         strIndexName = beIndexName.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( rc, PDERROR,
                       "create index failed, occured unexpected error:%s",
                       e.what() );
      }

      rc = rtnCoordGetCataInfo( cb, strCollectionName, FALSE, cataInfo,
                                NULL ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to create index(%s), "
                    "get catalogue failed(rc=%d)",
                    strCollectionName, rc ) ;

   retry :
      try
      {
         BSONObj arg ( pQuery ) ;
         BSONObj indexObj ;
         BSONObj indexKey ;
         BSONElement indexUnique ;
         BOOLEAN isUnique = TRUE ;

         rc = rtnGetObjElement ( arg, FIELD_NAME_INDEX, indexObj ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to get object index, rc = %d", rc ) ;

         rc = rtnGetObjElement ( indexObj, IXM_KEY_FIELD, indexKey ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to get key for index: %s, rc = %d",
                       indexObj.toString().c_str(), rc ) ;

         // index key obj shouldn't has more than 32 field
         try
         {
            bson::Ordering::make ( indexKey ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "%s", e.what() ) ;
            PD_LOG( PDERROR, "Create index, index obj: %s, rc: %d.",
                    indexKey.toString().c_str(), rc ) ;
            goto error ;
         }

         // if the collection is sharded, we have to extract the index key and
         // make sure sharding key is included if it's unique index
         if ( cataInfo->isSharded() )
         {
            indexUnique = indexObj.getField ( IXM_UNIQUE_FIELD ) ;
            if ( indexUnique.type() != Bool )
            {
               isUnique = FALSE ;
            }
            else
            {
               isUnique = indexUnique.boolean () ;
            }
            // now the index def is in indexObj, so we need to compare
            // sharding key and indexKey if it's unique
            if ( isUnique )
            {
               set< UINT32 > haveSet ;
               rc = checkIndexKey( cataInfo, indexKey, haveSet, cb ) ;
               if ( SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY == rc &&
                    !emptyUpdateCata )
               {
                  rc = rtnCoordGetCataInfo( cb, strCollectionName, TRUE,
                                            cataInfo ) ;
                  PD_RC_CHECK( rc, PDERROR, "Update failed, failed to get the "
                               "catalogue info(collection name: %s), rc: %d",
                               strCollectionName, rc ) ;
                  emptyUpdateCata = TRUE ;
                  goto retry ;
               }
               PD_RC_CHECK( rc, PDERROR, "Create index[%s] of "
                            "collection[%s] failed, rc: %d",
                            indexObj.toString().c_str(),
                            strCollectionName, rc ) ;
            }
         } // if ( beCollectionName.type()!=String )
      } // try
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception during extracting unique key: %s",
                       e.what() ) ;
      }


      rc = executeOnCL( pMsg, cb, strCollectionName, FALSE, NULL,
                        NULL, &sucGrpLst ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index of collection[%s] on data group, "
                 "rc: %d", strCollectionName, rc ) ;
         goto rollback ;
      }

   done :
      if ( strCollectionName )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_CREATE_INDEX, AUDIT_OBJ_CL,
                           strCollectionName, rc, "Option:%s",
                           BSONObj(pQuery).toString().c_str() ) ;
      }
      if ( pDropMsg )
      {
         SDB_OSS_FREE ( pDropMsg ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTIND_EXE, rc ) ;
      return rc;
   rollback :
      // if create index happened on some nodes, we have to rollback the
      // operation
      // first let's build a package
      tempRC = msgBuildDropIndexMsg( &pDropMsg, &bufferSize,
                                     strCollectionName,
                                     strIndexName, 0 );
      PD_RC_CHECK ( tempRC, PDERROR,
                    "Failed to build drop index message, rc = %d",
                    tempRC ) ;

      // let's get most current version again
      tempRC = rtnCoordGetCataInfo( cb, strCollectionName,
                                    FALSE, cataInfo ) ;
      PD_RC_CHECK ( tempRC, PDERROR,
                    "Failed to rollback create index(%s), "
                    "get catalogue failed(rc=%d)",
                    strCollectionName, tempRC ) ;

      // don't rollback for main-collection
      if ( cataInfo->isMainCL() )
      {
         PD_LOG( PDWARNING, "Main-collection create index failed "
                 "and will not rollback" ) ;
         goto error ;
      }

      ignoreRC.insert( SDB_IXM_NOTEXIST ) ;
      tempRC = executeOnCL( (MsgHeader *)pDropMsg, cb, strCollectionName,
                            FALSE, &sucGrpLst, &ignoreRC, NULL ) ;
      if ( tempRC != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to rollback create index, rc = %d",
                  tempRC ) ;
         goto error ;
      }
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDDPIN_EXE, "rtnCoordCMDDropIndex::execute" )
   INT32 rtnCoordCMDDropIndex::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDDPIN_EXE ) ;

      // fill default-reply(active group success)
      contextID                        = -1 ;
      string realCLName ;
      CHAR *pQuery = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL );
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to parse drop index request(rc=%d)",
                  rc ) ;
         goto error ;
      }

      try
      {
         BSONObj boQuery( pQuery ) ;
         BSONElement beCollectionName =
            boQuery.getField( FIELD_NAME_COLLECTION );
         if ( beCollectionName.eoo() || beCollectionName.type()!=String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Drop index failed, failed to get the "
                    "field(%s)", FIELD_NAME_COLLECTION );
            goto error ;
         }
         realCLName = beCollectionName.str() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Drop index failed, occured unexpected "
                  "error:%s", e.what() ) ;
         goto error ;
      }

      rc = executeOnCL( pMsg, cb, realCLName.c_str(), FALSE, NULL,
                        NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to drop index in collection(%s), "
                  "drop on data-node failed(rc=%d)",
                  realCLName.c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( !realCLName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_DROP_INDEX, AUDIT_OBJ_CL,
                           realCLName.c_str(), rc, "Option:%s",
                           BSONObj(pQuery).toString().c_str() ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDDPIN_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDOPONNODE_EXE, "rtnCoordCMDOperateOnNode::execute" )
   INT32 rtnCoordCMDOperateOnNode::execute( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDOPONNODE_EXE ) ;

      rtnQueryOptions queryOption ;
      const CHAR *strHostName = NULL ;
      const CHAR *svcname = NULL ;

      BSONObj boNodeConf;
      BSONObjBuilder bobNodeConf ;

      SINT32 opType = getOpType() ;
      SINT32 retCode = SDB_OK ;

      // fill default-reply(active group success)
      contextID                        = -1 ;

      rc = queryOption.fromQueryMsg( (CHAR *)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract msg failed, rc: %d", rc ) ;

      try
      {
         BSONObj objQuery = queryOption._query ;
         BSONElement ele = objQuery.getField( FIELD_NAME_HOST );
         if ( ele.eoo() || ele.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Field[%s] is invalid[%s]",
                     FIELD_NAME_HOST, objQuery.toString().c_str() ) ;
            goto error ;
         }
         strHostName = ele.valuestrsafe () ;

         ele = objQuery.getField( PMD_OPTION_SVCNAME ) ;
         if ( ele.eoo() || ele.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Field[%s] is invalid[%s]",
                     PMD_OPTION_SVCNAME, objQuery.toString().c_str() ) ;
            goto error ;
         }
         svcname = ele.valuestrsafe() ;

         bobNodeConf.append( ele ) ;
         boNodeConf = bobNodeConf.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "occured unexpected error:%s",
                  e.what() );
         goto error ;
      }

      /// execute on node
      rc = rtnRemoteExec ( opType, strHostName,
                           &retCode, &boNodeConf ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "Excute operate[%d] on node[%s:%s] failed, rc: %d",
                 opType, strHostName, svcname, rc ) ;
         goto error ;
      }
      if ( retCode != SDB_OK )
      {
         rc = retCode ;
         PD_LOG( PDERROR, "Excute operate[%d] on node[%s:%s] failed, rc: %d",
                 opType, strHostName, svcname, rc ) ;
         goto error ;
      }

   done:
      if ( strHostName && svcname )
      {
         PD_AUDIT_COMMAND( AUDIT_SYSTEM, queryOption._fullName + 1,
                           AUDIT_OBJ_NODE, "", rc,
                           "HostName:%s, ServiceName:%s", strHostName,
                           svcname ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDOPONNODE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   SINT32 rtnCoordCMDStartupNode::getOpType()
   {
      return SDBSTART;
   }

   SINT32 rtnCoordCMDShutdownNode::getOpType()
   {
      return SDBSTOP;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB_RTNCOCMDOPONGR_EXE, "rtnCoordCMDOperateOnGroup::execute" )
   INT32 rtnCoordCMDOperateOnGroup::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDOPONGR_EXE ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      contextID                        = -1 ;
      const CHAR *pGroupName           = NULL ;
      CHAR *pCMDName                   = NULL ;

      do
      {
         INT32 flag;
         SINT64 numToSkip;
         SINT64 numToReturn;
         CHAR *pQuery;
         CHAR *pFieldSelector;
         CHAR *pOrderBy;
         CHAR *pHint;
         rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName,
                               &numToSkip, &numToReturn, &pQuery,
                               &pFieldSelector, &pOrderBy, &pHint ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to parse the request(rc=%d)", rc ) ;
            break;
         }
         BSONObj boQuery;
         BSONObj boFieldSelector;
         BSONObj boOrderBy;
         BSONObj boHint;
         try
         {
            BSONObjBuilder bobQuery;
            BSONObj boReq(pQuery);
            BSONElement beGroupName = boReq.getField( FIELD_NAME_GROUPNAME );
            if ( beGroupName.eoo() || beGroupName.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to get the field(%s)",
                        FIELD_NAME_GROUPNAME ) ;
               break ;
            }
            pGroupName = beGroupName.valuestr() ;
            bobQuery.append( beGroupName );
            boQuery = bobQuery.obj();
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() ) ;
            break ;
         }

         rtnCoordProcesserFactory *pProcesserFactory
               = pCoordcb->getProcesserFactory() ;
         rtnCoordOperator *pCmdProcesser = NULL ;
         pCmdProcesser = pProcesserFactory->getCommandProcesser(
            COORD_CMD_LISTGROUPS ) ;
         SDB_ASSERT( pCmdProcesser , "pCmdProcesser can't be NULL" ) ;
         char *pListReq = NULL ;
         INT32 listReqSize = 0 ;
         rc = msgBuildQueryMsg( &pListReq, &listReqSize, "", 0, 0, 0, 1,
                                &boQuery, &boFieldSelector, &boOrderBy,
                                &boHint ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to build list request(rc=%d)", rc ) ;
            break;
         }
         rc = pCmdProcesser->execute( (MsgHeader*)pListReq, cb,
                                      contextID, buf ) ;
         if ( pListReq )
         {
            SDB_OSS_FREE( pListReq ) ;
            pListReq = NULL;
         }
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to list groups(rc=%d)", rc ) ;
            break;
         }

         rtnContextBuf buffObj ;
         rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
         if ( rc != SDB_OK )
         {
            contextID = -1 ;
            if ( rc == SDB_DMS_EOC || NULL == buffObj.data() )
            {
               rc = SDB_CLS_GRP_NOT_EXIST;
            }
            PD_LOG ( PDERROR, "Failed to get group info(rc=%d)", rc ) ;
            break;
         }

         BSONObj boGroupInfo ;
         try
         {
            boGroupInfo = BSONObj( buffObj.data() ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "occured unexpected error:%s", e.what() ) ;
            break;
         }
         rc = opOnGroup( boGroupInfo ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Operate failed(rc=%d)", rc ) ;
            break;
         }
      }while ( FALSE ) ;

      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }

      if ( pGroupName )
      {
         PD_AUDIT_COMMAND( AUDIT_SYSTEM, pCMDName, AUDIT_OBJ_GROUP,
                           pGroupName, rc, "" ) ;
      }

      PD_TRACE_EXITRC ( SDB_RTNCOCMDOPONGR_EXE, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTN_COCOMDOPONGR_OPONGR, "rtnCoordCMDOperateOnGroup::opOnGroup" )
   INT32 rtnCoordCMDOperateOnGroup::opOnGroup( bson::BSONObj &boGroupInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTN_COCOMDOPONGR_OPONGR ) ;
      SINT32 opType = getOpType();
      vector<BSONObj> objList;
      do
      {
         try
         {
            BSONElement beGroup = boGroupInfo.getField( FIELD_NAME_GROUP );
            if ( beGroup.eoo() || beGroup.type()!=Array )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "failed to get the field(%s)",
                        FIELD_NAME_GROUP );
               break;
            }
            BSONObjIterator i( beGroup.embeddedObject() );
            while ( i.more() )
            {
               BSONElement beTmp = i.next();
               BSONObj boTmp = beTmp.embeddedObject();
               BSONElement beHostName = boTmp.getField( FIELD_NAME_HOST );
               if ( beHostName.eoo() || beHostName.type()!=String )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDERROR, "failed to get the HostName");
                  break;
               }
               std::string strHostName = beHostName.str();
               BSONElement beService = boTmp.getField( FIELD_NAME_SERVICE );
               if ( beService.eoo() || beService.type()!=Array )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDWARNING, "failed to get the field(%s)",
                           FIELD_NAME_SERVICE );
                  break;
               }
               std::string strServiceName;
               rc = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE,
                                    strServiceName );
               if ( rc != SDB_OK )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDWARNING, "failed to get local-service-name" );
                  break;
               }
               SINT32 retCode;
               BSONObjBuilder bobLocalService;
               bobLocalService.append( PMD_OPTION_SVCNAME, strServiceName );
               BSONObj boLocalService = bobLocalService.obj();
               rc = rtnRemoteExec ( opType, strHostName.c_str(),
                                    &retCode, &boLocalService ) ;
               if ( SDB_OK == rc && SDB_OK == retCode )
               {
                  continue;
               }
               if ( rc != SDB_OK )
               {
                  PD_LOG( PDERROR, "Operate failed (HostName=%s, "
                          "LocalService=%s, rc=%d)", strHostName.c_str(),
                          strServiceName.c_str(), rc ) ;
               }
               else if ( retCode != SDB_OK )
               {
                  rc = retCode;
                  PD_LOG( PDERROR, "Remote node execute(opType=%d) failed "
                          "(HostName=%s, LocalService=%s, rc=%d)",
                          opType, strHostName.c_str(), strServiceName.c_str(),
                          rc ) ;
               }
               BSONObjBuilder bobReply;
               bobReply.append( FIELD_NAME_HOST, strHostName );
               bobReply.append( PMD_OPTION_SVCNAME, strServiceName );
               bobReply.append( FIELD_NAME_ERROR_NO, retCode );
               objList.push_back( bobReply.obj() );
            }
            break;
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "occured unexpected error:%s", e.what() ) ;
            break;
         }
      }while ( FALSE );
      if ( objList.size() != 0 )
      {
         rc = SDB_CM_OP_NODE_FAILED;
      }
      PD_TRACE_EXITRC ( SDB_RTN_COCOMDOPONGR_OPONGR, rc ) ;
      return rc ;
   }
   SINT32 rtnCoordCMDShutdownGroup::getOpType()
   {
      return SDBSTOP;
   }

   INT32 rtnCoordCMDSplit::getCLCount( const CHAR * clFullName,
                                       CoordGroupList & groupList,
                                       pmdEDUCB *cb,
                                       UINT64 & count )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKRCB                   = pmdGetKRCB () ;
      SDB_RTNCB *pRtncb                = pKRCB->getRTNCB() ;
      rtnContext *pContext             = NULL ;
      count                            = 0 ;
      CoordGroupList tmpGroupList      = groupList ;

      BSONObj collectionObj ;
      BSONObj dummy ;
      rtnContextBuf buffObj ;

      collectionObj = BSON( FIELD_NAME_COLLECTION << clFullName ) ;

      // send getcount to node
      rc = rtnCoordNodeQuery( CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                              dummy, dummy, dummy, collectionObj,
                              0, 1, tmpGroupList, cb, &pContext,
                              clFullName ) ;

      PD_RC_CHECK ( rc, PDERROR, "Failed to getcount from source node, rc = %d",
                    rc ) ;

      rc = pContext->getMore( -1, buffObj, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Get count getmore failed, rc: %d", rc ) ;
         goto error ;
      }
      else
      {
         // get count data
         BSONObj countObj ( buffObj.data() ) ;
         BSONElement beTotal = countObj.getField( FIELD_NAME_TOTAL );
         PD_CHECK( beTotal.isNumber(), SDB_INVALIDARG, error,
                   PDERROR, "count failed, failed to get the field(%s)",
                   FIELD_NAME_TOTAL ) ;
         count = beTotal.numberLong() ;
      }

   done:
      if ( pContext )
      {
         SINT64 contextID = pContext->contextID() ;
         pRtncb->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP_EXE, "rtnCoordCMDSplit::execute" )
   INT32 rtnCoordCMDSplit::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDSP_EXE ) ;
      pmdKRCB *pKRCB                   = pmdGetKRCB () ;
      SDB_RTNCB *pRtncb                = pKRCB->getRTNCB() ;
      CoordCB *pCoordcb                = pKRCB->getCoordCB () ;
      contextID                        = -1 ;

      CHAR *pCommandName               = NULL ;
      CHAR *pQuery                     = NULL ;

      CHAR szSource [ OSS_MAX_GROUPNAME_SIZE + 1 ] = {0} ;
      CHAR szTarget [ OSS_MAX_GROUPNAME_SIZE + 1 ] = {0} ;
      const CHAR *strName              = NULL ;
      CHAR *splitReadyBuffer           = NULL ;
      INT32 splitReadyBufferSz         = 0 ;
      CHAR *splitQueryBuffer           = NULL ;
      INT32 splitQueryBufferSz         = 0 ;
      MsgOpQuery *pSplitQuery          = NULL ;
      UINT64 taskID                    = 0 ;
      BOOLEAN async                    = FALSE ;
      BSONObj taskInfoObj ;

      BSONObj boShardingKey ;
      CoordCataInfoPtr cataInfo ;
      BSONObj boKeyStart ;
      BSONObj boKeyEnd ;
      FLOAT64 percent = 0.0 ;

      // first round we perform prepare, so catalog node is able to do sanity
      // check for collection name and nodes
      MsgOpQuery *pSplitReq            = (MsgOpQuery *)pMsg ;
      pSplitReq->header.opCode         = MSG_CAT_SPLIT_PREPARE_REQ ;

      CoordGroupList groupLst ;
      CoordGroupList groupDstLst ;

      /******************************************************************
       *              PREPARE PHASE                                     *
       ******************************************************************/
      // send request to catalog
      rc = executeOnCataGroup ( pMsg, cb, &groupLst ) ;
      PD_RC_CHECK ( rc, PDERROR, "Split failed on catalog, rc = %d", rc ) ;

      // here, in groupLst there should be one and only one group, for SOURCE
      // send request to data-node to find the partitioning key
      // Extract the SplitQuery Field and build a query request to send to data
      // node
      rc = msgExtractQuery ( (CHAR*)pSplitReq, NULL, &pCommandName,
                             NULL, NULL, &pQuery,
                             NULL, NULL, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      try
      {
         /***************************************************************
          *             DO SOME VALIDATION HERE                         *
          ***************************************************************/
         BSONObj boQuery ( pQuery ) ;

         // get collection name and query
         BSONElement beName = boQuery.getField ( CAT_COLLECTION_NAME ) ;
         BSONElement beSplitQuery =
               boQuery.getField ( CAT_SPLITQUERY_NAME ) ;
         BSONElement beSplitEndQuery ;
         BSONElement beSource = boQuery.getField ( CAT_SOURCE_NAME ) ;
         BSONElement beTarget = boQuery.getField ( CAT_TARGET_NAME ) ;
         BSONElement beAsync  = boQuery.getField ( FIELD_NAME_ASYNC ) ;
         percent = boQuery.getField( CAT_SPLITPERCENT_NAME ).numberDouble() ;
         // collection name verify
         PD_CHECK ( !beName.eoo() && beName.type () == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Failed to process split prepare, unable to find "
                    "collection name field" ) ;
         // now strName is the name of collection
         strName = beName.valuestr() ;
         // get source group name
         PD_CHECK ( !beSource.eoo() && beSource.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Unable to find source field" ) ;
         rc = catGroupNameValidate ( beSource.valuestr() ) ;
         PD_CHECK ( SDB_OK == rc, SDB_INVALIDARG, error, PDERROR,
                    "Source name is not valid: %s",
                    beSource.valuestr() ) ;
         ossStrncpy ( szSource, beSource.valuestr(), sizeof(szSource) ) ;

         // get target group name
         PD_CHECK ( !beTarget.eoo() && beTarget.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Unable to find target field" ) ;
         rc = catGroupNameValidate ( beTarget.valuestr() ) ;
         PD_CHECK ( SDB_OK == rc, SDB_INVALIDARG, error, PDERROR,
                    "Target name is not valid: %s",
                    beTarget.valuestr() ) ;
         ossStrncpy ( szTarget, beTarget.valuestr(), sizeof(szTarget) ) ;

         // async check
         if ( Bool == beAsync.type() )
         {
            async = beAsync.Bool() ? TRUE : FALSE ;
         }
         else if ( !beAsync.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] error", FIELD_NAME_ASYNC,
                    beAsync.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // make sure we have either split value or split query
         if ( !beSplitQuery.eoo() )
         {
            PD_CHECK ( beSplitQuery.type() == Object,
                       SDB_INVALIDARG, error, PDERROR,
                       "Split is not defined or not valid" ) ;
            beSplitEndQuery = boQuery.getField ( CAT_SPLITENDQUERY_NAME ) ;
            if ( !beSplitEndQuery.eoo() )
            {
               PD_CHECK ( beSplitEndQuery.type() == Object,
                          SDB_INVALIDARG, error, PDERROR,
                          "Split is not defined or not valid" ) ;
            }
         }
         else
         {
            PD_CHECK( percent > 0.0 && percent <= 100.0,
                      SDB_INVALIDARG, error, PDERROR,
                      "Split percent value is error" ) ;
         }

         // get sharding key, always get the newest version from catalog
         rc = rtnCoordGetCataInfo ( cb, strName, TRUE, cataInfo ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to get cata info for collection %s, rc = %d",
                       strName, rc ) ;
         // sharding key must exist, we should NEVER hit this check because the
         // check already done in catalog in PREPARE phase
         cataInfo->getShardingKey ( boShardingKey ) ;
         PD_CHECK ( !boShardingKey.isEmpty(), SDB_COLLECTION_NOTSHARD, error,
                    PDWARNING, "Collection must be sharded: %s", strName ) ;

         /*********************************************************************
          *           GET THE SHARDING KEY VALUE FROM SOURCE                  *
          *********************************************************************/
         if ( cataInfo->getCatalogSet()->isHashSharding() )
         {
            if ( !beSplitQuery.eoo() )
            {
               BSONObj tmpStart = beSplitQuery.embeddedObject() ;
               BSONObjBuilder tmpStartBuilder ;
               tmpStartBuilder.appendElementsWithoutName( tmpStart ) ;
               boKeyStart = tmpStartBuilder.obj() ;
               if ( !beSplitEndQuery.eoo() )
               {
                  BSONObj tmpEnd = beSplitEndQuery.embeddedObject() ;
                  BSONObjBuilder tmpEndBuilder ;
                  tmpEndBuilder.appendElementsWithoutName( tmpEnd ) ;
                  boKeyEnd = tmpEndBuilder.obj() ;
               }
            }
         }
         else
         {
            if ( beSplitQuery.eoo())
            {
               rc = _getBoundByPercent( strName, percent, cataInfo,
                                        groupLst, cb, boKeyStart, boKeyEnd ) ;
            }
            else
            {
               rc = _getBoundByCondition( strName,
                                          beSplitQuery.embeddedObject(),
                                          beSplitEndQuery.eoo() ?
                                          BSONObj():
                                          beSplitEndQuery.embeddedObject(),
                                          groupLst,
                                          cb,
                                          cataInfo,
                                          boKeyStart, boKeyEnd ) ;
            }

            PD_RC_CHECK( rc, PDERROR, "Failed to get bound, rc: %d",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when query from remote node: %s",
                       e.what() ) ;
      }

      /************************************************************************
       *         SHARDING READY REQUEST                                       *
       ************************************************************************/
      // now boKeyStart contains the key we want to split, let's construct a new
      // request for split ready
      try
      {
         BSONObj boSend ;
         vector<BSONObj> boRecv ;
         // construct the record that we are going to send to catalog
         boSend = BSON ( CAT_COLLECTION_NAME << strName <<
                         CAT_SOURCE_NAME << szSource <<
                         CAT_TARGET_NAME << szTarget <<
                         CAT_SPLITPERCENT_NAME << percent <<
                         CAT_SPLITVALUE_NAME << boKeyStart <<
                         CAT_SPLITENDVALUE_NAME << boKeyEnd ) ;
         taskInfoObj = boSend ;
         rc = msgBuildQueryMsg ( &splitReadyBuffer, &splitReadyBufferSz,
                                 CMD_ADMIN_PREFIX CMD_NAME_SPLIT, 0,
                                 0, 0, -1, &boSend, NULL,
                                 NULL, NULL ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to build query message, rc: %d",
                       rc ) ;
         pSplitReq                        = (MsgOpQuery *)splitReadyBuffer ;
         pSplitReq->header.opCode         = MSG_CAT_SPLIT_READY_REQ ;
         pSplitReq->version               = cataInfo->getVersion();

         rc = executeOnCataGroup ( (MsgHeader*)pSplitReq, cb,
                                   &groupDstLst, &boRecv ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to execute split ready on catalog, "
                       "rc = %d", rc ) ;
         if ( boRecv.empty() )
         {
            PD_LOG( PDERROR, "Failed to get task id from result msg" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         taskID = (UINT64)(boRecv.at(0).getField( CAT_TASKID_NAME ).numberLong()) ;

         // construct split query req
         boSend = BSON( CAT_TASKID_NAME << (long long)taskID ) ;
         rc = msgBuildQueryMsg( &splitQueryBuffer, &splitQueryBufferSz,
                                CMD_ADMIN_PREFIX CMD_NAME_SPLIT, 0,
                                0, 0, -1, &boSend, NULL,
                                NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build query message, rc: %d",
                      rc ) ;
         pSplitQuery                      = (MsgOpQuery *)splitQueryBuffer ;
         pSplitQuery->version             = cataInfo->getVersion() ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when building split ready message: %s",
                       e.what() ) ;
      }
      /************************************************************************
       *           SHARDING START REQUEST                                     *
       ************************************************************************/
      // before sending to data node, we have to convert the request to QUERY
      pSplitReq->header.opCode = MSG_BS_QUERY_REQ ;
      rc = executeOnCL( (MsgHeader *)splitReadyBuffer, cb, strName,
                        FALSE, &groupDstLst, NULL, NULL ) ;
      if ( rc )
      {
         // when we get here, something big happend. We have marked ready to
         // split on catalog but data node refused to do so.
         PD_LOG ( PDERROR, "Failed to execute split on data node, rc = %d",
                  rc ) ;
         goto cancel ;
      }

      // if sync, need to wait task finished
      if ( !async )
      {
         rtnCoordProcesserFactory *pFactory = pCoordcb->getProcesserFactory() ;
         rtnCoordCommand *pCmd = pFactory->getCommandProcesser(
                                 COORD_CMD_WAITTASK ) ;
         SDB_ASSERT( pCmd, "wait task command not found" ) ;
         rc = pCmd->execute( (MsgHeader*)splitQueryBuffer, cb,
                             contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Wait task[%lld] failed, rc: %d", taskID, rc ) ;
            rc = SDB_OK ;
            /// can not report error, because split already created
         }
      }
      else // return taskid to client
      {
         rtnContextDump *pContext = NULL ;
         rc = pRtncb->contextNew( RTN_CONTEXT_DUMP, (rtnContext**)&pContext,
                                  contextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create context, rc: %d", rc ) ;
         rc = pContext->open( BSONObj(), BSONObj(), 1, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open context, rc: %d", rc ) ;
         pContext->append( BSON( CAT_TASKID_NAME << (long long)taskID ) ) ;
      }

   done :
      if ( pCommandName && strName )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, pCommandName + 1, AUDIT_OBJ_CL,
                           strName, rc, "Option:%s, TaskID:%llu",
                           taskInfoObj.toString().c_str(), taskID ) ;
      }
      if ( splitReadyBuffer )
      {
         SDB_OSS_FREE ( splitReadyBuffer ) ;
      }
      if ( splitQueryBuffer )
      {
         SDB_OSS_FREE ( splitQueryBuffer ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDSP_EXE, rc ) ;
      return rc ;
   cancel :
      // convert request to split cancel and use all other arguments
      pSplitQuery->header.opCode       = MSG_CAT_SPLIT_CANCEL_REQ ;
      pSplitQuery->version             = cataInfo->getVersion() ;
      {
         INT32 rctmp = executeOnCataGroup ( (MsgHeader*)pSplitQuery,
                                            cb, TRUE ) ;
         if ( rctmp )
         {
            PD_LOG( PDWARNING, "Failed to execute split cancel on catalog, "
                    "rc = %d", rctmp ) ;
            goto error ;
         }
      }

   error :
      if ( SDB_RTN_INVALID_HINT == rc )
      {
         rc = SDB_COORD_SPLIT_NO_SHDIDX ;
      }
      if ( -1 != contextID )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP_GETBOUNDRONDATA, "rtnCoordCMDSplit::_getBoundRecordOnData" )
   INT32 rtnCoordCMDSplit::_getBoundRecordOnData( const CHAR *cl,
                                                  const BSONObj &condition,
                                                  const BSONObj &hint,
                                                  const BSONObj &sort,
                                                  INT32 flag,
                                                  INT64 skip,
                                                  CoordGroupList &groupList,
                                                  pmdEDUCB *cb,
                                                  BSONObj &shardingKey,
                                                  BSONObj &record )
   {
      PD_TRACE_ENTRY( SDB_RTNCOCMDSP_GETBOUNDRONDATA ) ;
      INT32 rc = SDB_OK ;
      BSONObj empty ;
      rtnContext *context = NULL ;
      rtnContextBuf buffObj ;
      BSONObj obj ;

      // check condition has invalid fileds
      if ( !condition.okForStorage() )
      {
         PD_LOG( PDERROR, "Condition[%s] has invalid field name",
                 condition.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( condition.isEmpty() )
      {
         rc = rtnCoordNodeQuery( cl, condition, empty, sort,
                                 hint, skip, 1, groupList,
                                 cb, &context, NULL, flag ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to query from data group, rc = %d",
                       rc ) ;
         rc = context->getMore( -1, buffObj, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         else
         {
            obj = BSONObj( buffObj.data() ) ;
         }
      }
      else
      {
         obj = condition ;
      }

      // product split key
      {
         PD_LOG ( PDINFO, "Split found record %s", obj.toString().c_str() ) ;
         // we need to compare with boShardingKey and extract the partition key
         ixmIndexKeyGen keyGen ( shardingKey ) ;
         BSONObjSet keys ;
         BSONObjSet::iterator keyIter ;
         rc = keyGen.getKeys ( obj, keys, NULL, TRUE ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to extract keys\nkeyDef = %s\n"
                       "record = %s\nrc = %d", shardingKey.toString().c_str(),
                       obj.toString().c_str(), rc ) ;
         // make sure there is one and only one element in the keys
         PD_CHECK ( keys.size() == 1, SDB_INVALID_SHARDINGKEY, error,
                    PDWARNING, "There must be a single key generate for "
                    "sharding\nkeyDef = %s\nrecord = %s\n",
                    shardingKey.toString().c_str(),
                    obj.toString().c_str() ) ;

         keyIter = keys.begin () ;
         record = (*keyIter).copy() ;

         // validate key does not contains Undefined
         /*{
            BSONObjIterator iter ( record ) ;
            while ( iter.more () )
            {
               BSONElement e = iter.next () ;
               PD_CHECK ( e.type() != Undefined, SDB_CLS_BAD_SPLIT_KEY,
                          error, PDERROR, "The split record does not contains "
                          "a valid key\nRecord: %s\nShardingKey: %s\n"
                          "SplitKey: %s", obj.toString().c_str(),
                          shardingKey.toString().c_str(),
                          record.toString().c_str() ) ;
            }
         }*/

        PD_LOG ( PDINFO, "Split found key %s", record.toString().c_str() ) ;
     }

   done:
      if ( NULL != context )
      {
         SINT64 contextID = context->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOCMDSP_GETBOUNDRONDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP__GETBOUNDBYC, "rtnCoordCMDSplit::_getBoundByCondition" )
   INT32 rtnCoordCMDSplit::_getBoundByCondition( const CHAR *cl,
                                                 const BSONObj &begin,
                                                 const BSONObj &end,
                                                 CoordGroupList &groupList,
                                                 pmdEDUCB *cb,
                                                 CoordCataInfoPtr &cataInfo,
                                                 BSONObj &lowBound,
                                                 BSONObj &upBound )
   {
      PD_TRACE_ENTRY( SDB_RTNCOCMDSP__GETBOUNDBYC ) ;
      INT32 rc = SDB_OK ;
      /// coord send will clear group list.
      CoordGroupList grpTmp = groupList ;
      BSONObj shardingKey ;
      cataInfo->getShardingKey ( shardingKey ) ;
      PD_CHECK ( !shardingKey.isEmpty(), SDB_COLLECTION_NOTSHARD, error,
                  PDWARNING, "Collection must be sharded: %s", cl ) ;

      rc = _getBoundRecordOnData( cl, begin, BSONObj(),BSONObj(),
                                  0, 0, grpTmp, cb,
                                  shardingKey, lowBound ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get begin bound:%d",rc ) ;
         goto error ;
      }

      if ( !end.isEmpty() )
      {
         grpTmp = groupList ;
         rc = _getBoundRecordOnData( cl, end, BSONObj(),BSONObj(),
                                     0, 0, grpTmp, cb,
                                     shardingKey, upBound ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get end bound:%d",rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOCMDSP__GETBOUNDBYC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP__GETBOUNDBYP, "rtnCoordCMDSplit::_getBoundByPercent" )
   INT32 rtnCoordCMDSplit::_getBoundByPercent( const CHAR *cl,
                                               FLOAT64 percent,
                                               CoordCataInfoPtr &cataInfo,
                                               CoordGroupList &groupList,
                                               pmdEDUCB *cb,
                                               BSONObj &lowBound,
                                               BSONObj &upBound )
   {
      PD_TRACE_ENTRY( SDB_RTNCOCMDSP__GETBOUNDBYP ) ;
      INT32 rc = SDB_OK ;
      BSONObj shardingKey ;
      cataInfo->getShardingKey ( shardingKey ) ;
      CoordGroupList grpTmp = groupList ;
      PD_CHECK ( !shardingKey.isEmpty(), SDB_COLLECTION_NOTSHARD, error,
                 PDWARNING, "Collection must be sharded: %s", cl ) ;

      // if split percent is 100.0%, get the group low bound
      if ( 100.0 - percent < OSS_EPSILON )
      {
         rc = cataInfo->getGroupLowBound( grpTmp.begin()->second,
                                          lowBound ) ;
         PD_RC_CHECK( rc, PDERROR, "Get group[%d] low bound failed, rc: %d",
                      grpTmp.begin()->second, rc ) ;
      }
      else
      {
         UINT64 totalCount = 0 ;
         INT64 skipCount = 0 ;
         INT32 flag = 0 ;
         BSONObj hint ;
         while ( TRUE )
         {
            rc = getCLCount( cl, grpTmp, cb, totalCount ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Get collection count failed, rc: %d",
                         rc ) ;
            if ( 0 == totalCount )
            {
               rc = SDB_DMS_EMPTY_COLLECTION ;
               PD_LOG( PDDEBUG, "collection[%s] is empty", cl ) ;
               break ;
            }

            skipCount = (INT64)(totalCount * ( ( 100 - percent ) / 100 ) ) ;
            hint = BSON( "" << "" ) ;
            flag = FLG_QUERY_FORCE_HINT ;

            /// sort by shardingKey that if $shard index does not exist
            /// can still match index.
            rc = _getBoundRecordOnData( cl, BSONObj(), hint, shardingKey,
                                        flag, skipCount, grpTmp,
                                        cb, shardingKey, lowBound ) ;
            if ( SDB_DMS_EOC == rc )
            {
               continue ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get bound from data:%d",rc ) ;
               goto error ;
            }
            else
            {
               break ;
            }
         }
      }

      /// upbound always be empty.
      upBound = BSONObj() ;
   done:
      PD_TRACE_EXITRC( SDB_RTNCOCMDSP__GETBOUNDBYP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordCmdWaitTask::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SET_RC ignoreRC ;
      rtnContextCoord *pContext        = NULL ;
      rtnContextBuf buffObj ;
      pmdKRCB *pKRCB                   = pmdGetKRCB() ;
      contextID                        = -1 ;
      pMsg->opCode                     = MSG_CAT_QUERY_TASK_REQ ;
      pMsg->TID                        = cb->getTID() ;

      ignoreRC.insert( SDB_DMS_EOC ) ;
      ignoreRC.insert( SDB_CAT_TASK_NOTFOUND ) ;

      while ( TRUE )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = executeOnCataGroup( pMsg, cb, TRUE, &ignoreRC, &pContext ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query task on catalog failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = pContext->getMore( -1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get more failed, rc: %d", rc ) ;
            goto error ;
         }

         pKRCB->getRTNCB()->contextDelete( pContext->contextID(), cb ) ;
         pContext = NULL ;
         ossSleep( OSS_ONE_SEC ) ;
      }

   done:
      if ( pContext )
      {
         pKRCB->getRTNCB()->contextDelete( pContext->contextID(),  cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordCmdListTask::_preProcess( rtnQueryOptions &queryOpt,
                                           string &clName )
   {
      clName = CAT_TASK_INFO_COLLECTION ;
      return SDB_OK ;
   }

   INT32 rtnCoordCmdCancelTask::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKRCB                   = pmdGetKRCB () ;
      CoordCB *pCoordcb                = pKRCB->getCoordCB () ;
      rtnCoordProcesserFactory *pFactory = pCoordcb->getProcesserFactory() ;
      BOOLEAN async                    = FALSE ;

      contextID                        = -1 ;

      CoordGroupList groupLst ;
      INT32 rcTmp = SDB_OK ;

      // extract msg
      CHAR *pQueryBuf = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL, &pQueryBuf,
                            NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      try
      {
         BSONObj matcher( pQueryBuf ) ;
         rc = rtnGetBooleanElement( matcher, FIELD_NAME_ASYNC, async ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_ASYNC, rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pMsg->opCode                     = MSG_CAT_SPLIT_CANCEL_REQ ;

      rc = executeOnCataGroup( pMsg, cb, &groupLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Excute on catalog failed, rc: %d", rc ) ;

      pMsg->opCode                     = MSG_BS_QUERY_REQ ;
      // notify to data node
      rcTmp = executeOnDataGroup( pMsg, cb, groupLst,
                                  TRUE, NULL, NULL, NULL ) ;
      if ( rcTmp )
      {
         PD_LOG( PDWARNING, "Failed to notify to data node, rc: %d", rcTmp ) ;
      }

      // if sync
      if ( !async )
      {
         rtnCoordCommand *pCmd = pFactory->getCommandProcesser(
                                           COORD_CMD_WAITTASK ) ;
         if ( !pCmd )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Command[%s] is null", COORD_CMD_WAITTASK ) ;
            goto error ;
         }
         rc = pCmd->execute( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSTB_EXE, "rtnCoordCMDStatisticsBase::execute" )
   INT32 rtnCoordCMDStatisticsBase::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDSTB_EXE ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();

      // fill default-reply(execute success)
      contextID                        = -1 ;

      rtnCoordQuery queryOpr( isReadonly() ) ;
      rtnContextCoord *pContext = NULL ;
      rtnQueryConf queryConf ;
      rtnSendOptions sendOpt ;
      queryConf._openEmptyContext = openEmptyContext() ;

      CHAR *pHint = NULL ;

      // extract request-message
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, NULL, NULL,
                            NULL, &pHint );
      PD_RC_CHECK ( rc, PDERROR, "Execute failed, failed to parse query "
                    "request(rc=%d)", rc ) ;

      try
      {
         BSONObj boHint( pHint ) ;
         //get collection name
         BSONElement ele = boHint.getField( FIELD_NAME_COLLECTION );
         PD_CHECK ( ele.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Execute failed, failed to get the field(%s)",
                    FIELD_NAME_COLLECTION ) ;
         queryConf._realCLName = ele.str() ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK ( rc, PDERROR, "Execute failed, occured unexpected "
                       "error:%s", e.what() ) ;
      }

      rc = queryOpr.queryOrDoOnCL( pMsg, pRouteAgent, cb, &pContext,
                                   sendOpt, &queryConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Query failed(rc=%d)", rc ) ;

      // statistics the result
      rc = generateResult( pContext, pRouteAgent, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute statistics(rc=%d)", rc ) ;

      contextID = pContext->contextID() ;
      pContext->reopen() ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDSTB_EXE, rc ) ;
      return rc;
   error:
      if ( pContext )
      {
         pRtncb->contextDelete( pContext->contextID(), cb ) ;
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDGETIXS_GENRT, "rtnCoordCMDGetIndexes::generateResult" )
   INT32 rtnCoordCMDGetIndexes::generateResult( rtnContext *pContext,
                                                netMultiRouteAgent *pRouteAgent,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDGETIXS_GENRT ) ;
      CoordIndexMap indexMap ;
      rtnContextBuf buffObj ;

      // get index from all nodes
      do
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( rc != SDB_OK )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to get index data(rc=%d)", rc );
            }
            break;
         }

         try
         {
            BSONObj boTmp( buffObj.data() ) ;
            BSONElement beIndexDef = boTmp.getField(
                                       IXM_FIELD_NAME_INDEX_DEF );
            PD_CHECK ( beIndexDef.type() == Object, SDB_INVALIDARG, error,
                       PDERROR, "Get index failed, failed to get the field(%s)",
                       IXM_FIELD_NAME_INDEX_DEF ) ;

            BSONObj boIndexDef = beIndexDef.embeddedObject() ;
            BSONElement beIndexName = boIndexDef.getField( IXM_NAME_FIELD ) ;
            PD_CHECK ( beIndexName.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Get index failed, failed to get the field(%s)",
                       IXM_NAME_FIELD ) ;

            std::string strIndexName = beIndexName.valuestr() ;
            CoordIndexMap::iterator iter = indexMap.find( strIndexName ) ;
            if ( indexMap.end() == iter )
            {
               indexMap[ strIndexName ] = boTmp.copy() ;
            }
            else
            {
               // check the index
               BSONObjIterator newIter( boIndexDef ) ;
               BSONObj boOldDef;
               BSONElement beOldDef =
                  iter->second.getField( IXM_FIELD_NAME_INDEX_DEF );
               PD_CHECK ( beOldDef.type() == Object, SDB_INVALIDARG, error,
                          PDERROR, "Get index failed, failed to get the field(%s)",
                          IXM_FIELD_NAME_INDEX_DEF ) ;
               boOldDef = beOldDef.embeddedObject();
               while( newIter.more() )
               {
                  BSONElement beTmp1 = newIter.next();
                  if ( 0 == ossStrcmp( beTmp1.fieldName(), "_id") )
                  {
                     continue;
                  }
                  BSONElement beTmp2 = boOldDef.getField( beTmp1.fieldName() );
                  if ( 0 != beTmp1.woCompare( beTmp2 ) )
                  {
                     PD_LOG( PDERROR, "Corrupted index(name:%s, define1:%s, "
                             "define2:%s)", strIndexName.c_str(),
                             boIndexDef.toString().c_str(),
                             boOldDef.toString().c_str() );
                     break ;
                  }
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to get index, occured unexpected"
                         "error:%s", e.what() ) ;
         }
      }while( SDB_OK == rc ) ;

      if ( rc != SDB_OK )
      {
         goto error;
      }

      {
         CoordIndexMap::iterator iterMap = indexMap.begin();
         while( iterMap != indexMap.end() )
         {
            rc = pContext->append( iterMap->second );
            PD_RC_CHECK( rc, PDERROR, "Failed to get index, append the data "
                         "failed(rc=%d)", rc ) ;
            ++iterMap;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDGETIXS_GENRT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDGETCT_GENRT, "rtnCoordCMDGetCount::generateResult" )
   INT32 rtnCoordCMDGetCount::generateResult( rtnContext *pContext,
                                              netMultiRouteAgent *pRouteAgent,
                                              pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDGETCT_GENRT ) ;
      SINT64 totalCount = 0 ;
      rtnContextBuf buffObj ;

      do
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( rc != SDB_OK )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to generate count result"
                        "get data failed(rc=%d)", rc );
            }
            break;
         }

         try
         {
            BSONObj boTmp( buffObj.data() );
            BSONElement beTotal = boTmp.getField( FIELD_NAME_TOTAL );
            PD_CHECK( beTotal.isNumber(), SDB_INVALIDARG, error,
                      PDERROR, "count failed, failed to get the field(%s)",
                      FIELD_NAME_TOTAL ) ;
            totalCount += beTotal.number() ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                         "occured unexpected error:%s", e.what() );
         }
      }while( SDB_OK == rc ) ;

      if ( rc != SDB_OK )
      {
         goto error;
      }
      try
      {
         BSONObjBuilder bobResult ;
         bobResult.append( FIELD_NAME_TOTAL, totalCount ) ;
         BSONObj boResult = bobResult.obj() ;
         rc = pContext->append( boResult ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                      "append the data failed(rc=%d)", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                      "occured unexpected error:%s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDGETCT_GENRT, rc ) ;
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordCMDGetDatablocks::generateResult( rtnContext * pContext,
                                                   netMultiRouteAgent * pRouteAgent,
                                                   pmdEDUCB * cb )
   {
      // don't merge data, do nothing
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTCAGP_EXE, "rtnCoordCMDCreateCataGroup::execute" )
   INT32 rtnCoordCMDCreateCataGroup::execute( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              INT64 &contextID,
                                              rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTCAGP_EXE ) ;
      contextID = -1 ;

      INT32 flag = 0 ;
      CHAR *pCMDName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      CHAR *pQuery = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;
      BSONObj boNodeConfig;
      BSONObj boNodeInfo;
      const CHAR *pHostName = NULL;
      SINT32 retCode = 0 ;
      BSONObj boLocalSvc ;
      BSONObj boBackup = BSON( "Backup" << true ) ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse create catalog-group "
                   "request(rc=%d)", rc ) ;

      rc = getNodeConf( pQuery, boNodeConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get configure info(rc=%d)", rc ) ;

      rc = getNodeInfo( pQuery, boNodeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node info(rc=%d)", rc ) ;

      try
      {
         BSONElement beHostName = boNodeInfo.getField( FIELD_NAME_HOST );
         PD_CHECK( beHostName.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", FIELD_NAME_HOST );
         pHostName = beHostName.valuestr() ;

         BSONElement beLocalSvc = boNodeInfo.getField( PMD_OPTION_SVCNAME );
         PD_CHECK( beLocalSvc.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", PMD_OPTION_SVCNAME );
         BSONObjBuilder bobLocalSvc ;
         bobLocalSvc.append( beLocalSvc ) ;
         boLocalSvc = bobLocalSvc.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to create catalog group, occured unexpected "
                   "error:%s", e.what() );
      }

      // Create
      rc = rtnRemoteExec( SDBADD, pHostName, &retCode, &boNodeConfig,
                          &boNodeInfo ) ;
      rc = rc ? rc : retCode ;
      PD_RC_CHECK( rc, PDERROR, "remote node execute(configure) "
                   "failed(rc=%d)", rc ) ;

      // Start
      rc = rtnRemoteExec( SDBSTART, pHostName, &retCode, &boLocalSvc ) ;
      rc = rc ? rc : retCode ;
      // if start catalog failed, need to remove config
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Remote node execute(start) failed(rc=%d)", rc ) ;
         // boBackup for remove node to backup node info
         rtnRemoteExec( SDBRM, pHostName, &retCode, &boNodeConfig, &boBackup ) ;
         goto error ;
      }

      /// fillback catalog addr.
      {
         CoordVecNodeInfo cataList ;
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->getCatNodeAddrList( cataList ) ;
         pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;

         for ( CoordVecNodeInfo::const_iterator itr = cataList.begin() ;
               itr != cataList.end() ;
               itr++ )
         {
            optCB->setCatAddr( itr->_host, itr->_service[
               MSG_ROUTE_CAT_SERVICE].c_str() ) ;
         }
         optCB->reflush2File() ;
         sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTCAGP_EXE, rc ) ;
      return rc;
   error:
      // clear the catalog-group info
      if ( rc != SDB_COORD_RECREATE_CATALOG )
      {
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->clearCatNodeAddrList() ;
         sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;

         CoordGroupInfo *pEmptyGroupInfo = NULL ;
         pEmptyGroupInfo = SDB_OSS_NEW CoordGroupInfo( CAT_CATALOG_GROUPID ) ;
         if ( NULL != pEmptyGroupInfo )
         {
            CoordGroupInfoPtr groupInfo( pEmptyGroupInfo ) ;
            sdbGetCoordCB()->updateCatGroupInfo( groupInfo ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTCAGP_GETNDCF, "rtnCoordCMDCreateCataGroup::getNodeConf" )
   INT32 rtnCoordCMDCreateCataGroup::getNodeConf( CHAR *pQuery,
                                                  BSONObj &boNodeConfig )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTCAGP_GETNDCF ) ;

      pmdOptionsCB *option = pmdGetOptionCB() ;
      std::string clusterName ;
      std::string businessName ;
      option->getFieldStr( PMD_OPTION_CLUSTER_NAME, clusterName, "" ) ;
      option->getFieldStr( PMD_OPTION_BUSINESS_NAME, businessName, "" ) ;

      try
      {
         std::string strCataHostName ;
         std::string strSvcName ;
         std::string strCataSvc ;
         CoordVecNodeInfo cataNodeLst ;

         BSONObj boInput( pQuery ) ;
         BSONObjBuilder bobNodeConf ;
         BSONObjIterator iter( boInput ) ;

         // loop through each input parameter
         while ( iter.more() )
         {
            BSONElement beField = iter.next() ;
            std::string strFieldName( beField.fieldName() ) ;

            // make sure to skip hostname, group name, and role
            if ( strFieldName == FIELD_NAME_HOST )
            {
               strCataHostName = beField.str();
               continue;
            }
            else if ( strFieldName == FIELD_NAME_GROUPNAME ||
                      strFieldName == PMD_OPTION_ROLE ||
                      strFieldName == PMD_OPTION_CATALOG_ADDR ||
                      strFieldName == PMD_OPTION_CLUSTER_NAME ||
                      strFieldName == PMD_OPTION_BUSINESS_NAME )
            {
               continue;
            }
            else if ( strFieldName == PMD_OPTION_CATANAME )
            {
               strCataSvc = beField.str() ;
            }
            else if ( strFieldName == PMD_OPTION_SVCNAME )
            {
               strSvcName = beField.str() ;
            }

            // append into beField
            bobNodeConf.append( beField ) ;
         }

         if ( strSvcName.empty() )
         {
            PD_LOG( PDERROR, "Service name can't be empty" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( strCataSvc.empty() )
         {
            UINT16 svcPort = 0 ;
            ossGetPort( strSvcName.c_str(), svcPort ) ;
            CHAR szPort[ 10 ] = { 0 } ;
            ossItoa( svcPort + MSG_ROUTE_CAT_SERVICE , szPort, 10 ) ;
            strCataSvc = szPort ;
         }

         // assign role
         bobNodeConf.append ( PMD_OPTION_ROLE, SDB_ROLE_CATALOG_STR ) ;
         if ( !clusterName.empty() )
         {
            bobNodeConf.append ( PMD_OPTION_CLUSTER_NAME, clusterName ) ;
         }
         if ( !businessName.empty() )
         {
            bobNodeConf.append ( PMD_OPTION_BUSINESS_NAME, businessName ) ;
         }

         // assign catalog address, make sure to include all catalog nodes
         // that configured in the system ( for HA ), each system should be
         // separated by "," and sit in a single key: PMD_OPTION_CATALOG_ADDR
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->getCatNodeAddrList( cataNodeLst ) ;

         // already exist catalog group
         if ( cataNodeLst.size() > 0 )
         {
            rc = SDB_COORD_RECREATE_CATALOG ;
            PD_LOG( PDERROR, "Repeat to create catalog-group" ) ;
            sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;
            goto error ;
         }
         else
         {
            std::string strCataNodeLst = strCataHostName + ":" + strCataSvc ;
            MsgRouteID routeID ;
            routeID.columns.groupID = CATALOG_GROUPID ;
            routeID.columns.nodeID = SYS_NODE_ID_BEGIN ;
            routeID.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
            sdbGetCoordCB()->addCatNodeAddr( routeID, strCataHostName.c_str(),
                                             strCataSvc.c_str() ) ;
            sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;

            bobNodeConf.append( PMD_OPTION_CATALOG_ADDR, strCataNodeLst ) ;
         }

         boNodeConfig = bobNodeConf.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTCAGP_GETNDCF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTCAGP_GETNDINFO, "rtnCoordCMDCreateCataGroup::getNodeInfo" )
   INT32 rtnCoordCMDCreateCataGroup::getNodeInfo( CHAR *pQuery, BSONObj &boNodeInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTCAGP_GETNDINFO ) ;

      try
      {
         BSONObj boConf( pQuery ) ;
         BSONObjBuilder bobNodeInfo ;
         BSONElement beHostName = boConf.getField( FIELD_NAME_HOST ) ;
         PD_CHECK( beHostName.type()==String, SDB_INVALIDARG, error, PDERROR,
                  "Failed to get the field(%s)", FIELD_NAME_HOST ) ;
         bobNodeInfo.append( beHostName ) ;

         BSONElement beDBPath = boConf.getField( PMD_OPTION_DBPATH ) ;
         PD_CHECK( beDBPath.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", PMD_OPTION_DBPATH );
         bobNodeInfo.append( beDBPath ) ;

         BSONElement beLocalSvc = boConf.getField( PMD_OPTION_SVCNAME ) ;
         PD_CHECK( beLocalSvc.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", PMD_OPTION_SVCNAME ) ;
         bobNodeInfo.append( beLocalSvc ) ;

         BSONElement beReplSvc = boConf.getField( PMD_OPTION_REPLNAME );
         if ( beReplSvc.type() == String )
         {
            bobNodeInfo.append( beReplSvc ) ;
         }

         BSONElement beShardSvc = boConf.getField( PMD_OPTION_SHARDNAME ) ;
         if ( beShardSvc.type() == String )
         {
            bobNodeInfo.append( beShardSvc ) ;
         }

         BSONElement beCataSvc = boConf.getField( PMD_OPTION_CATANAME ) ;
         if ( beCataSvc.type() == String )
         {
            bobNodeInfo.append( beCataSvc ) ;
         }
         boNodeInfo = bobNodeInfo.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Occured unexpected error:%s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTCAGP_GETNDINFO, rc ) ;
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordCMDTraceStart::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      contextID = -1 ;

      INT32 flag;
      CHAR *pCMDName;
      SINT64 numToSkip;
      SINT64 numToReturn;
      CHAR *pQuery;
      CHAR *pFieldSelector;
      CHAR *pOrderBy;
      CHAR *pHint;
      _rtnTraceStart tracestart ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = tracestart.init ( flag, numToSkip, numToReturn, pQuery,
                             pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestart, rc = %d", rc ) ;
      rc = tracestart.doit ( cb, NULL, NULL, NULL, 0, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestart, rc = %d", rc ) ;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordCMDTraceResume::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      contextID = -1 ;

      INT32 flag;
      CHAR *pCMDName;
      SINT64 numToSkip;
      SINT64 numToReturn;
      CHAR *pQuery;
      CHAR *pFieldSelector;
      CHAR *pOrderBy;
      CHAR *pHint;
      _rtnTraceResume traceResume ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = traceResume.init ( flag, numToSkip, numToReturn, pQuery,
                             pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestart, rc = %d", rc ) ;
      rc = traceResume.doit ( cb, NULL, NULL, NULL, 0, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestart, rc = %d", rc ) ;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordCMDTraceStop::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      contextID = -1 ;

      INT32 flag;
      CHAR *pCMDName;
      SINT64 numToSkip;
      SINT64 numToReturn;
      CHAR *pQuery;
      CHAR *pFieldSelector;
      CHAR *pOrderBy;
      CHAR *pHint;
      _rtnTraceStop tracestop ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = tracestop.init ( flag, numToSkip, numToReturn, pQuery,
                            pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestop, rc = %d", rc ) ;
      rc = tracestop.doit ( cb, NULL, NULL, NULL, 0, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestop, rc = %d", rc ) ;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordCMDTraceStatus::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB();
      contextID = -1 ;

      INT32 flag;
      CHAR *pCMDName;
      SINT64 numToSkip;
      SINT64 numToReturn;
      CHAR *pQuery;
      CHAR *pFieldSelector;
      CHAR *pOrderBy;
      CHAR *pHint;
      _rtnTraceStatus tracestatus ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = tracestatus.init ( flag, numToSkip, numToReturn, pQuery,
                              pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestop, rc = %d", rc ) ;
      rc = tracestatus.doit ( cb, NULL, pRtncb, NULL, 0, &contextID ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestop, rc = %d", rc ) ;

   done:
      return rc;
   error:
      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done;
   }

   INT32 rtnCoordCMDExpConfig::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      ROUTE_RC_MAP faileds ;
      rtnCoordCtrlParam ctrlParam ;

      ctrlParam._isGlobal = FALSE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      contextID = -1 ;

      rc = executeOnNodes( pMsg, cb, ctrlParam, RTN_CTRL_MASK_ALL, faileds ) ;
      if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
      {
         /// submmit to local command
         rc = SDB_COORD_UNKNOWN_OP_REQ ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Execute on nodes failed, rc: %d", rc ) ;

      /// do on local
      rc = pmdGetKRCB()->getOptionCB()->reflush2File() ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Flush local config to file failed, rc: %d", rc ) ;
         rc = SDB_RTN_EXPORTCONF_NOT_COMPLETE ;
         goto error ;
      }

      if ( faileds.size() > 0 )
      {
         rc = SDB_RTN_EXPORTCONF_NOT_COMPLETE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordAggrCmdBase::appendObjs( const CHAR *pInputBuffer,
                                          CHAR *&pOutputBuffer,
                                          INT32 &bufferSize,
                                          INT32 &bufUsed,
                                          INT32 &buffObjNum )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pEnd = pInputBuffer ;
      string strline ;
      BSONObj obj ;

      while ( *pEnd != '\0' )
      {
         strline.clear() ;
         while( *pEnd && *pEnd != '\r' && *pEnd != '\n' )
         {
            strline += *pEnd ;
            ++pEnd ;
         }

         if ( strline.empty() )
         {
            ++pEnd ;
            continue ;
         }

         rc = fromjson( strline, obj ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse string[%s] to json failed, rc: %d",
                      strline.c_str(), rc ) ;

         rc = appendObj( obj, pOutputBuffer, bufferSize,
                         bufUsed, buffObjNum ) ;
         PD_RC_CHECK( rc, PDERROR, "Append obj failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordCMDSnapShotBase::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      CHAR *pOutBuff = NULL ;
      INT32 buffSize = 0 ;
      INT32 buffUsedSize = 0 ;
      INT32 buffObjNum = 0 ;

      rtnQueryOptions queryOption ;
      rtnCoordCtrlParam ctrlParam ;
      vector< BSONObj > vecUserAggr ;

      contextID = -1 ;

      rc = queryOption.fromQueryMsg( (CHAR*)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract command failed, rc: %d", rc ) ;

      rc = rtnCoordParseControlParam( queryOption._query, ctrlParam,
                                      RTN_CTRL_MASK_RAWDATA ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse control param failed, rc: %d", rc ) ;

      rc = parseUserAggr( queryOption._hint, vecUserAggr ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse user define aggr[%s] failed, rc: %d",
                   queryOption._hint.toString().c_str(), rc ) ;

      if ( !ctrlParam._rawData || vecUserAggr.size() > 0 )
      {
         /// add aggr operators
         BSONObj nodeMatcher ;
         BSONObj newMatcher ;
         rc = parseMatcher( queryOption._query, nodeMatcher, newMatcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse matcher failed, rc: %d", rc ) ;

         /// add nodes matcher to the botton
         if ( !nodeMatcher.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_MATCH_PARSER_NAME << nodeMatcher ),
                            pOutBuff, buffSize, buffUsedSize,
                            buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append node matcher failed, rc: %d",
                         rc ) ;
         }

         if ( !ctrlParam._rawData )
         {
            rc = appendObjs( getInnerAggrContent(), pOutBuff, buffSize,
                             buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append objs[%s] failed, rc: %d",
                         getInnerAggrContent(), rc ) ;
         }

         /// add new matcher
         if ( !newMatcher.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_MATCH_PARSER_NAME << newMatcher ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append new matcher failed, rc: %d",
                         rc ) ;
         }

         /// order by
         if ( !queryOption._orderBy.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_SORT_PARSER_NAME <<
                                  queryOption._orderBy ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append order by failed, rc: %d",
                         rc ) ;
         }

         for ( UINT32 i = 0 ; i < vecUserAggr.size() ; ++i )
         {
            rc = appendObj( vecUserAggr[ i ], pOutBuff, buffSize,
                            buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append user define aggr[%s] failed, "
                         "rc: %d", vecUserAggr[ i ].toString().c_str(),
                         rc ) ;
         }

         /// open context
         rc = openContext( pOutBuff, buffObjNum, getIntrCMDName(),
                           queryOption._selector, cb, contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;
      }
      else
      {
         CoordCB *pCoordCB = pmdGetKRCB()->getCoordCB() ;
         rtnCoordProcesserFactory *pCmdFactor = NULL ;
         rtnCoordCommand *pCmd = NULL ;
         pCmdFactor = pCoordCB->getProcesserFactory() ;
         pCmd = pCmdFactor->getCommandProcesser( getIntrCMDName() ) ;
         if ( !pCmd )
         {
            PD_LOG( PDERROR, "Get command[%s] failed", getIntrCMDName() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         rc = pCmd->execute( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( pOutBuff )
      {
         SDB_OSS_FREE( pOutBuff ) ;
         pOutBuff = NULL ;
         buffSize = 0 ;
         buffUsedSize = 0 ;
      }
      if ( -1 != contextID && !_useContext() )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done;
   }

   const CHAR* rtnCoordCMDSnapshotDataBase::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTDBINTR;
   }

   const CHAR* rtnCoordCMDSnapshotDataBase::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTDB_INPUT ;
   }

   const CHAR* rtnCoordCMDSnapshotSystem::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTSYSINTR;
   }

   const CHAR* rtnCoordCMDSnapshotSystem::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTSYS_INPUT ;
   }

   const CHAR* rtnCoordCMDSnapshotCollections::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTCLINTR;
   }

   const CHAR* rtnCoordCMDSnapshotCollections::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTCL_INPUT ;
   }

   const CHAR* rtnCoordCMDSnapshotSpaces::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTCSINTR;
   }

   const CHAR* rtnCoordCMDSnapshotSpaces::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTCS_INPUT ;
   }

   const CHAR* rtnCoordCMDSnapshotContexts::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTCTXINTR;
   }

   const CHAR* rtnCoordCMDSnapshotContexts::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTCONTEXTS_INPUT ;
   }

   const CHAR* rtnCoordCMDSnapshotContextsCur::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTCTXCURINTR;
   }

   const CHAR* rtnCoordCMDSnapshotContextsCur::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTCONTEXTSCUR_INPUT ;
   }

   const CHAR* rtnCoordCMDSnapshotSessions::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTSESSINTR;
   }

   const CHAR* rtnCoordCMDSnapshotSessions::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTSESS_INPUT ;
   }

   const CHAR* rtnCoordCMDSnapshotSessionsCur::getIntrCMDName()
   {
      return COORD_CMD_SNAPSHOTSESSCURINTR;
   }

   const CHAR* rtnCoordCMDSnapshotSessionsCur::getInnerAggrContent()
   {
      return RTNCOORD_SNAPSHOTSESSCUR_INPUT ;
   }

   INT32 rtnCoordCMDSnapshotCata::_preProcess( rtnQueryOptions &queryOpt,
                                               string &clName )
   {
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCMDCRTPROCEDURE_EXE, "rtnCoordCMDCrtProcedure::execute" )
   INT32 rtnCoordCMDCrtProcedure::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB_RTNCOORDCMDCRTPROCEDURE_EXE) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_CRT_PROCEDURES_REQ ;

      _printDebug ( (CHAR*)pMsg, "rtnCoordCMDCrtProcedure" ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to crt procedures, rc = %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC(SDB_RTNCOORDCMDCRTPROCEDURE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDEVAL_EXE, "rtnCoordCMDEval::execute" )
   INT32 rtnCoordCMDEval::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCMDEVAL_EXE ) ;
      spdSession *session = NULL ;
      contextID           = -1 ;

      CHAR *pQuery = NULL ;
      BSONObj procedures ;
      spcCoordDownloader downloader( this, cb ) ;
      BSONObj runInfo ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery, NULL,
                            NULL, NULL );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract eval msg:%d", rc) ;
         goto error ;
      }

      try
      {
         procedures = BSONObj( pQuery ) ;
         PD_LOG( PDDEBUG, "eval:%s", procedures.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      session = SDB_OSS_NEW _spdSession() ;
      if ( NULL == session )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = session->eval( procedures, &downloader, cb ) ;
      if ( SDB_OK != rc )
      {
         const BSONObj &errmsg = session->getErrMsg() ;
         if ( !errmsg.isEmpty() )
         {
            *buf = rtnContextBuf( errmsg.getOwned() ) ;
         }
         PD_LOG( PDERROR, "failed to eval store procedure:%d", rc ) ;
         goto error ;
      }

      if ( FMP_RES_TYPE_VOID != session->resType() )
      {
         rc = _buildContext( session, cb, contextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to prepare reply msg:%d", rc ) ;
            goto error ;
         }
      }

      runInfo = BSON( FIELD_NAME_RTYPE << session->resType() ) ;
      *buf = rtnContextBuf( runInfo ) ;

   done:
      /// when -1 != contextID, session will be freed
      /// in context destructor.
      if ( -1 == contextID )
      {
         SAFE_OSS_DELETE( session ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOCMDEVAL_EXE, rc ) ;
      return rc ;
   error:
      if ( contextID >= 0 )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 rtnCoordCMDEval::_buildContext( _spdSession *session,
                                         pmdEDUCB *cb,
                                         SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      const BSONObj &evalRes = session->getRetMsg() ;
      SDB_ASSERT( !evalRes.isEmpty(), "impossible" ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextSP *context = NULL ;
      rc = rtnCB->contextNew ( RTN_CONTEXT_SP, (rtnContext**)&context,
                               contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new context, rc: %d", rc ) ;

      rc = context->open( session ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context[%lld], rc: %d",
                   context->contextID(), rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCMDRMPROCEDURE_EXE, "rtnCoordCMDRmProcedure::execute" )
   INT32 rtnCoordCMDRmProcedure::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB_RTNCOORDCMDRMPROCEDURE_EXE) ;
      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_RM_PROCEDURES_REQ ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to rm procedures, rc = %d",
                  rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC(SDB_RTNCOORDCMDRMPROCEDURE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordCMDListProcedures::_preProcess( rtnQueryOptions &queryOpt,
                                                 string &clName )
   {
      clName = CAT_PROCEDURES_COLLECTION ;
      return SDB_OK ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDLINKCL_EXE, "rtnCoordCMDLinkCollection::execute" )
   INT32 rtnCoordCMDLinkCollection::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDLINKCL_EXE ) ;

      CHAR *pQuery                     = NULL;
      string mainCLName ;
      string strSubClName ;
      CoordGroupList groupLst ;

      // fill default-reply(delete success)
      contextID = -1 ;

      MsgOpQuery *pLinkReq           = (MsgOpQuery *)pMsg;

      try
      {
         rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                               NULL, NULL, &pQuery, NULL,
                               NULL, NULL ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to parse link collection request(rc=%d)",
                     rc );
            goto error ;
         }
         BSONObj boQuery( pQuery );
         {
            BSONElement beClNameTmp = boQuery.getField( CAT_SUBCL_NAME );
            PD_CHECK( beClNameTmp.type() == String, SDB_INVALIDARG, error,
                      PDERROR, "Failed to unlink collection, failed to get "
                      "sub-collection name" );
            strSubClName = beClNameTmp.str() ;
            PD_CHECK( !strSubClName.empty(), SDB_INVALIDARG, error, PDERROR,
                      "sub collection name can't be empty!" );

            BSONElement beMainCLName = boQuery.getField( CAT_COLLECTION_NAME );
            PD_CHECK( beMainCLName.type() == String, SDB_INVALIDARG, error,
                      PDERROR, "Failed to get the field(%s)",
                      CAT_COLLECTION_NAME );
            mainCLName = beMainCLName.str();
            PD_CHECK( !mainCLName.empty(), SDB_INVALIDARG,
                      error, PDERROR, "main collection name can't be empty!" ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      // send request to catalog
      pLinkReq->header.opCode        = MSG_CAT_LINK_CL_REQ;
      rc = executeOnCataGroup ( pMsg, cb, &groupLst ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "failed to execute on catalog(rc=%d)",
                   rc ) ;

      //send request to data-node
      pLinkReq->header.opCode        = MSG_BS_QUERY_REQ ;
      rc = executeOnCL( pMsg, cb, mainCLName.c_str(), TRUE,
                        &groupLst, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Link collection[MainCL: %s, subCL: %s] on "
                 "data node failed, rc: %d", mainCLName.c_str(),
                 strSubClName.c_str(), rc ) ;
         goto error_rollback ;
      }

   done :
      if ( !mainCLName.empty() && !strSubClName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_LINK_CL, AUDIT_OBJ_CL,
                           mainCLName.c_str(), rc, "Option:%s",
                           BSONObj(pQuery).toString().c_str() ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDLINKCL_EXE, rc ) ;
      return rc ;
   error_rollback:
      {
         INT32 rcRBk = SDB_OK;
         pMsg->opCode = MSG_CAT_UNLINK_CL_REQ ;
         rcRBk = executeOnCataGroup ( pMsg,  cb, &groupLst ) ;
         PD_RC_CHECK( rcRBk, PDERROR, "Failed to execute on catalog(rc=%d), "
                      "rollback failed!", rcRBk ) ;
      }
   error :
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDUNLINKCL_EXE, "rtnCoordCMDUnlinkCollection::execute" )
   INT32 rtnCoordCMDUnlinkCollection::execute( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               INT64 &contextID,
                                               rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDUNLINKCL_EXE ) ;

      // fill default-reply(delete success)
      contextID = -1 ;

      MsgOpQuery *pReqMsg              = (MsgOpQuery *)pMsg;

      CoordGroupList groupLst ;
      CHAR *pQuery                     = NULL ;
      string strMainCLName ;
      string strSubClName ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL,
                            NULL, &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse unlink collection request(rc=%d)",
                   rc ) ;

      try
      {
         BSONObj boQuery = BSONObj( pQuery );
         BSONElement beClNameTmp = boQuery.getField( CAT_SUBCL_NAME );
         PD_CHECK( beClNameTmp.type() == String, SDB_INVALIDARG, error,
                   PDERROR, "Failed to unlink collection, failed to get "
                   "sub-collection name" );
         strSubClName = beClNameTmp.str();

         beClNameTmp = boQuery.getField( CAT_COLLECTION_NAME );
         PD_CHECK( beClNameTmp.type() == String, SDB_INVALIDARG, error,
                   PDERROR, "Failed to unlink collection, failed to get "
                   "sub-collection name" ) ;
         strMainCLName = beClNameTmp.str() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Failed to unlink collection, "
                 "received unexpected error:%s", e.what() );
         goto error;
      }

      // send request to catalog
      pReqMsg->header.opCode = MSG_CAT_UNLINK_CL_REQ ;
      rc = executeOnCataGroup ( pMsg, cb, &groupLst ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Unlink collection failed on catalog, rc = %d",
                  rc ) ;
         goto error ;
      }

      // restore opcode
      pReqMsg->header.opCode = MSG_BS_QUERY_REQ ;
      rc = executeOnCL( pMsg, cb, strMainCLName.c_str(), TRUE,
                        &groupLst, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to unlink collection"
                   "(MainCL:%s, subCL:%s), execute on data-node failed, "
                   "rc: %d", strMainCLName.c_str(),
                   strSubClName.c_str(), rc ) ;

   done:
      if ( !strMainCLName.empty() && !strSubClName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_UNLINK_CL, AUDIT_OBJ_CL,
                           strMainCLName.c_str(), rc, "Option:%s",
                           BSONObj(pQuery).toString().c_str() ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDUNLINKCL_EXE, rc ) ;
      return rc;
   error :
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDSETSESSATTR_EXE, "rtnCoordCMDSetSessionAttr::execute" )
   INT32 rtnCoordCMDSetSessionAttr::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDSETSESSATTR_EXE ) ;
      // fill default-reply(delete success)
      contextID = -1 ;

      CHAR *pQuery                     = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse unlink collection request(rc=%d)",
                   rc ) ;

      try
      {
         CoordSession *pSession = NULL;
         BSONObj boQuery ;
         BSONElement bePreferRepl ;
         INT32 sessReplType = PREFER_REPL_TYPE_MIN ;
         GROUP_VEC groupLstTmp ;

         pSession = cb->getCoordSession();
         PD_CHECK( pSession != NULL, SDB_SYS, error, PDERROR,
                   "Failed to get session!" ) ;
         boQuery = BSONObj( pQuery );
         bePreferRepl = boQuery.getField( FIELD_NAME_PREFERED_INSTANCE );
         PD_CHECK( bePreferRepl.type() == NumberInt, SDB_INVALIDARG, error,
                   PDERROR, "Failed to set session attribute, failed to get "
                   "the field(%s)", FIELD_NAME_PREFERED_INSTANCE );
         sessReplType = bePreferRepl.Int();
         PD_CHECK( sessReplType > PREFER_REPL_TYPE_MIN &&
                   sessReplType < PREFER_REPL_TYPE_MAX,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to set prefer-replica-type, invalid value!"
                   "(range:%d~%d)", PREFER_REPL_TYPE_MIN,
                   PREFER_REPL_TYPE_MAX ) ;
         pSession->setPreferReplType( sessReplType ) ;

         rc = rtnCoordGetAllGroupList( cb, groupLstTmp );
         PD_RC_CHECK( rc, PDERROR, "Failed to update all group info!(rc=%d)",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Failed to unlink collection, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDSETSESSATTR_EXE, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDCREATEDOMAIN_EXE, "rtnCoordCMDCreateDomain::execute" )
   INT32 rtnCoordCMDCreateDomain::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCREATEDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
      forward->header.opCode = MSG_CAT_CREATE_DOMAIN_REQ;

      _printDebug ( (CHAR*)pMsg, "rtnCoordCMDCreateDomain" ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to create domain, rc = %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCREATEDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDDROPDOMAIN_EXE, "rtnCoordCMDDropDomain::execute" )
   INT32 rtnCoordCMDDropDomain::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDDROPDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_DROP_DOMAIN_REQ;

      _printDebug ( (CHAR*)pMsg, "rtnCoordCMDDropDomain" ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to drop domain, rc = %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDDROPDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDALTERDOMAIN_EXE, "rtnCoordCMDAlterDomain::execute" )
   INT32 rtnCoordCMDAlterDomain::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDALTERDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
      forward->header.opCode = MSG_CAT_ALTER_DOMAIN_REQ;

      _printDebug ( (CHAR*)pMsg, "rtnCoordCMDAlterDomain" ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to alter domain, rc = %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDALTERDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDADDDOMAINGROUP_EXE, "rtnCoordCMDAddDomainGroup::execute" )
   INT32 rtnCoordCMDAddDomainGroup::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDREMOVEDOMAINGROUP_EXE, "rtnCoordCMDRemoveDomainGroup::execute" )
   INT32 rtnCoordCMDRemoveDomainGroup::execute( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                INT64 &contextID,
                                                rtnContextBuf *buf )
   {
      return SDB_OK ;
   }

   INT32 rtnCoordCMDListDomains::_preProcess( rtnQueryOptions &queryOpt,
                                              string &clName )
   {
      clName = CAT_DOMAIN_COLLECTION ;
      return SDB_OK ;
   }

   INT32 rtnCoordCMDListCSInDomain::_preProcess( rtnQueryOptions &queryOpt,
                                                 string &clName )
   {
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDLISTCLINDOMAIN_EXECUTE, "rtnCoordCMDListCLInDomain::execute" )
   INT32 rtnCoordCMDListCLInDomain::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDLISTCLINDOMAIN_EXECUTE ) ;
      BSONObj conObj ;
      BSONObj selObj ;
      BSONObj dummy ;
      CHAR *query = NULL ;
      CHAR *selector = NULL ;
      BSONElement domain ;
      CHAR *msgBuf = NULL ;
      rtnQueryOptions queryOptions ;

      vector<BSONObj> replyFromCata ;

      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &query,
                            &selector, NULL, NULL );
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "failed to parse query request(rc=%d)", rc ) ;
         goto error ;
      }

      try
      {
         conObj = BSONObj( query ) ;
         selObj = BSONObj( selector ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      domain = conObj.getField( FIELD_NAME_DOMAIN ) ;
      if ( String != domain.type() )
      {
         PD_LOG( PDERROR, "invalid domain field in object:%s",
                  conObj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      queryOptions._query = BSON( CAT_DOMAIN_NAME << domain.valuestr() ) ;
      queryOptions._fullName = CAT_COLLECTION_SPACE_COLLECTION ;

      rc = queryOnCataAndPushToVec( queryOptions, cb, replyFromCata ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute query on catalog:%d", rc ) ;
         goto error ;
      }

      {
         rc = _rebuildListResult( replyFromCata, cb, contextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to rebuild list result:%d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( NULL != msgBuf )
      {
         SDB_OSS_FREE( msgBuf ) ;
         msgBuf = NULL ;
      }
      PD_TRACE_EXITRC( CMD_RTNCOCMDLISTCLINDOMAIN_EXECUTE, rc )  ;
      return rc ;
   error:
      if ( contextID >= 0 )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDLISTCLINDOMAIN__REBUILDRESULT, "rtnCoordCMDListCLInDomain::_rebuildListResult" )
   INT32 rtnCoordCMDListCLInDomain::_rebuildListResult(
                                    const vector<BSONObj> &infoFromCata,
                                    pmdEDUCB *cb,
                                    SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDLISTCLINDOMAIN__REBUILDRESULT ) ;
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP,
                              &context,
                              contextID,
                              cb ) ;
      if  ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create new context:%d", rc ) ;
         goto error ;
      }

      rc = (( rtnContextDump * )context)->open( BSONObj(), BSONObj() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open context:%d", rc ) ;
         goto error ;
      }

      for ( vector<BSONObj>::const_iterator itr = infoFromCata.begin();
            itr != infoFromCata.end();
            itr++ )
      {
         BSONElement cl ;
         BSONElement cs = itr->getField( FIELD_NAME_NAME ) ;
         if ( String != cs.type() )
         {
            PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         cl = itr->getField( FIELD_NAME_COLLECTION ) ;
         if ( Array != cl.type() )
         {
            PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;

         }

         {
         BSONObjIterator clItr( cl.embeddedObject() ) ;
         while ( clItr.more() )
         {
            stringstream ss ;
            BSONElement clName ;
            BSONElement oneCl = clItr.next() ;
            if ( Object != oneCl.type() )
            {
               PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            clName = oneCl.embeddedObject().getField( FIELD_NAME_NAME ) ;
            if ( String != clName.type() )
            {
               PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            ss << cs.valuestr() << "." << clName.valuestr() ;
            context->append( BSON( FIELD_NAME_NAME << ss.str() ) ) ;
         }
         }
      }
   done:
      PD_TRACE_EXITRC( CMD_RTNCOCMDLISTCLINDOMAIN__REBUILDRESULT, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDINVALIDATECACHE_EXEC, "rtnCoordCMDInvalidateCache::execute" )
   INT32 rtnCoordCMDInvalidateCache::execute( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              INT64 &contextID,
                                              rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDINVALIDATECACHE_EXEC ) ;

      ROUTE_RC_MAP uncompleted ;
      rtnCoordCtrlParam ctrlParam ;

      contextID = -1 ;
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;

      /// invalidate local catalog cache and group cache
      sdbGetCoordCB()->invalidateCataInfo() ;
      sdbGetCoordCB()->invalidateGroupInfo() ;

      rc = executeOnNodes( pMsg, cb, ctrlParam, RTN_CTRL_MASK_ALL,
                           uncompleted ) ;
      if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Execute on nodes failed, rc: %d", rc ) ;

      if ( !uncompleted.empty() )
      {
         rc = SDB_COORD_NOT_ALL_DONE ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( CMD_RTNCOCMDINVALIDATECACHE_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDLISTLOBS_EXEC, "rtnCoordListLobs::execute" )
   INT32 rtnCoordCMDListLobs::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDLISTLOBS_EXEC ) ;

      CHAR *pQuery = NULL ;
      BSONObj query ;

      rtnContextCoord *context = NULL ;
      rtnCoordQuery queryOpr( isReadonly() ) ;
      rtnQueryConf queryConf ;
      rtnSendOptions sendOpt ;

      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      CoordCB *pCoordcb = pmdGetKRCB()->getCoordCB() ;
      netMultiRouteAgent *pRouteAgent = pCoordcb->getRouteAgent() ;

      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL ) ;

      PD_RC_CHECK( rc, PDERROR, "Snapshot failed, failed to parse query "
                   "request(rc=%d)", rc ) ;

      try
      {
         query = BSONObj( pQuery ) ;
         BSONElement ele = query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid obj of list lob:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         queryConf._realCLName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      queryConf._openEmptyContext = TRUE ;
      queryConf._allCataGroups = TRUE ;
      rc = queryOpr.queryOrDoOnCL( pMsg, pRouteAgent, cb, &context,
                                   sendOpt, &queryConf ) ;
      PD_RC_CHECK( rc, PDERROR, "List lobs[%s] on groups failed, rc: %d",
                   queryConf._realCLName.c_str(), rc ) ;

      // set context id
      contextID = context->contextID() ;

   done:
      PD_TRACE_EXITRC( CMD_RTNCOCMDLISTLOBS_EXEC, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         pRtncb->contextDelete( context->contextID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDREELECTION_EXEC, "rtnCoordCMDReelection::execute" )
   INT32 rtnCoordCMDReelection::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDREELECTION_EXEC ) ;
      CHAR *pQuery = NULL ;
      BSONObj query ;
      BSONElement ele ;
      const CHAR *gpName = NULL ;
      CoordGroupInfoPtr gpInfo ;
      CoordGroupList gpLst ;
      GROUP_VEC gpVec ;
      BSONObj obj ;

      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse the "
                   "reelection-message(rc=%d)", rc ) ;

      try
      {
         query = BSONObj( pQuery ) ;
         ele = query.getField( FIELD_NAME_GROUPNAME ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid reelection msg:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         gpName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         goto error ;
      }
      PD_LOG( PDERROR, "Get groupName: %s", gpName ) ;
      rc = rtnCoordGetGroupInfo( cb, gpName, FALSE, gpInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get info of group[%s], rc:%d",
                 gpName, rc ) ;
         goto error ;
      }

      gpLst[gpInfo->groupID()] = gpInfo->groupID() ;
      PD_LOG( PDERROR, "Start Do on data group" ) ;
      rc = executeOnDataGroup( pMsg, cb, gpLst, TRUE, NULL, NULL, NULL ) ;
      PD_LOG( PDERROR , "End do on data group" ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute on group[%s], rc:%d",
                 gpName, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( CMD_RTNCOCMDREELECTION_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDTRUNCATE_EXEC, "rtnCoordCMDTruncate::execute" )
   INT32 rtnCoordCMDTruncate::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDTRUNCATE_EXEC ) ;
      CHAR *option = NULL;
      BSONObj boQuery ;
      const CHAR *fullName = NULL ;
      rc = msgExtractQuery( ( CHAR * )pMsg, NULL, NULL,
                            NULL, NULL, &option, NULL,
                            NULL, NULL );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         boQuery = BSONObj( option );
         BSONElement e = boQuery.getField( FIELD_NAME_COLLECTION );
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "invalid truncate msg:%s",
                    boQuery.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         fullName = e.valuestr() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error;
      }

      rc = executeOnCL( pMsg, cb, fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate cl:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }
   done:
      if ( fullName )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_TRUNCATE, AUDIT_OBJ_CL,
                           fullName, rc, "" ) ;
      }
      PD_TRACE_EXITRC( CMD_RTNCOCMDTRUNCATE_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDSYNCDB_EXEC, "rtnCoordCMDSyncDB::execute" )
   INT32 rtnCoordCMDSyncDB::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDSYNCDB_EXEC ) ;

      rc = _syncDB( pMsg, cb, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to sync db:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( CMD_RTNCOCMDSYNCDB_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDSYNCDB__SYNCDB, "rtnCoordCMDSyncDB::_syncDB" )
   INT32 rtnCoordCMDSyncDB::_syncDB( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( CMD_RTNCOCMDSYNCDB__SYNCDB ) ;
      ROUTE_RC_MAP errNodes ;
      rtnContextCoord *context = NULL ;
      rtnCoordCtrlParam ctrlParam ;

      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;

      rc = executeOnNodes( pMsg, cb, ctrlParam,
                           RTN_CTRL_MASK_ALL, errNodes, &context ) ;
      if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
      {
         PD_LOG( PDERROR, "options of node is necessary when db is coord" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute on nodes:%d", rc ) ;
         goto error ;
      }
      else
      {
         /// do nothing.
      }

      if ( NULL != context )
      {
         contextID = context->contextID() ;
      }

/*
      if ( !errNodes.empty() )
      {
         rc = SDB_COORD_NOT_ALL_DONE ;
         goto error ;
      }
*/

   done:
      PD_TRACE_EXITRC( CMD_RTNCOCMDSYNCDB__SYNCDB, rc ) ;
      return rc ;
   error:
      if ( SDB_COORD_NOT_ALL_DONE != rc && NULL != context )
      {
         sdbGetRTNCB()->contextDelete( context->contextID(), cb ) ;
         contextID = -1 ;
         context = NULL ;
      }
      goto done ;
   }

   INT32 rtnCoordCMDQueryOnMain::execute( MsgHeader * pMsg,
                                          pmdEDUCB * cb,
                                          INT64& contextID,
                                          rtnContextBuf * buf )
   {
      INT32 rc = SDB_OK ;
      rtnCoordCtrlParam ctrlParam ;
      ROUTE_RC_MAP faileds ;
      rtnContextCoord *pContext = NULL ;

      contextID = -1 ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;

      rc = executeOnNodes( pMsg, cb, ctrlParam, RTN_CTRL_MASK_ALL,
                           faileds, &pContext, FALSE, NULL, NULL ) ;
      if ( rc )
      {
         if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
         {
            rc = SDB_COORD_UNKNOWN_OP_REQ ;
         }
         else
         {
            PD_LOG( PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
         }
         goto error ;
      }

      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordSnapshotTransCur::getGroups( pmdEDUCB *cb,
                                              CoordGroupList &groupList )
   {
      INT32 rc = SDB_OK ;
      DpsTransNodeMap *pTransNodeMap = NULL ;
      DpsTransNodeMap::iterator iter ;

      pTransNodeMap = cb->getTransNodeLst() ;
      if ( NULL == pTransNodeMap )
      {
         goto done ;
      }

      iter = pTransNodeMap->begin() ;
      while( iter != pTransNodeMap->end() )
      {
         groupList[iter->first] = iter->first ;
         ++iter ;
      }

   done:
      return rc ;
   }

   INT32 rtnCoordSnapshotTrans::getGroups( pmdEDUCB *cb,
                                           CoordGroupList &groupList )
   {
      INT32 rc = SDB_OK ;

      rc = rtnCoordGetAllGroupList( cb, groupList, NULL, FALSE, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get all groups(rc = %d)!", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

