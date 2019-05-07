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
               PD_LOG( PDDEBUG, "get group into list %.f", beGrpId.number() ) ;
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
         else
         {
            PD_LOG( PDWARNING,
                    "Error reply flag in data[node: %s, opCode: %d], rc: %d",
                    routeID2String( nodeID ).c_str(),
                    pReply->header.opCode, pReply->flags ) ;
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
            coordErrorInfo errInfo( pReply ) ;
            PD_LOG( ( pContext ? PDINFO : PDWARNING ),
                    "Failed to process reply[node: %s, flag: %d, obj: %s]",
                    routeID2String( nodeID ).c_str(), pReply->flags,
                    errInfo._obj.toString().c_str() ) ;
            faileds[ nodeID.value ] = errInfo ;
         }

         if ( !takeOver )
         {
            SDB_OSS_FREE( pReply ) ;
            pReply = NULL ;
         }
      }

      return rc ;
   }

   INT32 rtnCoordCommand::_buildFailedNodeReply( ROUTE_RC_MAP &failedNodes,
                                                 rtnContextCoord *pContext )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( pContext != NULL, "pContext can't be NULL!" ) ;

      if ( failedNodes.size() > 0 )
      {
         BSONObjBuilder builder ;
         rtnBuildFailedNodeReply( failedNodes, builder ) ;

         rc = pContext->append( builder.obj() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to append obj, rc: %d", rc ) ;
      }

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
                                            rtnContextCoord **ppContext,
                                            rtnContextBuf *buf )
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

      BOOLEAN preRead = _flagCoordCtxPreRead() ;

      sendOpt._groupLst = groupLst ;
      sendOpt._svcType = type ;
      sendOpt._primary = onPrimary ;
      sendOpt._pIgnoreRC = pIgnoreRC ;

      ROUTE_REPLY_MAP okReply ;
      ROUTE_REPLY_MAP nokReply ;
      result._pOkReply = &okReply ;
      result._pNokReply = &nokReply ;

      ROUTE_RC_MAP nokRC ;

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
         if ( !pTmpContext->isOpened() )
         {
            rc = pTmpContext->open( BSONObj(), BSONObj(), -1, 0, preRead ) ;
            PD_RC_CHECK( rc, PDERROR, "Open context failed(rc=%d)", rc ) ;
         }
      }

      rc = doOnGroups( inMsg, sendOpt, pRouteAgent, cb, result ) ;
      /// process succeed reply msg
      rcTmp = _processSucReply( okReply, pTmpContext ) ;
      /// build nokRC
      if ( nokReply.size() > 0 )
      {
         ROUTE_REPLY_MAP::iterator itNok = nokReply.begin() ;
         while( itNok != nokReply.end() )
         {
            MsgOpReply *pReply = (MsgOpReply*)itNok->second ;
            nokRC[ itNok->first ] = coordErrorInfo( pReply ) ;
            SDB_OSS_FREE( pReply ) ;
            ++itNok ;
         }
      }

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

      if ( preRead && pTmpContext )
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
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( rtnBuildErrorObj( rc, cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 rtnCoordCommand::executeOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               const CoordGroupList &groupLst,
                                               BOOLEAN onPrimary,
                                               SET_RC *pIgnoreRC,
                                               CoordGroupList *pSucGrpLst,
                                               rtnContextCoord **ppContext,
                                               rtnContextBuf *buf )
   {
      return _executeOnGroups( pMsg, cb, groupLst, MSG_ROUTE_SHARD_SERVCIE,
                               onPrimary, pIgnoreRC, pSucGrpLst, ppContext,
                               buf ) ;
   }

   INT32 rtnCoordCommand::executeOnCataGroup( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              BOOLEAN onPrimary,
                                              SET_RC *pIgnoreRC,
                                              rtnContextCoord **ppContext,
                                              rtnContextBuf *buf )
   {
      CoordGroupList grpList ;
      grpList[ CATALOG_GROUPID ] = CATALOG_GROUPID ;
      return _executeOnGroups( pMsg, cb, grpList, MSG_ROUTE_CAT_SERVICE,
                               onPrimary, pIgnoreRC, NULL, ppContext,
                               buf ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCOM_EXEONCATAGR, "rtnCoordCommand::executeOnCataGroup" )
   INT32 rtnCoordCommand::executeOnCataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               CoordGroupList *pGroupList,
                                               vector<BSONObj> *pReplyObjs,
                                               BOOLEAN onPrimary,
                                               SET_RC *pIgnoreRC,
                                               rtnContextCoord **ppContext,
                                               rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCOM_EXEONCATAGR ) ;

      rtnContextBuf buffObj ;
      rtnContextCoord *pContext = NULL ;

      rc = executeOnCataGroup( pMsg, cb, onPrimary, pIgnoreRC,
                               &pContext, buf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute command[%d] on catalog, rc: %d",
                   pMsg->opCode, rc ) ;

      while ( pContext )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR,
                    "Failed to get more from context [%lld], rc: %d",
                    pContext->contextID(), rc ) ;
            goto error ;
         }

         try
         {
            BSONObj obj( buffObj.data() ) ;

            if ( pGroupList )
            {
               rc = _processCatReply( obj, *pGroupList ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to get groups from catalog reply [%s], "
                            "rc: %d",
                            obj.toString().c_str(), rc ) ;
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

         if ( ppContext )
         {
            // If we need the control of the context for further steps, set
            // the pointers and break the loop
            (*ppContext) = pContext ;
            pContext = NULL ;
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
      return rc ;

   error :
      goto done ;
   }

   INT32 rtnCoordCommand::executeOnCataCL( MsgOpQuery *pMsg,
                                           pmdEDUCB *cb,
                                           const CHAR *pCLName,
                                           BOOLEAN onPrimary,
                                           SET_RC *pIgnoreRC,
                                           rtnContextCoord **ppContext,
                                           rtnContextBuf *buf )
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
                               ppContext, buf ) ;
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
                                       rtnContextCoord **ppContext,
                                       rtnContextBuf *buf )
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
                                        sendOpt, &queryConf, buf ) ;
      }
      else
      {
         return queryOpr.queryOrDoOnCL( pMsg, pRouteAgent, cb, ppContext,
                                        sendOpt, *pSucGrpLst, &queryConf,
                                        buf ) ;
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
      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, &pContext, buf ) ;
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
                                          SINT64 &contextID,
                                          rtnContextBuf *buf )
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
                           contextID, buf ) ;
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
                                                   vector< BSONObj > &objs,
                                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOCOM_QUERYONCATAANDPUSHTOVEC ) ;
      SINT64 contextID = -1 ;
      rtnContextBuf bufObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = queryOnCatalog( options, cb, contextID, buf ) ;
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

      /// clear msg
      pMsg->TID = cb->getTID() ;
      pMsg->routeID.value = MSG_INVALID_ROUTEID ;

      /// send msg
      rtnCoordSendRequestToNodes( (void *)pMsg, nodes, pAgent, cb,
                                  sendNodes, faileds ) ;
      /// recv reply
      rcTmp = rtnCoordGetReply( cb, sendNodes, replyQue,
                                MAKE_REPLY_TYPE(pMsg->opCode),
                                TRUE, FALSE ) ;
      rc = rc ? rc : rcTmp ;

      /// process reply
      rcTmp = _processNodesReply( replyQue, faileds, pContext,
                                  pIgnoreRC, pSucNodes ) ;
      rc = rc ? rc : rcTmp ;

      if ( rc )
      {
         rtnClearReplyQue( &replyQue ) ;
         rtnCoordClearRequest( cb, sendNodes ) ;
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
      BOOLEAN specificRole = FALSE ;

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

      if ( ctrlParam._parseMask & RTN_CTRL_MASK_ROLE )
      {
         specificRole = TRUE ;
      }

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

            if ( specificRole )
            {
               /// filter group by role
               rtnCoordFilterGroupsByRole( groupLst, ctrlParam._role ) ;
            }
         }
         else if ( ctrlParam._useSpecialGrp )
         {
            CoordGroupList::iterator itGrp = expectGrpLst.begin() ;
            while( itGrp != expectGrpLst.end() )
            {
               if ( ctrlParam._specialGrps.find( itGrp->first ) ==
                    ctrlParam._specialGrps.end() )
               {
                 itGrp = expectGrpLst.erase( itGrp ) ;
               }
               else
               {
                  ++itGrp ;
               }
            }
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
      if ( sendNodes.size() == 0 && !hasParseRetry )
      {
         PD_LOG( PDWARNING, "No specific nodes[%s]",
                 pFilterObj->toString().c_str() ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }
      else if ( pFilterObj->objdata() != newFilterObj.objdata() )
      {
         hasNodeOrGroupFilter = TRUE ;

         if ( specificRole )
         {
            /// Filter nodes by role
            rtnCoordFilterNodesByRole( sendNodes, ctrlParam._role ) ;
         }
      }
      /// if has not specify group, use the specail groups
      else if ( 0 == groupLst.size() )
      {
         if ( ctrlParam._useSpecialNode )
         {
            sendNodes = ctrlParam._specialNodes ;
         }
         else if ( !hasParseRetry &&
                   allGroupLst.size() != expectGrpLst.size() )
         {
            hasParseRetry = TRUE ;
            goto parseNode ;
         }
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
                           faileds, pSucNodes, pIgnoreRC,
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

   done:
      if ( ( rc || failedNodes.size() > 0 ) && -1 == contextID && buf )
      {
         *buf = _rtnContextBuf( rtnBuildErrorObj( rc, cb, &failedNodes ) ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   FILTER_BSON_ID rtnCoordRemoveBackup::_getGroupMatherIndex ()
   {
      return FILTER_ID_MATCHER ;
   }

   NODE_SEL_STY rtnCoordRemoveBackup::_nodeSelWhenNoFilter ()
   {
      return NODE_SEL_ALL ;
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

   BOOLEAN rtnCoordBackupOffline::_useContext ()
   {
      return FALSE ;
   }

   UINT32 rtnCoordBackupOffline::_getMask() const
   {
      return ~RTN_CTRL_MASK_NODE_SELECT ;
   }

   /*
      rtnCoordCmdWithLocation implement
   */
   INT32 rtnCoordCmdWithLocation::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      rtnCoordCtrlParam ctrlParam ;
      ROUTE_RC_MAP faileds ;
      rtnContextCoord *pContext = NULL ;

      contextID = -1 ;

      _preSet( cb, ctrlParam ) ;

      rc = _preExcute( pMsg, cb, ctrlParam ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Pre-excute failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = executeOnNodes( pMsg, cb, ctrlParam, _getControlMask(),
                           faileds, _useContext() ? &pContext : NULL,
                           FALSE, NULL, NULL ) ;
      if ( rc )
      {
         if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
         {
            rc = _onLocalMode( rc ) ;
         }

         if ( rc )
         {
            if ( SDB_COORD_UNKNOWN_OP_REQ != rc )
            {
               PD_LOG( PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
            }
            goto error ;
         }
      }

      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }

      rc = _posExcute( pMsg, cb, faileds ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Post-excute failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || faileds.size() > 0 ) && -1 == contextID && buf )
      {
         *buf = _rtnContextBuf( rtnBuildErrorObj( rc, cb, &faileds ) ) ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 rtnCoordCmdWithLocation::_preExcute( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              rtnCoordCtrlParam &ctrlParam )
   {
      return SDB_OK ;
   }

   INT32 rtnCoordCmdWithLocation::_posExcute( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              ROUTE_RC_MAP &faileds )
   {
      return SDB_OK ;
   }

   /*
      rtnCoordCMDMonIntrBase implement
   */
   void rtnCoordCMDMonIntrBase::_preSet( pmdEDUCB *cb,
                                         rtnCoordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
   }

   INT32 rtnCoordCMDMonIntrBase::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   /*
      rtnCoordCMDMonCurIntrBase implement
   */
   void rtnCoordCMDMonCurIntrBase::_preSet( pmdEDUCB *cb,
                                            rtnCoordCtrlParam &ctrlParam )
   {
      ROUTE_SET tmpNodes ;
      MsgRouteID nodeID ;
      CoordSession *pSession = cb->getCoordSession() ;
      ctrlParam._useSpecialNode = TRUE ;
      if ( pSession )
      {
         pSession->getAllSessionRoute( tmpNodes ) ;
      }
      ROUTE_SET::iterator it = tmpNodes.begin() ;
      while( it != tmpNodes.end() )
      {
         nodeID.value = *it ;
         ++it ;
         if ( MSG_ROUTE_SHARD_SERVCIE == nodeID.columns.serviceID )
         {
            ctrlParam._specialNodes.insert( nodeID.value ) ;
         }
      }
   }

   /*
      rtnCoordAggrCmdBase implement
   */
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

   /*
      rtnCoordCMDMonBase implement
   */
   INT32 rtnCoordCMDMonBase::execute( MsgHeader *pMsg,
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

      if ( ( !ctrlParam._rawData && getInnerAggrContent() ) ||
           vecUserAggr.size() > 0 )
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
      BSONObj outSelector ;
      rtnQueryOptions queryOpt ;

      // parse msg
      rc = queryOpt.fromQueryMsg( (CHAR*)pMsg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract query message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = _preProcess( queryOpt, clName, outSelector ) ;
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
      rc = queryOnCatalog( queryOpt, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on catalog[%s] failed, rc: %d",
                 queryOpt.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( !outSelector.isEmpty() && -1 != contextID )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         rtnContext *pContext = rtnCB->contextFind( contextID ) ;
         if ( pContext )
         {
            /// re-load pattern
            pContext->getSelector().clear() ;
            pContext->getSelector().loadPattern( outSelector ) ;
         }
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

         rc = executeOnCataGroup( pMsg, cb, TRUE, &ignoreRC, &pContext, buf ) ;
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

      rc = executeOnCataGroup( pMsg, cb, &groupLst, NULL, TRUE,
                               NULL, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Excute on catalog failed, rc: %d", rc ) ;

      pMsg->opCode                     = MSG_BS_QUERY_REQ ;
      // notify to data node
      rcTmp = executeOnDataGroup( pMsg, cb, groupLst,
                                  TRUE, NULL, NULL, NULL,
                                  buf ) ;
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
                                   sendOpt, &queryConf, buf ) ;
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

   INT32 rtnCoordCMDExpConfig::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void rtnCoordCMDExpConfig::_preSet( pmdEDUCB *cb,
                                       rtnCoordCtrlParam &ctrlParam )
   {
      ctrlParam._isGlobal = FALSE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
   }

   UINT32 rtnCoordCMDExpConfig::_getControlMask() const
   {
      return RTN_CTRL_MASK_ALL ;
   }

   INT32 rtnCoordCMDExpConfig::_posExcute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           ROUTE_RC_MAP &faileds )
   {
      /// do on local
      INT32 rc = pmdGetKRCB()->getOptionCB()->reflush2File() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Flush local config to file failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
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

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
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

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
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

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDSETSESSATTR_EXE, "rtnCoordCMDSetSessionAttr::execute" )
   INT32 rtnCoordCMDSetSessionAttr::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDSETSESSATTR_EXE ) ;

      CoordSession *pSession = NULL ;
      CHAR *pQuery = NULL ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL );
      PD_RC_CHECK( rc, PDERROR, "Failed to parse set session attribute "
                   "request, rc: %d", rc ) ;

      pSession = cb->getCoordSession();
      PD_CHECK( pSession != NULL, SDB_SYS, error, PDERROR,
                "Failed to get coord session" ) ;

      try
      {
         BSONObj property( pQuery ) ;
         rc = pSession->parseProperty( property ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse session property, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to set sessionAttr, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

   done :
      // fill default-reply(delete success)
      contextID = -1 ;

      PD_TRACE_EXITRC ( SDB_RTNCOCMDSETSESSATTR_EXE, rc ) ;
      return rc;
   error:
      goto done;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDGETSESSATTR_EXE, "rtnCoordCMDGetSessionAttr::execute" )
   INT32 rtnCoordCMDGetSessionAttr::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCOCMDGETSESSATTR_EXE ) ;

      CoordSession *pSession = cb->getCoordSession() ;
      PD_CHECK( pSession != NULL, SDB_SYS, error, PDERROR,
                "Failed to get coord session" ) ;

      ( *buf ) = rtnContextBuf( pSession->toBSON() ) ;

   done :
      // fill default-reply(delete success)
      contextID = -1 ;

      PD_TRACE_EXITRC( SDB_RTNCOCMDGETSESSATTR_EXE, rc ) ;
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

   void rtnCoordCMDInvalidateCache::_preSet( pmdEDUCB * cb,
                                             rtnCoordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   INT32 rtnCoordCMDInvalidateCache::_preExcute( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnCoordCtrlParam &ctrlParam )
   {
      /// invalidate local catalog cache and group cache
      sdbGetCoordCB()->invalidateCataInfo() ;
      sdbGetCoordCB()->invalidateGroupInfo() ;
      return SDB_OK ;
   }

   UINT32 rtnCoordCMDInvalidateCache::_getControlMask() const
   {
      return RTN_CTRL_MASK_ALL ;
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

      rc = rtnCoordGetGroupInfo( cb, gpName, FALSE, gpInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get info of group[%s], rc:%d",
                 gpName, rc ) ;
         goto error ;
      }

      gpLst[gpInfo->groupID()] = gpInfo->groupID() ;
      rc = executeOnDataGroup( pMsg, cb, gpLst, TRUE, NULL, NULL,
                               NULL, buf ) ;
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

      rc = executeOnCL( pMsg, cb, fullName, FALSE, NULL, NULL,
                        NULL, NULL, buf ) ;
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

   void rtnCoordCMDSyncDB::_preSet( pmdEDUCB *cb,
                                    rtnCoordCtrlParam &ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   UINT32 rtnCoordCMDSyncDB::_getControlMask() const
   {
      return RTN_CTRL_MASK_NODE_SELECT|RTN_CTRL_MASK_ROLE ;
   }

   INT32 rtnCoordCMDSyncDB::_preExcute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnCoordCtrlParam &ctrlParam )
   {
      INT32 rc = SDB_OK ;
      CHAR *pQuery = NULL ;
      CHAR *pNewMsg = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         const CHAR *csName = NULL ;
         BSONObj obj( pQuery ) ;
         BSONElement e = obj.getField( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( String == e.type() )
         {
            csName = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTIONSPACE, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( csName )
         {
            INT32 newMsgSize = 0 ;
            CoordGroupList grpLst ;
            rtnQueryOptions queryOpt ;

            queryOpt._fullName = "CAT" ;
            queryOpt._query = BSON( CAT_COLLECTION_SPACE_NAME << csName <<
                                    CAT_INCLUDE_SUBCL << false ) ;
            rc = queryOpt.toQueryMsg( &pNewMsg, newMsgSize ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc query msg failed, rc: %d", rc ) ;
               goto error ;
            }
            /// chage the opCode
            ((MsgHeader*)pNewMsg)->opCode = MSG_CAT_QUERY_SPACEINFO_REQ ;
            rc = executeOnCataGroup( (MsgHeader*)pNewMsg, cb, &grpLst ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Query collectionspace[%s] info from catalog "
                       "failed, rc: %d", csName, rc ) ;
               goto error ;
            }
            ctrlParam._useSpecialGrp = TRUE ;
            ctrlParam._specialGrps = grpLst ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( pNewMsg )
      {
         SDB_OSS_FREE( pNewMsg ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void rtnCoordCmdLoadCS::_preSet( pmdEDUCB * cb,
                                    rtnCoordCtrlParam &ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
   }

   UINT32 rtnCoordCmdLoadCS::_getControlMask() const
   {
      return RTN_CTRL_MASK_NODE_SELECT ;
   }

   INT32 rtnCoordCmdLoadCS::_preExcute( MsgHeader * pMsg,
                                        pmdEDUCB * cb,
                                        rtnCoordCtrlParam &ctrlParam )
   {
      INT32 rc = SDB_OK ;
      CHAR *pQuery = NULL ;
      CHAR *pNewMsg = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
         goto error ;
      }
      try
      {
         const CHAR *csName = NULL ;
         BSONObj obj( pQuery ) ;
         BSONElement e = obj.getField( FIELD_NAME_NAME ) ;
         if ( String == e.type() )
         {
            csName = e.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
            INT32 newMsgSize = 0 ;
            CoordGroupList grpLst ;
            rtnQueryOptions queryOpt ;

            queryOpt._fullName = "CAT" ;
            queryOpt._query = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
            rc = queryOpt.toQueryMsg( &pNewMsg, newMsgSize ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc query msg failed, rc: %d", rc ) ;
               goto error ;
            }
            /// chage the opCode
            ((MsgHeader*)pNewMsg)->opCode = MSG_CAT_QUERY_SPACEINFO_REQ ;
            rc = executeOnCataGroup( (MsgHeader*)pNewMsg, cb, &grpLst ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Query collectionspace[%s] info from catalog "
                       "failed, rc: %d", csName, rc ) ;
               goto error ;
            }
            ctrlParam._useSpecialGrp = TRUE ;
            ctrlParam._specialGrps = grpLst ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( pNewMsg )
      {
         SDB_OSS_FREE( pNewMsg ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordForceSession::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void rtnCoordForceSession::_preSet( pmdEDUCB * cb,
                                       rtnCoordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = FALSE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   UINT32 rtnCoordForceSession::_getControlMask() const
   {
      return RTN_CTRL_MASK_ALL ;
   }

   INT32 rtnCoordSetPDLevel::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void rtnCoordSetPDLevel::_preSet( pmdEDUCB * cb,
                                     rtnCoordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = FALSE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   UINT32 rtnCoordSetPDLevel::_getControlMask() const
   {
      return RTN_CTRL_MASK_ALL ;
   }

   INT32 rtnCoordReloadConf::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void rtnCoordReloadConf::_preSet( pmdEDUCB * cb,
                                     rtnCoordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
   }

   UINT32 rtnCoordReloadConf::_getControlMask() const
   {
      return RTN_CTRL_MASK_ALL ;
   }

   /*
    * rtnCoordCMD2Phase implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_EXECUTE, "rtnCoordCMD2Phase::execute" )
   INT32 rtnCoordCMD2Phase::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_EXECUTE ) ;

      SET_RC ignoreRCList ;

      CoordGroupList groupLst ;
      CoordGroupList sucGroupLst ;
      vector<BSONObj> cataObjs ;
      rtnContextCoord *pCoordCtxForCata = NULL ; // rename
      rtnContextCoord *pCoordCtxForData = NULL ;
      UINT32 reservedTID = 0 ;
      CHAR *pCataMsgBuf = NULL ;
      CHAR *pDataMsgBuf = NULL ;
      CHAR *pRollbackMsgBuf = NULL ;
      MsgHeader *pCataMsg = NULL ;
      MsgHeader *pDataMsg = NULL ;
      MsgHeader *pRollbackMsg = NULL ;
      _rtnCMDArguments *pArguments = NULL ;

      contextID = -1 ;

      /************************************************************************
       * Prepare phase
       * 1. Parse message
       * 2. Sanity check for arguments
       ************************************************************************/
      pArguments = _generateArguments() ;
      PD_CHECK( pArguments,
                SDB_SYS, error, PDERROR,
                "Failed to allocate runtime arguments") ;
      pArguments->_pBuf = buf ;

      // Extract message
      rc = _extractMsg ( pMsg, pArguments ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to extract %s message (rc=%d)",
                   _getCommandName(), rc ) ;

      // Parse and check message
      rc = _parseMsg ( pMsg, pArguments ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse %s query (rc=%d)",
                   _getCommandName(), rc ) ;

      /************************************************************************
       * Phase 1
       * 1. Generate P1 message to Catalog
       * 2. Execute P1 on Catalog
       * 3. Generate P1 message to Data Groups
       * 4. Execute P1 on Data Groups
       ************************************************************************/
      // Generate P1 message to Catalog
      rc = _generateCataMsg( pMsg, cb, pArguments, &pCataMsgBuf, &pCataMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "generate message to catalog failed, rc: %d",
                   _getCommandName(), pArguments->_targetName.c_str(), rc ) ;
      SDB_ASSERT( pCataMsg, "Failed to create catalog message" ) ;

      // Reserve TID from client if needed
      if ( _flagReserveClientTID() )
      {
         reservedTID = pMsg->TID ;
      }

      // Execute P1 on Catalog
      rc = _doOnCataGroup( pCataMsg, cb, &pCoordCtxForCata, pArguments,
                           _flagGetGrpLstFromCata() ? &groupLst : NULL,
                           &cataObjs ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         PD_LOG( PDWARNING,
                 "Empty group list, done anyway" ) ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "phase 1 on catalog failed, rc: %d",
                   _getCommandName(), pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO,
              "%s on [%s] phase 1 on catalog done, get %u target groups back",
              _getCommandName(), pArguments->_targetName.c_str(), groupLst.size() ) ;

      // Generate P1 message to Data Groups
      rc = _generateDataMsg( pMsg, cb, pArguments, cataObjs,
                             &pDataMsgBuf, &pDataMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "generate message to data groups failed, rc: %d",
                   _getCommandName(), pArguments->_targetName.c_str(), rc ) ;

      // Using the TID from client
      if ( _flagReserveClientTID() )
      {
         PD_LOG( PDDEBUG,
                 "Using reserved TID %u instead of %u",
                 pMsg->TID, reservedTID ) ;
         pMsg->TID = reservedTID ;
      }

      // Execute P1 on Data Groups
      rc = _doOnDataGroup( pDataMsg, cb, &pCoordCtxForData, pArguments,
                           groupLst, cataObjs, sucGroupLst ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "phase 1 on data groups failed, rc: %d, suc: %u",
                   _getCommandName(), pArguments->_targetName.c_str(),
                   rc, sucGroupLst.size() ) ;

      PD_LOG( PDINFO,
              "%s on [%s] phase 1 on data done, done on %d groups",
              _getCommandName(), pArguments->_targetName.c_str(),
              sucGroupLst.size() ) ;

      /************************************************************************
       * Phase 2
       * 1. Execute P2 on Catalog
       * 2. Execute P2 on Data Groups
       ************************************************************************/
      // Execute P2 on Catalog
      rc = _doOnCataGroupP2( pCataMsg, cb, &pCoordCtxForCata, pArguments, groupLst ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "phase 2 on catalog failed, rc, %d",
                   _getCommandName(), pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO,
              "%s on [%s] phase 2 on catalog done",
              _getCommandName(), pArguments->_targetName.c_str() ) ;

      // Execute P2 on Data Groups
      rc = _doOnDataGroupP2( pDataMsg, cb, &pCoordCtxForData, pArguments,
                             groupLst, cataObjs ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "phase 2 on data groups failed, rc, %d",
                   _getCommandName(), pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO,
              "%s on [%s] phase 2 on data done",
              _getCommandName(), pArguments->_targetName.c_str() ) ;

      /************************************************************************
       * Phase Commit
       * 1. Commit on Catalog
       * 2. Update local catalog cache if needed
       ************************************************************************/
   commit :
      // Commit on Catalog
      rc = _doCommit( pMsg, cb, &pCoordCtxForCata, pArguments );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: commit phase failed, rc: %d",
                   _getCommandName(), pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO,
              "%s on [%s] commit done",
              _getCommandName(), pArguments->_targetName.c_str() ) ;

      // Update local catalog info caches if needed
      rc = _doComplete( pMsg, cb, pArguments ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: complete failed, rc: %d",
                   _getCommandName(), pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO,
              "%s on [%s] complete done",
              _getCommandName(), pArguments->_targetName.c_str() ) ;

   done :
      /************************************************************************
       * Phase Clean
       * 1. Audit the command Audit the command
       * 2. Delete contexts and free buffers
       ************************************************************************/
      // Audit command
      if ( pArguments )
      {
         _doAudit( pArguments, rc ) ;
      }

      // Clear allocated context and buffers
      if ( pCoordCtxForCata )
      {
         pmdKRCB *pKrcb = pmdGetKRCB();
         _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
         pRtncb->contextDelete ( pCoordCtxForCata->contextID(), cb ) ;
         pCoordCtxForCata = NULL ;
      }

      if ( pCoordCtxForData )
      {
         pmdKRCB *pKrcb = pmdGetKRCB();
         _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
         pRtncb->contextDelete ( pCoordCtxForData->contextID(), cb ) ;
         pCoordCtxForData = NULL ;
      }

      SAFE_OSS_FREE( pCataMsgBuf ) ;
      SAFE_OSS_FREE( pDataMsgBuf ) ;
      SAFE_OSS_FREE( pRollbackMsgBuf ) ;

      SAFE_OSS_DELETE( pArguments ) ;

      PD_TRACE_EXITRC ( CMD_RTNCOCMD2PH_EXECUTE, rc ) ;

      return rc ;

   error :
      /************************************************************************
       * Phase Rollback
       * 1. Generate rollback message to succeed Data Groups
       * 2. Execute rollback on succeed Data Groups
       * Note: updates to Catalog will be rollback by kill context
       ************************************************************************/
      // The command could be rollbacked in two conditions:
      // 1. There are succeed Data Groups
      // 2. There are Catalog context to rollback catalog
      if ( !sucGroupLst.empty () &&
           pCoordCtxForCata )
      {
         INT32 tmprc = SDB_OK ;

         // Rollback Catalog first if needed
         if ( _flagRollbackCataBeforeData() &&
              pCoordCtxForCata )
         {
            pmdKRCB *pKrcb = pmdGetKRCB();
            _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
            pRtncb->contextDelete ( pCoordCtxForCata->contextID(), cb ) ;
            pCoordCtxForCata = NULL ;
         }

         do
         {
            // Generate rollback message to succeed Data Groups
            tmprc = _generateRollbackDataMsg( pMsg, pArguments,
                                              &pRollbackMsgBuf, &pRollbackMsg ) ;
            if ( SDB_OK != tmprc )
            {
               PD_LOG( PDWARNING,
                       "%s on [%s]: "
                       "failed to generate rollback message, rc: %d",
                       _getCommandName(), pArguments->_targetName.c_str(),
                       tmprc ) ;
               sucGroupLst.clear() ;
               if ( _flagCommitOnRollbackFailed() &&
                    pCoordCtxForCata )
               {
                  goto commit ;
               }
               break ;
            }
            tmprc = _rollbackOnDataGroup( pRollbackMsg, cb, pArguments, sucGroupLst ) ;
            if ( SDB_OK != tmprc )
            {
               PD_LOG( PDWARNING,
                       "%s on [%s]: "
                       "failed to execute rollback, rc: %d",
                       _getCommandName(), pArguments->_targetName.c_str(),
                       tmprc ) ;
               sucGroupLst.clear() ;
               if ( _flagCommitOnRollbackFailed() &&
                    pCoordCtxForCata )
               {
                  goto commit ;
               }
               break ;
            }
            PD_LOG( PDINFO,
                    "%s on [%s] rollback done",
                    _getCommandName(), pArguments->_targetName.c_str() ) ;
         } while ( FALSE ) ;
      }
      goto done ;
   }

   rtnCoordCMD2Phase::_rtnCMDArguments *rtnCoordCMD2Phase::_generateArguments ()
   {
      return SDB_OSS_NEW _rtnCMDArguments () ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_EXTRACTMSG, "rtnCoordCMD2Phase::_extractMsg" )
   INT32 rtnCoordCMD2Phase::_extractMsg ( MsgHeader *pMsg,
                                          _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_EXTRACTMSG ) ;

      CHAR *pQuery = NULL ;

      try
      {
         _printDebug ( (CHAR*)pMsg, "rtnCoordCMD2Phase" ) ;

         rc = msgExtractQuery( (CHAR *)pMsg, NULL, NULL,
                               NULL, NULL, &pQuery, NULL,
                               NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse %s request, rc: %d",
                      _getCommandName(), rc ) ;

         pArgs->_boQuery.init( pQuery ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMD2PH_EXTRACTMSG, rc ) ;
      return rc;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_DOCOMPLETE, "rtnCoordCMD2Phase::_doComplete" )
   INT32 rtnCoordCMD2Phase::_doComplete ( MsgHeader *pMsg,
                                          pmdEDUCB * cb,
                                          _rtnCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_DOCOMPLETE ) ;

      // Do nothing

      PD_TRACE_EXIT ( CMD_RTNCOCMD2PH_DOCOMPLETE ) ;

      return SDB_OK;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_DOONCATA, "rtnCoordCMD2Phase::_doOnCataGroup" )
   INT32 rtnCoordCMD2Phase::_doOnCataGroup ( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **ppContext,
                                             _rtnCMDArguments *pArgs,
                                             CoordGroupList *pGroupLst,
                                             vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_DOONCATA ) ;

      rtnContextCoord *pContext = NULL ;

      // Send request to catalog, and get the control of CoordContext
      rc = executeOnCataGroup( pMsg, cb, pGroupLst, pReplyObjs,
                               TRUE, NULL, &pContext, pArgs->_pBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute on catalog, rc: %d",
                   rc ) ;

   done :
      if ( pContext )
      {
         (*ppContext) = pContext ;
      }
      PD_TRACE_EXITRC ( CMD_RTNCOCMD2PH_DOONCATA, rc ) ;
      return rc ;

   error :
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         pContext = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_DOONCATAP2, "rtnCoordCMD2Phase::_doOnCataGroupP2" )
   INT32 rtnCoordCMD2Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord **ppContext,
                                               _rtnCMDArguments *pArgs,
                                               const CoordGroupList &pGroupLst )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_DOONCATAP2 ) ;

      /// Do nothing

      PD_TRACE_EXIT ( CMD_RTNCOCMD2PH_DOONCATAP2 ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_DOONDATAP2, "rtnCoordCMD2Phase::_doOnDataGroupP2" )
   INT32 rtnCoordCMD2Phase::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord **ppContext,
                                               _rtnCMDArguments *pArgs,
                                               const CoordGroupList &groupLst,
                                               const vector<BSONObj> &cataObjs )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_DOONDATAP2 ) ;

      /// Do nothing

      PD_TRACE_EXIT ( CMD_RTNCOCMD2PH_DOONDATAP2 ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_ROLLBACKONDATA, "rtnCoordCMD2Phase::_rollbackOnDataGroup" )
   INT32 rtnCoordCMD2Phase::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   _rtnCMDArguments *pArgs,
                                                   const CoordGroupList &groupLst )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_ROLLBACKONDATA ) ;

      /// Do nothing

      PD_TRACE_EXIT ( CMD_RTNCOCMD2PH_ROLLBACKONDATA ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_DOCOMMIT, "rtnCoordCMD2Phase::_doCommit" )
   INT32 rtnCoordCMD2Phase::_doCommit ( MsgHeader *pMsg,
                                        pmdEDUCB * cb,
                                        rtnContextCoord **ppContext,
                                        _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_DOCOMMIT ) ;

      rc = _processContext( cb, ppContext, -1 ) ;
      if ( SDB_RTN_CONTEXT_NOTEXIST == rc )
      {
         PD_LOG( PDWARNING,
                 "Failed %s on [%s]: Get catalog context failed",
                 _getCommandName(), pArgs->_targetName.c_str() ) ;
      }

      PD_TRACE_EXITRC ( CMD_RTNCOCMD2PH_DOCOMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMD2PH_PROCESSCTX, "rtnCoordCMD2Phase::_processContext" )
   INT32 rtnCoordCMD2Phase::_processContext ( pmdEDUCB *cb,
                                              rtnContextCoord **ppContext,
                                              SINT32 maxNumSteps )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMD2PH_PROCESSCTX ) ;

      pmdKRCB *pKrcb = pmdGetKRCB() ;
      _SDB_RTNCB *pRtncb = pKrcb->getRTNCB() ;
      rtnContextBuf buffObj ;

      if ( !(*ppContext) )
      {
         goto done ;
      }

      rc = (*ppContext)->getMore( maxNumSteps, buffObj, cb ) ;
      if ( SDB_DMS_EOC == rc )
      {
         pRtncb->contextDelete ( (*ppContext)->contextID(), cb ) ;
         (*ppContext) = NULL ;
         PD_LOG( PDDEBUG, "Hit end of context" ) ;
         rc = SDB_OK;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to process context with %d steps, rc: %d",
                   maxNumSteps, rc ) ;
   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMD2PH_PROCESSCTX, rc ) ;
      return rc ;

   error :
      if ( ppContext && (*ppContext) )
      {
         pRtncb->contextDelete ( (*ppContext)->contextID(), cb ) ;
         (*ppContext) = NULL ;
      }
      goto done ;
   }
}
