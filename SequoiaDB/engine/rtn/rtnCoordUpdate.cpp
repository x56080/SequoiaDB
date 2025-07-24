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

   Source File Name = rtnCoordUpdate.cpp

   Descriptive Name = Runtime Coord Update

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   update operation on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCoordUpdate.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "mthModifier.hpp"

using namespace bson;

namespace engine
{
   //PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOUPDATE_EXECUTE, "rtnCoordUpdate::execute" )
   INT32 rtnCoordUpdate::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOUPDATE_EXECUTE ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB() ;
      CoordCB *pCoordcb                = pKrcb->getCoordCB() ;
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent() ;

      // process define
      rtnSendOptions sendOpt( TRUE ) ;
      rtnSendMsgIn inMsg( pMsg ) ;
      rtnProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;

      CoordCataInfoPtr cataInfo ;
      MsgRouteID errNodeID ;
      UINT64 updateNum = 0 ;
      INT32  insertNum = 0 ;
      inMsg._pvtData = ( CHAR* )&updateNum ;
      inMsg._pvtType = PRIVATE_DATA_NUMBERLONG ;

      BSONObj newUpdator ;
      CHAR *pMsgBuff = NULL ;
      INT32 buffLen  = 0 ;
      MsgOpUpdate *pNewUpdate          = NULL ;
      BOOLEAN emptyUpdateCata          = FALSE ;

