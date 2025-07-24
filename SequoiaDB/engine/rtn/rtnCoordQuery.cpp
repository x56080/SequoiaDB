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

   Source File Name = rtnCoordQuery.cpp

   Descriptive Name = Runtime Coord Query

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   query on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "coordSession.hpp"
#include "rtnCoord.hpp"
#include "rtnContext.hpp"
#include "rtnCB.hpp"
#include "msg.h"
#include "coordCB.hpp"
#include "rtnCoordCommon.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "msgMessage.hpp"
#include "rtnCoordQuery.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtn.hpp"
#include "rtnCoordCommands.hpp"
#include "pmdEDU.hpp"

using namespace bson;

namespace engine
{
   INT32 rtnCoordQuery::_checkQueryModify( rtnSendMsgIn &inMsg,
                                           rtnSendOptions &options,
                                           CoordGroupSubCLMap *grpSubCl )
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )inMsg.msg() ;
      INT32 rc = SDB_OK ;

      if ( queryMsg->flags & FLG_QUERY_MODIFY )
      {
         if ( ( options._groupLst.size() > 1 ) ||
              ( grpSubCl && grpSubCl->size() >= 1 &&
                grpSubCl->begin()->second.size() > 1 ) )
         {
            rtnQueryPvtData *privateData = ( rtnQueryPvtData* )inMsg._pvtData ;
            if ( privateData->_pContext->getLimitNum() > 0 ||
                 privateData->_pContext->getSkipNum() > 0 ||
                 queryMsg->numToReturn > 0 ||
                 queryMsg->numToSkip > 0 )
            {
               rc = SDB_RTN_QUERYMODIFY_MULTI_NODES ;
               PD_LOG( PDERROR, "query and modify can't use skip and limit "
                       "in multiple nodes or sub-collections, rc: %d", rc ) ;
            }
         }

         // modification can only be executed in primary node
         options._primary = TRUE ;
      }

      return rc ;
   }

   void rtnCoordQuery::_optimize( rtnSendMsgIn &inMsg,
                                  rtnSendOptions &options,
                                  rtnProcessResult &result )
   {
      SDB_ASSERT( inMsg._pvtData &&
                  inMsg._pvtType == PRIVATE_DATA_USER,
                  "Private data invalid" ) ;

      rtnQueryPvtData *pvtData = ( rtnQueryPvtData* )inMsg._pvtData ;

      if ( pvtData->_pContext && 0 == result._sucGroupLst.size() )
      {
         MsgOpQuery *pQueryMsg = ( MsgOpQuery* )inMsg.msg() ;
         INT64 ctxRetNum = pvtData->_pContext->getLimitNum() ;
         INT64 ctxSkipNum = pvtData->_pContext->getSkipNum() ;

         /// if send to one node
         if ( options._groupLst.size() <= 1 )
         {
            if ( ctxSkipNum > 0 )
            {
               pQueryMsg->numToSkip = ctxSkipNum ;
               pvtData->_pContext->setSkipNum( 0 ) ;
               if ( ctxRetNum > 0 &&
                    pQueryMsg->numToReturn == ctxRetNum + ctxSkipNum )
               {
                  pQueryMsg->numToReturn -= ctxSkipNum ;
               }
            }
         }
         else
         {
            if ( pQueryMsg->numToSkip > 0 )
            {
               if ( pQueryMsg->numToReturn > 0 )
               {
                  pvtData->_pContext->setLimitNum( pQueryMsg->numToReturn ) ;
                  pQueryMsg->numToReturn += pQueryMsg->numToSkip ;
               }
               pvtData->_pContext->setSkipNum( pQueryMsg->numToSkip ) ;
               pQueryMsg->numToSkip = 0 ;
            }
         }
      }
   }

   INT32 rtnCoordQuery::_prepareCLOp( CoordCataInfoPtr &cataInfo,
                                      rtnSendMsgIn &inMsg,
                                      rtnSendOptions &options,
                                      netMultiRouteAgent *pRouteAgent,
                                      pmdEDUCB *cb,
                                      rtnProcessResult &result,
                                      ossValuePtr &outPtr )
   {
      INT32 rc = SDB_OK ;

      _optimize( inMsg, options, result ) ;

      rc = _checkQueryModify( inMsg, options, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void rtnCoordQuery::_doneCLOp( ossValuePtr itPtr,
                                  CoordCataInfoPtr &cataInfo,
                                  rtnSendMsgIn &inMsg,
                                  rtnSendOptions &options,
                                  netMultiRouteAgent *pRouteAgent,
                                  pmdEDUCB *cb,
                                  rtnProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      /// add succeed reply to context, and release the reply
      SDB_ASSERT( inMsg._pvtData &&
                  inMsg._pvtType == PRIVATE_DATA_USER,
                  "Private data invalid" ) ;

      rtnQueryPvtData *pvtData = ( rtnQueryPvtData* )inMsg._pvtData ;
      ROUTE_REPLY_MAP *pOkReply = result._pOkReply ;

      SDB_ASSERT( pOkReply, "Ok reply invalid" ) ;

      BOOLEAN takeOver = FALSE ;
      MsgOpReply *pReply = NULL ;
      MsgRouteID nodeID ;
      ROUTE_REPLY_MAP::iterator it = pOkReply->begin() ;
      while( it != pOkReply->end() )
      {
         takeOver = FALSE ;
         pReply = (MsgOpReply*)(it->second) ;
         nodeID.value = pReply->header.routeID.value ;

         if ( SDB_OK == pReply->flags )
         {
            if ( pvtData->_pContext )
            {
               rc = pvtData->_pContext->addSubContext( pReply, takeOver ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Add sub data[node: %s, context: %lld] to "
                          "context[%s] failed, rc: %d",
                          routeID2String( nodeID ).c_str(), pReply->contextID,
                          pvtData->_pContext->toString().c_str(), rc ) ;
                  pvtData->_ret = rc ;
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
      pOkReply->clear() ;
   }

   INT32 rtnCoordQuery::_prepareMainCLOp( CoordCataInfoPtr &cataInfo,
                                          CoordGroupSubCLMap &grpSubCl,
                                          rtnSendMsgIn &inMsg,
                                          rtnSendOptions &options,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          rtnProcessResult &result,
                                          ossValuePtr &outPtr )
   {
      INT32 rc                = SDB_OK ;
      MsgOpQuery *pQueryMsg   = ( MsgOpQuery* )inMsg.msg() ;

      INT32 flags             = 0 ;
      CHAR *pCollectionName   = NULL ;
      INT64 numToSkip         = 0 ;
      INT64 numToReturn       = -1 ;
      CHAR *pQuery            = NULL ;
      CHAR *pFieldSelector    = NULL ;
      CHAR *pOrderBy          = NULL ;
      CHAR *pHint             = NULL ;

      BSONObj objQuery ;
      BSONObj objSelector ;
      BSONObj objOrderby ;
      BSONObj objHint ;
      BSONObj newQuery ;

      CHAR *pBuff             = NULL ;
      UINT32 buffLen          = 0 ;
      UINT32 buffPos          = 0 ;
      vector<CHAR*> *pBlock   = NULL ;

      outPtr                  = (ossValuePtr)0 ;

      CoordGroupSubCLMap::iterator it ;

      _optimize( inMsg, options, result ) ;

      rc = _checkQueryModify( inMsg, options, &grpSubCl ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( options._useSpecialGrp )
      {
         goto done ;
      }

      inMsg.data()->clear() ;

      rc = msgExtractQuery( (CHAR*)pQueryMsg, &flags, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery,
                            &pFieldSelector, &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query msg failed, rc: %d", rc ) ;

      try
      {
         objQuery = BSONObj( pQuery ) ;
         objSelector = BSONObj( pFieldSelector ) ;
         objOrderby = BSONObj( pOrderBy ) ;
         objHint = BSONObj( pHint ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace query msg occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pBlock = new vector< CHAR* >( 16 ) ;
      if ( !pBlock )
      {
         PD_LOG( PDERROR, "Alloc vector failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      it = grpSubCl.begin() ;
      while( it != grpSubCl.end() )
      {
         CoordSubCLlist &subCLLst = it->second ;

         netIOVec &iovec = inMsg._datas[ it->first ] ;
         netIOV ioItem ;

         // 1. first vec
         ioItem.iovBase = (CHAR*)inMsg.msg() + sizeof( MsgHeader ) ;
         ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpQuery, name) +
                         pQueryMsg->nameLength + 1, 4 ) - sizeof( MsgHeader ) ;
         iovec.push_back( ioItem ) ;

         // 2. new query vec
         newQuery = _buildNewQuery( objQuery, subCLLst ) ;
         // 2.1 add to buff
         UINT32 roundLen = ossRoundUpToMultipleX( newQuery.objsize(), 4 ) ;
         if ( buffPos + roundLen > buffLen )
         {
            UINT32 alignLen = ossRoundUpToMultipleX( roundLen,
                                                     DMS_PAGE_SIZE4K ) ;
            rc = cb->allocBuff( alignLen, &pBuff, &buffLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Alloc buff[%u] failed, rc: %d",
                         alignLen, rc ) ;
            pBlock->push_back( pBuff ) ;
            buffPos = 0 ;
         }
         ossMemcpy( &pBuff[ buffPos ], newQuery.objdata(),
                    newQuery.objsize() ) ;
         ioItem.iovBase = &pBuff[ buffPos ] ;
         ioItem.iovLen = roundLen ;
         buffPos += roundLen ;
         iovec.push_back( ioItem ) ;

         // 3. last vec
         ioItem.iovBase = objSelector.objdata() ;
         ioItem.iovLen = ossRoundUpToMultipleX( objSelector.objsize(), 4 ) +
                         ossRoundUpToMultipleX( objOrderby.objsize(), 4 ) +
                         objHint.objsize() ;
         iovec.push_back( ioItem ) ;

         ++it ;
      }

      outPtr = ( ossValuePtr )pBlock ;

   done:
      return rc ;
   error:
      if ( pBlock )
      {
         for ( UINT32 i = 0 ; i < pBlock->size() ; ++i )
         {
            cb->releaseBuff( (*pBlock)[ i ] ) ;
         }
         delete pBlock ;
         pBlock = NULL ;
      }
      goto done ;
   }

   void rtnCoordQuery::_doneMainCLOp( ossValuePtr itPtr,
                                      CoordCataInfoPtr &cataInfo,
                                      CoordGroupSubCLMap &grpSubCl,
                                      rtnSendMsgIn &inMsg,
                                      rtnSendOptions &options,
                                      netMultiRouteAgent *pRouteAgent,
                                      pmdEDUCB *cb,
                                      rtnProcessResult &result )
   {
      vector<CHAR*> *pBlock = ( vector<CHAR*>* )itPtr ;
      if ( NULL != pBlock )
      {
         for ( UINT32 i = 0 ; i < pBlock->size() ; ++i )
         {
            cb->releaseBuff( (*pBlock)[ i ] ) ;
         }
         delete pBlock ;
         pBlock = NULL ;
      }
      inMsg._datas.clear() ;

      _doneCLOp( itPtr, cataInfo, inMsg, options,
                 pRouteAgent, cb, result ) ;
   }

   BSONObj rtnCoordQuery::_buildNewQuery( const BSONObj &query,
                                          const CoordSubCLlist &subCLList )
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder babSubCL ;
      CoordSubCLlist::const_iterator iterCL = subCLList.begin();
      while( iterCL != subCLList.end() )
      {
         babSubCL.append( *iterCL ) ;
         ++iterCL ;
      }
      builder.appendElements( query ) ;
      builder.appendArray( CAT_SUBCL_NAME, babSubCL.arr() ) ;
      return builder.obj() ;
   }

   INT32 rtnCoordQuery::execute( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();
      rtnContextCoord *pContext        = NULL ;

      // fill default-reply(query success)
      contextID                        = -1 ;

      CHAR *pCollectionName            = NULL ;
      INT32 flag                       = 0 ;
      INT64 numToSkip                  = 0 ;
      INT64 numToReturn                = 0 ;
      CHAR *pQuery                     = NULL ;
      CHAR *pSelector                  = NULL ;
      CHAR *pOrderby                   = NULL ;
      CHAR *pHint                      = NULL ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery, &pSelector,
                            &pOrderby, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse query request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      // process command
      if ( pCollectionName != NULL && '$' == pCollectionName[0] )
      {
         rtnCoordCommand *pCmdProcesser = NULL;
         rtnCoordProcesserFactory *pProcesserFactory
                  = pCoordcb->getProcesserFactory();
         pCmdProcesser = pProcesserFactory->getCommandProcesser(
                                             pCollectionName ) ;
         PD_CHECK( pCmdProcesser != NULL, SDB_INVALIDARG, error, PDERROR,
                  "unknown command:%s", pCollectionName ) ;

         // add last op info
         MON_SAVE_CMD_DETAIL( cb->getMonAppCB(), CMD_UNKNOW - 1,
                              "Command:%s, Match:%s, "
                              "Selector:%s, OrderBy:%s, Hint:%s, Skip:%llu, "
                              "Limit:%lld, Flag:0x%08x(%u)",
                              pCollectionName,
                              BSONObj(pQuery).toString().c_str(),
                              BSONObj(pSelector).toString().c_str(),
                              BSONObj(pOrderby).toString().c_str(),
                              BSONObj(pHint).toString().c_str(),
                              numToSkip, numToReturn, flag, flag ) ;

         rc = pCmdProcesser->execute( pMsg, cb, contextID, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to execute the "
                      "command(command:%s, rc=%d)",
                      pCollectionName, rc ) ;
      }
      else
      {
         rtnSendOptions sendOpt ;

         // add last op info
         MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                             "Collection:%s, Matcher:%s, Selector:%s, "
                             "OrderBy:%s, Hint:%s, Skip:%llu, Limit:%lld, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             BSONObj(pQuery).toString().c_str(),
                             BSONObj(pSelector).toString().c_str(),
                             BSONObj(pOrderby).toString().c_str(),
                             BSONObj(pHint).toString().c_str(),
                             numToSkip, numToReturn,
                             flag, flag ) ;

         rc = queryOrDoOnCL( pMsg, pRouteAgent, cb, &pContext,
                             sendOpt, NULL, buf ) ;
         /// AUDIT
         PD_AUDIT_OP( ( flag & FLG_QUERY_MODIFY ? AUDIT_DML : AUDIT_DQL ),
                      MSG_BS_QUERY_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "ContextID:%lld, Matcher:%s, Selector:%s, OrderBy:%s, "
                      "Hint:%s, Skip:%llu, Limit:%lld, Flag:0x%08x(%u)",
                      pContext ? pContext->contextID() : -1,
                      BSONObj(pQuery).toString().c_str(),
                      BSONObj(pSelector).toString().c_str(),
                      BSONObj(pOrderby).toString().c_str(),
                      BSONObj(pHint).toString().c_str(),
                      numToSkip, numToReturn,
                      flag, flag ) ;
         PD_RC_CHECK( rc, PDERROR, "query failed, rc: %d", rc ) ;

         contextID = pContext->contextID() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN rtnCoordQuery::_isUpdate( const BSONObj &hint,
                                     INT32 flags )
   {
      if ( flags & FLG_QUERY_MODIFY )
      {
         BSONObj updator ;
         BSONObj newUpdator ;
         BSONElement modifierEle ;
         BSONObj modifier ;

         modifierEle = hint.getField( FIELD_NAME_MODIFY ) ;
         if ( Object != modifierEle.type() )
         {
            return FALSE ;
         }

         modifier = modifierEle.Obj() ;

         BSONElement updatorEle = modifier.getField( FIELD_NAME_OP ) ;
         if ( String != updatorEle.type() ||
              0 != ossStrcmp( updatorEle.valuestr(), FIELD_OP_VALUE_UPDATE ) )
         {
            return FALSE ;
         }

         return TRUE ;
      }

      return FALSE ;
   }

   INT32 rtnCoordQuery::_generateNewHint( const CoordCataInfoPtr &cataInfo,
                                          const BSONObj &selector,
                                          const BSONObj &hint, BSONObj &newHint,
                                          BOOLEAN &isChanged, BOOLEAN &isEmpty,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj updator ;
      BSONObj newUpdator ;
      BSONElement modifierEle ;
      BSONObj modifier ;

      modifierEle = hint.getField( FIELD_NAME_MODIFY ) ;
      SDB_ASSERT( Object == modifierEle.type(),
                  "modifierELe must be an Object" ) ;

      modifier = modifierEle.Obj() ;

      BSONElement updatorEle = modifier.getField( FIELD_NAME_OP_UPDATE ) ;
      SDB_ASSERT( Object == updatorEle.type(), "updatorEle must be an Object" ) ;
      updator = updatorEle.Obj() ;

      rc = _generateShardUpdator( cataInfo, selector, updator, newUpdator,
                                  isChanged, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "kick shardingkey for updator failed:rc=%d" ) ;
         goto error ;
      }

      isEmpty = newUpdator.isEmpty() ? TRUE : FALSE ;
      if ( isChanged )
      {
         BSONObjBuilder builder( hint.objsize() ) ;
         BSONObjIterator itr( hint ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            if( 0 == ossStrcmp( FIELD_NAME_MODIFY, e.fieldName() ) &&
                Object == e.type() )
            {
               if ( isEmpty )
               {
                  /// new updator is empty, the whole $Modify will be removed
                  continue ;
               }

               BSONObjBuilder subBuild( builder.subobjStart( FIELD_NAME_MODIFY ) ) ;
               BSONObjIterator subItr( e.embeddedObject() ) ;
               while( subItr.more() )
               {
                  BSONElement subE = subItr.next() ;
                  if ( 0 == ossStrcmp( FIELD_NAME_OP_UPDATE,
                                       subE.fieldName() ) )
                  {
                     subBuild.append( FIELD_NAME_OP_UPDATE, newUpdator ) ;
                  }
                  else
                  {
                     subBuild.append( subE ) ;
                  }
               } /// end while ( subItr.more() )
               subBuild.done() ;
            } //end if
            else
            {
               builder.append( e ) ;
            }
         } /// end while( itr.more() )

         newHint = builder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordQuery::_generateShardUpdator( const CoordCataInfoPtr &cataInfo,
                                               const BSONObj &selector,
                                               const BSONObj &updator,
                                               BSONObj &newUpdator,
                                               BOOLEAN &isChanged,
                                               pmdEDUCB *cb )
   {
      INT32 rc               = SDB_OK ;
      BOOLEAN hasShardingKey = FALSE ;
      rtnCoordShardKicker shardKicker ;

      newUpdator = updator ;
      rc = shardKicker.kickShardingKey( cataInfo, updator, newUpdator,
                                        hasShardingKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Update failed, failed to kick the "
                   "sharding-key field(rc=%d)", rc ) ;

      if ( cataInfo->isMainCL() )
      {
         INT32 rcTmp = SDB_OK ;
         BSONObj newSubObj ;
         CoordSubCLlist subCLList ;
         rcTmp = cataInfo->getMatchSubCLs( selector, subCLList ) ;
         if ( rcTmp )
         {
            rc = rcTmp ;
            PD_LOG( PDERROR,"Failed to get match sub-collection:rc=%d",
                    rcTmp ) ;
            goto error ;
         }

         rc = shardKicker.kickShardingKeyForSubCL( subCLList, newUpdator,
                                                   newSubObj,
                                                   hasShardingKey, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to kick the sharding-key field "
                      "for sub-collection, rc: %d", rc ) ;
         newUpdator = newSubObj ;
      }

      isChanged = hasShardingKey ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordQuery::queryOrDoOnCL( MsgHeader *pMsg,
                                       netMultiRouteAgent *pRouteAgent,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **pContext,
                                       rtnSendOptions & sendOpt,
                                       rtnQueryConf *pQueryConf,
                                       rtnContextBuf *buf )
   {
      return _queryOrDoOnCL( pMsg, pRouteAgent, cb, pContext,
                             sendOpt, NULL, pQueryConf, buf ) ;
   }

   INT32 rtnCoordQuery::queryOrDoOnCL( MsgHeader *pMsg,
                                       netMultiRouteAgent *pRouteAgent,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **pContext,
                                       rtnSendOptions &sendOpt,
                                       CoordGroupList &sucGrpLst,
                                       rtnQueryConf *pQueryConf,
                                       rtnContextBuf *buf )
   {
      return _queryOrDoOnCL( pMsg, pRouteAgent, cb, pContext,
                             sendOpt, &sucGrpLst, pQueryConf, buf ) ;
   }

   INT32 rtnCoordQuery::_queryOrDoOnCL( MsgHeader *pMsg,
                                        netMultiRouteAgent *pRouteAgent,
                                        pmdEDUCB *cb,
                                        rtnContextCoord **pContext,
                                        rtnSendOptions &sendOpt,
                                        CoordGroupList *pSucGrpLst,
                                        rtnQueryConf *pQueryConf,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      INT64 contextID = -1 ;
      CoordCataInfoPtr cataInfo ;
      const CHAR *pRealCLName = NULL ;
      BOOLEAN openEmptyContext = FALSE ;
      BOOLEAN updateCata = FALSE ;
      BOOLEAN allCataGroup = FALSE ;

      MsgRouteID errNodeID ;
      rtnSendMsgIn inMsg( pMsg ) ;

      rtnProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;
      ROUTE_REPLY_MAP okReply ;
      result._pOkReply = &okReply ;

      rtnQueryPvtData pvtData ;
      inMsg._pvtType = PRIVATE_DATA_USER ;
      inMsg._pvtData = (CHAR*)&pvtData ;

      if ( pQueryConf )
      {
         openEmptyContext = pQueryConf->_openEmptyContext ;
         updateCata = pQueryConf->_updateAndGetCata ;
         if ( !pQueryConf->_realCLName.empty() )
         {
            pRealCLName = pQueryConf->_realCLName.c_str() ;
         }
         allCataGroup = pQueryConf->_allCataGroups ;
      }

      SET_RC *pOldIgnoreRC = sendOpt._pIgnoreRC ;
      SET_RC ignoreRC ;
      if ( pOldIgnoreRC )
      {
         ignoreRC = *pOldIgnoreRC ;
      }
      ignoreRC.insert( SDB_DMS_EOC ) ;
      sendOpt._pIgnoreRC = &ignoreRC ;

      MsgOpQuery *pQueryMsg   = ( MsgOpQuery* )pMsg ;
      SDB_RTNCB *pRtncb       = pmdGetKRCB()->getRTNCB() ;
      CHAR *pNewMsg           = NULL ;
      INT32 newMsgSize        = 0 ;
      const CHAR* pLastMsg    = NULL ;
      CHAR *pModifyMsg        = NULL ;
      INT32 modifyMsgSize     = 0 ;

      BOOLEAN isUpdate        = FALSE ;
      INT32 flags             = 0 ;
      CHAR *pCollectionName   = NULL ;
      INT64 numToSkip         = 0 ;
      INT64 numToReturn       = -1 ;
      CHAR *pQuery            = NULL ;
      CHAR *pFieldSelector    = NULL ;
      CHAR *pOrderBy          = NULL ;
      CHAR *pHint             = NULL ;

      BSONObj objQuery ;
      BSONObj objSelector ;
      BSONObj objOrderby ;
      BSONObj objHint ;
      BSONObj objNewHint ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flags, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery,
                            &pFieldSelector, &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query msg failed, rc: %d", rc ) ;

      try
      {
         if ( !allCataGroup )
         {
            objQuery = BSONObj( pQuery ) ;
         }
         objSelector = BSONObj( pFieldSelector ) ;
         objOrderby = BSONObj( pOrderBy ) ;
         objHint = BSONObj( pHint ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace query msg occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( pContext )
      {
         if ( NULL == *pContext )
         {
            // create context
            rc = pRtncb->contextNew( RTN_CONTEXT_COORD,
                                     (rtnContext **)pContext,
                                     contextID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                         rc ) ;
         }
         else
         {
            contextID = (*pContext)->contextID() ;
            // the context is create in out side, do nothing
         }
         pvtData._pContext = *pContext ;
      }

      if ( pvtData._pContext && !pvtData._pContext->isOpened() )
      {
         // open context, explain only in query msg
         if ( ( ( FLG_QUERY_EXPLAIN & pQueryMsg->flags ) &&
                '$' != pCollectionName[ 0 ] ) ||
              openEmptyContext )
         {
            rc = pvtData._pContext->open( BSONObj(), BSONObj(), -1, 0 ) ;
         }
         else
         {
            BOOLEAN needReset = FALSE ;
            if ( FLG_QUERY_STRINGOUT & pQueryMsg->flags )
            {
               needReset = TRUE ;
            }
            else
            {
               rtnNeedResetSelector( objSelector, objOrderby, needReset ) ;
            }

            // build new selector
            if ( needReset )
            {
               static BSONObj emptyObj = BSONObj() ;
               rc = _buildNewMsg( (const CHAR*)pMsg, &emptyObj, NULL,
                                  pNewMsg, newMsgSize ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to build new msg: %d", rc ) ;
                  goto error ;
               }
               pQueryMsg = (MsgOpQuery *)pNewMsg ;
               inMsg._pMsg = ( MsgHeader* )pNewMsg ;
            }

            // open context
            rc = pvtData._pContext->open( objOrderby,
                                          needReset ? objSelector : BSONObj(),
                                          pQueryMsg->numToReturn,
                                          pQueryMsg->numToSkip,
                                          ( FLG_QUERY_MODIFY & pQueryMsg->flags ) ? FALSE : TRUE ) ;

            // change some data
            if ( pQueryMsg->numToReturn > 0 && pQueryMsg->numToSkip > 0 )
            {
               // some record may skip on coord,
               // so the num of records from data-node must
               // more than "numToReturn + numToSkip"
               pQueryMsg->numToReturn += pQueryMsg->numToSkip ;
            }
            pQueryMsg->numToSkip = 0 ;

            if ( FLG_QUERY_STRINGOUT & pQueryMsg->flags )
            {
               pvtData._pContext->getSelector().setStringOutput( TRUE ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Open context failed(rc=%d)", rc ) ;
      }

   retry_cata :
      // get collection catalog info
      rc = rtnCoordGetCataInfo( cb, pRealCLName ? pRealCLName : pCollectionName,
                                updateCata, cataInfo ) ;
      if ( updateCata && SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
      {
         // If the rc is SDB_CLS_COORD_NODE_CAT_VER_OLD, it might be caused
         // by the out-of-date mainCL cache, removed the mainCL cache and
         // try again.
         CoordCB *pCoordcb = pmdGetKRCB()->getCoordCB() ;
         pCoordcb->delMainCLCataInfo( pRealCLName ? pRealCLName : pCollectionName ) ;
         goto retry_cata ;
      }

      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the catalog info(collection:%s), rc: %d",
                   pRealCLName ? pRealCLName : pCollectionName, rc ) ;
      if ( updateCata )
      {
         ++sendOpt._retryTimes ;
      }

      //e.g. objHint = {"$Modify":{"OP":"update", "Update":{"$set":{a:1}} } }
      isUpdate = _isUpdate( objHint, pQueryMsg->flags ) ;
      /// save the last msg
      pLastMsg = ( const CHAR* )pQueryMsg ;

   retry:
      do
      {
         /// restore the last msg
         pQueryMsg = ( MsgOpQuery* )pLastMsg ;
         inMsg._pMsg = ( MsgHeader* )pLastMsg ;

         if ( isUpdate && cataInfo->isSharded() )
         {
            //kick shardingKey
            BOOLEAN isChanged = FALSE ;
            BSONObj tmpNewHint = objHint ;
            BOOLEAN isEmpty = FALSE ;

            rc = _generateNewHint( cataInfo, objQuery, objHint,
                                   tmpNewHint, isChanged, isEmpty, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "generate new hint failed:rc=%d", rc ) ;
               goto error ;
            }
            else if ( isChanged )
            {
               if ( !pModifyMsg || !tmpNewHint.equal( objNewHint ) )
               {
                  rc = _buildNewMsg( (const CHAR*)inMsg._pMsg, NULL,
                                     &tmpNewHint, pModifyMsg, modifyMsgSize ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to build new msg: %d", rc ) ;
                     goto error ;
                  }
                  pQueryMsg   = (MsgOpQuery *)pModifyMsg ;
                  inMsg._pMsg = (MsgHeader*)pModifyMsg ;

                  if ( isEmpty )
                  {
                     pQueryMsg->flags &= ~FLG_QUERY_MODIFY ;
                  }
                  objNewHint = tmpNewHint ;
               }
               else
               {
                  pQueryMsg   = (MsgOpQuery *)pModifyMsg ;
                  inMsg._pMsg = (MsgHeader*)pModifyMsg ;
               }
            }
         }
         pQueryMsg->version = cataInfo->getVersion() ;

         if ( cataInfo->isMainCL() )
         {
            rcTmp = doOpOnMainCL( cataInfo, objQuery, inMsg, sendOpt,
                                  pRouteAgent, cb, result ) ;
         }
         else
         {
            rcTmp = doOpOnCL( cataInfo, objQuery, inMsg, sendOpt,
                              pRouteAgent, cb, result ) ;
         }
      }while( FALSE ) ;

      if ( SDB_OK != pvtData._ret )
      {
         rc = pvtData._ret ;
         PD_LOG( PDERROR, "Query failed, rc: %d", rc ) ;
         goto error ;
      }
      else if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         if ( pvtData._pContext )
         {
            pvtData._pContext->addSubDone( cb ) ;
         }
         goto done ;
      }
      else if ( checkRetryForCLOpr( rcTmp, &nokRC, inMsg.msg(),
                                    sendOpt._retryTimes,
                                    cataInfo, cb, rc, &errNodeID, TRUE ) )
      {
         nokRC.clear() ;
         ++sendOpt._retryTimes ;
         goto retry ;
      }
      else
      {
         PD_LOG( PDERROR, "Query failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      SAFE_OSS_FREE( pNewMsg ) ;
      SAFE_OSS_FREE( pModifyMsg ) ;
      if ( pSucGrpLst )
      {
         *pSucGrpLst = result._sucGroupLst ;
      }
      sendOpt._pIgnoreRC = pOldIgnoreRC ;
      return rc ;
   error:
      if ( SDB_CAT_NO_MATCH_CATALOG == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      if ( -1 != contextID  )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
         *pContext = NULL ;
      }
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( rtnBuildErrorObj( rc, cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 rtnCoordQuery::_buildNewMsg( const CHAR *msg,
                                      const bson::BSONObj *newSelector,
                                      const BSONObj *newHint,
                                      CHAR *&newMsg,
                                      INT32 &buffSize )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0;
      CHAR *pCollectionName = NULL;
      SINT64 numToSkip = 0;
      SINT64 numToReturn = 0;
      CHAR *pQuery = NULL;
      CHAR *pFieldSelector = NULL;
      CHAR *pOrderBy = NULL;
      CHAR *pHint = NULL;
      BSONObj query ;
      BSONObj selector ;
      BSONObj orderBy ;
      BSONObj hint ;
      MsgOpQuery *pSrc = (MsgOpQuery *)msg;

      SDB_ASSERT( newSelector || newHint, "Selector and hint are both NULL" ) ;

      rc = msgExtractQuery( ( CHAR * )msg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery,
                            &pFieldSelector, &pOrderBy, &pHint );
      PD_RC_CHECK( rc, PDERROR,
                  "failed to parse query request(rc=%d)", rc );

      try
      {
         query = BSONObj( pQuery ) ;
         selector = BSONObj( pFieldSelector ) ;
         orderBy = BSONObj( pOrderBy ) ;
         hint = BSONObj( pHint ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( newSelector )
      {
         selector = *newSelector ;
      }
      if ( newHint )
      {
         hint = *newHint ;
      }

      rc = msgBuildQueryMsg( &newMsg, &buffSize,
                             pCollectionName,
                             flag, pSrc->header.requestID,
                             numToSkip, numToReturn,
                             &query, &selector,
                             &orderBy, &hint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build new msg:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      SAFE_OSS_FREE( newMsg ) ;
      buffSize = 0 ;
      goto done ;
   }

}
