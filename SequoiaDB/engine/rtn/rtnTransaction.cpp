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

   Source File Name = rtnTransaction.cpp

   Descriptive Name = Runtime Transaction

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   runtime transaction management for data node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtn.hpp"
#include "pmdCB.hpp"
#include "dpsMessageBlock.hpp"
#include "clsReplayer.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsLogRecordDef.hpp"
#include "dpsOp2Record.hpp"
#include "dpsUtil.hpp"

namespace engine
{

   /// local define
   #define RTN_TRANS_ROLLBACK_RETRY_TIMES             ( 20 )
   #define RTN_TRANS_ROLLBACK_RETRY_INTERVAL          OSS_ONE_SEC

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSAVETRANSINFO, "_rtnSaveTransInfo" )
   static INT32 _rtnSaveTransInfo( pmdEDUCB *cb,
                                   DPS_TRANS_ID transID,
                                   DPS_LSN_OFFSET curLsnOffset )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSAVETRANSINFO ) ;

      // loop until we save the info successfully
      while ( !PMD_IS_DB_DOWN() && !cb->isForced() )
      {
         rc = sdbGetTransCB()->addTransInfo( transID, curLsnOffset,
                                             cb->getTransStatus() ) ;
         if ( SDB_OK != rc )
         {
            // report error
            sdbGetTransCB()->incErrCount() ;
            PD_LOG( PDERROR, "Failed to add transaction information [%s], "
                    "rc: %d", dpsTransIDToString( transID ).c_str(), rc ) ;
            ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
            continue ;
         }
         break ;
      }

      PD_TRACE_EXITRC( SDB__RTNSAVETRANSINFO, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRANSCHKRECORD, "_rtnTransCheckRecord" )
   static INT32 _rtnTransCheckRecord( const DPS_TRANS_ID &transID,
                                      DPS_LSN_OFFSET lsnOffset,
                                      const dpsMessageBlock &mb,
                                      dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNTRANSCHKRECORD ) ;

      DPS_TRANS_ID curTransID ;
      dpsLogRecord::iterator itr ;

      rc = record.load( mb.offset( 0 ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse log [LSN: %llu], rc: %d",
                   lsnOffset, rc ) ;

      rc = dpsGetTransIDFromRecord( record, curTransID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get transaction ID from "
                   "record [LSN: %llu]", lsnOffset ) ;

      curTransID = sdbGetTransCB()->getTransID( curTransID ) ;
      PD_CHECK( curTransID == transID.getOrigTransID(),
                SDB_DPS_CORRUPTED_LOG, error,
                PDERROR, "Failed to rollback(lsn=%llu, Log TransID:%s, "
                "Session TransID:%s), the log is damaged",
                lsnOffset, dpsTransIDToString( curTransID ).c_str(),
                dpsTransIDToString( transID.getOrigTransID() ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNTRANSCHKRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSBEGIN, "rtnTransBegin" )
   INT32 rtnTransBegin( _pmdEDUCB * cb,
                        BOOLEAN isAutoCommit,
                        BOOLEAN isGlobTrans,
                        const DPS_TRANS_ID &specID,
                        const stpLogicalTimeUS &specBeginTime )
   {
      PD_TRACE_ENTRY ( SDB_RTNTRANSBEGIN ) ;
      SDB_ASSERT( cb, "cb can't be null" ) ;
      INT32         rc          = SDB_OK ;
      dpsTransCB *  transCB     = sdbGetTransCB() ;
      pmdTransExecutor *transExecutor = cb->getTransExecutor() ;
      DPS_TRANS_ID  transID     = cb->getTransID() ;
      BOOLEAN       mvccOn      = pmdGetKRCB()->getOptionCB()->mvccOn() ;
      BOOLEAN       globTransOn = ( cb->isGlobTransOn() &&
                                    transCB->isGlobTransOn() ) ;

      SDB_ASSERT( NULL != transExecutor, "transaction executor is invalid" ) ;

      // transaction should be on
      if ( !transCB->isTransOn() )
      {
         rc = SDB_DPS_TRANS_DIABLED ;
         goto error;
      }

      // check global transaction
      // if global transaction is required, but global transaction on session
      // or node is not enabled, report error, also global transaction requires
      // MVCC
      if ( ( isGlobTrans ||
             ( specID.isValid() && specID.isGlobTrans() ) ) )
      {
         if ( !mvccOn )
         {
            PD_LOG_MSG( PDERROR, "Failed to begin global transaction, which is "
                        "only supported when mvccon is true" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( !globTransOn )
         {
            PD_LOG_MSG( PDERROR, "Failed to begin global transaction, global "
                        "transaction support ( globtranson ) is not enabled" ) ;
            rc = SDB_GLOB_TRANS_NOT_AVAILABLE ;
            goto error ;
         }
      }

      // RR isolation requires MVCC is on and global transaction is on
      if ( TRANS_ISOLATION_RR == transExecutor->getTransIsolation() )
      {
         if ( !mvccOn )
         {
            PD_LOG_MSG( PDERROR, "Failed to begin transaction, RR isolation is "
                        "only supported when mvccon is true" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( !globTransOn )
         {
            PD_LOG_MSG( PDERROR, "Failed to begin transaction, RR isolation is "
                        "only supported when global transaction is enabled" ) ;
            rc = SDB_GLOB_TRANS_NOT_AVAILABLE ;
            goto error ;
         }
      }

      // check if current EDU is already handling transaction
      // if not, we could start a transaction with current EDU
      // if EDU is already in transaction, just ignore new one
      if ( transID.isInvalid() )
      {
         DPS_TRANS_ID tempID ;
         stpLogicalTimeUS beginTime ;

      retry:
         if ( specID.isValid() )
         {
            // if specified transaction ID is global, we need to check
            // against primary active time
            if ( specID.isGlobTrans() )
            {
               rc = transCB->checkGlobTrans( specID, specBeginTime ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check global transaction "
                            "%s, rc: %d", dpsTransIDToString( specID ).c_str(),
                            rc ) ;
            }
            // transaction ID is given, use given time as begin time
            // and set first operator tag
            tempID = specID ;
            tempID.setFirstOp() ;
            beginTime = specBeginTime ;
         }
         else
         {
            // transaction ID is not given, allocate a new one
            rc = transCB->allocTransID( isAutoCommit,
                                        isGlobTrans,
                                        cb->getTransTimeout(),
                                        tempID,
                                        beginTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate transaction ID, "
                         "rc: %d", rc ) ;
         }

         // set global transaction tag if needed
         if ( tempID.isGlobTrans() )
         {
            cb->setGlobTrans( tempID, beginTime ) ;
         }
         else
         {
            cb->setTransID( tempID ) ;
         }

         cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
         // refresh local transID after set
         transID = cb->getTransID() ;

         if ( !transCB->addTransCB( transID, cb ) )
         {
            // found duplicated transaction ID
            if ( transID.isGlobTrans() && specID.isInvalid() )
            {
               // for global transaction and transaction ID is generated by
               // this node, we could retry to get a new logical time
               transCB->incTransIDConflict() ;
               cb->resetTransID() ;
               goto retry ;
            }
            else
            {
               PD_LOG( PDERROR, "Transaction(%s) is already exist",
                       dpsTransIDToString( transID ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }

      PD_LOG( PDINFO, "Begin transaction operations(ID:%s, IDAttr:%s)",
              dpsTransIDToString( transID ).c_str(),
              dpsTransIDAttrToString( transID ).c_str() ) ;


   done:
      PD_TRACE_EXIT ( SDB_RTNTRANSBEGIN ) ;
      return rc;
   error:
      // report error
      sdbGetTransCB()->incGlobErrCount( rc ) ;
      goto done ;
   }

   INT32 rtnTransPreCommit( _pmdEDUCB *cb,
                            UINT32 nodeNum,
                            const UINT64 *pNodes,
                            const stpLogicalTimeUS &preCommitTime,
                            INT16 w,
                            SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      dpsTransCB *transCB = sdbGetTransCB() ;
      DPS_LSN_OFFSET firstTransLsn = DPS_INVALID_LSN_OFFSET ;
      UINT8 attr = DPS_TS_COMMIT_ATTR_PRE ;

      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      dpsRecordTransInfo transInfo( cb->getTransID(),
                                    cb->getCurTransLsn(),
                                    DPS_INVALID_LSN_OFFSET,
                                    cb->getTransBeginTime(),
                                    preCommitTime,
                                    cb->getTransCommitTime() ) ;

      if ( transInfo._transID.isInvalid() ||
           DPS_INVALID_LSN_OFFSET == transInfo._preTransLSN )
      {
         goto done ;
      }

      if ( !dpsCB )
      {
         goto done ;
      }

      firstTransLsn = transCB->getBeginLsn( transInfo._transID ) ;
      SDB_ASSERT( firstTransLsn != DPS_INVALID_LSN_OFFSET,
                  "First transaction lsn can't be invalid" ) ;

      PD_LOG( PDINFO, "Execute pre-commit(ID:%s, LastLsn=%llu)",
              dpsTransIDToString( transInfo._transID ).c_str(),
              transInfo._preTransLSN ) ;

      rc = dpsTransCommit2Record( transInfo, firstTransLsn,
                                  attr, &nodeNum, pNodes, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build pre-commit log:%d",rc ) ;
         goto error ;
      }

      info.setInfoEx( ~0, DMS_INVALID_CLID, DMS_INVALID_EXTENT, cb ) ;
      info.enableTrans() ;
      rc = dpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                   "log(rc=%d)", rc ) ;
      dpsCB->writeData( info ) ;

      // set transaction pre-commit time
      cb->setTransPreCommitTime( preCommitTime ) ;

      // update transaction status to wait-commit
      cb->setTransStatus( DPS_TRANS_WAIT_COMMIT ) ;

   done:
      if ( cb )
      {
         if ( SDB_OK == rc && dpsCB )
         {
            rc = dpsCB->completeOpr( cb, w ) ;
         }
      }
      return rc ;
   error:
      // report error
      sdbGetTransCB()->incGlobErrCount( rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSCOMMIT, "rtnTransCommit" )
   INT32 rtnTransCommit( _pmdEDUCB *cb,
                         SDB_DPSCB *dpsCB,
                         const stpLogicalTimeUS &specCommitTime )
   {
      PD_TRACE_ENTRY ( SDB_RTNTRANSCOMMIT ) ;
      SDB_ASSERT( cb, "cb can't be null" ) ;
      INT32 rc = SDB_OK ;
      UINT8 attr = 0 ;

      IRemoteOperator *pRemoteOperator = NULL ;

      dpsTransCB *transCB = sdbGetTransCB() ;
      pmdTransExecutor *transExecutor = cb->getTransExecutor() ;
      DPS_LSN_OFFSET firstTransLsn = DPS_INVALID_LSN_OFFSET ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      pRemoteOperator = cb->getRemoteOperator() ;
      if ( NULL != pRemoteOperator )
      {
         rc = pRemoteOperator->transCommit() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to commit remote operator:rc=%d", rc ) ;
            rc = SDB_OK ;
         }
      }

      stpLogicalTimeUS commitTime = specCommitTime ;
      dpsRecordTransInfo transInfo( cb->getTransID(),
                                    cb->getCurTransLsn(),
                                    DPS_INVALID_LSN_OFFSET,
                                    cb->getTransBeginTime(),
                                    cb->getTransPreCommitTime(),
                                    commitTime ) ;

      SDB_ASSERT( NULL != transExecutor, "transaction executor is invalid" ) ;

      if ( transInfo._transID.isInvalid() ||
           DPS_INVALID_LSN_OFFSET == transInfo._preTransLSN )
      {
         cb->setTransStatus( DPS_TRANS_COMMIT ) ;

         // make sure to commit meta-block statistics
         // NOTE: actually it is empty
         transExecutor->commitMBStats( 0 ) ;

         // remove from transaction CB map
         transCB->delTransCB( transInfo._transID ) ;

         // clear records for transaction arbitration
         transExecutor->clearArbit() ;

         // reset transaction ID
         cb->resetTransID() ;

         // release all transactions lock
         transCB->transLockReleaseAll( cb ) ;
         // reduce the reservedLogSpace from dps for the transaction
         transCB->releaseRBLogSpace( cb ) ;
         goto done ;
      }

      if ( !dpsCB )
      {
         goto done ;
      }

      if ( DPS_TRANS_WAIT_COMMIT == cb->getTransStatus() )
      {
         attr = DPS_TS_COMMIT_ATTR_SND ;
      }
      else if ( cb->isGlobTrans() &&
                cb->isAutoCommitTrans() )
      {
         // generate pre-commit time for auto-commit transaction
         stpLogicalTimeUS localTime, preCommitTime ;

         // update status first
         rc = transCB->updateTransStatus( transInfo._transID,
                                          DPS_TRANS_PRE_WAIT_COMMIT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update status to [%s] for "
                      "transaction [%s], rc: %d",
                      dpsTransStatusToString( DPS_TRANS_PRE_WAIT_COMMIT ),
                      dpsTransIDToString( transInfo._transID ).c_str(),
                      rc ) ;

         // get pre-commit time
         rc = transCB->getGlobPreCommitTime( cb, localTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get global logical time for "
                      "pre-commit of transaction [%s], rc: %d",
                      dpsTransIDToString( transInfo._transID ).c_str(),
                      rc ) ;

         // check if we need to delay pre-commit time
         rc = transCB->getLocalPreCommitTime( localTime, preCommitTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get local logical time for "
                      "pre-commit of transaction [%s], rc: %d",
                      dpsTransIDToString( transInfo._transID ).c_str(),
                      rc ) ;

         cb->setTransPreCommitTime( preCommitTime ) ;

         commitTime.reset() ;
      }

      // need get commit time if needed
      if ( cb->isGlobTrans() && !commitTime.isValid() )
      {
         transCB->getGlobCommitTime( cb, commitTime ) ;
      }

      // update commit time
      transInfo._commitTime = commitTime.getTime() ;

      firstTransLsn = transCB->getBeginLsn( transInfo._transID ) ;
      SDB_ASSERT( firstTransLsn != DPS_INVALID_LSN_OFFSET,
                  "First transaction lsn can't be invalid" ) ;

      PD_LOG( PDINFO, "Execute commit(ID:%s, LastLsn=%llu)",
              dpsTransIDToString( transInfo._transID ).c_str(),
              transInfo._preTransLSN ) ;

      rc = dpsTransCommit2Record( transInfo, firstTransLsn,
                                  attr, NULL, NULL, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build commit log:%d",rc ) ;
         goto error ;
      }

      info.setInfoEx( ~0, DMS_INVALID_CLID, DMS_INVALID_EXTENT, cb ) ;
      info.enableTrans() ;
      info.setTransTime( commitTime.getTime() ) ;
      rc = dpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                   "log(rc=%d)", rc ) ;
      dpsCB->writeData( info ) ;

      // set transaction commit time
      cb->setTransCommitTime( commitTime ) ;

      cb->setTransStatus( DPS_TRANS_COMMIT ) ;

      // commit meta-block statistics
      // SHOULD commit this before release TX locks
      // the mbstat can be protected by TX locks
      transExecutor->commitMBStats(
            commitTime.isValid() ? commitTime.getTime() : 0 ) ;

      // remove from transaction CB map
      sdbGetTransCB()->delTransCB( transInfo._transID ) ;

      // clear records for transaction arbitration
      transExecutor->clearArbit() ;

      // reset transaction ID
      cb->resetTransID() ;

      // reset transaction LSN
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      // release all transactions lock
      transCB->transLockReleaseAll( cb ) ;

      // reduce the reservedLogSpace from dps for the transaction
      transCB->releaseRBLogSpace( cb ) ;

      // report succeed
      sdbGetTransCB()->incSucCount() ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNTRANSCOMMIT, rc ) ;
      return rc ;
   error:
      // report error
      sdbGetTransCB()->incErrCount() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSROLLBACK, "rtnTransRollback" )
   INT32 rtnTransRollback( _pmdEDUCB * cb, SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY ( SDB_RTNTRANSROLLBACK ) ;
      SDB_ASSERT( cb, "cb can't be null" ) ;
      INT32 rc = SDB_OK;
      IRemoteOperator *pRemoteOperator = NULL ;
      dpsTransCB *transCB = sdbGetTransCB() ;
      pmdTransExecutor *transExecutor = cb->getTransExecutor() ;
      _dpsMessageBlock mb( DPS_MSG_BLOCK_DEF_LEN );
      DPS_LSN dpsLsn ;
      DPS_LSN_OFFSET curLsnOffset = DPS_INVALID_LSN_OFFSET ;
      DPS_TRANS_ID transID ;
      DPS_TRANS_ID rollbackID ;
      UINT32 retryTimes = 0 ;
      BOOLEAN doRollback = FALSE ;
      _clsReplayer replayer( TRUE ) ;
      MAP_TRANS_PENDING_OBJ mapPendingObj ;

      SDB_ASSERT( NULL != transExecutor, "transaction executor is invalid" ) ;

      cb->startTransRollback() ;

      curLsnOffset = cb->getCurTransLsn() ;
      transID = cb->getTransID() ;
      rollbackID = transCB->getRollbackID( transID ) ;

      pRemoteOperator = cb->getRemoteOperator() ;
      if ( NULL != pRemoteOperator )
      {
         rc = pRemoteOperator->transRollback() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to commit remote operator:rc=%d", rc ) ;
            rc = SDB_OK ;
         }
      }

      if ( transID.isInvalid() ||
           DPS_INVALID_LSN_OFFSET == curLsnOffset )
      {
         goto done;
      }
      if ( !dpsCB )
      {
         goto done ;
      }

      PD_LOG ( PDEVENT, "Begin to rollback transaction[ID:%s, "
               "lastLsn:%llu]...", dpsTransIDToString( transID ).c_str(),
               curLsnOffset ) ;
      doRollback = TRUE ;

      cb->setTransID( rollbackID ) ;
      cb->setTransStatus( DPS_TRANS_ROLLBACK ) ;

      // read the log and rollback one by one
      while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )
      {
         dpsLogRecord record ;

         // in cluster mode, when not primary, need add trans info to map
         if ( pmdGetKRCB()->isCBValue( SDB_CB_CLS ) && !pmdIsPrimary() )
         {
            rc = _rtnSaveTransInfo( cb, transID, curLsnOffset ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save transaction info, "
                         "rc: %d", rc ) ;
            rc = SDB_CLS_NOT_PRIMARY ;
            goto error ;
         }

         mb.clear() ;
         dpsLsn.offset = curLsnOffset ;
         rc = dpsCB->search( dpsLsn, &mb ) ;
         if ( SDB_OK != rc )
         {
            // report error
            sdbGetTransCB()->incErrCount() ;
            PD_LOG( PDERROR, "Failed to get log LSN [%llu], rc: %d",
                    curLsnOffset, rc ) ;
            // retry after a while
            ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
            continue ;
         }

         // parse the record
         // WARNING: should not be failed
         rc = _rtnTransCheckRecord( transID, curLsnOffset, mb, record ) ;
         SDB_ASSERT( SDB_OK == rc, "DPS record is invalid for rollback" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check record [LSN: %llu] for "
                      "transaction [%s], rc: %d", curLsnOffset,
                      dpsTransIDToString( transID ).c_str(), rc ) ;

         {
            cb->setRelatedTransLSN( curLsnOffset ) ;
            dpsLogRecord::iterator tmpitr =
               record.find(DPS_LOG_PUBLIC_PRETRANS ) ;
            if ( !tmpitr.valid() )
            {
               /// it is the first log.
               curLsnOffset = DPS_INVALID_LSN_OFFSET ;
            }
            else
            {
               curLsnOffset = *(( DPS_LSN_OFFSET *)tmpitr.value());
            }
            cb->setCurTransLsn( curLsnOffset ) ;

            /// when rollback failed, need to retry some times.
            /// But all the way do it failed, need to restart the db
            rc = replayer.rollbackTrans( ( dpsLogRecordHeader *)mb.offset(0),
                                         cb, mapPendingObj ) ;
            if ( rc )
            {
               ++retryTimes ;
               PD_LOG( PDERROR, "Rollback transaction[ID:%s, lsn=%llu, "
                       "time=%u] failed, rc: %d",
                       dpsTransIDToString( transID ).c_str(),
                       dpsLsn.offset,
                       retryTimes, rc ) ;
               if ( retryTimes >= RTN_TRANS_ROLLBACK_RETRY_TIMES )
               {
                  PD_LOG( PDSEVERE, "Rollback transaction failed, need to "
                          "restart the system" ) ;
                  PMD_RESTART_DB( rc ) ;
                  goto error ;
               }
               ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
               /// set the current lsn to last lsn
               curLsnOffset = cb->getRelatedTransLSN() ;
            }
            else
            {
               retryTimes = 0 ;
            }
         }
      }

      // report succeed
      sdbGetTransCB()->incSucCount() ;

   done:
      // complete the transaction whether success or not,
      // this avoid infinite recursion when rollback failed

      // rollback meta-block statistics
      // if transaction has collection locks, no old version will be generated,
      // rollback will not put back old transaction ID to records, so any
      // old versions generated by commits transactions related to this
      // rolling back transaction may be lost
      transExecutor->rollbackMBStats( transCB->getMaxTransCommitTime() ) ;

      // remove from transaction CB map
      transCB->delTransCB( transID ) ;

      // clear records for transaction arbitration
      transExecutor->clearArbit() ;

      // reset transaction ID
      cb->resetTransID() ;

      // reset transaction LSN
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      cb->setRelatedTransLSN( DPS_INVALID_LSN_OFFSET ) ;
      transCB->transLockReleaseAll( cb ) ;

      // reduce the reservedLogSpace from dps for the transaction
      transCB->releaseRBLogSpace( cb ) ;

      cb->stopTransRollback() ;

      // should not have pending object if rollback succeed
      if ( !mapPendingObj.empty() && SDB_OK == rc )
      {
         SDB_ASSERT( FALSE, "Transaction's pending object map is "
                     "not empty" ) ;
         PD_LOG( PDERROR, "Transaction(%s)'s pending object map"
                 " is not empty(size:%d)",
                 dpsTransIDToString( transID ).c_str(),
                 mapPendingObj.size() ) ;
      }

      if ( doRollback )
      {
         PD_LOG ( PDEVENT, "Rollback transaction(ID:%s, IDAttr:%s) finished "
                  "with rc[%d]", dpsTransIDToString( transID ).c_str(),
                  dpsTransIDAttrToString( transID ).c_str(),
                  rc ) ;
      }

#if defined ( _DEBUG )
      // only for debug, wait the group other node sync complete
      if ( dpsCB && SDB_ROLE_CATALOG != pmdGetDBRole() )
      {
         dpsCB->completeOpr( cb, CLS_REPLSET_MAX_NODE_SIZE ) ;
      }
#endif // _DEBUG

      PD_TRACE_EXITRC ( SDB_RTNTRANSROLLBACK, rc ) ;
      return rc ;
   error:
      // report error
      sdbGetTransCB()->incErrCount() ;
      goto done ;
   }

   INT32 rtnTransRollbackAll( _pmdEDUCB * cb,
                              UINT64 doRollbackID )
   {
      INT32 rc = SDB_OK;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      SDB_DPSCB *pDpsCB = sdbGetDPSCB() ;
      TRANS_DUMP_MAP tmpTransMap ;
      DPS_LSN dpsLsn;
      DPS_TRANS_ID transID ;
      DPS_TRANS_ID rollbackID ;
      DPS_LSN_OFFSET curLsnOffset = DPS_INVALID_LSN_OFFSET ;
      UINT32 retryTimes = 0 ;
      _clsReplayer replayer( TRUE );
      _dpsMessageBlock mb( DPS_MSG_BLOCK_DEF_LEN ) ;

      pTransCB->cloneTransMap( tmpTransMap ) ;
      cb->startTransRollback( TRUE ) ;

      PD_LOG ( PDEVENT, "Begin to rollback all unfinished transactions "
               "[num of trans: %d, rollback ID: %u]...",
               tmpTransMap.size(), doRollbackID ) ;

      while ( tmpTransMap.size() != 0 )
      {
         TRANS_DUMP_MAP::iterator iterMap = tmpTransMap.begin();
         dpsTransBackInfo &transInfo = iterMap->second ;
         MAP_TRANS_PENDING_OBJ mapPendingObj ;
         transID = iterMap->first ;
         rollbackID = pTransCB->getRollbackID( transID ) ;
         curLsnOffset = transInfo._lsn ;
         cb->setTransID( rollbackID ) ;

         PD_LOG( PDEVENT, "Begin to rollback transaction[ID:%s, "
                 "LastLSN: %llu]...", dpsTransIDToString( transID ).c_str(),
                 curLsnOffset ) ;

         if ( DPS_INVALID_LSN_OFFSET != transInfo._curLSNWithRBPending )
         {
            PD_LOG( PDEVENT, "Transaction[ID:%s] is rollback pending, "
                    "restart from previous non pending LSN: %llu, "
                    "current pending LSN: %llu",
                    dpsTransIDToString( transID ).c_str(),
                    curLsnOffset, transInfo._curLSNWithRBPending ) ;
         }

         while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )
         {
            if ( !pTransCB->isDoRollback() )
            {
               PD_LOG ( PDEVENT, "Rollback is interrupted" ) ;
               rc = SDB_INTERRUPT ;
               goto error ;
            }
            dpsLogRecordHeader *recordHeader = NULL ;
            dpsLogRecord record ;
            mb.clear() ;
            dpsLsn.offset = curLsnOffset;
            rc = pDpsCB->search( dpsLsn, &mb ) ;
            if ( rc )
            {
               // report error
               pTransCB->incErrCount() ;
               PD_LOG( PDERROR, "Failed to get log LSN [%llu], rc: %d",
                       curLsnOffset, rc ) ;
               // retry after a while
               ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
               continue ;
            }

            // parse the record
            // WARNING: should not be failed
            rc = _rtnTransCheckRecord( transID, curLsnOffset, mb, record ) ;
            SDB_ASSERT( SDB_OK == rc, "DPS record is invalid for rollback" ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check record [LSN: %llu] for "
                         "transaction [%s], rc: %d", curLsnOffset,
                         dpsTransIDToString( transID ).c_str(), rc ) ;

            recordHeader = &( record.head() ) ;

            {
               cb->setRelatedTransLSN( curLsnOffset ) ;
               dpsLogRecord::iterator tmpitr =
                               record.find( DPS_LOG_PUBLIC_PRETRANS ) ;
               if ( !tmpitr.valid() )
               {
                  curLsnOffset = DPS_INVALID_LSN_OFFSET;
               }
               else
               {
                  curLsnOffset = *((DPS_LSN_OFFSET *)tmpitr.value() );
               }
               cb->setCurTransLsn( curLsnOffset ) ;

               // rollback pending record had been replayed earlier,
               // but interrupted by primary switch or reboot,
               // just reconstruct pending objects and move to next
               if ( DPS_INVALID_LSN_OFFSET != transInfo._curLSNWithRBPending &&
                    DPS_INVALID_LSN_OFFSET != curLsnOffset &&
                    curLsnOffset >= transInfo._curLSNWithRBPending )
               {
                  BOOLEAN removeOnly =
                        transInfo._curNonPendingLSN.count( curLsnOffset ) > 0 ;
                  // found a DPS log already rollbacked, but which may create
                  // or resolve a pending object, so try to rebuild pending
                  // objects from original DPS log
                  PD_LOG( PDDEBUG, "Rollback transaction [ID: %s] meets "
                          "older rollbacked record LSN [%llu], "
                          "created pending object: %s, "
                          "current pending LSN: [%llu]",
                          dpsTransIDToString( transID ).c_str(),
                          recordHeader->_lsn,
                          removeOnly ? "FALSE" : "TRUE",
                          transInfo._curLSNWithRBPending ) ;
                  rc = replayer.replayRBPending( recordHeader, removeOnly, cb,
                                                 mapPendingObj ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to replay rollback "
                               "pending record LSN [%llu], rc: %d",
                               recordHeader->_lsn, rc ) ;
                  continue ;
               }

               /// when rollback failed, need to retry some times.
               /// But all the way do it failed, need to restart the db
               rc = replayer.rollbackTrans( ( dpsLogRecordHeader *)mb.offset(0),
                                            cb, mapPendingObj ) ;
               if ( rc )
               {
                  ++retryTimes ;
                  PD_LOG( PDERROR, "Rollback transaction[ID:%s, "
                          "lsn=%llu, time=%u] failed,  rc: %d",
                          dpsTransIDToString( transID ).c_str(),
                          dpsLsn.offset, retryTimes, rc ) ;
                  if ( retryTimes >= RTN_TRANS_ROLLBACK_RETRY_TIMES )
                  {
                     PD_LOG( PDSEVERE, "Rollback transaction failed, need to "
                             "restart the system" ) ;
                     PMD_RESTART_DB( rc ) ;
                     goto error ;
                  }
                  ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
                  /// restore cur lsn to last lsn and retry
                  curLsnOffset = cb->getRelatedTransLSN() ;
               }
               else
               {
                  retryTimes = 0 ;
                  pTransCB->updateTransInfo( transInfo,
                                             DPS_TRANS_ROLLBACK,
                                             curLsnOffset,
                                             cb->isTransRBPending() ) ;
               }
            }
         } /// while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )

         if ( !mapPendingObj.empty() )
         {
            SDB_ASSERT( FALSE, "Transaction's pending object map is "
                        "not empty" ) ;
            PD_LOG( PDERROR, "Transaction(%s)'s pending object map"
                    " is not empty(size:%d)",
                    dpsTransIDToString( transID ).c_str(),
                    mapPendingObj.size() ) ;
         }
         else if ( cb->isTransRBPending() )
         {
            SDB_ASSERT( FALSE, "Transaction's rollback pending" ) ;
            PD_LOG( PDERROR, "Transaction(%s)'s rollback pending",
                    dpsTransIDToString( transID ).c_str() ) ;
         }

         /// remove the transaction
         pTransCB->removeTrans( transID ) ;
         tmpTransMap.erase( iterMap ) ;
         PD_LOG( PDEVENT, "Rollback transaction(ID:%s, IDAttr:%s) finished "
                 "with rc[%d]", dpsTransIDToString( transID ).c_str(),
                 dpsTransIDAttrToString( transID ).c_str(),
                 rc ) ;

         // report succeed
         pTransCB->incSucCount() ;
      } /// while ( tmpTransMap.size() != 0 )

   done:
      pTransCB->transLockReleaseAll( cb ) ;
      pTransCB->stopRollbackTask( doRollbackID ) ;

      cb->stopTransRollback() ;

      PD_LOG ( PDEVENT, "Rollback all unfinished transactions finished with "
               "rc[%d], rollback ID [%u]", rc, doRollbackID ) ;
      return rc ;
   error:
      pTransCB->incErrCount() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSSAVEWAITCOMMIT, "rtnTransSaveWaitCommit" )
   INT32 rtnTransSaveWaitCommit ( _pmdEDUCB * cb, SDB_DPSCB * dpsCB,
                                  BOOLEAN & savedAsWaitCommit )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNTRANSSAVEWAITCOMMIT ) ;

      SDB_ASSERT( cb, "cb can't be null" ) ;

      dpsTransCB *transCB = sdbGetTransCB() ;
      pmdTransExecutor *transExecutor = cb->getTransExecutor() ;

      DPS_LSN_OFFSET curLsnOffset = cb->getCurTransLsn() ;
      DPS_TRANS_ID transID = cb->getTransID() ;

      SDB_ASSERT( NULL != transExecutor, "transaction executor is invalid" ) ;

      savedAsWaitCommit = FALSE ;

      if ( transID.isInvalid() ||
           DPS_INVALID_LSN_OFFSET == curLsnOffset ||
           !dpsCB ||
           !pmdGetKRCB()->isCBValue( SDB_CB_CLS ) ||
           pmdIsPrimary() )
      {
         // no trans ID/LSN, or no dps, or not cluster mode,
         // or is primary, in those cases, should not clear, goto done
         goto done ;
      }

      // in cluster mode, when not primary, need add trans info to map
      rc = _rtnSaveTransInfo( cb, transID, curLsnOffset ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save transaction info, "
                   "rc: %d", rc ) ;

      savedAsWaitCommit = TRUE ;

      PD_LOG ( PDEVENT, "Save transaction(ID:%s, IDAttr:%s) as wait-commit "
               "finished", dpsTransIDToString( transID ).c_str(),
               dpsTransIDAttrToString( transID ).c_str() ) ;

      // report succeed
      sdbGetTransCB()->incSucCount() ;

   done:
      // just clear meta-block statistics
      transExecutor->clearMBStats() ;

      // remove from transaction CB map
      transCB->delTransCB( transID ) ;

      // clear records for transaction arbitration
      transExecutor->clearArbit() ;

      // reset transaction ID
      cb->resetTransID() ;

      // reset transaction LSN
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      cb->setRelatedTransLSN( DPS_INVALID_LSN_OFFSET ) ;

      transCB->transLockReleaseAll( cb ) ;

      // reduce the reservedLogSpace from dps for the transaction
      transCB->releaseRBLogSpace( cb ) ;

      PD_TRACE_EXITRC( SDB_RTNTRANSSAVEWAITCOMMIT, rc ) ;

      return rc ;

   error:
      // report error
      sdbGetTransCB()->incErrCount() ;
      goto done ;
   }

   INT32 rtnTransTryOrTestLockCL( const CHAR *pCollection,
                                  INT32 lockType,
                                  BOOLEAN isTest,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      const CHAR *pCollectionShortName = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      UINT16 collectionID = DMS_INVALID_MBID ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB  can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb  can't be NULL" ) ;
      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID );
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name"
                   "(collection:%s, rc=%d)", pCollection, rc ) ;
      rc = su->data()->findCollection( pCollectionShortName, collectionID ) ;
      logicCSID = su->LogicalCSID() ;
      dmsCB->suUnlock ( suID );
      PD_RC_CHECK( rc, PDERROR, "Failed to find the collection"
                   "(collection:%s, rc=%d)", pCollection, rc ) ;
      switch( lockType )
      {
      case DPS_TRANSLOCK_S:
         if ( isTest )
         {
            rc = pTransCB->transLockTestS( cb, logicCSID, collectionID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryS( cb, logicCSID, collectionID ) ;
         }
            break;
      case DPS_TRANSLOCK_X:
         if ( isTest )
         {
            rc = pTransCB->transLockTestX( cb, logicCSID, collectionID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryX( cb, logicCSID, collectionID ) ;
         }
         break;
      default:
         rc = SDB_INVALIDARG;
         PD_RC_CHECK( rc, PDERROR, "invalid lock-type" ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnTransTryOrTestLockCS( const CHAR *pSpace,
                                  INT32 lockType,
                                  BOOLEAN isTest,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      SDB_ASSERT ( pSpace, "space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB  can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb  can't be NULL" ) ;
      UINT32 length = ossStrlen ( pSpace );
      PD_CHECK( (length > 0 && length <= DMS_SU_NAME_SZ), SDB_INVALIDARG,
                error, PDERROR, "invalid length of collectionspace name:%s",
                pSpace );

      rc = dmsCB->nameToSUAndLock( pSpace, suID, &su );
      PD_CHECK(( su != NULL && suID != DMS_INVALID_SUID), SDB_DMS_CS_NOTEXIST,
               error, PDERROR, "lock collection space(%s) failed(rc=%d)",
               pSpace, rc );
      logicCSID = su->LogicalCSID();
      dmsCB->suUnlock ( suID ) ;
      switch( lockType )
      {
      case DPS_TRANSLOCK_S:
         if ( isTest )
         {
            rc = pTransCB->transLockTestS( cb, logicCSID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryS( cb, logicCSID ) ;
         }
         break;
      case DPS_TRANSLOCK_X:
         if ( isTest )
         {
            rc = pTransCB->transLockTestX( cb, logicCSID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryX( cb, logicCSID ) ;
         }
         break;
      default:
         rc = SDB_INVALIDARG;
         PD_RC_CHECK( rc, PDERROR, "invalid lock-type" );
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnTransReleaseLock( const CHAR *pCollection,
                              _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      const CHAR *pCollectionShortName = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      UINT16 collectionID = DMS_INVALID_MBID;
      CHAR *pDot = NULL;
      CHAR *pDot1 = NULL;
      pDot = (CHAR *)ossStrchr( pCollection, '.' );
      pDot1 = (CHAR *)ossStrrchr( pCollection, '.' );
      PD_CHECK( (pDot == pDot1 && pCollection != pDot), SDB_INVALIDARG,
                error, PDERROR, "invalid format for collection name:%s, "
                "expected format:<collectionspace>[.<collectionname>]",
                pCollection );
      if ( pDot )
      {
         rc = rtnResolveCollectionNameAndLock( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID );
         PD_RC_CHECK( rc, PDERROR,
                     "Failed to resolve collection name(collection:%s, rc=%d)",
                      pCollection, rc );
         rc = su->data()->findCollection( pCollectionShortName, collectionID ) ;
         logicCSID = su->LogicalCSID();
         dmsCB->suUnlock( suID );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to find collection(collection:%s, rc=%d)",
                      pCollection, rc );
      }
      else
      {
         rc = dmsCB->nameToSUAndLock( pCollection, suID, &su );
         PD_CHECK( ( su != NULL && suID != DMS_INVALID_SUID),
                   SDB_DMS_CS_NOTEXIST, error, PDERROR,
                   "lock collection space(%s) failed(rc=%d)",
                   pCollection, rc ) ;
         logicCSID = su->LogicalCSID();
         dmsCB->suUnlock( suID );
      }
      pTransCB->transLockRelease( cb, logicCSID, collectionID );
   done:
      return rc;
   error:
      goto done;
   }

}