      // fill default-reply(update success)
      MsgOpUpdate *pUpdate             = (MsgOpUpdate *)pMsg ;
      INT32 oldFlag                    = pUpdate->flags ;
      pUpdate->flags                  |= FLG_UPDATE_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag                       = 0;
      CHAR *pCollectionName            = NULL ;
      CHAR *pSelector                  = NULL ;
      CHAR *pUpdator                   = NULL ;
      CHAR *pHint                      = NULL ;
      BSONObj boSelector ;
      BSONObj boHint ;
      BSONObj boUpdator ;
      rc = msgExtractUpdate( (CHAR*)pMsg, &flag, &pCollectionName,
                             &pSelector, &pUpdator, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse update request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      try
      {
         boSelector = BSONObj( pSelector ) ;
         boHint = BSONObj( pHint ) ;
         boUpdator = BSONObj( pUpdator ) ;

         if ( boUpdator.isEmpty() )
         {
            PD_LOG( PDERROR, "modifier can't be empty" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // add last op info
         MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                             "Collection:%s, Matcher:%s, Updator:%s, Hint:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             boSelector.toString().c_str(),
                             boUpdator.toString().c_str(),
                             boHint.toString().c_str(),
                             oldFlag, oldFlag ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "Update failed, received unexpected error: %s",
                      e.what() ) ;
      }

      rc = rtnCoordGetCataInfo( cb, pCollectionName, FALSE, cataInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Update failed, failed to get the "
                   "catalogue info(collection name: %s), rc: %d",
                   pCollectionName, rc ) ;

      pNewUpdate = pUpdate ;

   retry:
      do
      {
         BSONObj tmpNewObj = boUpdator ;
         BOOLEAN hasShardingKey = FALSE ;

         if ( cataInfo->isSharded() )
         {
            rtnCoordShardKicker shardKicker ;
            rc = shardKicker.kickShardingKey( cataInfo, boUpdator, tmpNewObj,
                                              hasShardingKey ) ;
            PD_RC_CHECK( rc, PDERROR, "Update failed, failed to kick the "
                         "sharding-key field(rc=%d)", rc ) ;

            if ( cataInfo->isMainCL() )
            {
               BSONObj newSubObj ;
               CoordSubCLlist subCLList ;
               rcTmp = cataInfo->getMatchSubCLs( boSelector, subCLList ) ;
               if ( rcTmp )
               {
                  PD_LOG( PDERROR,"Failed to get match sub-collection, "
                          "rc: %d", rcTmp ) ;
                  break ;
               }

               rc = shardKicker.kickShardingKeyForSubCL( subCLList, tmpNewObj,
                                                         newSubObj,
                                                         hasShardingKey, cb ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to kick the sharding-key field "
                            "for sub-collection, rc: %d",
                            rc ) ;
               tmpNewObj = newSubObj ;
            }
         }

         if ( !hasShardingKey )
         {
            // no sharding key
            pNewUpdate = pUpdate ;
         }
         else if ( !pMsgBuff || !tmpNewObj.equal( newUpdator ) )
         {
            if ( tmpNewObj.isEmpty() )
            {
               if ( flag & FLG_UPDATE_UPSERT )
               {
                  tmpNewObj = BSON( "$null" << BSON( "null" << 1 ) ) ;
               }
               else if ( !emptyUpdateCata )
               {
                  rc = rtnCoordGetCataInfo( cb, pCollectionName, TRUE,
                                            cataInfo ) ;
                  PD_RC_CHECK( rc, PDERROR, "Update failed, failed to get the "
                               "catalogue info(collection name: %s), rc: %d",
                               pCollectionName, rc ) ;
                  emptyUpdateCata = TRUE ;
                  ++sendOpt._retryTimes ;
                  goto retry ;
               }
               else
               {
                  // don't do anything( return error?)
                  goto done ;
               }
            }

            newUpdator = tmpNewObj ;
            rc = msgBuildUpdateMsg( &pMsgBuff, &buffLen, pUpdate->name,
                                    flag, 0, &boSelector,
                                    &newUpdator, &boHint ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build update request, rc: %d", rc ) ;
            pNewUpdate = (MsgOpUpdate *)pMsgBuff ;
         }

         pNewUpdate->version = cataInfo->getVersion() ;
         pNewUpdate->w = 0 ;
         if ( pNewUpdate->flags & FLG_UPDATE_UPSERT )
         {
            pNewUpdate->flags &= ~FLG_UPDATE_UPSERT ;
         }
         inMsg._pMsg = ( MsgHeader* )pNewUpdate ;

         if ( cataInfo->isMainCL() )
         {
            rcTmp = doOpOnMainCL( cataInfo, boSelector, inMsg, sendOpt,
                                  pRouteAgent, cb, result ) ;
         }
         else
         {
            rcTmp = doOpOnCL( cataInfo, boSelector, inMsg, sendOpt,
                              pRouteAgent, cb, result ) ;
         }
      }while( FALSE ) ;

      if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         // do nothing, for upsert
      }
      else if ( checkRetryForCLOpr( rcTmp, &nokRC, inMsg.msg(),
                                    sendOpt._retryTimes,
                                    cataInfo, cb, rc, &errNodeID, TRUE ) )
      {
         nokRC.clear() ;
         ++sendOpt._retryTimes ;
         goto retry ;
      }
      else if ( SDB_CAT_NO_MATCH_CATALOG == rcTmp )
      {
         /// ignore
         rc = SDB_OK ;
      }
      else
      {
         PD_LOG( PDERROR, "Update failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

      // upsert
      if ( ( flag & FLG_UPDATE_UPSERT ) && 0 == updateNum )
      {
         _mthMatchTree matcher ;
         mthModifier modifier;
         BSONObj source ;
         BSONObj target ;
         rtnCoordProcesserFactory *pProcesserFactory = NULL ;
         rtnCoordOperator *pOpProcesser = NULL ;

         rc = matcher.loadPattern ( boSelector ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to load matcher, "
                       "query: %s, rc: %d",
                       boSelector.toString().c_str(), rc ) ;
         source = matcher.getEqualityQueryObject() ;

         rc = modifier.loadPattern( boUpdator ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for "
                      "updator: %s, rc: %d",
                      boUpdator.toString().c_str(), rc ) ;

         rc = modifier.modify( source, target ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to generate upsertor "
                      "record(rc=%d)", rc ) ;

         BSONElement setOnInsert = boHint.getField( FIELD_NAME_SET_ON_INSERT ) ;
         if ( !setOnInsert.eoo() )
         {
            rc = rtnUpsertSet( setOnInsert, target ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to set when upsert, rc: %d", rc ) ;
         }

         pProcesserFactory = pCoordcb->getProcesserFactory() ;
         pOpProcesser = pProcesserFactory->getOperator( MSG_BS_INSERT_REQ ) ;
         SDB_ASSERT( pOpProcesser , "pCmdProcesser can't be NULL" ) ;

         rc = msgBuildInsertMsg( &pMsgBuff, &buffLen, pUpdate->name, 0,
                                 0, &target ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to build insert message, rc: %d",
                      rc ) ;

         rc = pOpProcesser->execute( (MsgHeader*)pMsgBuff,
                                     cb, contextID, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert the data[%s], rc: %d",
                      target.toString().c_str(), rc ) ;
         insertNum = 1 ;
      }

   done:
      if ( oldFlag & FLG_UPDATE_RETURNNUM )
      {
         contextID = updateNum ;
      }
      if ( pCollectionName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_UPDATE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "UpdatedNum:%llu, InsertedNum:%u, Matcher:%s, "
                      "Updator:%s, Hint:%s, Flag:0x%08x(%u)",
                      updateNum, insertNum,
                      boSelector.toString().c_str(),
                      boUpdator.toString().c_str(),
                      boHint.toString().c_str(), oldFlag, oldFlag ) ;
      }
      if ( pMsgBuff )
      {
         SDB_OSS_FREE( pMsgBuff ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOUPDATE_EXECUTE, rc ) ;
      return rc ;
   error:
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( rtnBuildErrorObj( rc, cb, &nokRC ) ) ;
      }
      goto done;
   }

   void rtnCoordUpdate::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      pMsg->opCode = MSG_BS_TRANS_UPDATE_REQ ;
   }

   INT32 rtnCoordUpdate::_prepareMainCLOp( CoordCataInfoPtr &cataInfo,
                                           CoordGroupSubCLMap &grpSubCl,
                                           rtnSendMsgIn &inMsg,
                                           rtnSendOptions &options,
                                           netMultiRouteAgent *pRouteAgent,
                                           pmdEDUCB *cb,
                                           rtnProcessResult &result,
                                           ossValuePtr &outPtr )
   {
      INT32 rc                = SDB_OK ;
      MsgOpUpdate *pUpMsg     = ( MsgOpUpdate* )inMsg.msg() ;

      INT32 flag              = 0 ;
      CHAR *pCollectionName   = NULL;
      CHAR *pSelector         = NULL ;
      CHAR *pUpdator          = NULL ;
      CHAR *pHint             = NULL;
      BSONObj boSelector ;
      BSONObj boUpdator ;
      BSONObj boHint ;
      BSONObj boNew ;

      CHAR *pBuff             = NULL ;
      UINT32 buffLen          = 0 ;
      UINT32 buffPos          = 0 ;
      vector<CHAR*> *pBlock   = NULL ;

      CoordGroupSubCLMap::iterator it ;

      outPtr                  = (ossValuePtr)0 ;
      inMsg.data()->clear() ;

      rc = msgExtractUpdate( (CHAR*)pUpMsg, &flag, &pCollectionName,
                             &pSelector, &pUpdator, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse update request, rc: %d",
                   rc ) ;

      boSelector = BSONObj( pSelector ) ;
      boUpdator = BSONObj( pUpdator ) ;
      boHint = BSONObj( pHint ) ;

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
         ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpUpdate, name) +
                                                 pUpMsg->nameLength + 1, 4 ) -
                         sizeof( MsgHeader ) ;
         iovec.push_back( ioItem ) ;

         // 2. new deletor vec( selector )
         boNew = _buildNewSelector( boSelector, subCLLst ) ;
         // 2.1 add to buff
         UINT32 roundLen = ossRoundUpToMultipleX( boNew.objsize(), 4 ) ;
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
         ossMemcpy( &pBuff[ buffPos ], boNew.objdata(), boNew.objsize() ) ;
         ioItem.iovBase = &pBuff[ buffPos ] ;
         ioItem.iovLen = roundLen ;
         buffPos += roundLen ;
         iovec.push_back( ioItem ) ;

         // 3. for last( updator + hint )
         ioItem.iovBase = boUpdator.objdata() ;
         ioItem.iovLen = ossRoundUpToMultipleX( boUpdator.objsize(), 4 ) +
                         boHint.objsize() ;
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

   void rtnCoordUpdate::_doneMainCLOp( ossValuePtr itPtr,
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
   }

   BSONObj rtnCoordUpdate::_buildNewSelector( const BSONObj &selector,
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
      builder.appendElements( selector ) ;
      builder.appendArray( CAT_SUBCL_NAME, babSubCL.arr() ) ;
      return builder.obj() ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOUPDATE_CKIFINSHKEY, "rtnCoordUpdate::checkIfIncludeShardingKey" )
   INT32 rtnCoordUpdate::checkIfIncludeShardingKey ( const CoordCataInfoPtr &cataInfo,
                                                     const CHAR *pUpdator,
                                                     BOOLEAN &isInclude,
                                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOUPDATE_CKIFINSHKEY ) ;
      isInclude = FALSE;
      try
      {
         BSONObj boUpdator( pUpdator );
         BSONObjIterator iter( boUpdator );
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next();
            BSONObj boTmp = beTmp.Obj();
            isInclude = cataInfo->isIncludeShardingKey( boTmp );
            if ( isInclude )
            {
               goto done;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Failed to check the record is include sharding-key,"
                  "occured unexpected error:%s", e.what() );
         goto error;
      }
      done :
         PD_TRACE_EXITRC ( SDB_RTNCOUPDATE_CKIFINSHKEY, rc ) ;
         return rc;
      error :
         goto done;
   }

   INT32 rtnCoordUpdate::checkModifierForSubCL ( const CoordSubCLlist &subCLList,
                                                 const CHAR *pUpdator,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      BOOLEAN isInclude;
      CoordSubCLlist::const_iterator iterCL = subCLList.begin() ;
      while( iterCL != subCLList.end() )
      {
         CoordCataInfoPtr subCataInfo ;
         rc = rtnCoordGetCataInfo( cb, (*iterCL).c_str(), FALSE,
                                   subCataInfo ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "get catalog of sub-collection(%s) failed(rc=%d)",
                      (*iterCL).c_str(), rc ) ;
         rc = checkIfIncludeShardingKey( subCataInfo, pUpdator,
                                         isInclude, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "failed to check if include sharding-key(rc=%d)",
                      rc ) ;
         if ( isInclude )
         {
            rc = SDB_UPDATE_SHARD_KEY ;
            goto done ;
         }
         ++iterCL ;
      }
   done:
      return rc;
   error:
      goto done;
   }
}
