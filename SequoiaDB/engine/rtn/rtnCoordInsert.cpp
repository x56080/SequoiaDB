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

   Source File Name = rtnCoordInsert.cpp

   Descriptive Name = Runtime Coord Insert

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   insert options on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCoordInsert.hpp"
#include "msgMessage.hpp"
#include "coordCB.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtnCommandDef.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOINS_EXECUTE, "rtnCoordInsert::execute" )
   INT32 rtnCoordInsert::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOINS_EXECUTE ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();

      // process define
      rtnSendOptions sendOpt( TRUE ) ;
      sendOpt._useSpecialGrp = TRUE ;

      rtnSendMsgIn inMsg( pMsg ) ;
      rtnProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;

      CoordCataInfoPtr cataInfo ;
      MsgRouteID errNodeID ;
      rtnCoordInsertPvtData pvtData ;
      inMsg._pvtData = ( CHAR* )&pvtData ;
      inMsg._pvtType = PRIVATE_DATA_USER ;

      // fill default-reply(insert success)
      MsgOpInsert *pInsertMsg          = (MsgOpInsert *)pMsg ;
      INT32 oldFlag                    = pInsertMsg->flags ;
      pInsertMsg->flags               |= FLG_INSERT_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pInsertor = NULL;
      INT32 count = 0 ;
      rc = msgExtractInsert( (CHAR*)pMsg, &flag,
                             &pCollectionName, &pInsertor, count ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to parse insert request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      // add list op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Collection:%s, Insertors:%s, ObjNum:%d, "
                          "Flag:0x%08x(%u)",
                          pCollectionName,
                          BSONObj(pInsertor).toString().c_str(),
                          count, oldFlag, oldFlag ) ;

      rc = rtnCoordGetCataInfo( cb, pCollectionName, FALSE, cataInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert failed, failed to get the "
                   "catalogue info(collection name: %s), rc: %d",
                   pCollectionName, rc ) ;

   retry:
      do
      {
         pInsertMsg->version = cataInfo->getVersion() ;
         pInsertMsg->w = 0 ;

         if ( !cataInfo->isMainCL() )
         {
            rcTmp = doOpOnCL( cataInfo, BSONObj(), inMsg, sendOpt,
                              pRouteAgent, cb, result ) ;

         }
         else
         {
            rcTmp = doOpOnMainCL( cataInfo, BSONObj(), inMsg, sendOpt,
                                  pRouteAgent, cb, result ) ;
         }
      }while( FALSE ) ;

      if ( SDB_OK == rcTmp && nokRC.empty() )
      {
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
         PD_LOG( PDERROR, "Insert failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      /// AUDIT
      if ( pCollectionName )
      {
         UINT32 insertedNum = 0 ;
         UINT32 ignoredNum = 0 ;
         ossUnpack32From64( pvtData._insertMixNum, insertedNum, ignoredNum ) ;

         PD_AUDIT_OP( AUDIT_DML, MSG_BS_INSERT_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc, "InsertedNum:%u, IgnoredNum:%u, "
                      "ObjNum:%u, Insertor:%s, Flag:0x%08x(%u)", insertedNum,
                      ignoredNum, count, BSONObj(pInsertor).toString().c_str(),
                      oldFlag, oldFlag ) ;
      }
      if ( oldFlag & FLG_INSERT_RETURNNUM )
      {
         contextID = pvtData._insertMixNum ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOINS_EXECUTE, rc ) ;
      return rc;
   error:
      goto done;
   }

   INT32 rtnCoordInsert::_prepareCLOp( CoordCataInfoPtr &cataInfo,
                                       rtnSendMsgIn &inMsg,
                                       rtnSendOptions &options,
                                       netMultiRouteAgent *pRouteAgent,
                                       pmdEDUCB *cb,
                                       rtnProcessResult &result,
                                       ossValuePtr &outPtr )
   {
      INT32 rc = SDB_OK ;
      MsgOpInsert *pInsertMsg = ( MsgOpInsert* )inMsg.msg() ;
      netIOV fixed( ( CHAR*)inMsg.msg() + sizeof( MsgHeader ),
                    ossRoundUpToMultipleX ( offsetof(MsgOpInsert, name) +
                                            pInsertMsg->nameLength + 1, 4 ) -
                    sizeof( MsgHeader ) ) ;

      // clear send groups
      options._groupLst.clear() ;

      if ( !cataInfo->isSharded() )
      {
         // get group
         cataInfo->getGroupLst( options._groupLst ) ;
         // don't change the msg
         goto done ;
      }
      else if ( inMsg.data()->size() == 0 )
      {
         INT32 flag = 0 ;
         CHAR *pCollectionName = NULL ;
         CHAR *pInsertor = NULL ;
         INT32 count = 0 ;

         rc = msgExtractInsert( (CHAR *)inMsg.msg(), &flag, &pCollectionName,
                                &pInsertor, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Extrace insert msg failed, rc: %d",
                      rc ) ;

         rc = shardDataByGroup( cataInfo, count, pInsertor, fixed,
                                inMsg._datas ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to shard data by group, rc: %d",
                      rc ) ;

         // only one group, send by normal
         if ( 1 == inMsg._datas.size() )
         {
            UINT32 groupID = inMsg._datas.begin()->first ;
            options._groupLst[ groupID ] = groupID ;
            inMsg._datas.clear() ;
         }
         else
         {
            GROUP_2_IOVEC::iterator it = inMsg._datas.begin() ;
            while( it != inMsg._datas.end() )
            {
               options._groupLst[ it->first ] = it->first ;
               ++it ;
            }
         }
      }
      // reshard
      else
      {
         rc = reshardData( cataInfo, fixed, inMsg._datas ) ;
         PD_RC_CHECK( rc, PDERROR, "Re-shard data failed, rc: %d", rc ) ;

         // build groups
         {
            GROUP_2_IOVEC::iterator it = inMsg._datas.begin() ;
            while( it != inMsg._datas.end() )
            {
               options._groupLst[ it->first ] = it->first ;
               ++it ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void rtnCoordInsert::_doneCLOp( ossValuePtr itPtr,
                                   CoordCataInfoPtr &cataInfo,
                                   rtnSendMsgIn &inMsg,
                                   rtnSendOptions &options,
                                   netMultiRouteAgent *pRouteAgent,
                                   pmdEDUCB *cb,
                                   rtnProcessResult &result )
   {
      // remove the datas by succeed group
      if ( inMsg._datas.size() > 0 )
      {
         CoordGroupList::iterator it = result._sucGroupLst.begin() ;
         while( it != result._sucGroupLst.end() )
         {
            inMsg._datas.erase( it->second ) ;
            ++it ;
         }
      }

      // clear all succeed group
      result._sucGroupLst.clear() ;
   }

   void rtnCoordInsert::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      pMsg->opCode = MSG_BS_TRANS_INSERT_REQ ;
   }

   void rtnCoordInsert::_onNodeReply( INT32 processType,
                                      MsgOpReply *pReply,
                                      pmdEDUCB *cb,
                                      rtnSendMsgIn &inMsg )
   {
      if ( inMsg._pvtType == PRIVATE_DATA_USER && inMsg._pvtData )
      {
         rtnCoordInsertPvtData *pvtData = ( rtnCoordInsertPvtData* )inMsg._pvtData ;

         if ( pReply->contextID > 0 )
         {
            UINT32 hi1 = 0, lo1 = 0 ;
            UINT32 hi2 = 0, lo2 = 0 ;
            /// (UINT32)insertedNum + (UINT32)ignoredNum
            ossUnpack32From64( pReply->contextID, hi1, lo1 ) ;
            ossUnpack32From64( pvtData->_insertMixNum, hi2, lo2 ) ;
            hi2 += hi1 ;
            lo2 += lo1 ;
            pvtData->_insertMixNum = ossPack32To64( hi2, lo2 ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOINS_SHARDANOBJ, "rtnCoordInsert::shardAnObj" )
   INT32 rtnCoordInsert::shardAnObj( CHAR *pInsertor,
                                     CoordCataInfoPtr &cataInfo,
                                     const netIOV &fixed,
                                     GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOINS_SHARDANOBJ ) ;
      try
      {
         BSONObj insertObj( pInsertor ) ;
         UINT32 roundLen = ossRoundUpToMultipleX( insertObj.objsize(), 4 ) ;
         UINT32 groupID = 0 ;

         rc = cataInfo->getGroupByRecord( insertObj, groupID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the groupid for obj[%s] "
                      "from catalog info[%s], rc: %d",
                      insertObj.toString().c_str(),
                      cataInfo->getCatalogSet()->toCataInfoBson(
                      ).toString().c_str(),
                      rc ) ;
         // add 2 group
         {
            netIOVec &iovec = datas[ groupID ] ;
            UINT32 size = iovec.size() ;
            if( size > 0 )
            {
               if ( (const CHAR*)( iovec[size-1].iovBase ) +
                    iovec[size-1].iovLen == pInsertor )
               {
                  // only change the length
                  iovec[size-1].iovLen += roundLen ;
               }
               else
               {
                  iovec.push_back( netIOV( pInsertor, roundLen ) ) ;
               }
            }
            else
            {
               iovec.push_back( fixed ) ;
               iovec.push_back( netIOV( pInsertor, roundLen ) ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to shard the data, received unexpected error: %s",
                   e.what() );
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOINS_SHARDANOBJ, rc ) ;
      return rc ;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOINS_SHARDDBGROUP, "rtnCoordInsert::shardDataByGroup" )
   INT32 rtnCoordInsert::shardDataByGroup( CoordCataInfoPtr &cataInfo,
                                           INT32 count,
                                           CHAR *pInsertor,
                                           const netIOV &fixed,
                                           GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOINS_SHARDDBGROUP ) ;
      while ( count > 0 )
      {
         rc = shardAnObj( pInsertor, cataInfo, fixed, datas );
         PD_RC_CHECK( rc, PDERROR, "Failed to shard the obj, rc: %d", rc ) ;

         BSONObj boInsertor ;
         try
         {
            boInsertor = BSONObj( pInsertor ) ;
         }
         catch ( std::exception &e )
         {
            PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse the insert-obj: %s", e.what() ) ;
         }
         --count ;
         pInsertor += ossRoundUpToMultipleX( boInsertor.objsize(), 4 ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOINS_SHARDDBGROUP, rc ) ;
      return rc;
   error:
      datas.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOINS_RESHARDDATA, "rtnCoordInsert::reshardData" )
   INT32 rtnCoordInsert::reshardData( CoordCataInfoPtr &cataInfo,
                                      const netIOV &fixed,
                                      GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOINS_RESHARDDATA ) ;
      CHAR *pData = NULL ;
      UINT32 offset = 0 ;
      UINT32 roundSize = 0 ;
      BSONObj obInsert ;

      GROUP_2_IOVEC newDatas ;
      GROUP_2_IOVEC::iterator it = datas.begin() ;
      while ( it != datas.end() )
      {
         netIOVec &iovec = it->second ;
         UINT32 size = iovec.size() ;
         // skip the first
         for ( UINT32 i = 1 ; i < size ; ++i )
         {
            netIOV &ioItem = iovec[ i ] ;
            pData = ( CHAR* )ioItem.iovBase ;
            offset = 0 ;

            while( offset < ioItem.iovLen )
            {
               try
               {
                  obInsert = BSONObj( pData ) ;
               }
               catch( std::exception &e )
               {
                  PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                            "Failed to parse the insert-obj: %s", e.what() ) ;
               }

               rc = shardAnObj( pData, cataInfo, fixed, newDatas ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to re-shard the obj, rc: %d",
                            rc ) ;

               roundSize = ossRoundUpToMultipleX( obInsert.objsize(), 4 ) ;
               pData += roundSize ;
               offset += roundSize ;
            }
         }
         ++it ;
      }
      datas = newDatas ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOINS_RESHARDDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordInsert::_prepareMainCLOp( CoordCataInfoPtr &cataInfo,
                                           CoordGroupSubCLMap &grpSubCl,
                                           rtnSendMsgIn &inMsg,
                                           rtnSendOptions &options,
                                           netMultiRouteAgent *pRouteAgent,
                                           pmdEDUCB *cb,
                                           rtnProcessResult &result,
                                           ossValuePtr &outPtr )
   {
      INT32 rc = SDB_OK ;

      vector< BSONObj > *pVecObj = NULL ;
      outPtr = ( ossValuePtr )NULL ;

      SDB_ASSERT( inMsg._pvtType == PRIVATE_DATA_USER &&
                  inMsg._pvtData, "Private data is error" ) ;
      rtnCoordInsertPvtData *pvtData = ( rtnCoordInsertPvtData*)inMsg._pvtData ;
      GroupSubCLMap *pGrpSubCLDatas = NULL ;

      MsgOpInsert *pInsertMsg = ( MsgOpInsert* )inMsg.msg() ;
      netIOV fixed( ( CHAR*)inMsg.msg() + sizeof( MsgHeader ),
                    ossRoundUpToMultipleX ( offsetof(MsgOpInsert, name) +
                                            pInsertMsg->nameLength + 1, 4 ) -
                    sizeof( MsgHeader ) ) ;

      GROUP_2_IOVEC::iterator it ;

      pVecObj = new vector< BSONObj >() ;
      if ( !pVecObj )
      {
         PD_LOG( PDERROR, "Alloc vector failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      if ( !pvtData )
      {
         PD_LOG( PDSEVERE, "System error, group sub collection map is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pGrpSubCLDatas = &(pvtData->_grpSubCLDatas) ;
      if ( pGrpSubCLDatas->size() == 0 )
      {
         INT32 flag = 0 ;
         CHAR *pCollectionName = NULL ;
         CHAR *pInsertor = NULL ;
         INT32 count = 0 ;

         rc = msgExtractInsert( (CHAR *)inMsg.msg(), &flag, &pCollectionName,
                                &pInsertor, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Extrace insert msg failed, rc: %d",
                      rc ) ;

         rc = shardDataByGroup( cataInfo, count, pInsertor, cb,
                                *pGrpSubCLDatas ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to shard data by group, rc: %d",
                      rc ) ;
      }
      else
      {
         rc = reshardData( cataInfo, cb, *pGrpSubCLDatas ) ;
         PD_RC_CHECK( rc, PDERROR, "Re-shard data failed, rc: %d", rc ) ;
      }

      // build msg
      inMsg._datas.clear() ;

      rc = buildInsertMsg( fixed, *pGrpSubCLDatas, *pVecObj, inMsg._datas ) ;
      PD_RC_CHECK( rc, PDERROR, "Build insert msg failed, rc: %d" ) ;

      // clear send groups
      options._groupLst.clear() ;
      // build group list
      it = inMsg._datas.begin() ;
      while( it != inMsg._datas.end() )
      {
         options._groupLst[ it->first ] = it->first ;
         ++it ;
      }

      outPtr = ( ossValuePtr )pVecObj ;

   done:
      return rc ;
   error:
      if ( pVecObj )
      {
         delete pVecObj ;
      }
      goto done ;
   }

   void rtnCoordInsert::_doneMainCLOp( ossValuePtr itPtr,
                                       CoordCataInfoPtr &cataInfo,
                                       CoordGroupSubCLMap &grpSubCl,
                                       rtnSendMsgIn &inMsg,
                                       rtnSendOptions &options,
                                       netMultiRouteAgent *pRouteAgent,
                                       pmdEDUCB *cb,
                                       rtnProcessResult &result )
   {
      SDB_ASSERT( inMsg._pvtType == PRIVATE_DATA_USER &&
                  inMsg._pvtData, "Private data is error" ) ;
      rtnCoordInsertPvtData *pvtData = (rtnCoordInsertPvtData*)inMsg._pvtData ;
      vector< BSONObj > *pVecObj = ( vector< BSONObj > * )itPtr ;

      // remove the datas by succeed group
      if ( pvtData && pvtData->_grpSubCLDatas.size() > 0 )
      {
         CoordGroupList::iterator it = result._sucGroupLst.begin() ;
         while( it != result._sucGroupLst.end() )
         {
            pvtData->_grpSubCLDatas.erase( it->second ) ;
            ++it ;
         }
      }

      // clear all succeed group
      result._sucGroupLst.clear() ;

      // release obj vector
      if ( pVecObj )
      {
         delete pVecObj ;
      }
   }

   INT32 rtnCoordInsert::shardAnObj( CHAR *pInsertor,
                                     CoordCataInfoPtr &cataInfo,
                                     pmdEDUCB * cb,
                                     GroupSubCLMap &groupSubCLMap )
   {
      INT32 rc = SDB_OK ;
      string subCLName ;
      UINT32 groupID = CAT_INVALID_GROUPID ;

      try
      {
         BSONObj insertObj( pInsertor ) ;
         CoordCataInfoPtr subClCataInfo ;
         UINT32 roundLen = ossRoundUpToMultipleX( insertObj.objsize(), 4 ) ;

         rc = cataInfo->getSubCLNameByRecord( insertObj, subCLName ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Couldn't find the match[%s] sub-collection "
                      "in cl's(%s) catalog info[%s], rc: %d",
                      insertObj.toString().c_str(), cataInfo->getName(),
                      cataInfo->getCatalogSet()->toCataInfoBson(
                      ).toString().c_str(), rc ) ;

         rc = rtnCoordGetCataInfo( cb, subCLName.c_str(), FALSE,
                                   subClCataInfo ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get catalog of "
                      "sub-collection(%s), rc: %d",
                      subCLName.c_str(), rc ) ;

         rc = subClCataInfo->getGroupByRecord( insertObj, groupID );
         PD_RC_CHECK( rc, PDWARNING, "Couldn't find the match[%s] catalog of "
                      "sub-collection(%s), rc: %d",
                      insertObj.toString().c_str(),
                      subClCataInfo->getCatalogSet()->toCataInfoBson(
                      ).toString().c_str(), rc ) ;

         (groupSubCLMap[ groupID ])[ subCLName ].push_back(
            netIOV( (const void*)pInsertor, roundLen ) ) ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to shard the data, occur unexpected error:%s",
                   e.what() );
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordInsert::shardDataByGroup( CoordCataInfoPtr &cataInfo,
                                           INT32 count,
                                           CHAR *pInsertor,
                                           pmdEDUCB *cb,
                                           GroupSubCLMap &groupSubCLMap )
   {
      INT32 rc = SDB_OK ;

      while ( count > 0 )
      {
         rc = shardAnObj( pInsertor, cataInfo, cb, groupSubCLMap ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to shard the obj, rc: %d", rc ) ;

         try
         {
            BSONObj boInsertor( pInsertor ) ;
            pInsertor += ossRoundUpToMultipleX( boInsertor.objsize(), 4 ) ;
            --count ;
         }
         catch ( std::exception &e )
         {
            PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse the insert-obj, "
                      "occur unexpected error:%s", e.what() ) ;
         }
      }

   done:
      return rc;
   error:
      groupSubCLMap.clear() ;
      goto done ;
   }

   INT32 rtnCoordInsert::reshardData( CoordCataInfoPtr &cataInfo,
                                      pmdEDUCB *cb,
                                      GroupSubCLMap &groupSubCLMap )
   {
      INT32 rc = SDB_OK;
      GroupSubCLMap groupSubCLMapNew ;

      GroupSubCLMap::iterator iterGroup = groupSubCLMap.begin() ;
      while ( iterGroup != groupSubCLMap.end() )
      {
         SubCLObjsMap::iterator iterCL = iterGroup->second.begin() ;
         while( iterCL != iterGroup->second.end() )
         {
            netIOVec &iovec = iterCL->second ;
            UINT32 size = iovec.size() ;

            for ( UINT32 i = 0 ; i < size ; ++i )
            {
               netIOV &ioItem = iovec[ i ] ;
               rc = shardAnObj( (CHAR*)ioItem.iovBase, cataInfo,
                                cb, groupSubCLMapNew ) ;
               PD_RC_CHECK( rc, PDWARNING, "Failed to shard the obj, rc: %d",
                            rc ) ;
            }
            ++iterCL ;
         }
         ++iterGroup ;
      }
      groupSubCLMap = groupSubCLMapNew ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordInsert::buildInsertMsg( const netIOV &fixed,
                                         GroupSubCLMap &groupSubCLMap,
                                         vector< BSONObj > &subClInfoLst,
                                         GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK ;
      static CHAR _fillData[ 8 ] = { 0 } ;

      GroupSubCLMap::iterator iterGroup = groupSubCLMap.begin() ;
      while ( iterGroup != groupSubCLMap.end() )
      {
         UINT32 groupID = iterGroup->first ;
         netIOVec &iovec = datas[ groupID ] ;
         iovec.push_back( fixed ) ;

         SubCLObjsMap &subCLDataMap = iterGroup->second ;
         SubCLObjsMap::iterator iterCL = subCLDataMap.begin();
         while ( iterCL != iterGroup->second.end() )
         {
            netIOVec &subCLIOVec = iterCL->second ;
            UINT32 dataLen = netCalcIOVecSize( subCLIOVec ) ;
            UINT32 objNum = subCLIOVec.size() ;

            // first for sub cl info
            BSONObjBuilder subCLInfoBuild ;
            subCLInfoBuild.append( FIELD_NAME_SUBOBJSNUM, (INT32)objNum ) ;
            subCLInfoBuild.append( FIELD_NAME_SUBOBJSSIZE, (INT32)dataLen ) ;
            subCLInfoBuild.append( FIELD_NAME_SUBCLNAME, iterCL->first ) ;
            BSONObj subCLInfoObj = subCLInfoBuild.obj() ;
            subClInfoLst.push_back( subCLInfoObj ) ;
            netIOV ioCLInfo ;
            ioCLInfo.iovBase = (const void*)subCLInfoObj.objdata() ;
            ioCLInfo.iovLen = subCLInfoObj.objsize() ;
            iovec.push_back( ioCLInfo ) ;

            // need fill
            UINT32 infoRoundSize = ossRoundUpToMultipleX( ioCLInfo.iovLen,
                                                          4 ) ;
            if ( infoRoundSize > ioCLInfo.iovLen )
            {
               iovec.push_back( netIOV( (const void*)_fillData,
                                infoRoundSize - ioCLInfo.iovLen ) ) ;
            }

            for ( UINT32 i = 0 ; i < objNum ; ++i )
            {
               iovec.push_back( subCLIOVec[ i ] ) ;
            }
            ++iterCL ;
         }
         ++iterGroup ;
      }

      return rc ;
   }

}

