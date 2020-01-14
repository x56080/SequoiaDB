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

   Source File Name = clsGTSAgent.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2012  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsGTSAgent.hpp"
#include "clsShardMgr.hpp"
#include "msgReplicator.hpp"
#include "pmdEnv.hpp"
#include "pmd.hpp"
#include "dpsTransCB.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsOp2Record.hpp"
#include "dms.hpp"
#include "dpsLogRecordDef.hpp"
#include "dpsUtil.hpp"
#include "clsTrace.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      _clsGTSAgent implement
   */
   _clsGTSAgent::_clsGTSAgent( _clsShardMgr *pShardMgr )
   {
      SDB_ASSERT( pShardMgr, "Invalid param" ) ;

      _pShardMgr = pShardMgr ;
   }

   _clsGTSAgent::~_clsGTSAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_ONROLLBACKALL, "_clsGTSAgent::onRollbackAll" )
   INT32 _clsGTSAgent::onRollbackAll()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_ONROLLBACKALL ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;

      DPS_TRANS_ID transID ;
      TRANS_MAP *pTransMap = _transCB->getTransMap() ;
      TRANS_MAP tmpTransMap ;
      TRANS_MAP::iterator it ;
      BOOLEAN isStoped = FALSE ;

      while ( TRUE )
      {
         UINT32 transCBSize = _transCB->getTransCBSize() ;
         if ( transCBSize == 0 )
         {
            break ;
         }
         if ( !_transCB->isDoRollback() )
         {
            isStoped = TRUE ;
            goto done ;
         }
         PD_LOG( PDDEBUG, "There are still %u EDUs under rollback or commit",
                 transCBSize ) ;
         ossSleep( OSS_ONE_SEC ) ;
      }

      // to avoid erase iterator and insert in the same map
      _transCB->cloneTransMap( tmpTransMap ) ;

      // check doing transactions, update to doing interrupted
      it = tmpTransMap.begin() ;
      while ( it != tmpTransMap.end() )
      {
         transID = it->first ;
         dpsTransBackInfo &transInfo = it->second ;
         if ( DPS_TRANS_DOING == transInfo._status )
         {
            PD_LOG( PDDEBUG, "Transaction(ID:%s, IDAttr:%s) is doing, "
                    "need interrupt", dpsTransIDToString( transID ).c_str(),
                    dpsTransIDAttrToString( transID ).c_str() ) ;
            pTransCB->updateTransStatus( transID, DPS_TRANS_DOING_INTERRUPT ) ;
         }
         ++ it ;
      }

      // check pre-commit transactions, commit if passes check
      it = tmpTransMap.begin() ;
      while ( it != tmpTransMap.end() )
      {
         DPS_TRANS_STATUS status = DPS_TRANS_UNKNOWN ;
         transID = it->first ;
         dpsTransBackInfo &transInfo = it->second ;

         if ( DPS_TRANS_WAIT_COMMIT == transInfo._status &&
              DPS_INVALID_LSN_OFFSET != transInfo._lsn )
         {
            PD_LOG( PDWARNING, "Transaction(ID:%s, IDAttr:%s) is in-doubt",
                    dpsTransIDToString( transID ).c_str(),
                    dpsTransIDAttrToString( transID ).c_str() ) ;

            rc = _syncCheckTransStatus( transID, transInfo._lsn, status ) ;
            if ( SDB_OK == rc && DPS_TRANS_COMMIT == status )
            {
               rc = _commitTrans( transID, transInfo._lsn, transInfo._lsn ) ;
               if ( SDB_OK == rc )
               {
                  pTransMap->erase( transID ) ;
                  tmpTransMap.erase( it++ ) ;
                  continue ;
               }
            }
            else if ( !_transCB->isDoRollback() )
            {
               isStoped = TRUE ;
               break ;
            }
         }
         ++it ;
      }

   done :
      rc = isStoped ? SDB_CLS_NOT_PRIMARY : SDB_OK ;
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_ONROLLBACKALL, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_CHKTRANSSTATUS, "_clsGTSAgent::checkTransStatus" )
   INT32 _clsGTSAgent::checkTransStatus( DPS_TRANS_ID transID,
                                         UINT32 nodeNum,
                                         const UINT64 *pNodes,
                                         IExecutor *cb,
                                         DPS_TRANS_STATUS &status )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_CHKTRANSSTATUS ) ;

      MsgRouteID nodeID ;

      BOOLEAN hasCommit = FALSE ;
      BOOLEAN hasRollback = FALSE ;

      for ( UINT32 i = 0 ; i < nodeNum; ++i )
      {
         nodeID.value = pNodes[ i ] ;
         if ( pmdGetNodeID().columns.groupID == nodeID.columns.groupID )
         {
            continue ;
         }

         rc = _checkTransStatus( transID, nodeID.columns.groupID, cb, status ) ;
         if ( rc )
         {
            goto error ;
         }

         // NOTE:
         // backward compatibility for DOING status
         // old version will return DOING status,
         // new version will return DOING_INTERRUPT status if it is checking
         // transaction status as well, ( if peer node is not checking, it
         // will report an error to retry, since peer node might be processing
         // pre-commit/rollback request )
         switch( status )
         {
            case DPS_TRANS_COMMIT :
               hasCommit= TRUE ;
               break ;
            case DPS_TRANS_DOING :
            case DPS_TRANS_DOING_INTERRUPT :
            case DPS_TRANS_ROLLBACK :
               hasRollback = TRUE ;
               break ;
            case DPS_TRANS_UNKNOWN :
               /// ignore unknown
               break ;
            default :
               break ;
         }
      }

      /// has rollback
      if ( hasRollback )
      {
         status = DPS_TRANS_ROLLBACK ;
      }
      else if ( hasCommit )
      {
         status = DPS_TRANS_COMMIT ;
      }
      else
      {
         status = DPS_TRANS_COMMIT ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_CHKTRANSSTATUS, rc ) ;
      return rc ;
   error:
      status = DPS_TRANS_UNKNOWN ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__CHKTRANSSTATUS, "_clsGTSAgent::_checkTransStatus" )
   INT32 _clsGTSAgent::_checkTransStatus( DPS_TRANS_ID transID,
                                          UINT32 group,
                                          IExecutor *cb,
                                          DPS_TRANS_STATUS &status )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__CHKTRANSSTATUS ) ;

      MsgClsTransCheckReq checkMsg ;
      MsgHeader *pRecvMsg = NULL ;
      MsgOpReply *pReply = NULL ;
      const UINT32 maxRetryTimes = 3 ;
      UINT32 retryTimes = 0 ;

      // for backward compatibility, transID field is global serial number
      checkMsg.transID = transID.getGlobSN() ;
      // node ID of transaction ID
      checkMsg.transIDNodeID = transID.getNodeID() ;

      while( retryTimes++ < maxRetryTimes )
      {
         /// send message
         rc = _pShardMgr->syncSend( ( MsgHeader* )&checkMsg, group, TRUE,
                                    &pRecvMsg ) ;
         if ( rc )
         {
            rc = _pShardMgr->syncSend( ( MsgHeader* )&checkMsg, group, FALSE,
                                       &pRecvMsg ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         /// extrace reply
         pReply = ( MsgOpReply* )pRecvMsg ;
         rc = pReply->flags ;
         SDB_ASSERT( pReply->contextID == -1, "Context id must be -1" ) ;

         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            INT32 rcTmp = SDB_OK ;
            rcTmp = _pShardMgr->updatePrimaryByReply( pRecvMsg, group ) ;

            if ( SDB_NET_CANNOT_CONNECT == rcTmp )
            {
               /// the node is crashed, sleep some seconds
               PD_LOG( PDWARNING, "Group(%d) primary node is crashed "
                       "but other nodes not aware, sleep %d seconds",
                       group, NET_NODE_FAULTUP_MIN_TIME ) ;
               ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            }

            if ( rcTmp )
            {
               _pShardMgr->syncUpdateGroupInfo( group, CLS_SHARD_TIMEOUT ) ;
            }

            SDB_OSS_FREE( ( CHAR* )pRecvMsg ) ;
            pRecvMsg = NULL ;
            continue ;
         }
         else if ( SDB_RTN_EXIST_INDOUBT_TRANS == rc )
         {
            // peer node might be doing pre-commit or rollback
            // retry later
            SDB_OSS_FREE( (CHAR *)pRecvMsg ) ;
            pRecvMsg = NULL ;
            ossSleep( OSS_ONE_SEC ) ;
            continue ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Check trans(%s) by node(%u,%u) failed, "
                    "rc: %d", dpsTransIDToString( transID ).c_str(),
                    group, pReply->header.routeID.columns.nodeID, rc ) ;
            goto error ;
         }
         else if ( 1 == pReply->numReturned &&
                   pReply->header.messageLength >
                   (INT32)sizeof( MsgOpReply ) + 5 )
         {
            try
            {
               BSONObj obj( ( CHAR* )pRecvMsg + sizeof( MsgOpReply ) ) ;
               BSONElement e = obj.getField( FIELD_NAME_STATUS ) ;
               status = ( DPS_TRANS_STATUS )e.numberInt() ;

               PD_LOG( PDEVENT, "Check trans(%s) by node(%u,%u) succeed["
                       "Status:%s(%d)]", dpsTransIDToString( transID ).c_str(),
                       group, pReply->header.routeID.columns.nodeID,
                       dpsTransStatusToString( status ), status ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_SYS ;
            goto error ;
         }

         /// quit
         break ;
      }

   done:
      if ( pRecvMsg )
      {
         SDB_OSS_FREE( ( CHAR* )pRecvMsg ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT__CHKTRANSSTATUS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__SYNCCHKTRANSSTATUS, "_clsGTSAgent::_syncCheckTransStatus" )
   INT32 _clsGTSAgent::_syncCheckTransStatus( DPS_TRANS_ID transID,
                                              DPS_LSN_OFFSET curLsn,
                                              DPS_TRANS_STATUS &status )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__SYNCCHKTRANSSTATUS ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      SDB_DPSCB *pDpsCB = pmdGetKRCB()->getDPSCB() ;

      DPS_LSN lsn ;
      _dpsMessageBlock mb ;
      lsn.offset = curLsn ;

      DPS_TRANS_ID recordTransID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET firstLsn = DPS_INVALID_LSN_OFFSET ;
      UINT8 attr = 0 ;
      UINT32 nodeNum = 0 ;
      const UINT64 *pNodes = NULL ;

      /// load lsn
      rc = pDpsCB->search( lsn, &mb ) ;
      SDB_ASSERT( SDB_OK == rc, "Search lsn is error" ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Search lsn(%llu) failed, rc: %d",
                 lsn.offset, rc ) ;
         goto error ;
      }

      rc = dpsRecord2TransCommit( mb.offset( 0 ), recordTransID, preTransLsn,
                                  firstLsn, attr, nodeNum,
                                  &pNodes ) ;
      SDB_ASSERT( SDB_OK == rc &&
                  attr == DPS_TS_COMMIT_ATTR_PRE &&
                  nodeNum > 0,
                  "Invalid log" ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Log is invalid" ) ;
         goto error ;
      }

      do
      {
         rc = checkTransStatus( transID, nodeNum, pNodes, cb, status ) ;
         if ( rc )
         {
            ossSleep( OSS_ONE_SEC ) ;
            continue ;
         }
         break ;
      } while( pTransCB->isDoRollback() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT__SYNCCHKTRANSSTATUS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__COMMITTRANS, "_clsGTSAgent::_commitTrans" )
   INT32 _clsGTSAgent::_commitTrans( DPS_TRANS_ID transID,
                                     DPS_LSN_OFFSET lastLsn,
                                     DPS_LSN_OFFSET &curLsn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__COMMITTRANS ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      SDB_DPSCB *pDpsCB = pmdGetKRCB()->getDPSCB() ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      UINT8 attr = DPS_TS_COMMIT_ATTR_SND ;

      DPS_LSN_OFFSET firstLsn = DPS_INVALID_LSN_OFFSET ;

      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      dpsRecordTransInfo transInfo ;
      transInfo._transID = transID ;
      transInfo._preTransLSN = lastLsn ;

      cb->setTransID( transID ) ;
      cb->setCurTransLsn( lastLsn ) ;

      firstLsn = pTransCB->getBeginLsn( transID ) ;
      SDB_ASSERT( firstLsn != DPS_INVALID_LSN_OFFSET,
                  "First transaction lsn can't be invalid" ) ;

      PD_LOG( PDEVENT, "Execute commit(ID:%s, LastLsn=%llu)",
              dpsTransIDToString( transID ).c_str(),
              lastLsn ) ;

      rc = dpsTransCommit2Record( transInfo, firstLsn,
                                  attr, NULL, NULL, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build commit log:%d",rc ) ;
         goto error ;
      }

      info.setInfoEx( ~0, DMS_INVALID_CLID, DMS_INVALID_EXTENT, cb ) ;
      info.enableTrans() ;
      rc = pDpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                   "log(rc=%d)", rc ) ;
      pDpsCB->writeData( info ) ;

      curLsn = cb->getCurTransLsn() ;

      // make sure to commit meta-block statistics
      // NOTE: actually it is empty
      cb->getTransExecutor()->commitMBStats() ;

      cb->resetTransID() ;
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      // clear all lsn mapping
      cb->getTransExecutor()->clearRecordMap() ;
      // release all transactions lock
      pTransCB->transLockReleaseAll( cb ) ;
      // reduce the reservedLogSpace from dps for the transaction
      pTransCB->releaseRBLogSpace( cb ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT__COMMITTRANS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_UPDATEGLOBLOWTRAN, "_clsGTSAgent::updateGlobLowTran" )
   INT32 _clsGTSAgent::updateGlobLowTran()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_UPDATEGLOBLOWTRAN ) ;

      MsgGTSLowTranReq request ;
      MsgHeader *receiveMessage = NULL ;

      DPS_TRANSID_SN nodeLowTran = DPS_INVALID_TRANSID_SN ;
      BSONObj requestObject ;

      const UINT32 maxRetryTimes = 3 ;
      UINT32 retryTimes = 0 ;

      // only COORD and DATA need report
      // NOTE: here is DATA node
      if ( SDB_ROLE_DATA != pmdGetDBRole() )
      {
         goto done ;
      }

      // get local lowTran
      rc = _getLocalLowTran( nodeLowTran ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get local lowTran, rc: %d", rc ) ;

      // fill lowTran request
      rc = _fillLowTranRequest( &request, nodeLowTran, requestObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to fill lowTran request, rc: %d", rc ) ;

      while( ( retryTimes ++ ) < maxRetryTimes )
      {
         MsgOpReply *reply = NULL ;

         /// try to send message to CATALOG primary first
         rc = _pShardMgr->syncSend( (MsgHeader *)( &request ),
                                    CATALOG_GROUPID,
                                    TRUE,
                                    &receiveMessage,
                                    CLS_SHARD_TIMEOUT,
                                    requestObject.objdata(),
                                    (UINT32)( requestObject.objsize() ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to send lowTran to primary catalog, "
                    "rc: %d", rc ) ;
            // failed to send message to primary CATALOG, just send to any
            // CATALOG, it will report new primary if it is not
            rc = _pShardMgr->syncSend( (MsgHeader *)( &request ),
                                       CATALOG_GROUPID,
                                       FALSE,
                                       &receiveMessage,
                                       CLS_SHARD_TIMEOUT,
                                       requestObject.objdata(),
                                       (UINT32)( requestObject.objsize() ) ) ;
            if ( SDB_OK != rc )
            {
               // still failed, just return error, no need to retry
               PD_LOG( PDWARNING, "Failed to send lowTran to any catalog, "
                       "rc: %d", rc ) ;
               goto error ;
            }
         }

         SDB_ASSERT( NULL != receiveMessage, "receive message is invalid" ) ;
         SDB_ASSERT( MSG_GTS_LOWTRAN_RSP == receiveMessage->opCode,
                     "receive message is not global lowTran response" ) ;

         /// extract reply
         reply = (MsgOpReply *)receiveMessage ;
         rc = reply->flags ;
         SDB_ASSERT( reply->contextID == -1, "Context id must be -1" ) ;

         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            // failed to send message to primary CATALOG, update the primary
            // and retry
            INT32 rcTmp = SDB_OK ;
            rcTmp = _pShardMgr->updatePrimaryByReply( receiveMessage,
                                                      CATALOG_GROUPID ) ;

            if ( SDB_NET_CANNOT_CONNECT == rcTmp )
            {
               /// the catalog nodes are crashed, sleep some seconds
               PD_LOG( PDWARNING, "Group(%d) primary node is crashed "
                       "but other nodes not aware, sleep %d seconds",
                       CATALOG_GROUPID, NET_NODE_FAULTUP_MIN_TIME ) ;
               ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            }

            if ( SDB_OK != rcTmp )
            {
               // failed to update primary, update the whole CATALOG group
               _pShardMgr->updateCatGroup( CLS_SHARD_TIMEOUT ) ;
            }

            // go retry
            SDB_OSS_FREE( (CHAR *)receiveMessage ) ;
            receiveMessage = NULL ;
            continue ;
         }
         else if ( SDB_OK != rc )
         {
            // could not retry for other errors, just report and quit
            PD_LOG( PDERROR, "Failed to get global transaction ID, rc: %d",
                    rc ) ;
            goto error ;
         }
         else
         {
            // extract global lowTran from response, and update
            MsgGTSLowTranRsp *response = (MsgGTSLowTranRsp *)receiveMessage ;
            DPS_TRANSID_SN globLowTran = DPS_INVALID_TRANSID_SN ;

            rc = _parseLowTranResponse( response, globLowTran ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse lowTran response, "
                         "rc: %d", rc ) ;

            _setGlobLowTran( (DPS_TRANSID_SN)( globLowTran ) ) ;

            // quit
            break ;
         }

         SDB_ASSERT( FALSE, "Should not go here" ) ;
      }

   done:
      if ( NULL != receiveMessage )
      {
         SDB_OSS_FREE( (CHAR *)receiveMessage ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_UPDATEGLOBLOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
