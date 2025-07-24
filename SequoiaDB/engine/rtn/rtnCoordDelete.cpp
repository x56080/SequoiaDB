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

   Source File Name = rtnCoordDelete.cpp

   Descriptive Name = Runtime Coord Delete

   When/how to use: this program may be used on binary and text-formatted
   version of runtime component. This file contains code logic for
   data delete request from coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCoordDelete.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "msgMessage.hpp"
#include "rtnCommandDef.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson;

namespace engine
{
   //PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCODEL_EXECUTE, "rtnCoordDelete::execute" )
   INT32 rtnCoordDelete::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCODEL_EXECUTE ) ;
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
      UINT64 deleteNum = 0 ;
      inMsg._pvtData = ( CHAR* )&deleteNum ;
      inMsg._pvtType = PRIVATE_DATA_NUMBERLONG ;

      BSONObj boDeletor ;

      // fill default-reply(delete success)
      MsgOpDelete *pDelMsg             = (MsgOpDelete *)pMsg ;
      INT32 oldFlag                    = pDelMsg->flags ;
      pDelMsg->flags                  |= FLG_DELETE_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag = 0;
      CHAR *pCollectionName = NULL ;
      CHAR *pDeletor = NULL ;
      CHAR *pHint = NULL ;
      rc = msgExtractDelete( (CHAR*)pMsg, &flag, &pCollectionName,
                             &pDeletor, &pHint ) ;
      if( rc )
      {
         PD_LOG( PDERROR,"Failed to parse delete request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      try
      {
         boDeletor = BSONObj( pDeletor ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "Delete failed, received unexpected error:%s",
                      e.what() ) ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Collection:%s, Deletor:%s, Hint:%s, "
                          "Flag:0x%08x(%u)",
                          pCollectionName,
                          boDeletor.toString().c_str(),
                          BSONObj(pHint).toString().c_str(),
                          oldFlag, oldFlag ) ;

      rc = rtnCoordGetCataInfo( cb, pCollectionName, FALSE, cataInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Delete failed, failed to get the "
                   "catalogue info(collection name: %s), rc: %d",
                   pCollectionName, rc ) ;

   retry:
      do
      {
         pDelMsg->version = cataInfo->getVersion() ;
         pDelMsg->w = 0 ;

         if ( cataInfo->isMainCL() )
         {
            rcTmp = doOpOnMainCL( cataInfo, boDeletor, inMsg, sendOpt,
                                  pRouteAgent, cb, result ) ;
         }
         else
         {
            rcTmp = doOpOnCL( cataInfo, boDeletor, inMsg, sendOpt,
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
         PD_LOG( PDERROR, "Delete failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( oldFlag & FLG_DELETE_RETURNNUM )
      {
         contextID = deleteNum ;
      }
      if ( pCollectionName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_DELETE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "DeletedNum:%u, Deletor:%s, Hint:%s, Flag:0x%08x(%u)",
                      deleteNum, boDeletor.toString().c_str(),
                      BSONObj(pHint).toString().c_str(), oldFlag, oldFlag ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCODEL_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void rtnCoordDelete::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      pMsg->opCode = MSG_BS_TRANS_DELETE_REQ ;
   }

   INT32 rtnCoordDelete::_prepareMainCLOp( CoordCataInfoPtr &cataInfo,
                                           CoordGroupSubCLMap &grpSubCl,
                                           rtnSendMsgIn &inMsg,
                                           rtnSendOptions &options,
                                           netMultiRouteAgent *pRouteAgent,
                                           pmdEDUCB *cb,
                                           rtnProcessResult &result,
                                           ossValuePtr &outPtr )
   {
      INT32 rc                = SDB_OK ;
      MsgOpDelete *pDelMsg    = ( MsgOpDelete* )inMsg.msg() ;

      INT32 flag              = 0 ;
      CHAR *pCollectionName   = NULL;
      CHAR *pDeletor          = NULL;
      CHAR *pHint             = NULL;
      BSONObj boDeletor ;
      BSONObj boHint ;
      BSONObj boNew ;

      CHAR *pBuff             = NULL ;
      UINT32 buffLen          = 0 ;
      UINT32 buffPos          = 0 ;
      vector<CHAR*> *pBlock   = NULL ;

      CoordGroupSubCLMap::iterator it ;

      outPtr                  = (ossValuePtr)0 ;
      inMsg.data()->clear() ;

      rc = msgExtractDelete( (CHAR*)inMsg.msg(), &flag, &pCollectionName,
                             &pDeletor, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse delete request, rc: %d",
                   rc ) ;

      boDeletor = BSONObj( pDeletor ) ;
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
         ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpDelete, name) +
                                                 pDelMsg->nameLength + 1, 4 ) -
                         sizeof( MsgHeader ) ;
         iovec.push_back( ioItem ) ;

         // 2. new deletor vec
         boNew = _buildNewDeletor( boDeletor, subCLLst ) ;
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

         // 3. hinter vec
         ioItem.iovBase = boHint.objdata() ;
         ioItem.iovLen = boHint.objsize() ;
         iovec.push_back( ioItem ) ;         

         ++it ;
      }

      outPtr = ( ossValuePtr )pBlock ;

   done:
      return rc ;
   error:
      if ( pBlock )
      {
         for ( INT32 i = 0 ; i < (INT32)pBlock->size() ; ++i )
         {
            cb->releaseBuff( (*pBlock)[ i ] ) ;
         }
         delete pBlock ;
         pBlock = NULL ;
      }
      goto done ;
   }

   void rtnCoordDelete::_doneMainCLOp( ossValuePtr itPtr,
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
         for ( INT32 i = 0 ; i < (INT32)pBlock->size() ; ++i )
         {
            cb->releaseBuff( (*pBlock)[ i ] ) ;
         }
         delete pBlock ;
         pBlock = NULL ;
      }
      inMsg._datas.clear() ;
   }

   BSONObj rtnCoordDelete::_buildNewDeletor( const BSONObj &deletor,
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
      builder.appendElements( deletor ) ;
      builder.appendArray( CAT_SUBCL_NAME, babSubCL.arr() ) ;
      return builder.obj() ;
   }

}

