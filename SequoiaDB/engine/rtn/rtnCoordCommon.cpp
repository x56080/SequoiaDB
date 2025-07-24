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

   Source File Name = rtnCoordCommon.cpp

   Descriptive Name = Runtime Coord Common

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCoord.hpp"
#include "rtnCoordCommon.hpp"
#include "msg.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtnCoordQuery.hpp"
#include "msgMessage.hpp"
#include "coordCB.hpp"
#include "rtnContext.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "coordSession.hpp"
#include "rtnCoordCommands.hpp"
#include "rtn.hpp"

using namespace bson;

namespace engine
{
   /*
      Local define
   */
   #define RTN_COORD_RSP_WAIT_TIME        1000     //1s
   #define RTN_COORD_RSP_WAIT_TIME_QUICK  10       //10ms

   #define RTN_COORD_MAX_RETRYTIMES       2

   /*
      Local function define
   */

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCOSENDREQUESTTOONE, "_rtnCoordSendRequestToOne" )
   static INT32 _rtnCoordSendRequestToOne( MsgHeader *pBuffer,
                                           CoordGroupInfoPtr &groupInfo,
                                           REQUESTID_MAP &sendNodes, // out
                                           netMultiRouteAgent *pRouteAgent,
                                           MSG_ROUTE_SERVICE_TYPE type,
                                           pmdEDUCB *cb,
                                           const netIOVec *pIOVec,
                                           BOOLEAN isResend )
   {
      INT32 rc                = SDB_OK ;
      CoordSession *pSession  = cb->getCoordSession() ;
      MsgRouteID routeID ;
      clsGroupItem *groupItem = NULL ;
      UINT32 nodeNum          = 0 ;
      UINT32 beginPos         = 0 ;
      UINT32 selTimes         = 0 ;

      BOOLEAN hasRetry        = FALSE ;
      UINT64 reqID            = 0 ;
      INT32 preferReplicaType = PREFER_REPL_ANYONE ;

      PD_TRACE_ENTRY ( SDB__RTNCOSENDREQUESTTOONE ) ;

      if ( pSession )
      {
         preferReplicaType = pSession->getPreferReplType() ;
         if ( PREFER_REPL_MASTER == preferReplicaType )
         {
            if ( MSG_BS_QUERY_REQ == pBuffer->opCode )
            {
               MsgOpQuery *pQuery = ( MsgOpQuery* )pBuffer ;
               if ( FALSE == isResend )
               {
                  pQuery->flags |= FLG_QUERY_PRIMARY ;
               }
               else
               {
                  pQuery->flags &= ~FLG_QUERY_PRIMARY ;
               }
            }
            else if ( pBuffer->opCode > ( SINT32 )MSG_LOB_BEGIN &&
                      pBuffer->opCode < ( SINT32 )MSG_LOB_END )
            {
               MsgOpLob *pLobMsg = ( MsgOpLob* )pBuffer ;
               if ( FALSE == isResend )
               {
                  pLobMsg->flags |= FLG_LOBREAD_PRIMARY ;
               }
               else
               {
                  pLobMsg->flags &= ~FLG_LOBREAD_PRIMARY ;
               }
            }
         }
      }

      /*******************************
      // send to last node
      ********************************/
      if ( NULL != pSession )
      {
         routeID = pSession->getLastNode( groupInfo->groupID() ) ;
         // last node is valid and in group info( when group or node 
         // is remove )
         if ( routeID.value != 0 &&
              groupInfo->nodePos( routeID.columns.nodeID ) >= 0 )
         {
            if ( pIOVec && pIOVec->size() > 0 )
            {
               rc = pRouteAgent->syncSend( routeID, pBuffer, *pIOVec,
                                           reqID, cb ) ;
            }
            else
            {
               rc = pRouteAgent->syncSend( routeID, (void *)pBuffer,
                                           reqID, cb, NULL, 0 ) ;
            }
            if ( SDB_OK == rc )
            {
               sendNodes[reqID] = routeID ;
               goto done ;
            }
            else
            {
               rtnCoordUpdateNodeStatByRC( cb, routeID, groupInfo, rc ) ;

               PD_LOG( PDWARNING, "Send msg[opCode: %d, TID: %u] to group[%u] 's "
                       "last node[%s] failed, rc: %d", pBuffer->opCode,
                       pBuffer->TID, groupInfo->groupID(),
                       routeID2String( routeID ).c_str(), rc ) ;
            }
         }
      }

   retry:
      /*******************************
      // send to new node
      ********************************/
      groupItem = groupInfo.get() ;
      nodeNum = groupInfo->nodeCount() ;
      if ( nodeNum <= 0 )
      {
         if ( !hasRetry && CATALOG_GROUPID != groupInfo->groupID() )
         {
            hasRetry = TRUE ;
            rc = rtnCoordGetGroupInfo( cb, groupInfo->groupID(),
                                       TRUE, groupInfo ) ; 
            PD_RC_CHECK( rc, PDERROR, "Get group info[%u] failed, rc: %d",
                         groupInfo->groupID(), rc ) ;
            goto retry ;
         }
         rc = SDB_CLS_EMPTY_GROUP ;
         PD_LOG ( PDERROR, "Couldn't send the request to empty group" ) ;
         goto error ;
      }

      routeID.value = MSG_INVALID_ROUTEID ;
      rtnCoordGetNodePos( preferReplicaType, groupItem, ossRand(), beginPos ) ;

      selTimes = 0 ;
      while( selTimes < nodeNum )
      {
         INT32 status = NET_NODE_STAT_NORMAL ;
         rtnCoordGetNextNode( preferReplicaType, groupItem,
                              selTimes, beginPos ) ;

         rc = groupItem->getNodeID( beginPos, routeID, type ) ;
         if ( rc )
         {
            break ;
         }
         rc = groupInfo->getNodeInfo( beginPos, status ) ;
         if ( rc )
         {
            break ;
         }
         if ( NET_NODE_STAT_NORMAL == status )
         {
            if ( pIOVec )
            {
               rc = pRouteAgent->syncSend( routeID, pBuffer, *pIOVec,
                                           reqID, cb ) ;
            }
            else
            {
               rc = pRouteAgent->syncSend( routeID, (void *)pBuffer,
                                           reqID, cb, NULL, 0 ) ;
            }
            if ( SDB_OK == rc )
            {
               sendNodes[ reqID ] = routeID ;

               if ( pSession )
               {
                  pSession->addLastNode( routeID ) ;
               }
               break ;
            }
            else
            {
               rtnCoordUpdateNodeStatByRC( cb, routeID, groupInfo, rc ) ;
            }
         }
         else
         {
            rc = SDB_CLS_NODE_BSFAULT ;
         }
      }

      if ( rc && !hasRetry )
      {
         hasRetry = TRUE ;
         if ( CATALOG_GROUPID != groupInfo->groupID() )
         {
            rc = rtnCoordGetGroupInfo( cb, groupInfo->groupID(),
                                       TRUE, groupInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Get group info[%u] failed, rc: %d",
                         groupInfo->groupID(), rc ) ;
         }
         else
         {
            groupInfo->clearNodesStat() ;
            rc = SDB_OK ;
         }
         goto retry ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCOSENDREQUESTTOONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCOSENDREQUESTTOPRIMARY, "_rtnCoordSendRequestToPrimary" )
   static INT32 _rtnCoordSendRequestToPrimary( MsgHeader *pBuffer,
                                               CoordGroupInfoPtr &groupInfo,
                                               netMultiRouteAgent *pRouteAgent,
                                               MSG_ROUTE_SERVICE_TYPE type,
                                               pmdEDUCB *cb,
                                               REQUESTID_MAP &sendNodes, // out
                                               const netIOVec *pIOVec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCOSENDREQUESTTOPRIMARY ) ;

      MsgRouteID primaryRouteID ;
      UINT64 reqID = 0 ;
      BOOLEAN hasRetry = FALSE ;
      CoordSession *pSession  = cb->getCoordSession() ;

   retry:
      primaryRouteID = groupInfo->primary( type ) ;
      if ( primaryRouteID.value != 0 )
      {
         if ( pIOVec && pIOVec->size() > 0 )
         {
            rc = pRouteAgent->syncSend( primaryRouteID, pBuffer, *pIOVec,
                                        reqID, cb ) ;
         }
         else
         {
            rc = pRouteAgent->syncSend( primaryRouteID, (void *)pBuffer,
                                        reqID, cb, NULL, 0 ) ;
         }
         if ( SDB_OK == rc )
         {
            sendNodes[ reqID ] = primaryRouteID ;
            if ( pSession )
            {
               pSession->addLastNode( primaryRouteID ) ;
            }
            goto done ;
         }
         else
         {
            rtnCoordUpdateNodeStatByRC( cb, primaryRouteID, groupInfo, rc ) ;

            // update and retry
            if ( !hasRetry )
            {
               goto update ;
            }

            PD_LOG( PDWARNING, "Send msg[opCode: %d, TID: %u] to group[%u] 's "
                    "primary node[%s] failed, rc: %d", pBuffer->opCode,
                    pBuffer->TID, groupInfo->groupID(),
                    routeID2String( primaryRouteID ).c_str(), rc ) ;
            // not go to error, send to any one node
         }
      }

      // send to any one node
      rc = SDB_RTN_NO_PRIMARY_FOUND ;
      if ( !hasRetry )
      {
         goto update ;
      }
      // send to any one node, so the group has no primary, will wait some
      // time at data node
      rc = _rtnCoordSendRequestToOne( pBuffer, groupInfo, sendNodes,
                                      pRouteAgent, type, cb,
                                      pIOVec, TRUE ) ;
      goto done ;

   update:
      if ( rc && !hasRetry )
      {
         hasRetry = TRUE ;
         rc = rtnCoordGetGroupInfo( cb, groupInfo->groupID(),
                                    TRUE, groupInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Get group info[%u] failed, rc: %d",
                      groupInfo->groupID(), rc ) ;
         goto retry ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__RTNCOSENDREQUESTTOPRIMARY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCOSENDREQUESTTONODEGROUP, "_rtnCoordSendRequestToNodeGroup" )
   static INT32 _rtnCoordSendRequestToNodeGroup( MsgHeader *pBuffer,
                                                 CoordGroupInfoPtr &groupInfo,
                                                 BOOLEAN isSendPrimary,
                                                 netMultiRouteAgent *pRouteAgent,
                                                 MSG_ROUTE_SERVICE_TYPE type,
                                                 pmdEDUCB *cb,
                                                 REQUESTID_MAP &sendNodes, // out
                                                 const netIOVec *pIOVec,
                                                 BOOLEAN isResend )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__RTNCOSENDREQUESTTONODEGROUP ) ;

      MsgRouteID routeID ;
      UINT64 reqID = 0 ;
      UINT32 groupID = groupInfo->groupID() ;

      // if trans valid, send to trans node
      cb->getTransNodeRouteID( groupID, routeID ) ;
      if ( routeID.value != 0 )
      {
         if ( pIOVec && pIOVec->size() > 0 )
         {
            rc = pRouteAgent->syncSend( routeID, pBuffer, *pIOVec,
                                        reqID, cb ) ;
         }
         else
         {
            rc = pRouteAgent->syncSend( routeID, (void *)pBuffer,
                                        reqID, cb, NULL, 0 ) ;
         }
         if ( SDB_OK == rc )
         {
            sendNodes[ reqID ] = routeID ;
         }
         else
         {
            PD_LOG( PDERROR, "Send msg[opCode: %d, TID: %u] to group[%u] 's "
                    "transaction node[%s] failed, rc: %d", pBuffer->opCode,
                    pBuffer->TID, groupID, routeID2String( routeID ).c_str(),
                    rc ) ;
            rtnCoordUpdateNodeStatByRC( cb, routeID, groupInfo, rc ) ;
            goto error ;
         }
      }
      // not trans, send to node by isSendPrimary
      else
      {
         if ( isSendPrimary )
         {
            rc = _rtnCoordSendRequestToPrimary( pBuffer, groupInfo,
                                                pRouteAgent, type, cb,
                                                sendNodes, pIOVec ) ;
         }
         else
         {
            rc = _rtnCoordSendRequestToOne( pBuffer, groupInfo, sendNodes,
                                            pRouteAgent, type, cb,
                                            pIOVec, isResend ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Send msg[opCode: %d, TID: %u] to "
                      "group[%u]'s %s node failed, rc: %d",
                      pBuffer->opCode, pBuffer->TID, groupID,
                      isSendPrimary ? "primary" : "one", rc ) ; 
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCOSENDREQUESTTONODEGROUP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   #define RTN_COORD_PARSE_MASK_ET_DFT       0x00000001
   #define RTN_COORD_PARSE_MASK_IN_DFT       0x00000002
   #define RTN_COORD_PARSE_MASK_ET_OPR       0x00000004
   #define RTN_COORD_PARSE_MASK_IN_OPR       0x00000008

   #define RTN_COORD_PARSE_MASK_ET           ( RTN_COORD_PARSE_MASK_ET_DFT|\
                                               RTN_COORD_PARSE_MASK_ET_OPR )
   #define RTN_COORD_PARSE_MASK_ALL          0xFFFFFFFF

   static INT32 _rtnCoordParseBoolean( BSONElement &e, BOOLEAN &value,
                                       UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      /// a:true
      if ( e.isBoolean() && ( mask & RTN_COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.boolean() ? TRUE : FALSE ;
         rc = SDB_OK ;
      }
      /// a:1
      else if ( e.isNumber() && ( mask & RTN_COORD_PARSE_MASK_ET_DFT ) )
      {
         value = 0 != e.numberInt() ? TRUE : FALSE ;
         rc = SDB_OK ;
      }
      /// a:{$et:true} or a:{$et:1}
      else if ( Object == e.type() && ( mask & RTN_COORD_PARSE_MASK_ET_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() &&
              0 == ossStrcmp( "$et", tmpE.fieldName() ) )
         {
            rc = _rtnCoordParseBoolean( tmpE, value,
                                        RTN_COORD_PARSE_MASK_ET_DFT ) ;
         }
      }
      return rc ;
   }

   static INT32 _rtnCoordParseInt( BSONElement &e, INT32 &value, UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      /// a:1
      if ( e.isNumber() && ( mask & RTN_COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.numberInt() ;
         rc = SDB_OK ;
      }
      /// a:{$et:1}
      else if ( Object == e.type() && ( mask & RTN_COORD_PARSE_MASK_ET_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() &&
              0 == ossStrcmp( "$et", tmpE.fieldName() ) )
         {
            rc = _rtnCoordParseInt( tmpE, value,
                                    RTN_COORD_PARSE_MASK_ET_DFT ) ;
         }
      }
      return rc ;
   }

   static INT32 _rtnCoordParseInt( BSONElement &e, vector<INT32> &vecValue,
                                   UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      INT32 value = 0 ;

      /// a:1 or a:{$et:1}
      if ( ( !e.isABSONObj() ||
             0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                             "$et") ) &&
           ( mask & RTN_COORD_PARSE_MASK_ET ) )
      {
         rc = _rtnCoordParseInt( e, value, mask ) ;
         if ( SDB_OK == rc )
         {
            vecValue.push_back( value ) ;
         }
      }
      /// a:[1,2,3]
      else if ( Array == e.type() && ( mask & RTN_COORD_PARSE_MASK_IN_DFT ) )
      {
         BSONObjIterator it( e.embeddedObject() ) ;
         while ( it.more() )
         {
            BSONElement tmpE = it.next() ;
            rc = _rtnCoordParseInt( tmpE, value,
                                    RTN_COORD_PARSE_MASK_ET_DFT ) ;
            if ( rc )
            {
               break ;
            }
            vecValue.push_back( value ) ;
         }
      }
      /// a:{$in:[1,2,3]}
      else if ( Object == e.type() &&
                0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                                "$in") &&
                ( mask & RTN_COORD_PARSE_MASK_IN_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() )
         {
            rc = _rtnCoordParseInt( tmpE, vecValue,
                                    RTN_COORD_PARSE_MASK_IN_DFT ) ;
         }
      }
      return rc ;
   }

   static INT32 _rtnCoordParseString( BSONElement &e, const CHAR *&value,
                                      UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      /// a:"xxx"
      if ( String == e.type() && ( mask & RTN_COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.valuestr() ;
         rc = SDB_OK ;
      }
      /// a:{$et:"xxx"}
      else if ( Object == e.type() && ( mask & RTN_COORD_PARSE_MASK_ET_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() &&
              0 == ossStrcmp( "$et", tmpE.fieldName() ) )
         {
            rc = _rtnCoordParseString( tmpE, value,
                                       RTN_COORD_PARSE_MASK_ET_DFT ) ;
         }
      }
      return rc ;
   }

   static INT32 _rtnCoordParseString( BSONElement &e,
                                      vector<const CHAR*> &vecValue,
                                      UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      const CHAR *value = NULL ;

      /// a:"xxx" or a:{$et:"xxx"}
      if ( ( !e.isABSONObj() ||
             0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                             "$et") ) &&
           ( mask & RTN_COORD_PARSE_MASK_ET ) )
      {
         rc = _rtnCoordParseString( e, value, mask ) ;
         if ( SDB_OK == rc )
         {
            vecValue.push_back( value ) ;
         }
      }
      /// a:["xxx", "yyy", "zzz"]
      else if ( Array == e.type() && ( mask & RTN_COORD_PARSE_MASK_IN_DFT ) )
      {
         BSONObjIterator it( e.embeddedObject() ) ;
         while ( it.more() )
         {
            BSONElement tmpE = it.next() ;
            rc = _rtnCoordParseString( tmpE, value,
                                       RTN_COORD_PARSE_MASK_ET_DFT ) ;
            if ( rc )
            {
               break ;
            }
            vecValue.push_back( value ) ;
         }
      }
      /// a:{$in:["xxx", "yyy", "zzz"]}
      else if ( Object == e.type() &&
                0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                                "$in") &&
                ( mask & RTN_COORD_PARSE_MASK_IN_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() )
         {
            rc = _rtnCoordParseString( tmpE, vecValue,
                                       RTN_COORD_PARSE_MASK_IN_DFT ) ;
         }
      }
      return rc ;
   }

   /*
      End local function define
   */

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCATAQUERY, "rtnCoordCataQuery" )
   INT32 rtnCoordCataQuery ( const CHAR *pCollectionName,
                             const BSONObj &selector,
                             const BSONObj &matcher,
                             const BSONObj &orderBy,
                             const BSONObj &hint,
                             INT32 flag,
                             pmdEDUCB *cb,
                             SINT64 numToSkip,
                             SINT64 numToReturn,
                             SINT64 &contextID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCATAQUERY ) ;
      contextID = -1 ;
      rtnCoordCommand commandOpr ;
      rtnQueryOptions queryOpt( matcher.objdata(),
                                selector.objdata(),
                                orderBy.objdata(),
                                hint.objdata(),
                                pCollectionName,
                                numToSkip,
                                numToReturn,
                                flag,
                                FALSE ) ;
      rc = commandOpr.queryOnCatalog( queryOpt, cb, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query[%s] on catalog failed, rc: %d",
                   queryOpt.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCATAQUERY, rc ) ;
      return rc ;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONODEQUERY, "rtnCoordNodeQuery" )
   INT32 rtnCoordNodeQuery ( const CHAR *pCollectionName,
                             const BSONObj &condition,
                             const BSONObj &selector,
                             const BSONObj &orderBy,
                             const BSONObj &hint,
                             INT64 numToSkip, INT64 numToReturn,
                             CoordGroupList &groupLst,
                             pmdEDUCB *cb,
                             rtnContext **ppContext,
                             const CHAR *realCLName,
                             INT32 flag )
   {
      INT32 rc                        = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCONODEQUERY ) ;
      CHAR *pBuffer                   = NULL ;
      INT32 bufferSize                = 0 ;

      pmdKRCB *pKrcb                  = pmdGetKRCB() ;
      SDB_RTNCB *pRtnCB               = pKrcb->getRTNCB() ;
      CoordCB *pCoordcb               = pKrcb->getCoordCB() ;
      netMultiRouteAgent *pRouteAgent = pCoordcb->getRouteAgent();

      SDB_ASSERT ( ppContext, "ppContext can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;

      rtnCoordQuery newQuery ;
      rtnQueryConf queryConf ;
      rtnSendOptions sendOpt ;

      BOOLEAN needReset = FALSE ;
      BOOLEAN onlyOneNode = groupLst.size() == 1 ? TRUE : FALSE ;
      INT64 tmpSkip = numToSkip ;
      INT64 tmpReturn = numToReturn ;

      if ( !onlyOneNode )
      {
         if ( numToReturn > 0 && numToSkip > 0 )
         {
            tmpReturn = numToReturn + numToSkip ;
         }
         tmpSkip = 0 ;

         rtnNeedResetSelector( selector, orderBy, needReset ) ;
      }
      else
      {
         numToSkip = 0 ;
      }

      const CHAR *realCLFullName = realCLName ? realCLName : pCollectionName ;
      rtnContextCoord *pTmpContext = NULL ;
      INT64 contextID = -1 ;

      if ( groupLst.size() == 0 )
      {
         PD_LOG( PDERROR, "Send group list is empty" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = pRtnCB->contextNew ( RTN_CONTEXT_COORD, (rtnContext**)&pTmpContext,
                                contextID, cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to allocate new context, rc = %d",
                  rc ) ;
         goto error ;
      }
      rc = pTmpContext->open( orderBy,
                              needReset ? selector : BSONObj(),
                              numToReturn, numToSkip ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open context failed, rc: %d", rc ) ;
         goto error ;
      }

      // build a query request based on the provided information
      rc = msgBuildQueryMsg ( &pBuffer, &bufferSize, pCollectionName,
                              flag, 0, tmpSkip, tmpReturn,
                              condition.isEmpty()?NULL:&condition,
                              selector.isEmpty()?NULL:&selector,
                              orderBy.isEmpty()?NULL:&orderBy,
                              hint.isEmpty()?NULL:&hint ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to build query message, rc = %d",
                  rc ) ;
         goto error ;
      }

      queryConf._realCLName = realCLFullName ;
      sendOpt._groupLst = groupLst ;
      sendOpt._useSpecialGrp = TRUE ;

      rc = newQuery.queryOrDoOnCL( (MsgHeader*)pBuffer, pRouteAgent, cb,
                                   &pTmpContext, sendOpt, &queryConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] from data node "
                   "failed, rc = %d", realCLFullName, rc ) ;

   done :
      if ( pBuffer )
      {
         SDB_OSS_FREE ( pBuffer ) ;
      }
      if ( pTmpContext )
      {
         *ppContext = pTmpContext ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCONODEQUERY, rc ) ;
      return rc ;
   error :
      if ( contextID >= 0 )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
         pTmpContext = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETREPLY, "rtnCoordGetReply" )
   INT32 rtnCoordGetReply ( pmdEDUCB *cb,  REQUESTID_MAP &requestIdMap,
                            REPLY_QUE &replyQue, const SINT32 opCode,
                            BOOLEAN isWaitAll, BOOLEAN clearReplyIfFailed )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOGETREPLY ) ;

      ossQueue<pmdEDUEvent> tmpQue ;
      REQUESTID_MAP::iterator iterMap ;
      INT64 waitTime = RTN_COORD_RSP_WAIT_TIME ;

      while ( requestIdMap.size() > 0 )
      {
         pmdEDUEvent pmdEvent ;
         BOOLEAN isGotMsg = cb->waitEvent( pmdEvent, waitTime ) ;
         // if we hit interrupt, let's just get out of here. Don't need to worry
         // about cb queue, pmdEDUCB::clear() is going to clean it up.
         if ( cb->isInterrupted() || cb->isForced() )
         {
            if ( isGotMsg )
            {
               pmdEduEventRelase( pmdEvent, cb ) ;
            }
            rc = SDB_APP_INTERRUPT ;
            PD_LOG( PDERROR, "Interrupt! stop receiving reply!" ) ;
            goto error ;
         }

         // if we didn't receive anything
         if ( FALSE == isGotMsg )
         {
            if ( !isWaitAll && !replyQue.empty() )
            {
               break ;
            }
            else
            {
               continue ;
            }
         }

         if ( pmdEvent._eventType != PMD_EDU_EVENT_MSG )
         {
            PD_LOG ( PDWARNING, "Received unknown event(eventType:%d)",
                     pmdEvent._eventType ) ;
            pmdEduEventRelase( pmdEvent, cb ) ;
            pmdEvent.reset () ;
            continue;
         }
         MsgHeader *pReply = (MsgHeader *)(pmdEvent._Data) ;
         if ( NULL == pReply )
         {
            PD_LOG ( PDWARNING, "Received invalid event(data is null)" );
            continue ;
         }

         if ( MSG_COM_REMOTE_DISC == pReply->opCode )
         {
            // check if transaction-node
            MsgRouteID routeID;
            routeID.value = pReply->routeID.value;
            if ( cb->isTransNode( routeID ) )
            {
               cb->setTransRC( SDB_COORD_REMOTE_DISC ) ;
               PD_LOG ( PDERROR, "Transaction operation interrupt, "
                        "remote-node[%s] disconnected",
                        routeID2String( routeID ).c_str() ) ;
            }

            iterMap = requestIdMap.begin() ;
            while ( iterMap != requestIdMap.end() )
            {
               if ( iterMap->second.value == routeID.value )
               {
                  break ;
               }
               ++iterMap ;
            }
            if ( iterMap != requestIdMap.end() &&
                 iterMap->first <= pReply->requestID )
            {
               // remote node disconnected, go on receive all reply then
               // clear all. Don't return the rc, the error will be detected
               // while process the reply
               PD_LOG ( PDERROR, "Get reply failed, remote-node[%s] "
                        "disconnected",
                        routeID2String( iterMap->second ).c_str() ) ;

               MsgHeader *pDiscMsg = NULL ;
               pDiscMsg = (MsgHeader *)SDB_OSS_MALLOC( pReply->messageLength ) ;
               if ( NULL == pDiscMsg )
               {
                  rc = SDB_OOM ;
                  PD_LOG( PDERROR, "Malloc failed(size=%d)",
                          pReply->messageLength ) ;
                  goto error ;
               }
               else
               {
                  ossMemcpy( (CHAR *)pDiscMsg, (CHAR *)pReply,
                              pReply->messageLength );
                  replyQue.push( (CHAR *)pDiscMsg ) ;
               }
               cb->getCoordSession()->delRequest( iterMap->first ) ;
               requestIdMap.erase( iterMap ) ;
            }
            if ( cb->getCoordSession()->isValidResponse( routeID,
                                                         pReply->requestID ) )
            {
               tmpQue.push( pmdEvent );
            }
            else
            {
               SDB_OSS_FREE ( pmdEvent._Data ) ;
               pmdEvent.reset () ;
            }
            continue ;
         } // if ( MSG_COOR_REMOTE_DISC == pReply->opCode )

         iterMap = requestIdMap.find( pReply->requestID ) ;
         if ( iterMap == requestIdMap.end() )
         {
            if ( cb->getCoordSession()->isValidResponse( pReply->requestID ) )
            {
               tmpQue.push( pmdEvent ) ;
            }
            else
            {
               PD_LOG ( PDWARNING, "Received expired or unexpected msg( "
                        "opCode=[%d]%d, expectOpCode=[%d]%d, requestID=%lld, "
                        "TID=%d) from node[%s]",
                        IS_REPLY_TYPE( pReply->opCode ),
                        GET_REQUEST_TYPE( pReply->opCode ),
                        IS_REPLY_TYPE( opCode ),
                        GET_REQUEST_TYPE( opCode ),
                        pReply->requestID, pReply->TID,
                        routeID2String( pReply->routeID ).c_str() ) ;
               SDB_OSS_FREE( pReply ) ;
            }
         }
         else
         {
            if ( opCode != pReply->opCode ||
                 pReply->routeID.value != iterMap->second.value )
            {
               PD_LOG ( PDWARNING, "Received unexpected msg(opCode=[%d]%d,"
                        "requestID=%lld, TID=%d, expectOpCode=[%d]%d, "
                        "expectNode=%s) from node[%s]",
                        IS_REPLY_TYPE( pReply->opCode ),
                        GET_REQUEST_TYPE( pReply->opCode ),
                        pReply->requestID, pReply->TID,
                        IS_REPLY_TYPE( opCode ),
                        GET_REQUEST_TYPE( opCode ),
                        routeID2String( iterMap->second ).c_str(),
                        routeID2String( pReply->routeID ).c_str() ) ;
            }
            else
            {
               PD_LOG ( PDDEBUG , "Received the reply(opCode=[%d]%d, "
                        "requestID=%llu, TID=%u) from node[%s]",
                        IS_REPLY_TYPE( pReply->opCode ),
                        GET_REQUEST_TYPE( pReply->opCode ),
                        pReply->requestID, pReply->TID,
                        routeID2String( pReply->routeID ).c_str() ) ;
            }

            requestIdMap.erase( iterMap ) ;
            cb->getCoordSession()->delRequest( pReply->requestID ) ;
            replyQue.push( (CHAR *)( pmdEvent._Data ) ) ;
            pmdEvent.reset () ;
            if ( !isWaitAll )
            {
               waitTime = RTN_COORD_RSP_WAIT_TIME_QUICK ;
            }
         } // if ( iterMap == requestIdMap.end() )
      } // while ( requestIdMap.size() > 0 )

      if ( rc )
      {
         goto error;
      }

   done:
      while( !tmpQue.empty() )
      {
         pmdEDUEvent otherEvent;
         tmpQue.wait_and_pop( otherEvent );
         cb->postEvent( otherEvent );
      }
      PD_TRACE_EXITRC ( SDB_RTNCOGETREPLY, rc ) ;
      return rc;
   error:
      //clear the incomplete reply-queue
      if ( clearReplyIfFailed )
      {
         while ( !replyQue.empty() )
         {
            CHAR *pData = replyQue.front();
            replyQue.pop();
            SDB_OSS_FREE( pData );
         }
      }
      rtnCoordClearRequest( cb, requestIdMap ) ;
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GETSERVNAME, "getServiceName" )
   INT32 getServiceName ( BSONElement &beService,
                          INT32 serviceType,
                          std::string &strServiceName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_GETSERVNAME ) ;
      strServiceName = "";
      do
      {
         if ( beService.type() != Array )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR,
                     "failed to get the service-name(type=%d),\
                     the service field is invalid", serviceType );
            break;
         }
         try
         {
            BSONObjIterator i( beService.embeddedObject() );
            while ( i.more() )
            {
               BSONElement beTmp = i.next();
               BSONObj boTmp = beTmp.embeddedObject();
               BSONElement beServiceType = boTmp.getField(
                  CAT_SERVICE_TYPE_FIELD_NAME );
               if ( beServiceType.eoo() || !beServiceType.isNumber() )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDERROR, "Failed to get the service-name(type=%d),"
                           "get the field(%s) failed", serviceType,
                           CAT_SERVICE_TYPE_FIELD_NAME );
                  break;
               }
               if ( beServiceType.numberInt() == serviceType )
               {
                  BSONElement beServiceName = boTmp.getField(
                     CAT_SERVICE_NAME_FIELD_NAME );
                  if ( beServiceType.eoo() || beServiceName.type() != String )
                  {
                     rc = SDB_INVALIDARG;
                     PD_LOG ( PDERROR, "Failed to get the service-name"
                              "(type=%d), get the field(%s) failed",
                              serviceType, CAT_SERVICE_NAME_FIELD_NAME ) ;
                     break;
                  }
                  strServiceName = beServiceName.String();
                  break;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "unexpected exception: %s", e.what() ) ;
            rc = SDB_INVALIDARG ;
         }
      }while ( FALSE );
      PD_TRACE_EXITRC ( SDB_GETSERVNAME, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETCATAINFO, "rtnCoordGetCataInfo" )
   INT32 rtnCoordGetCataInfo( pmdEDUCB *cb,
                              const CHAR *pCollectionName,
                              BOOLEAN isNeedRefreshCata,
                              CoordCataInfoPtr &cataInfo,
                              BOOLEAN *pHasUpdate )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETCATAINFO ) ;
      while( TRUE )
      {
         if ( isNeedRefreshCata )
         {
            rc = rtnCoordGetRemoteCata( cb, pCollectionName, cataInfo ) ;
            if ( SDB_OK == rc && pHasUpdate )
            {
               *pHasUpdate = TRUE ;
            }
         }
         else
         {
            rc = rtnCoordGetLocalCata ( pCollectionName, cataInfo );
            if ( SDB_CAT_NO_MATCH_CATALOG == rc )
            {
               // couldn't find the match catalogue,
               // then get from catalogue-node
               isNeedRefreshCata = TRUE ;
               continue;
            }
         }
         break;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOGETCATAINFO, rc ) ;
      return rc ;
   }

   void rtnCoordRemoveGroup( UINT32 group )
   {
      pmdGetKRCB()->getCoordCB()->removeGroupInfo( group ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETLOCALCATA, "rtnCoordGetLocalCata" )
   INT32 rtnCoordGetLocalCata( const CHAR *pCollectionName,
                               CoordCataInfoPtr &cataInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETLOCALCATA ) ;
      pmdKRCB *pKrcb          = pmdGetKRCB();
      CoordCB *pCoordcb       = pKrcb->getCoordCB();
      rc = pCoordcb->getCataInfo( pCollectionName, cataInfo ) ;
      PD_TRACE_EXITRC ( SDB_RTNCOGETLOCALCATA, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETREMOTECATA, "rtnCoordGetRemoteCata" )
   INT32 rtnCoordGetRemoteCata( pmdEDUCB *cb,
                                const CHAR *pCollectionName,
                                CoordCataInfoPtr &cataInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETREMOTECATA ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();
      CoordGroupInfoPtr cataGroupInfo ;
      MsgRouteID nodeID ;
      UINT32 primaryID = 0 ;
      UINT32 times = 0 ;

      rc = rtnCoordGetCatGroupInfo( cb, FALSE, cataGroupInfo, NULL ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to get the catalogue-node-group "
                  "info, rc: %d", rc ) ;
         goto error ;
      }

      do
      {
         BSONObj boQuery;
         BSONObj boFieldSelector;
         BSONObj boOrderBy;
         BSONObj boHint;
         try
         {
            boQuery = BSON( CAT_CATALOGNAME_NAME << pCollectionName ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS;
            PD_LOG ( PDERROR, "Get reomte catalogue failed, while "
                     "build query-obj received unexception error:%s",
                     e.what() );
            break ;
         }
         CHAR *pBuffer = NULL;
         INT32 bufferSize = 0;
         // the buffer will be free after call sendRequestToPrimary
         rc = msgBuildQueryCatalogReqMsg ( &pBuffer, &bufferSize,
                                           0, 0, 0, 1, cb->getTID(),
                                           &boQuery, &boFieldSelector,
                                           &boOrderBy, &boHint );
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to build query-catalog-message, rc: %d",
                     rc );
            break ;
         }

         REQUESTID_MAP sendNodes;
         rc = rtnCoordSendRequestToPrimary( pBuffer, cataGroupInfo, sendNodes,
                                            pRouteAgent, MSG_ROUTE_CAT_SERVICE,
                                            cb ) ;
         if ( pBuffer != NULL )
         {
            SDB_OSS_FREE( pBuffer );
         }
         if ( rc != SDB_OK )
         {
            rtnCoordClearRequest( cb, sendNodes ) ;

            PD_LOG ( PDERROR, "Failed to send catalog-query msg to "
                     "catalogue-group, rc: %d", rc ) ;
            break ;
         }

         REPLY_QUE replyQue;
         rc = rtnCoordGetReply( cb, sendNodes, replyQue,
                                MSG_CAT_QUERY_CATALOG_RSP ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get reply from catalogue-node, "
                    "rc: %d", rc ) ;
            break ;
         }

         // process reply
         BOOLEAN isGetExpectReply = FALSE ;
         while ( !replyQue.empty() )
         {
            MsgCatQueryCatRsp *pReply = NULL ;
            pReply = (MsgCatQueryCatRsp *)(replyQue.front());
            replyQue.pop();

            if ( FALSE == isGetExpectReply )
            {
               nodeID.value = pReply->header.routeID.value ;
               primaryID = pReply->startFrom ;
               rc = rtnCoordProcessQueryCatReply( pReply, cataInfo ) ;
               if( SDB_OK == rc )
               {
                  isGetExpectReply = TRUE ;
                  pCoordcb->updateCataInfo( pCollectionName, cataInfo ) ;
               }
            }
            if ( NULL != pReply )
            {
               SDB_OSS_FREE( pReply ) ;
            }
         }

         // catalogue-node reply no primary
         // and the catalogue-group info have not be refreshed,
         // maybe the catalogue-group-info is expired and refresh it
         if ( rc )
         {
            if ( rtnCoordGroupReplyCheck( cb, rc, rtnCoordCanRetry( times++ ),
                                          nodeID, cataGroupInfo, NULL, TRUE,
                                          primaryID, TRUE ) )
            {
               continue ;
            }

            PD_LOG( PDERROR, "Get catalog info[%s] from remote failed, rc: %d",
                    pCollectionName, rc ) ;

            if ( ( SDB_DMS_NOTEXIST == rc || SDB_DMS_EOC == rc ) &&
                 pCoordcb->isSubCollection( pCollectionName ) )
            {
               /// change the error
               rc = SDB_CLS_COORD_NODE_CAT_VER_OLD ;
            }
         }
         break ;
      }while ( TRUE ) ;

      if ( SDB_OK == rc && cataInfo->isMainCL() )
      {
         vector< string > subCLLst ;
         cataInfo->getSubCLList( subCLLst ) ;

         vector< string >::iterator iterLst = subCLLst.begin() ;
         while ( iterLst != subCLLst.end() )
         {
            CoordCataInfoPtr subCataInfoTmp ;
            rc = rtnCoordGetRemoteCata( cb, iterLst->c_str(),
                                        subCataInfoTmp ) ;
            if( rc )
            {
               PD_LOG( PDERROR, "Get main collection[%s]'s sub collection[%s] "
                       "failed, rc: %d", pCollectionName, iterLst->c_str(),
                       rc ) ;
               // remove main catalog info
               pCoordcb->delCataInfo( pCollectionName ) ;
               break ;
            }
            ++iterLst ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOGETREMOTECATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETGROUPINFO, "rtnCoordGetGroupInfo" )
   INT32 rtnCoordGetGroupInfo ( pmdEDUCB *cb,
                                UINT32 groupID,
                                BOOLEAN isNeedRefresh,
                                CoordGroupInfoPtr &groupInfo,
                                BOOLEAN *pHasUpdate )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETGROUPINFO ) ;

      while( TRUE )
      {
         if ( isNeedRefresh )
         {
            rc = rtnCoordGetRemoteGroupInfo( cb, groupID, NULL, groupInfo ) ;
            if ( SDB_OK == rc && pHasUpdate )
            {
               *pHasUpdate = TRUE ;
            }
         }
         else
         {
            rc = rtnCoordGetLocalGroupInfo ( groupID, groupInfo );
            if ( SDB_COOR_NO_NODEGROUP_INFO == rc )
            {
               // couldn't find the match catalogue,
               // then get from catalogue-node
               isNeedRefresh = TRUE ;
               continue;
            }
         }
         break ;
      }

      PD_TRACE_EXITRC ( SDB_RTNCOGETGROUPINFO, rc ) ;

      return rc;
   }

   INT32 rtnCoordGetGroupInfo( pmdEDUCB *cb,
                               const CHAR *groupName,
                               BOOLEAN isNeedRefresh,
                               CoordGroupInfoPtr &groupInfo,
                               BOOLEAN *pHasUpdate )
   {
      INT32 rc = SDB_OK;

      while( TRUE )
      {
         if ( isNeedRefresh )
         {
            rc = rtnCoordGetRemoteGroupInfo( cb, 0, groupName, groupInfo ) ;
            if ( SDB_OK == rc && pHasUpdate )
            {
               *pHasUpdate = TRUE ;
            }
         }
         else
         {
            rc = rtnCoordGetLocalGroupInfo ( groupName, groupInfo ) ;
            if ( SDB_COOR_NO_NODEGROUP_INFO == rc )
            {
               // couldn't find the match catalogue,
               // then get from catalogue-node
               isNeedRefresh = TRUE ;
               continue ;
            }
         }
         break ;
      }

      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETLOCALGROUPINFO, "rtnCoordGetLocalGroupInfo" )
   INT32 rtnCoordGetLocalGroupInfo ( UINT32 groupID,
                                     CoordGroupInfoPtr &groupInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETLOCALGROUPINFO ) ;
      pmdKRCB *pKrcb          = pmdGetKRCB();
      CoordCB *pCoordcb       = pKrcb->getCoordCB();

      if ( CATALOG_GROUPID == groupID )
      {
         groupInfo = pCoordcb->getCatGroupInfo() ;
      }
      else
      {
         rc = pCoordcb->getGroupInfo( groupID, groupInfo ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOGETLOCALGROUPINFO, rc ) ;
      return rc ;
   }

   INT32 rtnCoordGetLocalGroupInfo ( const CHAR *groupName,
                                     CoordGroupInfoPtr &groupInfo )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb          = pmdGetKRCB() ;
      CoordCB *pCoordcb       = pKrcb->getCoordCB() ;
      if ( 0 == ossStrcmp( CATALOG_GROUPNAME, groupName ) )
      {
         groupInfo = pCoordcb->getCatGroupInfo() ;
      }
      else
      {
         rc = pCoordcb->getGroupInfo( groupName, groupInfo ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETREMOTEGROUPINFO, "rtnCoordGetRemoteGroupInfo" )
   INT32 rtnCoordGetRemoteGroupInfo ( pmdEDUCB *cb,
                                      UINT32 groupID,
                                      const CHAR *groupName,
                                      CoordGroupInfoPtr &groupInfo,
                                      BOOLEAN addToLocal )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOGETREMOTEGROUPINFO ) ;
      pmdKRCB *pKrcb          = pmdGetKRCB();
      CoordCB *pCoordcb       = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();
      CHAR *buf = NULL ;
      MsgCatGroupReq *msg = NULL ;
      MsgCatGroupReq msgGroupReq ;
      CoordGroupInfoPtr cataGroupInfo ;
      MsgRouteID nodeID ;
      UINT32 times= 0 ;

      // if catalogure group
      if ( CATALOG_GROUPID == groupID ||
           ( groupName && 0 == ossStrcmp( groupName, CATALOG_GROUPNAME ) ) )
      {
         rc = rtnCoordGetRemoteCatGroupInfo( cb, groupInfo ) ;
         goto done ;
      }

      rc = rtnCoordGetCatGroupInfo( cb, FALSE, cataGroupInfo, NULL );
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to get cata-group-info,"
                  "no catalogue-node-group info" );
        goto error ;
      }

      if ( NULL == groupName )
      {
         msgGroupReq.id.columns.groupID = groupID;
         msgGroupReq.id.columns.nodeID = 0;
         msgGroupReq.id.columns.serviceID = 0;
         msgGroupReq.header.messageLength = sizeof( MsgCatGroupReq );
         msgGroupReq.header.opCode = MSG_CAT_GRP_REQ;
         msgGroupReq.header.routeID.value = 0;
         msgGroupReq.header.TID = cb->getTID();
         msg = &msgGroupReq ;
      }
      else
      {
         UINT32 nameLen = ossStrlen( groupName ) + 1 ;
         UINT32 msgLen = nameLen +  sizeof(MsgCatGroupReq) ;
         buf = ( CHAR * )SDB_OSS_MALLOC( msgLen ) ;
         if ( NULL == buf )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         msg = ( MsgCatGroupReq * )buf ;
         msg->id.value = 0 ;
         msg->header.messageLength = msgLen ;
         msg->header.opCode = MSG_CAT_GRP_REQ;
         msg->header.routeID.value = 0;
         msg->header.TID = cb->getTID();
         ossMemcpy( buf + sizeof(MsgCatGroupReq),
                    groupName, nameLen ) ;
      }

      do
      {
         REQUESTID_MAP sendNodes ;
         rc = rtnCoordSendRequestToPrimary( (CHAR *)(msg),
                                            cataGroupInfo, sendNodes,
                                            pRouteAgent,
                                            MSG_ROUTE_CAT_SERVICE,
                                            cb );
         if ( rc != SDB_OK )
         {
            rtnCoordClearRequest( cb, sendNodes ) ;

            if ( groupName )
            {
               PD_LOG ( PDERROR, "Failed to get group info[%s], because of "
                        "send group-info-request failed, rc: %d",
                        groupName, rc ) ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to get group info[%u], because of "
                        "send group-info-request failed, rc: %d",
                        groupID, rc ) ;
            }
            break ;
         }

         REPLY_QUE replyQue;
         rc = rtnCoordGetReply( cb, sendNodes, replyQue,
                                MSG_CAT_GRP_RES ) ;
         if ( rc != SDB_OK )
         {
            if ( groupName )
            {
               PD_LOG ( PDWARNING, "Failed to get group info[%s], because of "
                        "get reply failed, rc: %d", groupName, rc ) ;
            }
            else
            {
               PD_LOG ( PDWARNING, "Failed to get group info[%u], because of "
                        "get reply failed, rc: %d", groupID, rc ) ;
            }
            break ;
         }

         // process reply
         BOOLEAN getExpected = FALSE ;
         UINT32 primaryID = 0 ;
         while ( !replyQue.empty() )
         {
            MsgHeader *pReply = NULL;
            pReply = (MsgHeader *)(replyQue.front());
            replyQue.pop() ;

            if ( FALSE == getExpected )
            {
               nodeID.value = pReply->routeID.value ;
               primaryID = MSG_GET_INNER_REPLY_STARTFROM( pReply ) ;
               rc = rtnCoordProcessGetGroupReply( pReply, groupInfo ) ;
               if ( SDB_OK == rc )
               {
                  getExpected = TRUE ;
                  if ( addToLocal )
                  {
                     pCoordcb->addGroupInfo( groupInfo ) ;
                     rc = rtnCoordUpdateRoute( groupInfo, pRouteAgent,
                                               MSG_ROUTE_SHARD_SERVCIE ) ;
                  }
               }
            }
            if ( NULL != pReply )
            {
               SDB_OSS_FREE( pReply );
            }
         }

         if ( rc != SDB_OK )
         {
            if ( rtnCoordGroupReplyCheck( cb, rc, rtnCoordCanRetry( times++ ),
                                          nodeID, cataGroupInfo, NULL, TRUE,
                                          primaryID, TRUE ) )
            {
               continue;
            }

            if ( groupName )
            {
               PD_LOG ( PDERROR, "Failed to get group info[%s], because of "
                        "reply error, flag: %d", groupName, rc ) ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to get group info[%u], because of "
                        "reply error, flag: %d", groupID, rc ) ;
            }
         }
         break ;
      }while ( TRUE ) ;

   done:
      if ( NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
         buf = NULL ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOGETREMOTEGROUPINFO, rc ) ;
      return rc;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETCATGROUPINFO, "rtnCoordGetCatGroupInfo" )
   INT32 rtnCoordGetCatGroupInfo ( pmdEDUCB *cb,
                                   BOOLEAN isNeedRefresh,
                                   CoordGroupInfoPtr &groupInfo,
                                   BOOLEAN *pHasUpdate )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETCATGROUPINFO ) ;
      UINT32 retryCount = 0 ;

      while( TRUE )
      {
         if ( isNeedRefresh )
         {
            rc = rtnCoordGetRemoteCatGroupInfo( cb, groupInfo ) ;
            if ( SDB_CLS_GRP_NOT_EXIST == rc && retryCount < 30 )
            {
               ++retryCount ;
               ossSleep( 100 ) ;
               continue ;
            }
            if ( SDB_OK == rc && pHasUpdate )
            {
               *pHasUpdate = TRUE ;
            }
         }
         else
         {
            rc = rtnCoordGetLocalCatGroupInfo ( groupInfo ) ;
            if ( ( SDB_OK == rc && groupInfo->nodeCount() == 0 ) ||
                   SDB_COOR_NO_NODEGROUP_INFO == rc )
            {
               // couldn't find the match group-info,
               // then get from catalogue-node
               isNeedRefresh = TRUE;
               continue ;
            }
         }
         break;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOGETCATGROUPINFO, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETLOCALCATGROUPINFO, "rtnCoordGetLocalCatGroupInfo" )
   INT32 rtnCoordGetLocalCatGroupInfo ( CoordGroupInfoPtr &groupInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETLOCALCATGROUPINFO ) ;
      pmdKRCB *pKrcb          = pmdGetKRCB();
      CoordCB *pCoordcb       = pKrcb->getCoordCB();
      groupInfo = pCoordcb->getCatGroupInfo();
      PD_TRACE_EXITRC ( SDB_RTNCOGETLOCALCATGROUPINFO, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETREMOTECATAGROUPINFOBYADDR, "rtnCoordGetRemoteCataGroupInfoByAddr" )
   INT32 rtnCoordGetRemoteCataGroupInfoByAddr ( pmdEDUCB *cb,
                                                CoordGroupInfoPtr &groupInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETREMOTECATAGROUPINFOBYADDR ) ;
      UINT64 reqID;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();
      MsgHeader *pReply                = NULL;
      ossQueue<pmdEDUEvent> tmpQue ;
      UINT32 sendPos                   = 0 ;

      MsgCatCatGroupReq msgGroupReq;
      msgGroupReq.id.columns.groupID = CAT_CATALOG_GROUPID ;
      msgGroupReq.id.columns.nodeID = 0;
      msgGroupReq.id.columns.serviceID = 0;
      msgGroupReq.header.messageLength = sizeof( MsgCatGroupReq );
      msgGroupReq.header.opCode = MSG_CAT_GRP_REQ ;
      msgGroupReq.header.routeID.value = 0;
      msgGroupReq.header.TID = cb->getTID();

      CoordVecNodeInfo cataNodeAddrList ;
      pCoordcb->getCatNodeAddrList( cataNodeAddrList ) ;

      if ( cataNodeAddrList.size() == 0 )
      {
         rc = SDB_CAT_NO_ADDR_LIST ;
         PD_LOG ( PDERROR, "no catalog node info" );
         goto error ;
      }

      while( sendPos < cataNodeAddrList.size() )
      {
         BOOLEAN disconnected = FALSE ;

         for ( ; sendPos < cataNodeAddrList.size() ; ++sendPos )
         {
            rc = pRouteAgent->syncSendWithoutCheck( cataNodeAddrList[sendPos]._id,
                                                    (MsgHeader*)&msgGroupReq,
                                                    reqID, cb ) ;
            if ( SDB_OK == rc )
            {
               break ;
            }
         }
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to send the request to catalog-node"
                     "(rc=%d)", rc );
            break ;
         }
         while ( TRUE )
         {
            pmdEDUEvent pmdEvent;
            BOOLEAN isGotMsg = cb->waitEvent( pmdEvent,
                                              RTN_COORD_RSP_WAIT_TIME ) ;
            if ( cb->isForced() ||
                 ( cb->isInterrupted() && !( cb->isDisconnected() ) ) )
            {
               if ( isGotMsg )
               {
                  pmdEduEventRelase( pmdEvent, cb ) ;
               }
               rc = SDB_APP_INTERRUPT ;
               break ;
            }

            if ( FALSE == isGotMsg )
            {
               continue ;
            }
            if ( PMD_EDU_EVENT_MSG != pmdEvent._eventType )
            {
               PD_LOG ( PDWARNING, "Received unknown event(eventType=%d)",
                        pmdEvent._eventType ) ;
               pmdEduEventRelase( pmdEvent, cb ) ;
               pmdEvent.reset () ;
               continue ;
            }
            MsgHeader *pMsg = (MsgHeader *)(pmdEvent._Data) ;
            if ( NULL == pMsg )
            {
               PD_LOG ( PDWARNING, "Received invalid msg-event(data is null)" );
               continue ;
            }

            // Process MSG_COM_REMOTE_DISC first
            if ( MSG_COM_REMOTE_DISC == pMsg->opCode )
            {
               PD_LOG ( PDERROR, "Get reply failed, remote-node[%s] disconnected",
                        routeID2String( pMsg->routeID ).c_str() ) ;

               // The Catalog is not transaction node, so no need to remove
               // transactions

               // The disconnected is later than we send, the reply would not
               // return, remove the request ID
               // Otherwise, the node might be restarted, we could wait
               if ( reqID <= pMsg->requestID )
               {
                  cb->getCoordSession()->delRequest( reqID ) ;
                  disconnected = TRUE ;
               }

               if ( cb->getCoordSession()->isValidResponse( pMsg->routeID,
                                                            pMsg->requestID ) )
               {
                  // For later process
                  tmpQue.push( pmdEvent ) ;
               }
               else
               {
                  SDB_OSS_FREE( pmdEvent._Data ) ;
                  pmdEvent.reset () ;
               }

               if ( disconnected )
               {
                  break ;
               }
               else
               {
                  continue ;
               }
            }

            if ( pMsg->opCode != MSG_CAT_GRP_RES ||
                 pMsg->requestID != reqID )
            {
               if ( cb->getCoordSession()->isValidResponse( reqID ) )
               {
                  tmpQue.push( pmdEvent );
               }
               else
               {
                  PD_LOG( PDWARNING, "Received unexpected message"
                        "(opCode=[%d]%d, requestID=%llu, TID=%u, "
                        "groupID=%u, nodeID=%u, serviceID=%u )",
                        IS_REPLY_TYPE( pMsg->opCode ),
                        GET_REQUEST_TYPE( pMsg->opCode ),
                        pMsg->requestID, pMsg->TID,
                        pMsg->routeID.columns.groupID,
                        pMsg->routeID.columns.nodeID,
                        pMsg->routeID.columns.serviceID );
                  SDB_OSS_FREE( pMsg ) ;
               }
               continue ;
            }
            // recv the reply msg
            cb->getCoordSession()->delRequest( reqID ) ;
            pReply = (MsgHeader *)pMsg ;
            break ;
         }

         // Current one is disconnected, goto to the next one
         if ( disconnected )
         {
            // Set the rc in case that disconnected one is the last one to try
            rc = SDB_NETWORK_CLOSE ;
            ++sendPos ;
            continue ;
         }

         if ( rc != SDB_OK || NULL == pReply )
         {
            PD_LOG ( PDERROR, "Failed to get reply(rc=%d)", rc );
            break ;
         }
         rc = rtnCoordProcessGetGroupReply( pReply, groupInfo ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR,
                     "Failed to get catalog-group info, reply error(rc=%d)",
                     rc ) ;
            SDB_OSS_FREE( pReply ) ;
            pReply = NULL ;
            continue ;
         }
         rc = rtnCoordUpdateRoute( groupInfo, pRouteAgent,
                                   MSG_ROUTE_CAT_SERVICE ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to get catalog-group info,"
                     "update route failed(rc=%d)", rc );
            ++sendPos ;
            SDB_OSS_FREE( pReply ) ;
            pReply = NULL ;
            continue ;
         }

         // update shard sevice also
         rtnCoordUpdateRoute( groupInfo, pRouteAgent,
                              MSG_ROUTE_SHARD_SERVCIE ) ;

         pCoordcb->updateCatGroupInfo( groupInfo ) ;
         break ;
      }

   done:
      if ( NULL != pReply )
      {
         SDB_OSS_FREE( pReply ) ;
      }
      while ( !tmpQue.empty() )
      {
         pmdEDUEvent otherEvent;
         tmpQue.wait_and_pop( otherEvent );
         cb->postEvent( otherEvent );
      }
      PD_TRACE_EXITRC ( SDB_RTNCOGETREMOTECATAGROUPINFOBYADDR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETREMOTECATGROUPINFO, "rtnCoordGetRemoteCatGroupInfo" )
   INT32 rtnCoordGetRemoteCatGroupInfo ( pmdEDUCB *cb,
                                         CoordGroupInfoPtr &groupInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOGETREMOTECATGROUPINFO ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();

      MsgRouteID nodeID ;
      CoordGroupInfoPtr cataGroupInfo ;
      rtnCoordGetLocalCatGroupInfo( cataGroupInfo ) ;
      UINT32 times = 0 ;

      MsgCatCatGroupReq msgGroupReq;
      msgGroupReq.id.columns.groupID = CAT_CATALOG_GROUPID;
      msgGroupReq.id.columns.nodeID = 0;
      msgGroupReq.id.columns.serviceID = 0;
      msgGroupReq.header.messageLength = sizeof( MsgCatGroupReq );
      msgGroupReq.header.opCode = MSG_CAT_GRP_REQ ;

      if ( cataGroupInfo->nodeCount() == 0 )
      {
         rc = rtnCoordGetRemoteCataGroupInfoByAddr( cb, groupInfo ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to get cata-group-info by addr, "
                     "rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }

      do
      {
         msgGroupReq.header.routeID.value = 0;
         msgGroupReq.header.TID = cb->getTID();
         REQUESTID_MAP sendNodes ;
         rc = rtnCoordSendRequestToOne( (CHAR *)(&msgGroupReq),
                                        cataGroupInfo, sendNodes,
                                        pRouteAgent, MSG_ROUTE_CAT_SERVICE,
                                        cb, FALSE ) ;
         if ( rc != SDB_OK )
         {
            rtnCoordClearRequest( cb, sendNodes );
            PD_LOG ( PDERROR, "Failed to get cata-group-info from "
                     "catalogue-node,send group-info-request failed, rc: %d",
                     rc ) ;
            break ;
         }

         REPLY_QUE replyQue;
         rc = rtnCoordGetReply( cb, sendNodes, replyQue,
                                MSG_CAT_GRP_RES ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDWARNING, "Failed to get cata-group-info from "
                     "catalogue-node, get reply failed, rc: %d", rc );
            break ;
         }

         BOOLEAN getExpected = FALSE ;
         UINT32 primaryID = 0 ;
         while ( !replyQue.empty() )
         {
            MsgHeader *pReply = NULL;
            pReply = (MsgHeader *)(replyQue.front());
            replyQue.pop();

            if ( FALSE == getExpected )
            {
               nodeID.value = pReply->routeID.value ;
               primaryID = MSG_GET_INNER_REPLY_STARTFROM(pReply) ;
               rc = rtnCoordProcessGetGroupReply( pReply, groupInfo ) ;
               if ( SDB_OK == rc )
               {
                  getExpected = TRUE ;
                  pCoordcb->updateCatGroupInfo( groupInfo );
                  rc = rtnCoordUpdateRoute( groupInfo,  pRouteAgent,
                                            MSG_ROUTE_CAT_SERVICE ) ;
                  // update shard service also
                  rtnCoordUpdateRoute( groupInfo, pRouteAgent,
                                       MSG_ROUTE_SHARD_SERVCIE ) ;
               }
            }
            if ( NULL != pReply )
            {
               SDB_OSS_FREE( pReply );
            }
         }

         if ( rc != SDB_OK )
         {
            if ( rtnCoordGroupReplyCheck( cb, rc, rtnCoordCanRetry( times++ ),
                                          nodeID, cataGroupInfo, NULL,
                                          TRUE, primaryID, TRUE ) )
            {
               continue ;
            }
            PD_LOG( PDERROR, "Get catalog group info from remote failed, "
                    "rc: %d", rc ) ;
         }
         break ;
      }while ( TRUE ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOGETREMOTECATGROUPINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOUPDATEROUTE, "rtnCoordUpdateRoute" )
   INT32 rtnCoordUpdateRoute ( CoordGroupInfoPtr &groupInfo,
                               netMultiRouteAgent *pRouteAgent,
                               MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOUPDATEROUTE ) ;

      string host ;
      string service ;
      MsgRouteID routeID ;
      routeID.value = MSG_INVALID_ROUTEID ;

      UINT32 index = 0 ;
      clsGroupItem *groupItem = groupInfo.get() ;
      while ( SDB_OK == groupItem->getNodeInfo( index++, routeID, host,
                                                service, type ) )
      {
         rc = pRouteAgent->updateRoute( routeID, host.c_str(),
                                        service.c_str() ) ;
         if ( rc != SDB_OK )
         {
            if ( SDB_NET_UPDATE_EXISTING_NODE == rc )
            {
               rc = SDB_OK;
            }
            else
            {
               PD_LOG ( PDERROR, "update route failed (rc=%d)", rc ) ;
               break;
            }
         }
      }
      PD_TRACE_EXITRC ( SDB_RTNCOUPDATEROUTE, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETGROUPSBYCATAINFO, "rtnCoordGetGroupsByCataInfo" )
   INT32 rtnCoordGetGroupsByCataInfo( CoordCataInfoPtr &cataInfo,
                                      CoordGroupList &sendGroupLst,
                                      CoordGroupList &groupLst,
                                      pmdEDUCB *cb,
                                      BOOLEAN *pHasUpdate,
                                      const BSONObj *pQuery )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOGETGROUPSBYCATAINFO ) ;
      if ( !cataInfo->isMainCL() )
      {
         // normal collection or sub-collection
         if ( NULL == pQuery || pQuery->isEmpty() )
         {
            cataInfo->getGroupLst( groupLst ) ;
         }
         else
         {
            rc = cataInfo->getGroupByMatcher( *pQuery, groupLst ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to get group by matcher(rc=%d)",
                         rc ) ;
         }

         if ( groupLst.size() <= 0 )
         {
            if ( pQuery )
            {
               PD_LOG( PDWARNING, "Failed to get groups for obj[%s] from "
                       "catalog info[%s]", pQuery->toString().c_str(),
                       cataInfo->getCatalogSet()->toCataInfoBson(
                       ).toString().c_str() ) ;
            }
         }
         else
         {
            //don't resend to the node which reply ok
            CoordGroupList::iterator iter = sendGroupLst.begin();
            while( iter != sendGroupLst.end() )
            {
               groupLst.erase( iter->first );
               ++iter;
            }
         }
      }
      else
      {
         // main-collection
         vector< string > subCLLst ;

         if ( NULL == pQuery || pQuery->isEmpty() )
         {
            cataInfo->getSubCLList( subCLLst ) ;
         }
         else
         {
            cataInfo->getMatchSubCLs( *pQuery, subCLLst ) ;
         }

         if ( 0 == subCLLst.size() &&
              ( !pHasUpdate ||
                ( pHasUpdate && !(*pHasUpdate) ) ) )
         {
            if ( SDB_OK == rtnCoordGetRemoteCata( cb, cataInfo->getName(),
                                                  cataInfo ) )
            {
               if ( pHasUpdate )
               {
                  *pHasUpdate = TRUE ;
               }
               if ( NULL == pQuery || pQuery->isEmpty() )
               {
                  cataInfo->getSubCLList( subCLLst ) ;
               }
               else
               {
                  cataInfo->getMatchSubCLs( *pQuery, subCLLst ) ;
               }
            }
         }

         vector< string >::iterator iterCL = subCLLst.begin() ;
         while( iterCL != subCLLst.end() )
         {
            CoordCataInfoPtr subCataInfo ;
            CoordGroupList groupLstTmp ;
            CoordGroupList::iterator iterGrp ;
            rc = rtnCoordGetCataInfo( cb, (*iterCL).c_str(), FALSE,
                                      subCataInfo, NULL ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to get sub-collection catalog-info(rc=%d)",
                         rc ) ;
            rc = rtnCoordGetGroupsByCataInfo( subCataInfo,
                                              sendGroupLst,
                                              groupLstTmp,
                                              cb,
                                              NULL,
                                              pQuery ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection group-info(rc=%d)",
                         rc ) ;

            iterGrp = groupLstTmp.begin() ;
            while ( iterGrp != groupLstTmp.end() )
            {
               groupLst[ iterGrp->first ] = iterGrp->second ;
               ++iterGrp ;
            }
            ++iterCL ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOGETGROUPSBYCATAINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODESWITHOUTREPLY, "rtnCoordSendRequestToNodesWithOutReply" )
   void rtnCoordSendRequestToNodesWithOutReply( void *pBuffer,
                                                ROUTE_SET &nodes,
                                                netMultiRouteAgent *pRouteAgent )
   {
      PD_TRACE_ENTRY ( SDB_RTNCOSENDREQUESTTONODESWITHOUTREPLY ) ;
      pRouteAgent->multiSyncSend( nodes, pBuffer ) ;
      PD_TRACE_EXIT ( SDB_RTNCOSENDREQUESTTONODESWITHOUTREPLY ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODEWITHOUTCHECK, "rtnCoordSendRequestToNodeWithoutCheck" )
   INT32 rtnCoordSendRequestToNodeWithoutCheck( void *pBuffer,
                                                const MsgRouteID &routeID,
                                                netMultiRouteAgent *pRouteAgent,
                                                pmdEDUCB *cb,
                                                REQUESTID_MAP &sendNodes )
   {
      INT32 rc = SDB_OK;
      UINT64 reqID = 0;
      PD_TRACE_ENTRY ( SDB_RTNCOSENDREQUESTTONODEWITHOUTCHECK ) ;
      rc = pRouteAgent->syncSendWithoutCheck( routeID, pBuffer, reqID, cb );
      PD_RC_CHECK ( rc, PDERROR, "Failed to send the request to node"
                    "(groupID=%u, nodeID=%u, serviceID=%u, rc=%d)",
                    routeID.columns.groupID,
                    routeID.columns.nodeID,
                    routeID.columns.serviceID,
                    rc ) ;
      sendNodes[ reqID ] = routeID ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOSENDREQUESTTONODEWITHOUTCHECK, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODEWITHOUTREPLY, "rtnCoordSendRequestToNodeWithoutReply" )
   INT32 rtnCoordSendRequestToNodeWithoutReply( void *pBuffer,
                                                MsgRouteID &routeID,
                                                netMultiRouteAgent *pRouteAgent )
   {
      INT32 rc = SDB_OK;
      UINT64 reqID = 0;
      PD_TRACE_ENTRY ( SDB_RTNCOSENDREQUESTTONODEWITHOUTREPLY ) ;
      rc = pRouteAgent->syncSend( routeID, pBuffer, reqID, NULL );
      PD_RC_CHECK ( rc, PDERROR, "Failed to send the request to node"
                    "(groupID=%u, nodeID=%u, serviceID=%u, rc=%d)",
                    routeID.columns.groupID,
                    routeID.columns.nodeID,
                    routeID.columns.serviceID,
                    rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOSENDREQUESTTONODEWITHOUTREPLY, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODE, "rtnCoordSendRequestToNode" )
   INT32 rtnCoordSendRequestToNode( void *pBuffer,
                                    MsgRouteID routeID,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb,
                                    REQUESTID_MAP &sendNodes )
   {
      INT32 rc = SDB_OK;
      UINT64 reqID = 0;
      PD_TRACE_ENTRY ( SDB_RTNCOSENDREQUESTTONODE ) ;
      rc = pRouteAgent->syncSend( routeID, pBuffer, reqID, cb );
      PD_RC_CHECK ( rc, PDERROR, "Failed to send the request to node"
                    "(groupID=%u, nodeID=%u, serviceID=%u, rc=%d)",
                    routeID.columns.groupID,
                    routeID.columns.nodeID,
                    routeID.columns.serviceID,
                    rc ) ;
      sendNodes[ reqID ] = routeID ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOSENDREQUESTTONODE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODE2, "rtnCoordSendRequestToNode" )
   INT32 rtnCoordSendRequestToNode( void *pBuffer,
                                    MsgRouteID routeID,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb,
                                    const netIOVec &iov,
                                    REQUESTID_MAP &sendNodes )
   {
      INT32 rc = SDB_OK;
      UINT64 reqID = 0;
      PD_TRACE_ENTRY ( SDB_RTNCOSENDREQUESTTONODE2 ) ;
      rc = pRouteAgent->syncSend( routeID, ( MsgHeader *)pBuffer, iov, reqID, cb );
      PD_RC_CHECK ( rc, PDERROR,
                  "failed to send the request to node"
                  "(groupID=%u, nodeID=%u, serviceID=%u, rc=%d)",
                  routeID.columns.groupID,
                  routeID.columns.nodeID,
                  routeID.columns.serviceID,
                  rc );
      sendNodes[ reqID ] = routeID;
   done:
      PD_TRACE_EXITRC ( SDB_RTNCOSENDREQUESTTONODE2, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODEGROUPS, "rtnCoordSendRequestToNodeGroups" )
   INT32 rtnCoordSendRequestToNodeGroups( CHAR *pBuffer,
                                          CoordGroupList &groupLst,
                                          CoordGroupMap &mapGroupInfo,
                                          BOOLEAN isSendPrimary,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          REQUESTID_MAP &sendNodes,
                                          BOOLEAN isResend,
                                          MSG_ROUTE_SERVICE_TYPE type,
                                          BOOLEAN ignoreError )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOSENDREQUESTTONODEGROUPS ) ;
      CoordGroupList::iterator iter ;

      rc = rtnGroupList2GroupPtr( cb, groupLst, mapGroupInfo, FALSE ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Get groups info failed, rc: %d", rc ) ;
         goto error ;
      }

      iter = groupLst.begin() ;
      while ( iter != groupLst.end() )
      {
         rc = rtnCoordSendRequestToNodeGroup( pBuffer,
                                              mapGroupInfo[ iter->first ],
                                              isSendPrimary, pRouteAgent,
                                              cb, sendNodes, isResend,
                                              type ) ;
         if ( SDB_OK != rc )
         {
            MsgHeader *pMsg = ( MsgHeader* )pBuffer ;
            PD_LOG( PDERROR, "Send msg[opCode: %d, TID: %u] to group[%u] "
                    "failed, rc:%d", pMsg->opCode, pMsg->TID,
                    iter->first, rc ) ;
            if ( !ignoreError )
            {
               goto error ;
            }
         }
         ++iter;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOSENDREQUESTTONODEGROUPS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODEGROUPS2, "rtnCoordSendRequestToNodeGroups" )
   INT32 rtnCoordSendRequestToNodeGroups( MsgHeader *pBuffer,
                                          CoordGroupList &groupLst,
                                          CoordGroupMap &mapGroupInfo,
                                          BOOLEAN isSendPrimary,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          const netIOVec &iov,
                                          REQUESTID_MAP &sendNodes, // out
                                          BOOLEAN isResend,
                                          MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOSENDREQUESTTONODEGROUPS2 ) ;
      CoordGroupList::iterator iter ;

      rc = rtnGroupList2GroupPtr( cb, groupLst, mapGroupInfo, FALSE ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Get groups info failed, rc: %d" ) ;
         goto error ;
      }

      iter = groupLst.begin() ;
      while ( iter != groupLst.end() )
      {
         rc = rtnCoordSendRequestToNodeGroup( pBuffer,
                                              mapGroupInfo[ iter->first ],
                                              isSendPrimary, pRouteAgent,
                                              cb, iov, sendNodes,
                                              isResend, type ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Send msg[opCode: %d, TID: %u] to group[%u] "
                    "failed, rc:%d", pBuffer->opCode, pBuffer->TID,
                    iter->first, rc ) ;
            goto error ;
         }
         ++iter ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCOSENDREQUESTTONODEGROUPS2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOSENDREQUESTTONODEGROUPS3, "rtnCoordSendRequestToNodeGroups" )
   INT32 rtnCoordSendRequestToNodeGroups( MsgHeader *pBuffer,
                                          CoordGroupList &groupLst,
                                          CoordGroupMap &mapGroupInfo,
                                          BOOLEAN isSendPrimary,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          GROUP_2_IOVEC &iov,
                                          REQUESTID_MAP &sendNodes, // out
                                          BOOLEAN isResend,
                                          MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOSENDREQUESTTONODEGROUPS3 ) ;
      CoordGroupList::iterator iter ;
      GROUP_2_IOVEC::iterator itIO ;
      netIOVec *pCommonIO = NULL ;
      netIOVec *pIOVec = NULL ;

      rc = rtnGroupList2GroupPtr( cb, groupLst, mapGroupInfo, FALSE ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Get groups info failed, rc: %d" ) ;
         goto error ;
      }

      // find common iovec
      itIO = iov.find( 0 ) ; // group id is 0 for common iovec
      if ( iov.end() != itIO )
      {
         pCommonIO = &( itIO->second ) ;
      }

      iter = groupLst.begin() ;
      while ( iter != groupLst.end() )
      {
         // find groups iovec
         itIO = iov.find( iter->first ) ;
         if ( itIO != iov.end() )
         {
            pIOVec = &( itIO->second ) ;
         }
         else if ( pCommonIO )
         {
            pIOVec = pCommonIO ;
         }
         else
         {
            // error, not find the io datas
            PD_LOG( PDERROR, "Can't find the group[%d]'s iovec datas",
                    iter->first ) ;
            rc = SDB_SYS ;
            SDB_ASSERT( FALSE, "Group iovec is null" ) ;
            goto error ;
         }

         rc = rtnCoordSendRequestToNodeGroup( pBuffer,
                                              mapGroupInfo[ iter->first ],
                                              isSendPrimary, pRouteAgent,
                                              cb, *pIOVec, sendNodes,
                                              isResend, type ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Send msg[opCode: %d, TID: %u] to group[%u] "
                    "failed, rc:%d", pBuffer->opCode, pBuffer->TID,
                    iter->first, rc ) ;
            goto error ;
         }
         ++iter ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCOSENDREQUESTTONODEGROUPS3, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordSendRequestToNodeGroup( MsgHeader *pBuffer,
                                         CoordGroupInfoPtr &groupInfo,
                                         BOOLEAN isSendPrimary,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         const netIOVec &iov,
                                         REQUESTID_MAP &sendNodes,
                                         BOOLEAN isResend,
                                         MSG_ROUTE_SERVICE_TYPE type )
   {
      return _rtnCoordSendRequestToNodeGroup( pBuffer, groupInfo, isSendPrimary,
                                              pRouteAgent, type, cb,
                                              sendNodes, &iov, isResend ) ;
   }

   INT32 rtnCoordSendRequestToNodeGroup( CHAR *pBuffer,
                                         CoordGroupInfoPtr &groupInfo,
                                         BOOLEAN isSendPrimary,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         REQUESTID_MAP &sendNodes,
                                         BOOLEAN isResend,
                                         MSG_ROUTE_SERVICE_TYPE type )
   {
      return _rtnCoordSendRequestToNodeGroup( (MsgHeader*)pBuffer, groupInfo,
                                              isSendPrimary, pRouteAgent,
                                              type, cb, sendNodes, NULL,
                                              isResend ) ;
   }

   INT32 rtnCoordSendRequestToPrimary( CHAR *pBuffer,
                                       CoordGroupInfoPtr &groupInfo,
                                       REQUESTID_MAP &sendNodes,
                                       netMultiRouteAgent *pRouteAgent,
                                       MSG_ROUTE_SERVICE_TYPE type,
                                       pmdEDUCB *cb )
   {
      return _rtnCoordSendRequestToPrimary( (MsgHeader*)pBuffer, groupInfo,
                                            pRouteAgent, type, cb,
                                            sendNodes, NULL ) ;
   }

   INT32 rtnCoordSendRequestToPrimary( MsgHeader *pBuffer,
                                       CoordGroupInfoPtr &groupInfo,
                                       REQUESTID_MAP &sendNodes,
                                       netMultiRouteAgent *pRouteAgent,
                                       const netIOVec &iov,
                                       MSG_ROUTE_SERVICE_TYPE type,
                                       pmdEDUCB *cb )
   {
      return _rtnCoordSendRequestToPrimary( pBuffer, groupInfo, pRouteAgent,
                                            type, cb, sendNodes, &iov ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOGETNODEPOS, "rtnCoordGetNodePos" )
   void rtnCoordGetNodePos( INT32 preferReplicaType,
                            clsGroupItem *groupItem,
                            UINT32 random,
                            UINT32 &pos )
   {
      UINT32 posTmp = 0 ;

      PD_TRACE_ENTRY( SDB_RTNCOGETNODEPOS ) ;

      switch( preferReplicaType )
      {
         case PREFER_REPL_NODE_1:
         case PREFER_REPL_NODE_2:
         case PREFER_REPL_NODE_3:
         case PREFER_REPL_NODE_4:
         case PREFER_REPL_NODE_5:
         case PREFER_REPL_NODE_6:
         case PREFER_REPL_NODE_7:
            {
               posTmp = preferReplicaType - 1 ;
               break;
            }
         case PREFER_REPL_MASTER:
            {
               posTmp = groupItem->getPrimaryPos() ;
               // if there is no primary,
               // then do not break and go on to
               // get random node
               if ( CLS_RG_NODE_POS_INVALID != posTmp )
               {
                  break ;
               }
            }
         case PREFER_REPL_ANYONE:
         case PREFER_REPL_SLAVE:
         default:
            {
               posTmp = random ;
               break;
            }
      }

      if( groupItem->nodeCount() > 0 )
      {
         pos = posTmp % groupItem->nodeCount() ;
      }
      PD_TRACE_EXIT( SDB_RTNCOGETNODEPOS ) ;
   }

   void rtnCoordGetNextNode( INT32 preferReplicaType,
                             clsGroupItem *groupItem,
                             UINT32 &selTimes,
                             UINT32 &curPos )
   {
      if( selTimes >= groupItem->nodeCount() )
      {
         curPos = CLS_RG_NODE_POS_INVALID ;
      }
      else
      {
         UINT32 tmpPos = curPos ;
         if ( selTimes > 0 )
         {
            tmpPos = ( curPos + 1 ) % groupItem->nodeCount() ;
         }

         if ( PREFER_REPL_SLAVE == preferReplicaType )
         {
            UINT32 pimaryPos = groupItem->getPrimaryPos() ;

            if ( CLS_RG_NODE_POS_INVALID != pimaryPos &&
                 selTimes + 1 == groupItem->nodeCount() )
            {
               tmpPos = pimaryPos ;
            }
            else if ( tmpPos == pimaryPos )
            {
               tmpPos = ( tmpPos + 1 ) % groupItem->nodeCount() ;
            }
         }

         curPos = tmpPos ;
         ++selTimes ;
      }
   }

   INT32 rtnCoordSendRequestToOne( CHAR *pBuffer,
                                   CoordGroupInfoPtr &groupInfo,
                                   REQUESTID_MAP &sendNodes,
                                   netMultiRouteAgent *pRouteAgent,
                                   MSG_ROUTE_SERVICE_TYPE type,
                                   pmdEDUCB *cb,
                                   BOOLEAN isResend )
   {
      return _rtnCoordSendRequestToOne( ( MsgHeader*) pBuffer, groupInfo,
                                         sendNodes, pRouteAgent, type,
                                         cb, NULL, isResend ) ;
   }

   INT32 rtnCoordSendRequestToOne( MsgHeader *pBuffer,
                                   CoordGroupInfoPtr &groupInfo,
                                   REQUESTID_MAP &sendNodes,
                                   netMultiRouteAgent *pRouteAgent,
                                   const netIOVec &iov,
                                   MSG_ROUTE_SERVICE_TYPE type,
                                   pmdEDUCB *cb,
                                   BOOLEAN isResend )
   {
      return _rtnCoordSendRequestToOne( pBuffer, groupInfo, sendNodes,
                                        pRouteAgent, type, cb,
                                        &iov, isResend ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOPROCESSGETGROUPREPLY, "rtnCoordProcessGetGroupReply" )
   INT32 rtnCoordProcessGetGroupReply ( MsgHeader *pReply,
                                        CoordGroupInfoPtr &groupInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOPROCESSGETGROUPREPLY ) ;
      UINT32 headerLen = MSG_GET_INNER_REPLY_HEADER_LEN(pReply) ;

      do
      {
         if ( SDB_OK == MSG_GET_INNER_REPLY_RC(pReply) &&
              (UINT32)pReply->messageLength >= headerLen + 5 )
         {
            try
            {
               BSONObj boGroupInfo( MSG_GET_INNER_REPLY_DATA(pReply) );
               BSONElement beGroupID = boGroupInfo.getField( CAT_GROUPID_NAME );
               if ( beGroupID.eoo() || !beGroupID.isNumber() )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDERROR,
                           "Process get-group-info-reply failed,"
                           "failed to get the field(%s)", CAT_GROUPID_NAME );
                  break;
               }
               CoordGroupInfo *pGroupInfo
                           = SDB_OSS_NEW CoordGroupInfo( beGroupID.number() );
               if ( NULL == pGroupInfo )
               {
                  rc = SDB_OOM;
                  PD_LOG ( PDERROR, "Process get-group-info-reply failed,"
                           "new failed ");
                  break;
               }
               CoordGroupInfoPtr groupInfoTmp( pGroupInfo );
               rc = groupInfoTmp->updateGroupItem( boGroupInfo );
               if ( rc != SDB_OK )
               {
                  PD_LOG ( PDERROR, "Process get-group-info-reply failed,"
                           "failed to parse the groupInfo(rc=%d)", rc );
                  break;
               }
               groupInfo = groupInfoTmp;
            }
            catch ( std::exception &e )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Process get-group-info-reply failed,"
                        "received unexpected error:%s", e.what() );
               break;
            }
         }
         else
         {
            rc = MSG_GET_INNER_REPLY_RC(pReply) ;
         }
      }while ( FALSE );
      PD_TRACE_EXITRC ( SDB_RTNCOPROCESSGETGROUPREPLY, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOPROCESSQUERYCATREPLY, "rtnCoordProcessQueryCatReply" )
   INT32 rtnCoordProcessQueryCatReply ( MsgCatQueryCatRsp *pReply,
                                        CoordCataInfoPtr &cataInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOPROCESSQUERYCATREPLY ) ;

      do
      {
         if ( 0 == pReply->flags )
         {
            try
            {
               BSONObj boCataInfo( (CHAR *)pReply + sizeof(MsgCatQueryCatRsp) );
               BSONElement beName = boCataInfo.getField( CAT_CATALOGNAME_NAME );
               if ( beName.eoo() || beName.type() != String )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDERROR, "Failed to parse query-catalogue-reply,"
                           "failed to get the field(%s)",
                           CAT_CATALOGNAME_NAME );
                  break;
               }
               BSONElement beVersion = boCataInfo.getField( CAT_CATALOGVERSION_NAME );
               if ( beVersion.eoo() || !beVersion.isNumber() )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG ( PDERROR, "Failed to parse query-catalogue-reply, "
                           "failed to get the field(%s)",
                           CAT_CATALOGVERSION_NAME );
                  break;
               }

               // the pCataInfoTmp will be deleted by smart-point automatically
               CoordCataInfo *pCataInfoTmp = NULL;
               pCataInfoTmp = SDB_OSS_NEW CoordCataInfo( beVersion.number(),
                                                         beName.str().c_str() );
               if ( NULL == pCataInfoTmp )
               {
                  rc = SDB_OOM;
                  PD_LOG ( PDERROR,
                           "Failed to parse query-catalogue-reply, new failed");
                  break;
               }
               CoordCataInfoPtr cataInfoPtr( pCataInfoTmp );
               rc = cataInfoPtr->fromBSONObj( boCataInfo );
               if ( rc != SDB_OK )
               {
                  PD_LOG ( PDERROR, "Failed to parse query-catalogue-reply, "
                           "parse catalogue info from bson-obj failed(rc=%d)",
                           rc );
                  break;
               }
               PD_LOG ( PDDEBUG, "new catalog info: %s",
                        boCataInfo.toString().c_str() );
               cataInfo = cataInfoPtr ;
            }
            catch ( std::exception &e )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to parse query-catalogue-reply,"
                        "received unexcepted error:%s", e.what() );
               break;
            }
         }
         else
         {
            
            PD_LOG ( PDWARNING, "Recieve unexcepted reply while query "
                     "catalogue(flag=%d)", pReply->flags ) ;
         }
         rc= pReply->flags ;
      }while ( FALSE ) ;

      PD_TRACE_EXITRC ( SDB_RTNCOPROCESSQUERYCATREPLY, rc ) ;
      return rc;
   }

   INT32 rtnCoordGetAllGroupList( pmdEDUCB * cb, CoordGroupList &groupList,
                                  const BSONObj *query, BOOLEAN exceptCata,
                                  BOOLEAN exceptCoord,
                                  BOOLEAN useLocalWhenFailed )
   {
      INT32 rc = SDB_OK ;
      GROUP_VEC vecGrpPtr ;

      rc = rtnCoordGetAllGroupList( cb, vecGrpPtr, query, exceptCata,
                                    exceptCoord, useLocalWhenFailed ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         GROUP_VEC::iterator it = vecGrpPtr.begin() ;
         while ( it != vecGrpPtr.end() )
         {
            CoordGroupInfoPtr &ptr = *it ;
            groupList[ ptr->groupID() ] = ptr->groupID() ;
            ++it ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordGetAllGroupList( pmdEDUCB * cb, GROUP_VEC &groupLst,
                                  const BSONObj *query, BOOLEAN exceptCata,
                                  BOOLEAN exceptCoord,
                                  BOOLEAN useLocalWhenFailed )
   {
      INT32 rc = SDB_OK;
      pmdKRCB *pKrcb = pmdGetKRCB();
      SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
      CoordCB *pCoordcb = pKrcb->getCoordCB();
      INT32 bufferSize = 0;
      rtnCoordCommand *pCmdProcesser = NULL;
      CHAR *pListReq = NULL;
      SINT64 contextID = -1;
      rtnContextBuf buffObj ;
      UINT64 identify = pCoordcb->getGrpIdentify() ;

      rtnCoordProcesserFactory *pProcesserFactory
               = pCoordcb->getProcesserFactory();
      pCmdProcesser = pProcesserFactory->getCommandProcesser(
         COORD_CMD_LISTGROUPS ) ;
      SDB_ASSERT( pCmdProcesser, "pCmdProcesser can't be NULL!" );
      rc = msgBuildQueryMsg( &pListReq, &bufferSize, COORD_CMD_LISTGROUPS,
                             0, 0, 0, -1, query ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to build list groups request(rc=%d)",
                   rc );
      rc = pCmdProcesser->execute( (MsgHeader*)pListReq, cb,
                                   contextID, &buffObj ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to list groups(rc=%d)", rc ) ;

      while ( TRUE )
      {
         rc = rtnGetMore( contextID, 1, buffObj, cb, pRtncb ) ;
         if ( rc )
         {
            if ( rc != SDB_DMS_EOC )
            {
               PD_RC_CHECK( rc, PDERROR, "failed to execute getmore(rc=%d)",
                            rc );
            }
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }

         try
         {
            CoordGroupInfo *pGroupInfo = NULL ;
            CoordGroupInfoPtr groupInfoTmp ;
            BSONObj boGroupInfo( buffObj.data() ) ;
            BSONElement beGroupID = boGroupInfo.getField( CAT_GROUPID_NAME );
            PD_CHECK( beGroupID.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "failed to process group info, failed to get the field"
                      "(%s)", CAT_GROUPID_NAME ) ;
            pGroupInfo = SDB_OSS_NEW CoordGroupInfo( beGroupID.number() );
            PD_CHECK( pGroupInfo != NULL, SDB_OOM, error, PDERROR,
                      "malloc failed!" );
            groupInfoTmp = CoordGroupInfoPtr( pGroupInfo ) ;
            rc = groupInfoTmp->updateGroupItem( boGroupInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to parse the group info(rc=%d)",
                         rc ) ;

            if ( groupInfoTmp->groupID() == CATALOG_GROUPID )
            {
               if ( !exceptCata )
               {
                  groupLst.push_back( groupInfoTmp ) ;
               }
            }
            else if ( groupInfoTmp->groupID() == COORD_GROUPID )
            {
               if ( !exceptCoord )
               {
                  groupLst.push_back( groupInfoTmp ) ;
               }
            }
            else
            {
               groupLst.push_back( groupInfoTmp );
            }
            pCoordcb->addGroupInfo( groupInfoTmp, TRUE ) ;
            rc = rtnCoordUpdateRoute( groupInfoTmp, pCoordcb->getRouteAgent(),
                                      MSG_ROUTE_SHARD_SERVCIE ) ;
            // update cata service also
            if ( groupInfoTmp->groupID() == CATALOG_GROUPID )
            {
               rtnCoordUpdateRoute( groupInfoTmp, pCoordcb->getRouteAgent(),
                                    MSG_ROUTE_CAT_SERVICE ) ;
               pCoordcb->updateCatGroupInfo( groupInfoTmp, TRUE ) ;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_RC_CHECK( rc, PDERROR, "Failed to process group info, received "
                         "unexpected error:%s", e.what() ) ;
         }
      }

      if ( NULL == query || query->isEmpty() )
      {
         /// clear the out-of-date group info
         pCoordcb->invalidateGroupInfo( identify ) ;
      }

   done:
      if ( -1 != contextID )
      {
         pRtncb->contextDelete( contextID, cb );
      }
      SAFE_OSS_FREE( pListReq ) ;
      return rc;
   error:
      if ( useLocalWhenFailed )
      {
         pCoordcb->getLocalGroupList( groupLst, exceptCata, exceptCoord ) ;
         rc = SDB_OK ;
      }
      goto done;
   }

   INT32 rtnCoordSendRequestToNodes( void *pBuffer,
                                     ROUTE_SET &nodes,
                                     netMultiRouteAgent *pRouteAgent,
                                     pmdEDUCB *cb,
                                     REQUESTID_MAP &sendNodes,
                                     ROUTE_RC_MAP &failedNodes )
   {
      INT32 rc = SDB_OK;
      ROUTE_SET::iterator iter = nodes.begin();
      while( iter != nodes.end() )
      {
         MsgRouteID routeID;
         routeID.value = *iter;
         rc = rtnCoordSendRequestToNode( pBuffer, routeID,
                                         pRouteAgent, cb,
                                         sendNodes );
         if ( rc != SDB_OK )
         {
            rtnCoordUpdateNodeStatByRC( cb, routeID, rc ) ;
            failedNodes[ *iter ] = rc ;
         }
         ++iter;
      }
      return SDB_OK ;
   }

   INT32 rtnCoordReadALine( const CHAR *&pInput,
                            CHAR *pOutput )
   {
      INT32 rc = SDB_OK;
      while( *pInput != 0x0D && *pInput != 0x0A && *pInput != '\0' )
      {
         *pOutput = *pInput;
         ++pInput;
         ++pOutput;
      }
      *pOutput = '\0';
      return rc;
   }

   void rtnCoordClearRequest( pmdEDUCB *cb, REQUESTID_MAP &sendNodes )
   {
      REQUESTID_MAP::iterator iterMap = sendNodes.begin();
      while( iterMap != sendNodes.end() )
      {
         cb->getCoordSession()->delRequest( iterMap->first );
         iterMap = sendNodes.erase( iterMap );
      }
   }

   INT32 rtnCoordGetSubCLsByGroups( const CoordSubCLlist &subCLList,
                                    const CoordGroupList &sendGroupList,
                                    pmdEDUCB *cb,
                                    CoordGroupSubCLMap &groupSubCLMap,
                                    const BSONObj *query )
   {
      INT32 rc = SDB_OK;
      CoordGroupList::const_iterator iterSend;
      CoordSubCLlist::const_iterator iterCL = subCLList.begin();
      while( iterCL != subCLList.end() )
      {
         CoordCataInfoPtr cataInfo;
         CoordGroupList groupList;
         CoordGroupList::iterator iterGroup;
         rc = rtnCoordGetCataInfo( cb, (*iterCL).c_str(),
                                   FALSE, cataInfo );
         PD_RC_CHECK( rc, PDWARNING, "Failed to get catalog info of "
                      "sub-collection[%s], rc: %d", (*iterCL).c_str(),
                      rc ) ;
         if ( NULL == query || query->isEmpty() )
         {
            cataInfo->getGroupLst( groupList );
         }
         else
         {
            cataInfo->getGroupByMatcher( *query, groupList ) ;
         }
         // SDB_ASSERT( groupList.size() > 0, "group list can't be empty!" );
         iterGroup = groupList.begin();
         while( iterGroup != groupList.end() )
         {
            groupSubCLMap[ iterGroup->first ].push_back( (*iterCL) );
            ++iterGroup;
         }
         ++iterCL;
      }

      iterSend = sendGroupList.begin();
      while( iterSend != sendGroupList.end() )
      {
         groupSubCLMap.erase( iterSend->first ) ;
         ++iterSend;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static INT32 _rtnCoordParseGroupsInfo( const BSONObj &obj,
                                          vector< INT32 > &vecID,
                                          vector< const CHAR* > &vecName,
                                          BSONObj *pNewObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BOOLEAN isModify = FALSE ;

      BSONObjIterator it( obj ) ;
      while( it.more() )
      {
         BSONElement ele = it.next() ;

         // $and:[{GroupID:1000}, {GroupName:"xxx"}]
         if ( Array == ele.type() &&
              0 == ossStrcmp( ele.fieldName(), "$and" ) )
         {
            BSONArrayBuilder sub( builder.subarrayStart( ele.fieldName() ) ) ;
            BSONObj tmpNew ;
            BSONObjIterator tmpItr( ele.embeddedObject() ) ;
            while ( tmpItr.more() )
            {
               BSONElement tmpE = tmpItr.next() ;
               if ( Object != tmpE.type() )
               {
                  PD_LOG( PDERROR, "Parse obj[%s] groups failed: "
                          "invalid $and", obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  BSONObj tmpObj = tmpE.embeddedObject() ;
                  rc = _rtnCoordParseGroupsInfo( tmpObj, vecID, vecName,
                                                 pNewObj ? &tmpNew : NULL ) ;
                  PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] groups failed",
                               obj.toString().c_str() ) ;
                  if ( pNewObj )
                  {
                     if ( tmpNew.objdata() != tmpObj.objdata() )
                     {
                        isModify = TRUE ;
                        sub.append( tmpNew ) ;
                     }
                     else
                     {
                        sub.append( tmpObj ) ;
                     }
                  }
               }
            }
            sub.done() ;
         } /// end $and
         // group id
         else if ( 0 == ossStrcasecmp( ele.fieldName(), CAT_GROUPID_NAME ) &&
                   SDB_OK == _rtnCoordParseInt( ele, vecID,
                                                RTN_COORD_PARSE_MASK_ALL ) )
         {
            isModify = TRUE ;
         }
         // group name
         else if ( ( 0 == ossStrcasecmp( ele.fieldName(),
                                       FIELD_NAME_GROUPNAME ) ||
                     0 == ossStrcasecmp( ele.fieldName(),
                                       FIELD_NAME_GROUPS ) ) &&
                    SDB_OK == _rtnCoordParseString( ele, vecName,
                                                    RTN_COORD_PARSE_MASK_ALL ) )
         {
            isModify = TRUE ;
         }
         else if ( pNewObj )
         {
            builder.append( ele ) ;
         }
      }

      if ( pNewObj )
      {
         if ( isModify )
         {
            *pNewObj = builder.obj() ;
         }
         else
         {
            *pNewObj = obj ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordParseGroupList( pmdEDUCB *cb,
                                 const BSONObj &obj,
                                 CoordGroupList &groupList,
                                 BSONObj *pNewObj )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr grpPtr ;
      vector< INT32 > tmpVecInt ;
      vector< const CHAR* > tmpVecStr ;
      UINT32 i = 0 ;

      rc = _rtnCoordParseGroupsInfo( obj, tmpVecInt, tmpVecStr, pNewObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse object[%s] group list failed, rc: %d",
                   obj.toString().c_str(), rc ) ;

      /// group id
      for ( i = 0 ; i < tmpVecInt.size() ; ++i )
      {
         groupList[(UINT32)tmpVecInt[i]] = (UINT32)tmpVecInt[i] ;
      }

      /// group name
      for ( i = 0 ; i < tmpVecStr.size() ; ++i )
      {
         rc = rtnCoordGetGroupInfo( cb, tmpVecStr[i], FALSE, grpPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Get group[%s] info failed, rc: %d",
                      tmpVecStr[i], rc ) ;
         groupList[ grpPtr->groupID() ] = grpPtr->groupID() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj* rtnCoordGetFilterByID( FILTER_BSON_ID filterID,
                                   rtnQueryOptions &queryOption )
   {
      BSONObj *pFilter = NULL ;
      switch ( filterID )
      {
         case FILTER_ID_SELECTOR:
            pFilter = &queryOption._selector ;
            break ;
         case FILTER_ID_ORDERBY:
            pFilter = &queryOption._orderBy ;
            break ;
         case FILTER_ID_HINT:
            pFilter = &queryOption._hint ;
            break ;
         default:
            pFilter = &queryOption._query ;
            break ;
      }
      return pFilter ;
   }

   INT32 rtnCoordParseGroupList( pmdEDUCB *cb, MsgOpQuery *pMsg,
                                 FILTER_BSON_ID filterObjID,
                                 CoordGroupList &groupList )
   {
      INT32 rc = SDB_OK ;
      rtnQueryOptions queryOption ;
      BSONObj *pFilterObj = NULL ;

      rc = queryOption.fromQueryMsg( (CHAR *)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query msg failed, rc: %d", rc ) ;

      pFilterObj = rtnCoordGetFilterByID( filterObjID, queryOption ) ;
      try
      {
         if ( !pFilterObj->isEmpty() )
         {
            rc = rtnCoordParseGroupList( cb, *pFilterObj, groupList ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnGroupList2GroupPtr( pmdEDUCB *cb, CoordGroupList &groupList,
                                CoordGroupMap &groupMap,
                                BOOLEAN reNew )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr ptr ;

      if ( reNew )
      {
         groupMap.clear() ;
      }

      CoordGroupList::iterator it = groupList.begin() ;
      while ( it != groupList.end() )
      {
         if ( !reNew && groupMap.end() != groupMap.find( it->second ) )
         {
            // alredy exist, don't update group info
            ++it ;
            continue ;
         }
         rc = rtnCoordGetGroupInfo( cb, it->second, FALSE, ptr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group[%d] info, rc: %d",
                      it->second, rc ) ;
         groupMap[ it->second ] = ptr ;
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnGroupList2GroupPtr( pmdEDUCB * cb, CoordGroupList & groupList,
                                GROUP_VEC & groupPtrs )
   {
      INT32 rc = SDB_OK ;
      groupPtrs.clear() ;
      CoordGroupInfoPtr ptr ;

      CoordGroupList::iterator it = groupList.begin() ;
      while ( it != groupList.end() )
      {
         rc = rtnCoordGetGroupInfo( cb, it->second, FALSE, ptr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group[%d] info, rc: %d",
                      it->second, rc ) ;
         groupPtrs.push_back( ptr ) ;
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnGroupPtr2GroupList( pmdEDUCB * cb, GROUP_VEC & groupPtrs,
                                CoordGroupList & groupList )
   {
      groupList.clear() ;

      GROUP_VEC::iterator it = groupPtrs.begin() ;
      while ( it != groupPtrs.end() )
      {
         CoordGroupInfoPtr &ptr = *it ;
         groupList[ ptr->groupID() ] = ptr->groupID() ;
         ++it ;
      }

      return SDB_OK ;
   }

   static INT32 _rtnCoordParseNodesInfo( const BSONObj &obj,
                                         vector< INT32 > &vecNodeID,
                                         vector< const CHAR* > &vecHostName,
                                         vector< const CHAR* > &vecSvcName,
                                         BSONObj *pNewObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BOOLEAN isModify = FALSE ;

      BSONObjIterator itr( obj ) ;
      while( itr.more() )
      {
         BSONElement ele = itr.next() ;

         // $and:[{NodeID:1001, HostName:"xxxx" }]
         if ( Array == ele.type() &&
              0 == ossStrcmp( ele.fieldName(), "$and" ) )
         {
            BSONArrayBuilder sub( builder.subarrayStart( ele.fieldName() ) ) ;
            BSONObj tmpNew ;
            BSONObjIterator tmpItr( ele.embeddedObject() ) ;
            while ( tmpItr.more() )
            {
               BSONElement tmpE = tmpItr.next() ;
               if ( Object != tmpE.type() )
               {
                  PD_LOG( PDERROR, "Parse obj[%s] nodes failed: "
                          "invalid $and", obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  BSONObj tmpObj = tmpE.embeddedObject() ;
                  rc = _rtnCoordParseNodesInfo( tmpObj, vecNodeID,
                                                vecHostName, vecSvcName,
                                                pNewObj ? &tmpNew : NULL ) ;
                  PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] nodes failed ",
                               obj.toString().c_str() ) ;
                  if ( pNewObj )
                  {
                     if ( tmpNew.objdata() != tmpObj.objdata() )
                     {
                        isModify = TRUE ;
                        sub.append( tmpNew ) ;
                     }
                     else
                     {
                        sub.append( tmpObj ) ;
                     }
                  }
               }
            }
            sub.done() ;
         } /// end $and
         else if ( 0 == ossStrcasecmp( ele.fieldName(), CAT_NODEID_NAME ) &&
                   SDB_OK == _rtnCoordParseInt( ele, vecNodeID,
                                                RTN_COORD_PARSE_MASK_ALL ) )
         {
            isModify = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_HOST ) &&
                   SDB_OK == _rtnCoordParseString( ele, vecHostName,
                                                   RTN_COORD_PARSE_MASK_ALL ) )
         {
            isModify = TRUE ;
         }
         else if ( ( 0 == ossStrcasecmp( ele.fieldName(),
                                         FIELD_NAME_SERVICE_NAME ) ||
                     0 == ossStrcasecmp( ele.fieldName(),
                                         PMD_OPTION_SVCNAME ) ) &&
                    SDB_OK == _rtnCoordParseString( ele, vecSvcName,
                                                    RTN_COORD_PARSE_MASK_ALL ) )
         {
            isModify = TRUE ;
         }
         else if ( pNewObj )
         {
            builder.append( ele ) ;
         } // end if
      } // end while

      if ( pNewObj )
      {
         if ( isModify )
         {
            *pNewObj = builder.obj() ;
         }
         else
         {
            *pNewObj = obj ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordGetGroupNodes( pmdEDUCB *cb, const BSONObj &filterObj,
                                NODE_SEL_STY emptyFilterSel,
                                CoordGroupList &groupList, ROUTE_SET &nodes,
                                BSONObj *pNewObj )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr ptr ;
      MsgRouteID routeID ;
      GROUP_VEC groupPtrs ;
      GROUP_VEC::iterator it ;

      vector< INT32 > vecNodeID ;
      vector< const CHAR* > vecHostName ;
      vector< const CHAR* > vecSvcName ;
      BOOLEAN emptyFilter = TRUE ;

      nodes.clear() ;

      rc = rtnGroupList2GroupPtr( cb, groupList, groupPtrs ) ;
      PD_RC_CHECK( rc, PDERROR, "Group ids to group info failed, rc: %d", rc ) ;

      rc = _rtnCoordParseNodesInfo( filterObj, vecNodeID, vecHostName,
                                    vecSvcName, pNewObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] nodes info failed, rc: %d",
                   filterObj.toString().c_str(), rc ) ;

      if ( vecNodeID.size() > 0 || vecHostName.size() > 0 ||
           vecSvcName.size() > 0 )
      {
         emptyFilter = FALSE ;
      }

      /// parse nodes
      it = groupPtrs.begin() ;
      while ( it != groupPtrs.end() )
      {
         UINT32 calTimes = 0 ;
         UINT32 randNum = ossRand() ;
         ptr = *it ;

         routeID.value = MSG_INVALID_ROUTEID ;
         clsGroupItem *grp = ptr.get() ;
         if ( grp->nodeCount() > 0 )
         {
            randNum %= grp->nodeCount() ;
         }

         /// calc pos
         while ( calTimes++ < grp->nodeCount() )
         {
            if ( NODE_SEL_SECONDARY == emptyFilterSel &&
                 randNum == grp->getPrimaryPos() )
            {
               randNum = ( randNum + 1 ) % grp->nodeCount() ;
               continue ;
            }
            else if ( NODE_SEL_PRIMARY == emptyFilterSel &&
                      CLS_RG_NODE_POS_INVALID != grp->getPrimaryPos() )
            {
               randNum = grp->getPrimaryPos() ;
            }
            break ;
         }

         routeID.columns.groupID = grp->groupID() ;
         const VEC_NODE_INFO *nodesInfo = grp->getNodes() ;
         for ( VEC_NODE_INFO::const_iterator itrn = nodesInfo->begin() ;
               itrn != nodesInfo->end();
               ++itrn, --randNum )
         {
            if ( FALSE == emptyFilter )
            {
               BOOLEAN findNode = FALSE ;
               UINT32 index = 0 ;
               /// check node id
               for ( index = 0 ; index < vecNodeID.size() ; ++index )
               {
                  if ( (UINT16)vecNodeID[ index ] == itrn->_id.columns.nodeID )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }

               findNode = FALSE ;
               /// check host name
               for ( index = 0 ; index < vecHostName.size() ; ++index )
               {
                  if ( 0 == ossStrcmp( vecHostName[ index ],
                                       itrn->_host ) )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }

               findNode = FALSE ;
               /// check svcname
               for ( index = 0 ; index < vecSvcName.size() ; ++index )
               {
                  if ( 0 == ossStrcmp( vecSvcName[ index ],
                                       itrn->_service[MSG_ROUTE_LOCAL_SERVICE].c_str() ) )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }
            }
            else if ( NODE_SEL_ALL != emptyFilterSel && 0 != randNum )
            {
               continue ;
            }
            routeID.columns.nodeID = itrn->_id.columns.nodeID ;
            routeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
            nodes.insert( routeID.value ) ;
         }
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOUPNODESTATBYRC, "rtnCoordUpdateNodeStatByRC" )
   void rtnCoordUpdateNodeStatByRC( pmdEDUCB *cb,
                                    const MsgRouteID &routeID,
                                    CoordGroupInfoPtr &groupInfo,
                                    INT32 retCode )
   {
      PD_TRACE_ENTRY ( SDB_RTNCOUPNODESTATBYRC ) ;

      CoordSession *pSession = cb->getCoordSession() ;
      if ( MSG_INVALID_ROUTEID == routeID.value )
      {
         goto done;
      }
      else if ( SDB_OK != retCode && pSession )
      {
         pSession->removeLastNode( routeID.columns.groupID,
                                   routeID ) ;
      }

      if( groupInfo.get() )
      {
         groupInfo->updateNodeStat( routeID.columns.nodeID,
                                    netResult2Status( retCode ) ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB_RTNCOUPNODESTATBYRC ) ;
      return ;
   }

   void rtnCoordUpdateNodeStatByRC( pmdEDUCB *cb,
                                    const MsgRouteID &routeID,
                                    INT32 retCode )
   {
      CoordGroupInfoPtr groupInfo ;

      CoordSession *pSession = cb->getCoordSession() ;
      if ( MSG_INVALID_ROUTEID == routeID.value )
      {
         goto done;
      }
      else if ( SDB_OK != retCode && pSession )
      {
         pSession->removeLastNode( routeID.columns.groupID,
                                   routeID ) ;
      }

      if ( SDB_OK == rtnCoordGetGroupInfo( cb, routeID.columns.groupID,
                                           FALSE, groupInfo ) )
      {
         groupInfo->updateNodeStat( routeID.columns.nodeID,
                                    netResult2Status( retCode ) ) ;
      }

   done:
      return ;
   }

   BOOLEAN rtnCoordGroupReplyCheck( pmdEDUCB *cb, INT32 flag,
                                    BOOLEAN canRetry,
                                    const MsgRouteID &nodeID,
                                    CoordGroupInfoPtr &groupInfo,
                                    BOOLEAN *pUpdate,
                                    BOOLEAN canUpdate,
                                    UINT32 primaryID,
                                    BOOLEAN isReadCmd )
   {
      /// remove the last node
      if ( cb && cb->getCoordSession() )
      {
         cb->getCoordSession()->removeLastNode( nodeID.columns.groupID,
                                                nodeID ) ;
      }

      if ( SDB_CLS_NOT_PRIMARY == flag && 0 != primaryID &&
           groupInfo.get() )
      {
         INT32 preStat = NET_NODE_STAT_NORMAL ;
         MsgRouteID primaryNodeID ;
         primaryNodeID.value = nodeID.value ;
         primaryNodeID.columns.nodeID = primaryID ;
         if ( SDB_OK == groupInfo->updatePrimary( primaryNodeID,
                                                  TRUE, &preStat ) )
         {
            /// when primay's crash has not discoverd by other nodes,
            /// new primary's nodeid may still be old one. 
            /// To avoid send msg to crashed node frequently,
            /// sleep some times.
            if ( NET_NODE_STAT_NORMAL != preStat )
            {
               groupInfo->cancelPrimary() ;
               PD_LOG( PDWARNING, "Primary node[%d.%d] is crashed, sleep "
                       "%d seconds", primaryNodeID.columns.groupID,
                       primaryNodeID.columns.nodeID,
                       NET_NODE_FAULTUP_MIN_TIME ) ;
               ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            }
            return canRetry ;
         }
      }

      if ( !canRetry )
      {
         return FALSE ;
      }

      if ( SDB_CLS_NOT_PRIMARY == flag )
      {
         if ( groupInfo.get() )
         {
            groupInfo->updatePrimary( nodeID, FALSE ) ;
         }

         if ( canUpdate )
         {
            UINT32 groupID = nodeID.columns.groupID ;
            INT32 rc = rtnCoordGetRemoteGroupInfo( cb, groupID, NULL,
                                                   groupInfo, TRUE ) ;
            if ( SDB_OK == rc )
            {
               if( pUpdate )
               {
                  *pUpdate = TRUE ;
               }
            }
            else
            {
               PD_LOG( PDWARNING, "Get group info[%u] from remote failed, "
                       "rc: %d", groupID, rc ) ;
               return FALSE ;
            }
         }
         return TRUE ;
      }
      // [SDB_COORD_REMOTE_DISC] can't use in write command,
      // because when some insert/update opr do partibal,
      // if retry, data will repeat. The code can't update status,
      // because it maybe occured in long time ago
      if ( ( isReadCmd && SDB_COORD_REMOTE_DISC == flag ) ||
           SDB_CLS_NODE_NOT_ENOUGH == flag )
      {
         return TRUE ;
      }
      else if ( SDB_CLS_FULL_SYNC == flag ||
                SDB_RTN_IN_REBUILD == flag )
      {
         // don't update group info
         rtnCoordUpdateNodeStatByRC( cb, nodeID, groupInfo, flag ) ;
         return TRUE ;
      }

      return FALSE ;
   }

   BOOLEAN rtnCoordCataReplyCheck( pmdEDUCB *cb, INT32 flag,
                                   BOOLEAN canRetry,
                                   CoordCataInfoPtr &cataInfo,
                                   BOOLEAN *pUpdate,
                                   BOOLEAN canUpdate )
   {
      if ( canRetry && (
           SDB_CLS_COORD_NODE_CAT_VER_OLD == flag ||
           SDB_CLS_NO_CATALOG_INFO == flag ||
           SDB_CLS_GRP_NOT_EXIST == flag ||
           SDB_CLS_NODE_NOT_EXIST == flag ||
           SDB_CAT_NO_MATCH_CATALOG == flag )
          )
      {
         if ( canUpdate && cataInfo.get() )
         {
            CoordCataInfoPtr newCataInfo ;
            const CHAR *collectionName = cataInfo->getName() ;
            INT32 rc = rtnCoordGetRemoteCata( cb, collectionName,
                                              newCataInfo ) ;
            if ( SDB_OK == rc )
            {
               cataInfo = newCataInfo ;
               if( pUpdate )
               {
                  *pUpdate = TRUE ;
               }
            }
            else
            {
               PD_LOG( PDWARNING, "Get catalog info[%s] from remote failed, "
                       "rc: %d", collectionName, rc ) ;
               return FALSE ;
            }
         }
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 rtnCataChangeNtyToAllNodes( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      netMultiRouteAgent *pRouteAgent = sdbGetCoordCB()->getRouteAgent() ;
      MsgHeader ntyMsg ;
      ntyMsg.messageLength = sizeof( MsgHeader ) ;
      ntyMsg.opCode = MSG_CAT_GRP_CHANGE_NTY ;
      ntyMsg.requestID = 0 ;
      ntyMsg.routeID.value = 0 ;
      ntyMsg.TID = cb->getTID() ;

      CoordGroupList groupLst ;
      ROUTE_SET sendNodes ;
      REQUESTID_MAP successNodes ;
      ROUTE_RC_MAP failedNodes ;

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

      // send msg, no response
      rtnCoordSendRequestToNodes( (void*)&ntyMsg, sendNodes, 
                                  pRouteAgent, cb, successNodes,
                                  failedNodes ) ;
      if ( failedNodes.size() != 0 )
      {
         rc = failedNodes.begin()->second ;
      }
      rtnCoordClearRequest( cb, successNodes ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordParseControlParam( const BSONObj &obj,
                                    rtnCoordCtrlParam &param,
                                    UINT32 mask,
                                    BSONObj *pNewObj )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN modify = FALSE ;
      BSONObjBuilder builder ;
      const CHAR *tmpStr = NULL ;
      vector< const CHAR* > tmpVecStr ;

      BSONObjIterator it( obj ) ;
      while( it.more() )
      {
         BSONElement e = it.next() ;

         /// $and:[{a:1},{b:2}]
         if ( Array == e.type() &&
              0 == ossStrcmp( e.fieldName(), "$and" ) )
         {
            BSONArrayBuilder sub( builder.subarrayStart( e.fieldName() ) ) ;
            BSONObj tmpNew ;
            BSONObjIterator tmpItr( e.embeddedObject() ) ;
            while ( tmpItr.more() )
            {
               BSONElement tmpE = tmpItr.next() ;
               if ( Object != tmpE.type() )
               {
                  PD_LOG( PDERROR, "Parse obj[%s] conrtol param failed: "
                          "invalid $and", obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  BSONObj tmpObj = tmpE.embeddedObject() ;
                  rc = rtnCoordParseControlParam( tmpObj, param, mask,
                                                  pNewObj ? &tmpNew : NULL ) ;
                  PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] conrtol param "
                               "failed", obj.toString().c_str() ) ;
                  if ( pNewObj )
                  {
                     if ( tmpNew.objdata() != tmpObj.objdata() )
                     {
                        modify = TRUE ;
                        sub.append( tmpNew ) ;
                     }
                     else
                     {
                        sub.append( tmpObj ) ;
                     }
                  }
               }
            }
            sub.done() ;
         } /// end $and
         else if ( ( mask & RTN_CTRL_MASK_GLOBAL ) &&
                   0 == ossStrcasecmp( e.fieldName(), FIELD_NAME_GLOBAL ) &&
                   SDB_OK == _rtnCoordParseBoolean( e, param._isGlobal,
                                                    RTN_COORD_PARSE_MASK_ET ) )
         {
            modify = TRUE ;
            param._parseMask |= RTN_CTRL_MASK_GLOBAL ;
         }
         else if ( ( mask & RTN_CTRL_MASK_GLOBAL ) &&
                   e.isNull() &&
                   ( 0 == ossStrcasecmp( e.fieldName(),
                                        FIELD_NAME_GROUPS ) ||
                     0 == ossStrcasecmp( e.fieldName(),
                                        FIELD_NAME_GROUPNAME ) ||
                     0 == ossStrcasecmp( e.fieldName(),
                                        FIELD_NAME_GROUPID ) )
                 )
         {
            param._isGlobal = FALSE ;
            modify = TRUE ;
            param._parseMask |= RTN_CTRL_MASK_GLOBAL ;
         }
         else if ( ( mask & RTN_CTRL_MASK_NODE_SELECT ) &&
                   0 == ossStrcasecmp( e.fieldName(),
                                       FIELD_NAME_NODE_SELECT ) &&
                   SDB_OK == _rtnCoordParseString( e, tmpStr,
                                                   RTN_COORD_PARSE_MASK_ET ) )
         {
            modify = TRUE ;
            param._parseMask |= RTN_CTRL_MASK_NODE_SELECT ;
            if ( 0 == ossStrcasecmp( tmpStr, "primary" ) ||
                 0 == ossStrcasecmp( tmpStr, "master" ) ||
                 0 == ossStrcasecmp( tmpStr, "m" ) ||
                 0 == ossStrcasecmp( tmpStr, "p" ) )
            {
               param._emptyFilterSel = NODE_SEL_PRIMARY ;
            }
            else if ( 0 == ossStrcasecmp( tmpStr, "any" ) ||
                      0 == ossStrcasecmp( tmpStr, "a" ) )
            {
               param._emptyFilterSel = NODE_SEL_ANY ;
            }
            else if ( 0 == ossStrcasecmp( tmpStr, "secondary" ) ||
                      0 == ossStrcasecmp( tmpStr, "s" ) )
            {
               param._emptyFilterSel = NODE_SEL_SECONDARY ;
            }
            else
            {
               param._emptyFilterSel = NODE_SEL_ALL ;
            }
         }
         else if ( ( mask & RTN_CTRL_MASK_ROLE ) &&
                   0 == ossStrcasecmp( e.fieldName(),
                                       FIELD_NAME_ROLE ) &&
                   SDB_OK == _rtnCoordParseString( e, tmpVecStr,
                                                   RTN_COORD_PARSE_MASK_ALL ) )
         {
            ossMemset( &param._role, 0, sizeof( param._role ) ) ;
            modify = TRUE ;
            param._parseMask |= RTN_CTRL_MASK_ROLE ;

            for ( UINT32 i = 0 ; i < tmpVecStr.size() ; ++i )
            {
               if ( 0 == ossStrcasecmp( tmpVecStr[i], "all" ) )
               {
                  for ( UINT32 k = 0 ; k < (UINT32)SDB_ROLE_MAX ; ++k )
                  {
                     param._role[ k ] = 1 ;
                  }
                  break ;
               }
               if ( SDB_ROLE_MAX != utilGetRoleEnum( tmpVecStr[ i ] ) )
               {
                  param._role[ utilGetRoleEnum( tmpVecStr[ i ] ) ] = 1 ;
               }
            }
            tmpVecStr.clear() ;
         }
         else if ( ( mask & RTN_CTRL_MASK_RAWDATA ) &&
                   0 == ossStrcasecmp( e.fieldName(), FIELD_NAME_RAWDATA ) &&
                   SDB_OK == _rtnCoordParseBoolean( e, param._rawData,
                                                    RTN_COORD_PARSE_MASK_ET ) )
         {
            modify = TRUE ;
            param._parseMask |= RTN_CTRL_MASK_RAWDATA ;
         }
         else if ( pNewObj )
         {
            builder.append( e ) ;
         }
      }

      if ( pNewObj )
      {
         if ( modify )
         {
            *pNewObj = builder.obj() ;
         }
         else
         {
            *pNewObj = obj ;
         }
      }

   done:
      return SDB_OK ;
   error:
      goto done ;
   }

   BOOLEAN rtnCoordCanRetry( UINT32 retryTimes )
   {
      return retryTimes < RTN_COORD_MAX_RETRYTIMES ? TRUE : FALSE ;
   }

}

