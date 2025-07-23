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
#include "msgMessageFormat.hpp"
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

   #define CLS_GTS_MAX_RETRY ( 3 )

   // wait time interval for GTS transaction
   #define CLS_GTS_WAIT_INTERVAL       ( OSS_ONE_SEC )

   // small wait time interval for GTS transaction
   #define CLS_GTS_WAIT_SMALL_INTERVAL    ( 10 )
   #define CLS_GTS_WAIT_SMALL_COUNT       ( 10 )
   // medium wait time interval for GTS transaction
   #define CLS_GTS_WAIT_MEDIUM_INTERVAL   ( 100 )
   #define CLS_GTS_WAIT_MEDIUM_COUNT      ( 100 )

   #define CLS_GTS_INC_TIME_ERROR_STEP    ( 1.1 )
   #define CLS_GTS_DEC_TIME_ERROR_STEP    ( 0.95 )

   #define CLS_GTS_DEC_TIME_ERROR_COUNT   ( 8192 )

   /*
      _clsGTSAgent implement
   */
   _clsGTSAgent::_clsGTSAgent( _clsShardMgr *pShardMgr )
   : _dpsGTSAgent(),
     _nodeTimeError( STP_DEF_TIME_ERROR ),
     _decTimeErrorCount( 0 )
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

      DPS_TRANS_ID transID ;
      TRANS_DUMP_MAP tmpTransMap ;
      TRANS_DUMP_MAP::iterator it ;
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
         ossSleep( CLS_GTS_WAIT_INTERVAL ) ;
      }

      // to avoid erase iterator and insert in the same map
      _transCB->cloneTransMap( tmpTransMap ) ;

      // update to DOING-INTERRUPTED in below cases
      // - DOING transaction: which means no pre-commit is received
      // - PRE_WAIT_COMMIT transaction: which means pre-commit is still
      //   processing, but the eduCB is detached, so it should be rolled back
      it = tmpTransMap.begin() ;
      while ( it != tmpTransMap.end() )
      {
         transID = it->first ;
         dpsTransBackInfo &transInfo = it->second ;
         if ( DPS_TRANS_DOING == transInfo._status ||
              DPS_TRANS_PRE_WAIT_COMMIT == transInfo._status )
         {
            PD_LOG( PDDEBUG, "Transaction(ID:%s, IDAttr:%s) is doing, "
                    "need interrupt", dpsTransIDToString( transID ).c_str(),
                    dpsTransIDAttrToString( transID ).c_str() ) ;
            _transCB->updateTransStatus( transID, DPS_TRANS_DOING_INTERRUPT ) ;
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
            UINT64 preCommitTime = transInfo._preCommitTime.getTime() ;
            UINT64 commitTime = 0LL ;
            PD_LOG( PDWARNING, "Transaction(ID:%s, IDAttr:%s) is in-doubt",
                    dpsTransIDToString( transID ).c_str(),
                    dpsTransIDAttrToString( transID ).c_str() ) ;

            // check transaction status on other groups involved
            rc = _syncCheckTransStatus( transID, transInfo._lsn, status,
                                        preCommitTime, commitTime ) ;
            if ( SDB_OK == rc && DPS_TRANS_COMMIT == status )
            {
               // get back commit time
               transInfo._commitTime.setTime( commitTime ) ;
               transInfo._commitTime.setTimeError(
                                    transInfo._beginTime.getTimeError() ) ;
               rc = _commitTrans( transID, transInfo._lsn, commitTime,
                                  transInfo._lsn ) ;
               if ( SDB_OK == rc )
               {
                  _transCB->removeTrans( transID ) ;
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
                                         UINT64 preCommitTime,
                                         DPS_TRANS_STATUS &status,
                                         UINT64 &commitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_CHKTRANSSTATUS ) ;

      rc = _checkTransStatus( transID, nodeNum, pNodes, cb, FALSE,
                              preCommitTime, status, commitTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check status for transaction [%s], "
                   "rc: %d", dpsTransIDToString( transID ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_CHKTRANSSTATUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__CHKTRANSSTATUS, "_clsGTSAgent::_checkTransStatus" )
   INT32 _clsGTSAgent::_checkTransStatus( DPS_TRANS_ID transID,
                                          UINT32 nodeNum,
                                          const UINT64 *pNodes,
                                          IExecutor *cb,
                                          BOOLEAN checkForArbit,
                                          UINT64 preCommitTime,
                                          DPS_TRANS_STATUS &status,
                                          UINT64 &commitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__CHKTRANSSTATUS ) ;

      MsgRouteID nodeID ;

      BOOLEAN hasCommit = FALSE ;
      BOOLEAN hasRollback = FALSE ;
      UINT64 tmpPreCommitTime = 0LL ;
      UINT64 tmpCommitTime = 0LL ;

      for ( UINT32 i = 0 ; i < nodeNum; ++i )
      {
         nodeID.value = pNodes[ i ] ;
         if ( _localRID.columns.groupID == nodeID.columns.groupID )
         {
            continue ;
         }

         rc = _checkTransStatus( transID, nodeID.columns.groupID, cb, status,
                                 tmpPreCommitTime, tmpCommitTime ) ;
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
            case DPS_TRANS_WAIT_COMMIT :
            {
               // if transaction is WAIT_COMMIT on peer node
               // use the maximum pre-commit time as the final commit time
               if ( transID.isGlobTrans() )
               {
                  preCommitTime = OSS_MAX( preCommitTime, tmpPreCommitTime ) ;
               }
               break ;
            }
            case DPS_TRANS_COMMIT :
            {
               if ( transID.isGlobTrans() )
               {
                  commitTime = tmpCommitTime ;
               }
               hasCommit= TRUE ;
               break ;
            }
            case DPS_TRANS_DOING :
            case DPS_TRANS_DOING_INTERRUPT :
            case DPS_TRANS_PRE_WAIT_COMMIT :
            {
               if ( !checkForArbit )
               {
                  hasRollback = TRUE ;
               }
               break ;
            }
            case DPS_TRANS_ROLLBACK :
            {
               hasRollback = TRUE ;
               break ;
            }
            case DPS_TRANS_UNKNOWN :
            {
               /// ignore unknown
               break ;
            }
            default :
            {
               break ;
            }
         }

         if ( checkForArbit && ( hasCommit || hasRollback ) )
         {
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
      else if ( checkForArbit )
      {
         status = DPS_TRANS_UNKNOWN ;
      }
      else
      {
         if ( transID.isGlobTrans() )
         {
            commitTime = preCommitTime ;
         }
         status = DPS_TRANS_COMMIT ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT__CHKTRANSSTATUS, rc ) ;
      return rc ;
   error:
      status = DPS_TRANS_UNKNOWN ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__CHKTRANSSTATUS_NODE, "_clsGTSAgent::_checkTransStatus" )
   INT32 _clsGTSAgent::_checkTransStatus( DPS_TRANS_ID transID,
                                          UINT32 group,
                                          IExecutor *cb,
                                          DPS_TRANS_STATUS &status,
                                          UINT64 &preCommitTime,
                                          UINT64 &commitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__CHKTRANSSTATUS_NODE ) ;

      MsgClsTransCheckReq checkMsg ;
      MsgHeader *pRecvMsg = NULL ;
      MsgOpReply *pReply = NULL ;
      UINT32 retryTimes = 0 ;

      // for backward compatibility, transID field is global serial number
      checkMsg.transID = transID.getGlobSN() ;
      // node ID of transaction ID
      checkMsg.transIDNodeID = transID.getNodeID() ;

      while( retryTimes++ < CLS_GTS_MAX_RETRY )
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

               // status
               BSONElement e = obj.getField( FIELD_NAME_STATUS ) ;
               status = ( DPS_TRANS_STATUS )e.numberInt() ;

               // pre-commit time
               e = obj.getField( FIELD_NAME_PRECOMMITTIME ) ;
               preCommitTime = (UINT64)( e.numberLong() ) ;

               // commit time
               e = obj.getField( FIELD_NAME_COMMITTIME ) ;
               commitTime = (UINT64)( e.numberLong() ) ;

               PD_LOG( PDEVENT, "Check trans(%s) by node(%u,%u) succeed["
                       "Status:%s(%d), pre-commit [%s], commit [%s]]",
                       dpsTransIDToString( transID ).c_str(),
                       group, pReply->header.routeID.columns.nodeID,
                       dpsTransStatusToString( status ), status,
                       dpsTransSNToString(
                             (DPS_TRANSID_SN)preCommitTime ).c_str(),
                       dpsTransSNToString(
                             (DPS_TRANSID_SN)commitTime ).c_str() ) ;
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
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT__CHKTRANSSTATUS_NODE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__GETCOMMITINFO, "_clsGTSAgent::_getCommitInfo" )
   INT32 _clsGTSAgent::_getCommitInfo( DPS_LSN_OFFSET commitLSN,
                                       dpsMessageBlock *mb,
                                       DPS_LOG_TYPE &logType,
                                       UINT8 &attr,
                                       UINT32 &nodeNum,
                                       const UINT64 **nodes )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__GETCOMMITINFO ) ;

      SDB_DPSCB *dpsCB = sdbGetDPSCB() ;

      DPS_LSN lsn ;
      dpsLogRecord record ;

      DPS_TRANS_ID recordTransID ;
      DPS_LSN_OFFSET preTransLSN = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET firstLSN = DPS_INVALID_LSN_OFFSET ;

      SDB_ASSERT( NULL != mb, "DPS message block is invalid" ) ;

      lsn.offset = commitLSN ;

      /// load lsn
      rc = dpsCB->search( lsn, mb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to search LSN [%llu], rc: %d",
                   commitLSN, rc ) ;

      rc = record.load( mb->offset( 0 ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load DPS record with LSN [%llu], "
                   "rc: %d", commitLSN, rc ) ;

      PD_CHECK( LOG_TYPE_TS_COMMIT == record.head()._type,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to get transaction commit record, it is not "
                "commit record, expected [%d], given [%d]",
                LOG_TYPE_TS_COMMIT, record.head()._type ) ;

      rc = dpsRecord2TransCommit( mb->offset( 0 ), recordTransID, preTransLSN,
                                  firstLSN, attr, nodeNum, nodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get transaction commit from DPS "
                   "record with LSN [%llu], rc: %d", commitLSN, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT__GETCOMMITINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__SYNCCHKTRANSSTATUS, "_clsGTSAgent::_syncCheckTransStatus" )
   INT32 _clsGTSAgent::_syncCheckTransStatus( DPS_TRANS_ID transID,
                                              DPS_LSN_OFFSET curLsn,
                                              DPS_TRANS_STATUS &status,
                                              UINT64 preCommitTime,
                                              UINT64 &commitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__SYNCCHKTRANSSTATUS ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      dpsMessageBlock mb ;
      DPS_LOG_TYPE logType = LOG_TYPE_DUMMY ;
      UINT8 attr = 0 ;
      UINT32 nodeNum = 0 ;
      const UINT64 *pNodes = NULL ;

      rc = _getCommitInfo( curLsn, &mb, logType, attr, nodeNum, &pNodes ) ;
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
         rc = checkTransStatus( transID, nodeNum, pNodes, cb, preCommitTime,
                                status, commitTime ) ;
         if ( rc )
         {
            ossSleep( CLS_GTS_WAIT_INTERVAL ) ;
            continue ;
         }
         break ;
      } while( _transCB->isDoRollback() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT__SYNCCHKTRANSSTATUS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT__COMMITTRANS, "_clsGTSAgent::_commitTrans" )
   INT32 _clsGTSAgent::_commitTrans( DPS_TRANS_ID transID,
                                     DPS_LSN_OFFSET lastLsn,
                                     UINT64 commitTime,
                                     DPS_LSN_OFFSET &curLsn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT__COMMITTRANS ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      SDB_DPSCB *pDpsCB = pmdGetKRCB()->getDPSCB() ;
      UINT8 attr = DPS_TS_COMMIT_ATTR_SND ;

      DPS_LSN_OFFSET firstLsn = DPS_INVALID_LSN_OFFSET ;

      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      dpsRecordTransInfo transInfo ;
      transInfo._transID = transID ;
      transInfo._preTransLSN = lastLsn ;
      transInfo._commitTime = commitTime ;

      cb->setTransID( transID ) ;
      cb->setCurTransLsn( lastLsn ) ;

      firstLsn = _transCB->getBeginLsn( transID ) ;
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
      info.setTransTime( commitTime ) ;
      rc = pDpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                   "log(rc=%d)", rc ) ;
      pDpsCB->writeData( info ) ;

      curLsn = cb->getCurTransLsn() ;

      // make sure to commit meta-block statistics
      // NOTE: actually it is empty
      cb->getTransExecutor()->commitMBStats( commitTime ) ;

      cb->resetTransID() ;
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      // release all transactions lock
      _transCB->transLockReleaseAll( cb ) ;
      // reduce the reservedLogSpace from dps for the transaction
      _transCB->releaseRBLogSpace( cb ) ;

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

      DPS_TRANSID_SN localLowTran = DPS_INVALID_TRANSID_SN ;
      DPS_TRANSID_SN localExpireTran = DPS_INVALID_TRANSID_SN ;
      BSONObj requestObject ;

      UINT32 retryTimes = 0 ;

      // only COORD and DATA need report
      // NOTE: here is DATA node
      if ( SDB_ROLE_DATA != pmdGetDBRole() )
      {
         goto done ;
      }

      // get local lowTran
      rc = _getLocalLowTran( localLowTran, localExpireTran ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get local lowTran, rc: %d", rc ) ;

      // fill lowTran request
      rc = _fillLowTranReq( &request, localLowTran, localExpireTran,
                            requestObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to fill lowTran request, rc: %d",
                   rc ) ;

      while( ( retryTimes ++ ) < CLS_GTS_MAX_RETRY )
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
         else if ( SDB_GLOB_LOWTRAN_UNKNOWN == rc )
         {
            // failed to calculate global lowTran, maybe someone has not
            // reported yet, post warning instead
            PD_LOG( PDWARNING, "Failed to get global transaction ID, someone "
                    "has not reported yet, rc: %d", rc ) ;
            // keep caller quiet
            rc = SDB_OK ;
            goto done ;
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
            DPS_TRANSID_SN globExpireTran = DPS_INVALID_TRANSID_SN ;

            rc = _parseLowTranRsp( response, globLowTran, globExpireTran ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse lowTran response, "
                         "rc: %d", rc ) ;

            _setGlobLowTran( globLowTran, globExpireTran ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_ARBITGLOBTRAN, "_clsGTSAgent::arbitGlobTrans" )
   INT32 _clsGTSAgent::arbitGlobTrans( pmdEDUCB *eduCB,
                                       const DPS_TRANS_ID &readTransID,
                                       const DPS_TRANS_ID &writeTransID,
                                       DPS_TRANS_STATUS writeTransStatus,
                                       BOOLEAN forceLocal,
                                       BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_ARBITGLOBTRAN ) ;

      visible = FALSE ;

      PD_CHECK( readTransID.isValid(), SDB_SYS, error, PDERROR,
                "Failed to arbitrate global transaction, "
                "read transaction is invalid" ) ;

      PD_CHECK( writeTransID.isValid(), SDB_SYS, error, PDERROR,
                "Failed to arbitrate global transaction, "
                "write transaction is invalid" ) ;

      PD_LOG( PDDEBUG, "Start arbitration: read transaction [%s] against "
              "write transaction [%s] with status [%s]",
              dpsTransIDToString( readTransID ).c_str(),
              dpsTransIDToString( writeTransID ).c_str(),
              dpsTransStatusToString( writeTransStatus ) ) ;

      // check if global transactions
      if ( !readTransID.isGlobTrans() )
      {
         PD_LOG( PDDEBUG, "Read transaction [%s] is not global transaction",
                 dpsTransIDToString( readTransID ).c_str() ) ;
         visible = TRUE ;
         goto done ;
      }
      else if ( !writeTransID.isGlobTrans() )
      {
         PD_LOG( PDDEBUG, "Write transaction [%s] is not global transaction",
                 dpsTransIDToString( writeTransID ).c_str() ) ;
         visible = TRUE ;
         goto done ;
      }

      if ( forceLocal ||
           _localRID.columns.nodeID == readTransID.getNodeID() )
      {
         // from local, do arbitrate on local
         rc = _arbitLocal( eduCB, readTransID, writeTransID, writeTransStatus,
                           visible ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to arbitrate transaction on "
                      "local, rc: %d", rc ) ;
      }
      else if ( readTransID.getNodeID() <= SYS_NODE_ID_END )
      {
         // from COORD, do arbitrate on remote node
         rc = _arbitRemote( eduCB, readTransID, writeTransID, writeTransStatus,
                            visible ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to arbitrate transaction on "
                      "remote, rc: %d", rc ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid node ID" ) ;
         PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to arbitrate "
                   "transaction with read transaction [%s], invalid node ID",
                   dpsTransIDToString( readTransID ).c_str() ) ;
      }

      PD_LOG( PDDEBUG, "Finish arbitration: read transaction [%s] against "
              "write transaction [%s] with status [%s], visible [%s]",
              dpsTransIDToString( readTransID ).c_str(),
              dpsTransIDToString( writeTransID ).c_str(),
              dpsTransStatusToString( writeTransStatus ),
              visible ? "TRUE" : "FALSE" ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_ARBITGLOBTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_WAITARBITCOMMIT, "_clsGTSAgent::waitArbitCommit" )
   INT32 _clsGTSAgent::waitArbitCommit( pmdEDUCB *eduCB,
                                        const DPS_TRANS_ID &arbitTransID,
                                        INT32 timeout,
                                        BOOLEAN &committed,
                                        BOOLEAN &multiGroups,
                                        stpLogicalTimeUS &commitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_WAITARBITCOMMIT ) ;

      INT32 waitedTime = 0 ;
      INT32 waitTime = CLS_GTS_WAIT_INTERVAL ;
      BOOLEAN retried = FALSE ;
      dpsTransBackInfo info ;

      // assuming that the transaction is not committed and involved in
      // multiple groups
      committed = FALSE ;
      multiGroups = TRUE ;

   retry:
      // check if EDUCB is interrupted
      PD_CHECK( !eduCB->isInterrupted(), SDB_APP_INTERRUPT, error, PDERROR,
                "Failed to wait commit for transaction [%s], "
                "EDUCB is interrupted",
                dpsTransIDToString( arbitTransID ).c_str() ) ;

      if ( !_transCB->getTransInfo( arbitTransID, info ) )
      {
         // not found, must be removed
         // set commit time to minimum, so running transactions can
         // access changes from this transaction
         commitTime.setTime( DPS_MIN_TRANS_TIME ) ;
         committed = TRUE ;
         goto done ;
      }

      switch ( info._status )
      {
         case DPS_TRANS_DOING :
         case DPS_TRANS_DOING_INTERRUPT :
         {
            // still doing, wait for a while, and retry
            waitTime = CLS_GTS_WAIT_INTERVAL ;
            break ;
         }
         case DPS_TRANS_WAIT_COMMIT :
         {
            // for wait-commit transaction, we first wait a short time
            // to check if it could commit itself
            if ( !retried )
            {
               // wait for a small interval, and retry
               waitTime = CLS_GTS_WAIT_SMALL_INTERVAL ;
            }
            else
            {
               // if still not commit, we need check with involved groups
               // if one of them has committed, we could treat this
               // transaction as committed
               DPS_TRANS_STATUS status = DPS_TRANS_UNKNOWN ;
               DPS_LSN_OFFSET commitLSN = info._lsn ;
               dpsMessageBlock mb ;
               DPS_LOG_TYPE logType = LOG_TYPE_DUMMY ;
               UINT8 attr = 0 ;
               UINT32 nodeNum = 0 ;
               UINT64 tmpPreCommitTime = 0 ;
               UINT64 tmpCommitTime = 0 ;
               const UINT64 *nodes = NULL ;

               // get commit info from DPS log
               rc = _getCommitInfo( info._lsn, &mb, logType, attr, nodeNum,
                                    &nodes ) ;
               if ( SDB_OK != rc &&
                    LOG_TYPE_TS_COMMIT != logType &&
                    LOG_TYPE_DUMMY != logType )
               {
                  // log type is not commit, the pre-commit might be still
                  // processing, wait a while again
                  waitTime = CLS_GTS_WAIT_SMALL_INTERVAL ;
                  rc = SDB_OK ;
                  break ;
               }
               PD_RC_CHECK( rc, PDERROR, "Failed to get commit info with "
                            "LSN [%llu] for transaction [%s], rc: %d",
                            commitLSN,
                            dpsTransIDToString( arbitTransID ).c_str(),
                            rc ) ;

               // mark multiple groups or not
               multiGroups = ( nodeNum > 1 ) ? TRUE : FALSE ;

               if ( DPS_TS_COMMIT_ATTR_PRE != attr )
               {
                  // not pre-commit record, transaction is committed
                  committed = TRUE ;
                  goto done ;
               }

               // check transaction status with involved groups
               rc = _checkTransStatus( arbitTransID, nodeNum, nodes, eduCB,
                                       TRUE, tmpPreCommitTime, status,
                                       tmpCommitTime ) ;
               if ( SDB_OK != rc )
               {
                  // failed, we could wait a while and retry
                  PD_LOG( PDWARNING, "Failed to check status for "
                          "transaction [%s], rc: %d",
                          dpsTransIDToString( arbitTransID ).c_str(), rc ) ;
                  rc = SDB_OK ;
               }
               else if ( DPS_TRANS_COMMIT == status )
               {
                  // it is committed
                  commitTime.setTime( tmpCommitTime ) ;
                  committed = TRUE ;
                  goto done ;
               }
               else if ( DPS_TRANS_ROLLBACK == status )
               {
                  // it is rollbacked
                  committed = FALSE ;
                  goto done ;
               }
               // doing or rollback status, we need to recheck
               waitTime = CLS_GTS_WAIT_MEDIUM_INTERVAL ;
            }
            break ;
         }
         case DPS_TRANS_COMMIT :
         {
            // it is commit
            commitTime.setTime( info._commitTime.getTime() ) ;
            committed = TRUE ;
            goto done ;
         }
         case DPS_TRANS_ROLLBACK :
         {
            // it is rollback
            PD_LOG( PDDEBUG, "Transaction [%s] is rollbacked",
                    dpsTransIDToString( arbitTransID ).c_str() ) ;
            committed = FALSE ;
            goto done ;
         }
         case DPS_TRANS_UNKNOWN :
         {
            // unknown means it is removed by lowTran
            // treat as commit
            commitTime.setTime( DPS_MIN_TRANS_TIME ) ;
            committed = TRUE ;
            goto done ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "Unknown transaction status, "
                        "should not go here" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      // check timeout
      PD_CHECK( timeout < 0 || timeout >= waitedTime,
                SDB_TIMEOUT, error, PDWARNING,
                "Failed to wait transaction [%s] to commit, "
                "timeout [%d], waited [%d]",
                dpsTransIDToString( arbitTransID ).c_str(), timeout,
                waitedTime ) ;

      ossSleep( waitTime ) ;
      waitedTime += waitTime ;

      retried = TRUE ;

      goto retry ;

   done:
#if defined (_DEBUG)
      if ( SDB_OK == rc )
      {
         PD_LOG( PDDEBUG, "Wait transaction [%s] commit done, committed [%s], "
                 "multi-groups [%s], commit time [%s]",
                 dpsTransIDToString( arbitTransID ).c_str(),
                 committed ? "TRUE" : "FALSE",
                 multiGroups ? "TRUE" : "FALSE",
                 dpsTransTimeToString( commitTime ).c_str() ) ;
      }
#endif
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_WAITARBITCOMMIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_WAITARBITCHANGE, "_clsGTSAgent::waitArbitChange" )
   INT32 _clsGTSAgent::waitArbitChange( pmdEDUCB *eduCB,
                                        const DPS_TRANS_ID &arbitTransID,
                                        DPS_TRANS_STATUS currentStatus,
                                        INT32 timeout,
                                        dpsTransBackInfo &newInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_WAITARBITCHANGE ) ;

      INT32 waitedTime = 0 ;
      UINT32 retryCount = 0 ;

   retry:
      // check if EDUCB is interrupted
      PD_CHECK( !eduCB->isInterrupted(), SDB_APP_INTERRUPT, error, PDERROR,
                "Failed to wait status change for transaction [%s], "
                "EDUCB is interrupted",
                dpsTransIDToString( arbitTransID ).c_str() ) ;

      if ( !_transCB->getTransInfo( arbitTransID, newInfo ) )
      {
         // not found, must be removed
         // NOTE: the history transaction map will be cleared by
         // global expired transaction ID
         goto done ;
      }

      // check if status is changed
      // NOTE: we only care status is changed, we don't care about what status
      //       we will wait for
      if ( newInfo._status == currentStatus )
      {
         INT32 waitTime = CLS_GTS_WAIT_MEDIUM_INTERVAL ;

         // check timeout
         PD_CHECK( timeout < 0 || timeout >= waitedTime,
                   SDB_TIMEOUT, error, PDWARNING,
                   "Failed to wait transaction [%s] status change, "
                   "timeout [%d], waited [%d], status [%s]",
                   dpsTransIDToString( arbitTransID ).c_str(), timeout,
                   waitedTime, dpsTransStatusToString( currentStatus ) ) ;

         // increase wait time if already waited for a while
         if ( retryCount < CLS_GTS_WAIT_SMALL_COUNT )
         {
            waitTime = (INT32)(
                  STP_NANOSEC_TO_MILLISEC( eduCB->getTransTimeError() ) ) ;
         }
         else if ( retryCount < 2 * CLS_GTS_WAIT_SMALL_COUNT )
         {
            waitTime = CLS_GTS_WAIT_SMALL_INTERVAL ;
         }

         ossSleep( waitTime ) ;
         waitedTime += waitTime ;
         ++ retryCount ;
         goto retry ;
      }

   done:
#if defined (_DEBUG)
      if ( SDB_OK == rc )
      {
         PD_LOG( PDDEBUG, "Wait transaction [%s] status change done, "
                 "old status [%s], new status [%s]",
                 dpsTransIDToString( arbitTransID ).c_str(),
                 dpsTransStatusToString( currentStatus ),
                 dpsTransStatusToString( newInfo._status ) ) ;
      }
#endif
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_WAITARBITCHANGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_ARBITREMOTE, "_clsGTSAgent::_arbitRemote" )
   INT32 _clsGTSAgent::_arbitRemote( pmdEDUCB *eduCB,
                                     const DPS_TRANS_ID &readTransID,
                                     const DPS_TRANS_ID &writeTransID,
                                     DPS_TRANS_STATUS writeTransStatus,
                                     BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_ARBITREMOTE ) ;

      MsgRouteID routeID ;
      MsgClsGTSArbitReq request ;
      MsgHeader *replyMessage = NULL ;
      MsgClsGTSArbitRsp *response = NULL ;

      // check if we have arbitrated this write transaction before
      if ( eduCB->getTransExecutor()->findArbit( writeTransID,
                                                 visible ) )
      {
         PD_LOG( PDDEBUG, "Write transaction [%s] already done "
                 "arbitration" ) ;
         goto done ;
      }

      // send to node of read transaction
      // should be a COORD
      routeID.columns.groupID = COORD_GROUPID ;
      routeID.columns.nodeID = readTransID.getNodeID() ;
      routeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;

      // fill request
      rc = _fillGTSArbitReq( &request,
                             readTransID,
                             writeTransID,
                             writeTransStatus ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to fill arbitrate request, rc: %d",
                   rc ) ;

      // set request
      rc = _pShardMgr->syncSend( (MsgHeader *)( &request ),
                                 routeID,
                                 &replyMessage,
                                 CLS_SHARD_TIMEOUT,
                                 NULL,
                                 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send arbitrate request to "
                   "route ID %s, rc: %d", routeID2String( routeID ).c_str(),
                   rc ) ;
      PD_CHECK( MSG_CLS_GTS_ARBIT_RSP == replyMessage->opCode,
                SDB_SYS, error, PDERROR,
                "Failed to receive reply from route ID %s, message type "
                "is not matched, given [%d], expected [%d]",
                routeID2String( routeID ).c_str(), replyMessage->opCode,
                MSG_CLS_GTS_ARBIT_RSP ) ;
      PD_CHECK( sizeof( MsgClsGTSArbitRsp ) == replyMessage->messageLength,
                SDB_SYS, error, PDERROR,
                "Failed to receive reply from route ID %s, message length "
                "is not matched, given [%d], expected [%d]",
                routeID2String( routeID ).c_str(), replyMessage->messageLength,
                sizeof( MsgClsGTSArbitRsp ) ) ;

      // check return code
      response = (MsgClsGTSArbitRsp *)replyMessage ;
      rc = response->header.res ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do arbitrate on route ID %s, "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

      // parse response
      rc = _parseGTSArbitRsp( response, visible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse arbitrate response, rc: %d",
                   rc ) ;

      // save arbitration results
      rc = eduCB->getTransExecutor()->saveArbit( writeTransID,
                                                 writeTransStatus,
                                                 visible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save arbitrate record, rc: %d",
                   rc ) ;

   done:
      SAFE_OSS_FREE( replyMessage ) ;

      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_ARBITREMOTE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_ARBITLOCAL, "_clsGTSAgent::_arbitLocal" )
   INT32 _clsGTSAgent::_arbitLocal( pmdEDUCB *eduCB,
                                    const DPS_TRANS_ID &readTransID,
                                    const DPS_TRANS_ID &writeTransID,
                                    DPS_TRANS_STATUS writeTransStatus,
                                    BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_ARBITLOCAL ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;

      // arbitrate with local transCB
      rc = eduCB->getTransExecutor()->arbit( writeTransID,
                                             writeTransStatus,
                                             visible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to arbitrate transaction, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSGTSAGENT_ARBITLOCAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_GETNODETIMEERROR, "_clsGTSAgent::getNodeTimeError" )
   UINT32 _clsGTSAgent::getNodeTimeError()
   {
      UINT32 timeError = 0 ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_GETNODETIMEERROR ) ;

      ossScopedLock lock( &_timeErrorLatch, SHARED ) ;
      timeError = _nodeTimeError ;

      PD_TRACE_EXIT( SDB__CLSGTSAGENT_GETNODETIMEERROR ) ;

      return timeError ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_INCNODETIMEERROR, "_clsGTSAgent::incNodeTimeError" )
   void _clsGTSAgent::incNodeTimeError( UINT32 currentTimeError )
   {
      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_INCNODETIMEERROR ) ;

      ossScopedLock lock( &_timeErrorLatch, EXCLUSIVE ) ;

      // calculate target time error by 1.1 x current time error
      UINT32 targetTimeError = OSS_MIN(
            (UINT32)( (double)currentTimeError * CLS_GTS_INC_TIME_ERROR_STEP ),
            _maxNodeTimeError ) ;

      // target time error is larger than node time error
      // increase node time error
      if ( targetTimeError > _nodeTimeError )
      {
         UINT32 oldTimeError = _nodeTimeError ;

         _nodeTimeError = targetTimeError ;

         PD_LOG( PDDEBUG, "Increase node time error from [%u] to [%u] by "
                 "current time error [%u]",
                 oldTimeError, _nodeTimeError, currentTimeError ) ;
      }

      _decTimeErrorCount = 0 ;

      PD_TRACE_EXIT( SDB__CLSGTSAGENT_INCNODETIMEERROR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_DECNODETIMEERROR, "_clsGTSAgent::decNodeTimeError" )
   void _clsGTSAgent::decNodeTimeError( UINT32 currentTimeError )
   {
      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_DECNODETIMEERROR ) ;

      ossScopedLock lock( &_timeErrorLatch, EXCLUSIVE ) ;

      // target time error is 0.9 x node time error
      UINT32 targetTimeError = OSS_MAX(
            (UINT32)( (double)_nodeTimeError * CLS_GTS_DEC_TIME_ERROR_STEP ),
            STP_MIN_TIME_ERROR ) ;

      // current time error is smaller than target time error
      // in this case, we could consider decrease the node time error
      if ( ( currentTimeError < targetTimeError ) &&
           ( ++ _decTimeErrorCount > CLS_GTS_DEC_TIME_ERROR_COUNT ) )
      {
         UINT32 oldTimeError = _nodeTimeError ;

         _nodeTimeError = targetTimeError ;
         _decTimeErrorCount = 0 ;

         PD_LOG( PDDEBUG, "Decrease node time error from [%u] to [%u] by "
                 "current time error [%u]",
                 oldTimeError, _nodeTimeError, currentTimeError ) ;
      }

      PD_TRACE_EXIT( SDB__CLSGTSAGENT_DECNODETIMEERROR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_RESETNODETIMEERROR, "_clsGTSAgent::resetNodeTimeError" )
   void _clsGTSAgent::resetNodeTimeError()
   {
      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_RESETNODETIMEERROR ) ;

      ossScopedLock lock( &_timeErrorLatch, EXCLUSIVE ) ;
      _nodeTimeError = STP_MIN_TIME_ERROR ;
      _decTimeErrorCount = 0 ;

      PD_TRACE_EXIT( SDB__CLSGTSAGENT_RESETNODETIMEERROR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGTSAGENT_GETACCEPTTIMEERROR, "_clsGTSAgent::getAcceptTimeError" )
   UINT32 _clsGTSAgent::getAcceptTimeError( const stpLogicalTimeUS &remoteTime,
                                            const stpLogicalTimeUS &localTime )
   {
      UINT64 timeError = 0LL ;

      PD_TRACE_ENTRY( SDB__CLSGTSAGENT_GETACCEPTTIMEERROR ) ;

      if ( remoteTime.getTime() > localTime.getTime() )
      {
         timeError = remoteTime.getTime() - localTime.getTime() ;
      }
      else if ( remoteTime.getTime() < localTime.getTime() )
      {
         timeError = localTime.getTime() - remoteTime.getTime() ;
      }

      // round to node maximum time error in nanosecond
      timeError = (UINT32)( OSS_MIN( STP_MICROSEC_TO_NANOSEC( timeError ),
                            _maxNodeTimeError ) ) ;

      PD_TRACE_EXIT( SDB__CLSGTSAGENT_GETACCEPTTIMEERROR ) ;

      return timeError ;
   }

}
