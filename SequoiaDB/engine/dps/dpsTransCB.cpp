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

   Source File Name = dpsTransCB.cpp

   Descriptive Name =

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
#include "dpsTransCB.hpp"
#include "dpsTransLockMgr.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "pmd.hpp"
#include "pmdDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsOp2Record.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsLogRecordDef.hpp"
#include "dpsTransLockCallback.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "dpsLogWrapper.hpp"
#include "pmdStartup.hpp"
#include "rtnCB.hpp"
#include "dpsGTSAgent.hpp"
#include "dpsUtil.hpp"

namespace engine
{

   // 0 offset to get transaction SN
   #define DPS_TRANSID_SN_NO_OFFSET ( 0 )

   // static minimum global transaction ID
   static const DPS_TRANS_ID &_dpsGetMinGlobTran()
   {
      static DPS_TRANS_ID s_minGlobTran( DPS_TRANSID_MIN_GLOB_SN,
                                         DPS_INVALID_TRANSID_NODEID ) ;
      return s_minGlobTran ;
   }

   dpsTransCB::dpsTransCB()
   :_TransIDL56Cur( 1 ) ,
    _lsnMapMutex( MON_LATCH_DPSTRANSCB_LSNMAPMUTEX ),
    _reservedRBSpace( 0 ) ,
    _reservedSpace( 0 ),
    _sucCount( 0LL ),
    _errCount( 0LL ),
    _primaryActiveTime( DPS_INVALID_TRANS_TIME ),
    _globLowTran( DPS_INVALID_TRANSID_SN ),
    _globExpireTran( DPS_INVALID_TRANSID_SN ),
    _archivedLowTran( DPS_INVALID_TRANSID_SN ),
    _maxReadTran( DPS_INVALID_TRANSID_SN ),
    _maxTransCommitTime( DPS_INVALID_TRANS_TIME ),
    _minRecoverableTime( DPS_INVALID_TRANS_TIME ),
    _restorePointTime( DPS_INVALID_TRANS_TIME ),
    _numTransIDConflict( 0LL ),
    _stpAgent(),
    _gtsAgent( NULL ),
    _rollbackLogTime( DPS_MAX_TRANSID_SN ),
    _rollbackLogLimit( DPS_MAX_TRANSID_SN )
   {
      _TransIDH16          = DPS_INVALID_TRANSID_NODEID ;
      _isOn                = FALSE ;
      _isGlobTransOn       = FALSE ;
      _isGlobTransSyncCheck = FALSE ;
      _isMVCCOn            = FALSE ;
      _doRollback          = FALSE ;
      _doRollbackID        = 1 ;
      _isNeedSyncTrans     = TRUE ;
      _logFileTotalSize    = 0 ;
      _transLockMgr        = NULL ;
      _indexLockMgr        = NULL ;
      _oldVCB              = NULL;
      _maxLRSize1          = 0 ;
      _maxLRSize2          = 0 ;
      _maxLRLSN1           = DPS_INVALID_LSN_OFFSET ;
      _maxLRLSN2           = DPS_INVALID_LSN_OFFSET ;

      _pEventHandler       = NULL ;

      _initTransMaps() ;
   }

   dpsTransCB::~dpsTransCB()
   {
   }

   SDB_CB_TYPE dpsTransCB::cbType () const
   {
      return SDB_CB_TRANS ;
   }

   const CHAR* dpsTransCB::cbName () const
   {
      return "TRANSCB" ;
   }

   INT32 dpsTransCB::init ()
   {
      INT32 rc = SDB_OK ;

      _isOn = pmdGetOptionCB()->transactionOn() ;
      _isGlobTransOn = pmdGetOptionCB()->globTransOn() ;
      // if --globtransmaxtimeerror < 0, means no need to check global time
      // synchronization between SequoiaDB nodes
      _isGlobTransSyncCheck =
            pmdGetOptionCB()->globTransMaxTimeError() >= 0 ? TRUE : FALSE ;
      _isMVCCOn = pmdGetOptionCB()->mvccOn() ;
      _rollbackEvent.signal() ;

      // register event handle
      pmdGetKRCB()->regEventHandler( this ) ;

      // only enable/setup old version CB if transaction is turned on
      // Each session decide when to setup/use the old copy based on
      // isolation level (TRANS_ISOLATION_RC)
      if( _isOn )
      {
         _TransIDL56Cur.init( ossRand() ) ;

         // create trans lock manager
         _transLockMgr = SDB_OSS_NEW dpsTransLockManager( LOCKMGR_TRANS_LOCK ) ;

         if ( !_transLockMgr )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         // Initialize trans lock manager
         rc = _transLockMgr->init( DPS_TRANS_LOCKBUCKET_SLOTS_MAX, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to initialize lock manager, rc: %d",
                    rc ) ;
            goto error ;
         }

         _oldVCB = SDB_OSS_NEW oldVersionCB() ;
         if ( !_oldVCB )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         rc = _oldVCB->init() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init OldVersionCB, rc: %d", rc ) ;
            goto error ;
         }
      }

      // if global transaction is not enabled, set global lowTran to maximum
      // value, which means all local transaction objects will be expired
      // once the transaction is finished
      if ( !_isGlobTransOn )
      {
         _globLowTran.init( DPS_MAX_TRANSID_SN ) ;
      }

      // if enabled dps
      if ( pmdGetKRCB()->isCBValue( SDB_CB_DPS ) &&
           !pmdGetKRCB()->isRestore() )
      {
         rc = _initFromDPS() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize transaction "
                      "information from DPS log, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsTransCB::active ()
   {
      if ( isGlobTransOn() )
      {
         stpClient client ;
         stpLogicalTimeNS currentTime ;
         ossTimestamp realTime ;
         UINT64 logicalTime = 0LL ;
         CHAR realTimeBuff[ OSS_TIMESTAMP_STRING_LEN ] = { 0 } ;

         // if global transaction feature is required, check available of STP
         // just test available, no need to report error
         if ( ( SDB_OK == _stpAgent.checkAvailable() ) &&
              ( SDB_OK == _stpAgent.getClient( client ) ) &&
              ( SDB_OK == client.getTime( currentTime ) ) &&
              ( SDB_OK ==
                    client.convLogicalTimeToRealTime(
                          currentTime.getTime().toMicroSecond(), realTime ) ) &&
              ( SDB_OK == client.convRealTimeToLogicalTime( realTime,
                                                            logicalTime ) ) )
         {
            ossTimestampToString( realTime, realTimeBuff ) ;
            PD_LOG( PDDEBUG, "Got time from STP [%llu], real time [%llu, %s], "
                    "local time [%llu]", currentTime.getTime().toMicroSecond(),
                    realTime.time, realTimeBuff, logicalTime ) ;
         }
      }

      return SDB_OK ;
   }

   INT32 dpsTransCB::deactive ()
   {
      PD_LOG( PDEVENT, "Counts of transID conflicts [%llu], "
              "generated the same timestamp by STP", _numTransIDConflict ) ;
      return SDB_OK ;
   }

   INT32 dpsTransCB::fini ()
   {
      unregisterGTSAgent() ;

      // unregister event handle
      pmdGetKRCB()->unregEventHandler( this ) ;

      if ( _transLockMgr )
      {
         _transLockMgr->fini() ;
         SDB_OSS_DEL _transLockMgr ;
         _transLockMgr = NULL ;
      }

      if ( _indexLockMgr )
      {
         _indexLockMgr->fini() ;
         SDB_OSS_DEL _indexLockMgr ;
         _indexLockMgr = NULL ;
      }

      if ( _oldVCB )
      {
         _oldVCB->fini() ;
         SDB_OSS_DEL _oldVCB ;
         _oldVCB = NULL ;
      }

      clearTransInfo() ;

      return SDB_OK ;
   }

   void dpsTransCB::onConfigChange()
   {
   }

   void dpsTransCB::_initTransMaps()
   {
      FOR_EACH_CMAP_BUCKET( TRANS_MAP, _transMap )
      {
         bucket.getLatch()->setLatchID( MON_LATCH_DPSTRANSCB_MAPMUTEX ) ;
      }
      FOR_EACH_CMAP_BUCKET_END

      FOR_EACH_CMAP_BUCKET( TRANS_CB_MAP, _cbMap )
      {
         bucket.getLatch()->setLatchID( MON_LATCH_DPSTRANSCB_CBMAPMUTEX ) ;
      }
      FOR_EACH_CMAP_BUCKET_END

      FOR_EACH_CMAP_BUCKET( TRANS_HIST_MAP, _histGlobMap )
      {
         bucket.getLatch()->setLatchID( MON_LATCH_DPSTRANSCB_HISMUTEX ) ;
      }
      FOR_EACH_CMAP_BUCKET_END

      FOR_EACH_CMAP_BUCKET( TRANS_HIST_MAP, _histRBMap )
      {
         bucket.getLatch()->setLatchID( MON_LATCH_DPSTRANSCB_HISMUTEX ) ;
      }
      FOR_EACH_CMAP_BUCKET_END
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__INITFROMDPS, "dpsTransCB::_initFromDPS" )
   INT32 dpsTransCB::_initFromDPS()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__INITFROMDPS ) ;

      SDB_DPSCB *dpsCB = sdbGetDPSCB() ;
      dpsReplicaLogMgr *logMgr = dpsCB->getLogMgr() ;
      UINT32 transMapSize = 0 ;
      dpsLogSummary summary ;
      BOOLEAN isSummaryValid = FALSE ;
      _logFileTotalSize = pmdGetOptionCB()->getTotalLogSpace() ;

      DPS_LSN_OFFSET startLsnOffset = dpsCB->readOldestBeginLsnOffset() ;
      DPS_LSN_OFFSET workBeginOffset = logMgr->getWorkBeginOffset() ;

      // first, try get summary from meta file
      rc = logMgr->getMetaSummary( summary ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get summary from meta, "
                   "rc: %d", rc ) ;

      if ( DPS_INVALID_TRANS_TIME == summary._maxTransCommitTime &&
           DPS_INVALID_TRANS_TIME == summary._minRecoverableTime &&
           DPS_INVALID_TRANS_TIME == summary._restorePointTime )
      {
         // summary from meta file is invalid, need get from log file
         summary.reset() ;
         // restore summary for DPS log file
         rc = dpsCB->getLogMgr()->getWorkSummary( summary, isSummaryValid ) ;
         if ( SDB_OK != rc )
         {
            // should not fail the initialize
            // let's start without log summary
            summary.reset() ;
            PD_LOG( PDWARNING, "Failed to get current log summary, "
                    "rc: %d", rc ) ;
            rc = SDB_OK ;
         }

         // since we are going to restore the summary from log file
         // adjust the start lsn to scan the log files
         if ( DPS_INVALID_LSN_OFFSET == startLsnOffset &&
              DPS_INVALID_LSN_OFFSET != workBeginOffset )
         {
            // start lsn from log meta is invalid, means there is no running
            // transactions before the last shutdown in this case, we only need
            // to scan from the beginning of the working log file
            startLsnOffset = workBeginOffset ;
         }
         else if ( DPS_INVALID_LSN_OFFSET != startLsnOffset &&
                   DPS_INVALID_LSN_OFFSET != workBeginOffset &&
                   startLsnOffset > workBeginOffset )
         {
            // start lsn from log meta is larger than begin lsn of working log
            // file means there is no running transactions is started after DPS
            // switched to working log file
            // so we need to scan from the beginning of the work file
            startLsnOffset = workBeginOffset ;
         }
      }
      else
      {
         isSummaryValid = TRUE ;
      }

      // restore valid summary back
      // NOTE: this is only the summary from log files before working log file
      if ( isSummaryValid )
      {
         setMinRecoverableTime( summary._minRecoverableTime ) ;
         setMaxTransCommitTime( summary._maxTransCommitTime ) ;
         setRestorePointTime( summary._restorePointTime ) ;
      }

      PD_LOG( PDEVENT, "Restored log summary [ minRecoverableTime: %llu,"
              "maxTransCommitTime: %llu ], begin LSN [%llu]",
              summary._minRecoverableTime, summary._maxTransCommitTime,
              startLsnOffset ) ;

      if ( _isOn && startLsnOffset != DPS_INVALID_LSN_OFFSET &&
           SDB_ROLE_STANDALONE != pmdGetDBRole() )
      {
         // if the meta file is valid, no need to check restore window
         rc = syncTransInfoFromLocal( startLsnOffset, !isSummaryValid ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to sync trans info from local, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      setIsNeedSyncTrans( FALSE ) ;

      transMapSize = getTransMapSize() ;
      // if have trans info, need log
      if ( transMapSize > 0 )
      {
         PD_LOG( PDEVENT, "Restored trans info, have %u trans not "
                 "be complete, the oldest lsn offset is %lld",
                 transMapSize, getOldestBeginLsn() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB__INITFROMDPS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ALLOCTRANSID, "dpsTransCB::allocTransID" )
   INT32 dpsTransCB::allocTransID( BOOLEAN isAutoCommit,
                                   BOOLEAN isGlobTrans,
                                   UINT32 timeout,
                                   DPS_TRANS_ID &transID,
                                   stpLogicalTimeUS &beginTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ALLOCTRANSID ) ;

      DPS_TRANS_ID newTransID ;

      if ( isGlobTrans )
      {
         PD_CHECK( DPS_INVALID_TRANSID_NODEID != _TransIDH16,
                   SDB_GLOB_TRANS_NOT_AVAILABLE, error, PDERROR,
                   "Failed to allocate global transaction ID, "
                   "node ID is invalid" ) ;
         // global transaction by global logical time
         stpLogicalTimeUS transTime ;
         rc = getGlobTransTime( transTime, (INT32)timeout ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get logical time of "
                      "transaction begin, rc: %d", rc ) ;

         // set serial number
         newTransID.resetSN( transTime.getTime() ) ;
         newTransID.setGlobTrans() ;

         // set begin time
         beginTime = transTime ;
      }
      else
      {
         // allocate serial number by atomic for non-global transaction
         do {
            newTransID.resetSN( _TransIDL56Cur.inc() ) ;
         }  while ( 0 == newTransID.getSN() ) ;
      }

      // set node ID
      newTransID.setNodeID( _TransIDH16 ) ;

      // after allocate transaction ID, will be the first operation of
      // transaction
      newTransID.setFirstOp() ;

      if ( isAutoCommit )
      {
         // set auto commit tag
         newTransID.setAutoCommit() ;
      }

      // copy new transaction ID to output
      transID = newTransID ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_ALLOCTRANSID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__ISGLOBDOINGVISIBLE, "dpsTransCB::_isGlobDoingVisible" )
   INT32 dpsTransCB::_isGlobDoingVisible( pmdEDUCB *eduCB,
                                          const DPS_TRANS_ID &recTransID,
                                          const dpsTransBackInfo &recTransInfo,
                                          const DPS_TRANS_ID &transID,
                                          const stpLogicalTimeUS &transBeginTime,
                                          BOOLEAN &visible,
                                          stpLogicalTimeUS &visibleTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__ISGLOBDOINGVISIBLE ) ;

      SDB_ASSERT( DPS_TRANS_DOING ==
                  (DPS_TRANS_STATUS)( recTransInfo._status ),
                  "record transaction should be doing status" ) ;
      SDB_ASSERT( NULL != _gtsAgent, "GTS agent is invalid" ) ;

      stpAgent timeAgent ;
      stpLogicalTimeUS currentTime ;

      visible = FALSE ;

      // passed doing arbitration, no need to do arbitration, the doing
      // record transaction can not commit before begin of current
      // transaction
      if ( eduCB->isPassedDoingArbit() )
      {
         visible = FALSE ;
         goto done ;
      }

      // check current time to find out if we need to do arbitration
      rc = timeAgent.getLogicalTimeUS( currentTime,
                                       eduCB->getTransTimeout(),
                                       FALSE ) ;
      if ( SDB_OK == rc )
      {
         currentTime.setTimeError( _gtsAgent->getMaxNodeTimeError() ) ;
         if ( transBeginTime < currentTime )
         {
            // if current transaction started before current time with
            // maximum time error, the doing record transaction could
            // not be commit before current transaction, so the record
            // should not be seen by current transaction
            // and current transaction could skip arbitration for
            // doing transactions, since it passed maximum time error,
            // and doing transactions could not commit before current
            // transaction any more
#if SDB_INTERNAL_DEBUG
            PD_LOG( PDDEBUG, "current transaction [%s] passed doing "
                    "arbit limit, current time [%s]",
                    dpsTransIDToString( transID ).c_str(),
                    dpsTransTimeToString( currentTime ).c_str() ) ;
#endif
            eduCB->setPassedDoingArbit( TRUE ) ;
            visible = FALSE ;
            goto done ;
         }

         // otherwise, we need arbitration further
      }
      else
      {
         // failed to get current time, go to arbitration directly
         PD_LOG( PDWARNING, "Failed to get current global logical "
                 "time, rc: %d", rc ) ;
      }

      rc = doArbitGlobTrans( eduCB, transID, recTransID,
                             (DPS_TRANS_STATUS)( recTransInfo._status ),
                             FALSE, visible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do arbitration for read "
                   "transaction [%s] against write transaction [%s] "
                   "with status [%s], rc: %d",
                   dpsTransIDToString( transID ).c_str(),
                   dpsTransIDToString( recTransID ).c_str(),
                   dpsTransStatusToString( recTransInfo._status ) ) ;

      // we use test lock for RR now, so if return visible, we need
      // to wait the record transaction commit
      if ( visible )
      {
         BOOLEAN committed = FALSE ;
         BOOLEAN multiGroups = FALSE ;
         stpLogicalTimeUS commitTime ;
         // the current transaction have a chance to see this record
         // but first we need to wait commit
         rc = _gtsAgent->waitArbitCommit( eduCB,
                                          recTransID,
                                          eduCB->getTransTimeout(),
                                          committed,
                                          multiGroups,
                                          commitTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to wait transaction [%s] to "
                      "commit, rc: %d",
                      dpsTransIDToString( recTransID ).c_str(), rc ) ;

         if ( !committed )
         {
            // the arbit result is visible, it must be committed in other
            // nodes
            SDB_ASSERT( FALSE, "should not be uncommitted" ) ;
            // failed to wait commit, record transaction is rollback,
            // the record should not be seen now
            PD_LOG( PDWARNING, "Failed to wait transaction [%s] "
                    "to be committed, it is rollbacked",
                    dpsTransIDToString( recTransID ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         visibleTime = commitTime ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB__ISGLOBDOINGVISIBLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__ISGLOBWCVISIBLE, "dpsTransCB::_isGlobWaitCommitVisible" )
   INT32 dpsTransCB::_isGlobWaitCommitVisible(
                                       pmdEDUCB *eduCB,
                                       const DPS_TRANS_ID &recTransID,
                                       const dpsTransBackInfo &recTransInfo,
                                       const DPS_TRANS_ID &transID,
                                       const stpLogicalTimeUS &transBeginTime,
                                       BOOLEAN &visible,
                                       stpLogicalTimeUS &visibleTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__ISGLOBWCVISIBLE ) ;

      BOOLEAN committed = FALSE ;
      BOOLEAN multiGroups = TRUE ;
      stpLogicalTimeUS commitTime ;

      visible = FALSE ;

      SDB_ASSERT( DPS_TRANS_WAIT_COMMIT ==
                  (DPS_TRANS_STATUS)( recTransInfo._status ),
                  "record transaction should be wait commit status" ) ;
      SDB_ASSERT( NULL != _gtsAgent, "GTS agent is invalid" ) ;

      // record transaction pre-committed after current transaction
      // the record should not be seen
      if ( transBeginTime <= recTransInfo._preCommitTime )
      {
         visible = FALSE ;
         goto done ;
      }

      // the current transaction have a chance to see this record
      // but first we need to wait commit for the record transaction, in case
      // that pre-commit command is failed to send to other groups
      rc = _gtsAgent->waitArbitCommit( eduCB,
                                       recTransID,
                                       eduCB->getTransTimeout(),
                                       committed,
                                       multiGroups,
                                       commitTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait transaction [%s] to "
                   "commit, rc: %d",
                   dpsTransIDToString( recTransID ).c_str(), rc ) ;

      if ( !committed )
      {
         // failed to wait commit, record transaction is rollback,
         // the record should not be seen now
         PD_LOG( PDWARNING, "Failed to wait transaction [%s] "
                 "to be committed, it is rollbacked",
                 dpsTransIDToString( recTransID ).c_str() ) ;
         visible = FALSE ;
         goto done ;
      }

      visibleTime = commitTime ;

      // record transaction committed after current transaction
      // the record should not be seen
      if ( transBeginTime <= recTransInfo._preCommitTime )
      {
         visible = FALSE ;
         goto done ;
      }

      // we need to do arbitration, to avoid already arbitrated to invisible
      // before
      // NOTE:
      //    - since the transaction is committed, we arbitrate it as committed
      //      status
      //    - if not involved in multiple groups, we could use local
      //      arbitration ( if it had been arbitrated earlier, it will have a
      //      cache in local, otherwise, we can decide visibility in local
      //      node, since only this node is involved in write transaction )
      rc = doArbitGlobTrans( eduCB, transID, recTransID,
                             DPS_TRANS_COMMIT, !multiGroups, visible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do arbitration for read "
                   "transaction [%s] against write transaction [%s] "
                   "with status [%s] ( waiting commit earlier ), rc: %d",
                   dpsTransIDToString( transID ).c_str(),
                   dpsTransIDToString( recTransID ).c_str(),
                   dpsTransStatusToString( DPS_TRANS_COMMIT ) ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB__ISGLOBWCVISIBLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__ISGLOBVISIBLE, "dpsTransCB::_isGlobVisible" )
   INT32 dpsTransCB::_isGlobVisible( pmdEDUCB *eduCB,
                                     const DPS_TRANS_ID &recTransID,
                                     const DPS_TRANS_ID &transID,
                                     const stpLogicalTimeUS &transBeginTime,
                                     BOOLEAN &visible,
                                     stpLogicalTimeUS &visibleTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__ISGLOBVISIBLE ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;

      DPS_TRANSID_SN minCheckSN = recTransID.getGlobSN() ;
      dpsTransBackInfo recTransInfo ;

      // NOTE: this functions checks visibility between transactions with
      //       arbitration to remote node if needed

      visible = FALSE ;

      DPS_ADJUST_TRANSID_SN( minCheckSN, -STP_MAX_TIME_ERROR_US ) ;

      // minimum checking SN is begin time of record transaction minus maximum
      // time error, transactions started before this time is definitely not
      // able to see this record
      // NOTE: in this case, we could determine the visibility quickly
      //       without getting transaction information for record
      if ( transID.getGlobSN() < minCheckSN )
      {
         // use FALSE visible
         goto done ;
      }
      // check if we already have arbitration done
      if ( eduCB->getTransExecutor()->findArbit( recTransID, visible ) )
      {
         // use visible from find result
         goto done ;
      }

      // otherwise, current transaction started after record transaction with
      // maximum time error, we need to check commit time of record
      // transaction further

      // NOTE: if not found, will return unknown status in transaction info
      getTransInfo( recTransID, recTransInfo ) ;

#if SDB_INTERNAL_DEBUG
      PD_LOG( PDDEBUG, "Check local visibility for current transaction [%s] "
              "with begin time [%s] against record transaction [%s] with "
              "status [%s], begin time [%s], pre-commit time [%s], "
              "commit time [%s]",
              dpsTransIDToString( transID ).c_str(),
              dpsTransTimeToString( transBeginTime ).c_str(),
              dpsTransIDToString( recTransID ).c_str(),
              dpsTransStatusToString( recTransInfo._status ),
              dpsTransTimeToString( recTransInfo._beginTime ).c_str(),
              dpsTransTimeToString( recTransInfo._preCommitTime ).c_str(),
              dpsTransTimeToString( recTransInfo._commitTime ).c_str() ) ;
#endif

      if ( transBeginTime < recTransInfo._beginTime )
      {
         // current transaction is definitely started before record
         // transaction, the record should not be seen by the
         // transaction
         // use FALSE visible
         goto done ;
      }
      else if ( ( DPS_TRANS_DOING == recTransInfo._status ||
                  DPS_TRANS_DOING_INTERRUPT == recTransInfo._status ) &&
                transBeginTime <=
                      recTransInfo._beginTime.getUpperLogicalTime() )
      {
         // in this case, record transaction could not be committed
         // before current transaction ( should be committed after an
         // interval given by time error of record transaction )
         // current transaction is definitely started before record
         // transaction, the record should not be seen by the
         // transaction
         // use FALSE visible
         goto done ;
      }
      else if ( DPS_TRANS_PRE_WAIT_COMMIT == recTransInfo._status )
      {
         // record transaction is processing pre-commit request
         // wait for status change
         rc = _gtsAgent->waitArbitChange( eduCB,
                                          recTransID,
                                          DPS_TRANS_PRE_WAIT_COMMIT,
                                          eduCB->getTransTimeout(),
                                          recTransInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to wait transaction [%s] "
                      "status change, rc: %d",
                      dpsTransIDToString( recTransID ).c_str(), rc ) ;
         SDB_ASSERT( DPS_TRANS_PRE_WAIT_COMMIT != recTransInfo._status,
                     "should not be PRE_WAIT_COMMIT for record transaction" ) ;
      }

      // record transaction is started before current transaction with time
      // error between them, we need to check commit time further
      switch ( recTransInfo._status )
      {
         case DPS_TRANS_DOING :
         {
            rc = _isGlobDoingVisible( eduCB, recTransID, recTransInfo,
                                      transID, transBeginTime, visible,
                                      visibleTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check visible for "
                         "transaction [%s] against doing transaction [%s], "
                         "rc: %d", dpsTransIDToString( transID ).c_str(),
                         dpsTransIDToString( recTransID ).c_str(), rc ) ;
            break ;
         }
         case DPS_TRANS_WAIT_COMMIT :
         {
            rc = _isGlobWaitCommitVisible( eduCB, recTransID, recTransInfo,
                                           transID, transBeginTime, visible,
                                           visibleTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check visible for "
                         "transaction [%s] against wait-commit "
                         "transaction [%s], rc: %d",
                         dpsTransIDToString( transID ).c_str(),
                         dpsTransIDToString( recTransID ).c_str(), rc ) ;
            break ;
         }
         case DPS_TRANS_COMMIT :
         {
            // the transaction of record is committed, check with commit
            // time if current transaction started after record's
            // transaction had been committed, the record is visible to current
            // transaction
            // NOTE: in the meantime in other groups, the record transaction
            //       should be at least wait-commit status, in arbitration of
            //       wait-commit status, it should wait for commit, so it is
            //       safe to decide the visibility here
            if ( transBeginTime > recTransInfo._commitTime )
            {
               visible = TRUE ;
            }
            visibleTime = recTransInfo._commitTime ;
            break ;
         }
         case DPS_TRANS_ROLLBACK :
         {
            // if rollbacked, record should be rollbacked to old version
            // before this transaction, if we could see rollbacked
            // transaction ID, which means the transaction is doing
            // rollback, or this is an old version record which was not
            // cleared in time
            // so, the record should not be seen anyway
            // use FALSE visible
            break ;
         }
         case DPS_TRANS_DOING_INTERRUPT :
         {
            // if transaction is DOING INTERRUPTED status,
            // it is going to rollback, so this record could not be seen
            // like ROLLBACK status
            // use FALSE visible
            break ;
         }
         case DPS_TRANS_UNKNOWN :
         {
            // if status is unknown, means transaction of record is before
            // lowTran, and status has been cleared
            visible = TRUE ;
            break ;
         }
         default :
         {
            PD_LOG( PDWARNING, "Invalid transaction status [%s]/[%d] for "
                    "transaction [%s]",
                    dpsTransStatusToString( recTransInfo._status ),
                    recTransInfo._status,
                    dpsTransIDToString( recTransID ).c_str() ) ;
            SDB_ASSERT( FALSE, "invalid status, should not go here" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE1 ( SDB_DPSTRANSCB__ISGLOBVISIBLE,
                  PD_PACK_UINT( visible ) ) ;

      PD_TRACE_EXITRC( SDB_DPSTRANSCB__ISGLOBVISIBLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__ISLOCALVISIBLE, "dpsTransCB::_isLocalVisible" )
   INT32 dpsTransCB::_isLocalVisible( pmdEDUCB *eduCB,
                                      const DPS_TRANS_ID &recTransID,
                                      const DPS_TRANS_ID &transID,
                                      const stpLogicalTimeUS &transBeginTime,
                                      BOOLEAN &visible,
                                      stpLogicalTimeUS &visibleTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__ISLOCALVISIBLE ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;

      dpsTransBackInfo recTransInfo ;

      visible = FALSE ;

      // NOTE: this functions checks visibility between transactions without
      //       arbitration or from the same node

      // check begin time of transactions, if current transaction started
      // before record transaction, the record should not be seen by the
      // transaction anyway
      if ( recTransID.getGlobSN() >= transID.getGlobSN() )
      {
         goto done ;
      }

      // if current transaction started after record transaction, we need to
      // check commit time of record transaction further

      // NOTE: if not found, will return unknown status in transaction info
      getTransInfo( recTransID, recTransInfo ) ;

#if SDB_INTERNAL_DEBUG
      PD_LOG( PDDEBUG, "Check local visibility for current transaction [%s] "
              "with begin time [%s] against record transaction [%s] with "
              "status [%s], begin time [%s], pre-commit time [%s], "
              "commit time [%s]",
              dpsTransIDToString( transID ).c_str(),
              dpsTransTimeToString( transBeginTime ).c_str(),
              dpsTransIDToString( recTransID ).c_str(),
              dpsTransStatusToString( recTransInfo._status ),
              dpsTransTimeToString( recTransInfo._beginTime ).c_str(),
              dpsTransTimeToString( recTransInfo._preCommitTime ).c_str(),
              dpsTransTimeToString( recTransInfo._commitTime ).c_str() ) ;
#endif

      if ( DPS_TRANS_PRE_WAIT_COMMIT == recTransInfo._status )
      {
         // record transaction is processing pre-commit request
         // wait for status change
         rc = _gtsAgent->waitArbitChange( eduCB,
                                          recTransID,
                                          DPS_TRANS_PRE_WAIT_COMMIT,
                                          eduCB->getTransTimeout(),
                                          recTransInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to wait transaction [%s] "
                      "status change, rc: %d",
                      dpsTransIDToString( recTransID ).c_str(), rc ) ;
         SDB_ASSERT( DPS_TRANS_PRE_WAIT_COMMIT != recTransInfo._status,
                     "should not be PRE_WAIT_COMMIT for record transaction" ) ;
      }

      switch ( recTransInfo._status )
      {
         case DPS_TRANS_DOING :
         {
            // transaction is still doing, the record should not be seen
            visible = FALSE ;
            break ;
         }
         case DPS_TRANS_WAIT_COMMIT :
         {
            if ( transBeginTime.getTime() <=
                        recTransInfo._preCommitTime.getTime() )
            {
               // record transaction pre-committed after current transaction
               // the record should not be seen
               visible = FALSE ;
            }
            else
            {
               // transaction is waiting commit, need to wait transaction
               // committed to find out commit time
               BOOLEAN committed = FALSE ;
               BOOLEAN multiGroups = FALSE ;
               stpLogicalTimeUS commitTime ;
               rc = _gtsAgent->waitArbitCommit( eduCB, recTransID,
                                                eduCB->getTransTimeout(),
                                                committed, multiGroups,
                                                commitTime ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to wait transaction [%s] to "
                            "commit, rc: %d",
                            dpsTransIDToString( recTransID ).c_str(), rc ) ;

               // if transaction is committed, and current transaction is
               // started after commit of record transaction, the record should
               // be seen by transaction
               // otherwise, the record should not be seen
               if ( committed &&
                    transBeginTime.getTime() > commitTime.getTime() )
               {
                  visible = TRUE ;
               }
               else if ( committed )
               {
                  visibleTime = commitTime ;
               }
            }

            break ;
         }
         case DPS_TRANS_COMMIT :
         {
            // the transaction of record is committed, check with commit
            // time if current transaction started after record's
            // transaction had been committed, the record is visible to current
            // transaction
            if ( transBeginTime.getTime() >
                        recTransInfo._commitTime.getTime() )
            {
               visible = TRUE ;
            }
            visibleTime = recTransInfo._commitTime ;
            break ;
         }
         case DPS_TRANS_ROLLBACK :
         case DPS_TRANS_DOING_INTERRUPT :
         {
            // if rollbacked, record should be rollbacked to old version
            // before this transaction, if we could see rollbacked
            // transaction ID, which means the transaction is doing
            // rollback, or this is an old version record which was not
            // cleared in time
            // if doing interrupted, the transaction is going to rollback
            // so, the record should not been seen anyway
            visible = FALSE ;
            break ;
         }
         case DPS_TRANS_UNKNOWN :
         {
            // if status is unknown, means transaction of record is before
            // lowTran, and status has been cleared
            visible = TRUE ;
            break ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "invalid status, should not go here" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                      "Failed to check visibility on local, "
                      "unknown status [%d] of transaction record",
                      recTransInfo._status ) ;

            visible = FALSE ;
            break ;
         }
      }

   done:
      PD_TRACE1 ( SDB_DPSTRANSCB__ISLOCALVISIBLE,
                  PD_PACK_UINT( visible ) ) ;
      PD_TRACE_EXITRC( SDB_DPSTRANSCB__ISLOCALVISIBLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ISVERSIONVISIBLE, "dpsTransCB::isVersionVisible" )
   INT32 dpsTransCB::isVersionVisible( pmdEDUCB *eduCB,
                                       const DPS_TRANS_ID &recTransID,
                                       const DPS_TRANS_ID &transID,
                                       const stpLogicalTimeUS &transBeginTime,
                                       INT32 isolation,
                                       BOOLEAN strictIsolation,
                                       BOOLEAN &visible,
                                       stpLogicalTimeUS *pVisibleTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ISVERSIONVISIBLE ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;

      DPS_TRANSID_SN globExpireTran = DPS_INVALID_TRANSID_SN ;
      stpLogicalTimeUS visibleTime( DPS_MAX_TRANS_TIME, STP_MAX_TIME_ERROR ) ;

      if ( TRANS_ISOLATION_RR != isolation )
      {
         // not RR isolation, always visible
         // NOTE: in this phase, transaction must be lock acquired
         visible = TRUE ;
      }
      else if ( !recTransID.isGlobTrans() ||
                !transID.isGlobTrans() )
      {
         // non-global transactions are always visible for each other
         // NOTE: in this phase, transaction must be lock acquired
         // strict isolation: when one of transaction IDs is not global,
         // we need to report error
         if ( strictIsolation &&
              transID.isGlobTrans() &&
              !recTransID.isGlobTrans() )
         {
            PD_LOG( PDWARNING, "Failed to check visibility for global read "
                    "transaction [%s] against non-global transaction [%s] in "
                    "strict isolation mode",
                    dpsTransIDToString( transID ).c_str(),
                    dpsTransIDToString( recTransID ).c_str() ) ;
            rc = SDB_OPERATION_INCOMPATIBLE ;
            goto error ;
         }
         visible = TRUE ;
      }
      else if ( recTransID.getOrigTransID() == transID.getOrigTransID() )
      {
         // the same transaction, should be visible
         visible = TRUE ;
      }
      else if ( recTransID.getGlobSN() < eduCB->getExpireTranCache() )
      {
         // cached expireTran is passed, should be visible
         visible = TRUE ;
      }
      else if ( recTransID.getGlobSN() <
                ( globExpireTran = getExpiredVersion() ) )
      {
         // global expireTran is passed, should be visible
         visible = TRUE ;
         // in this case, the global expireTran could updated
         // update local cache as well
         eduCB->setExpireTranCache( globExpireTran ) ;
      }
      else if ( recTransID.getNodeID() != transID.getNodeID() &&
                _TransIDH16 != recTransID.getNodeID() &&
                _TransIDH16 != transID.getNodeID() )
      {
         // from different node, check visible in global cluster with
         // time error
         rc = _isGlobVisible( eduCB, recTransID, transID, transBeginTime,
                              visible, visibleTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check global visible for "
                      "transaction [%s] against record transaction [%s], "
                      "rc: %d", dpsTransIDToString( transID ).c_str(),
                      dpsTransIDToString( recTransID ).c_str(), rc ) ;
      }
      else
      {
         // from the same node, or from this node, or arbitration is off
         // check visible in this node
         // NOTE: we could check visible without time error
         rc = _isLocalVisible( eduCB, recTransID, transID, transBeginTime,
                               visible, visibleTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check local visible for "
                      "transaction [%s] against record transaction [%s], "
                      "rc: %d", dpsTransIDToString( transID ).c_str(),
                      dpsTransIDToString( recTransID ).c_str(), rc ) ;
      }

      if ( NULL != pVisibleTime )
      {
         *pVisibleTime = visibleTime ;
      }

#if defined (_DEBUG)
      PD_LOG( PDDEBUG, "Check visibility for current transaction [%s] against "
              "record transaction [%s], visible: %s",
              dpsTransIDToString( transID ).c_str(),
              dpsTransIDToString( recTransID ).c_str(),
              visible ? "TRUE" : "FALSE" ) ;
#endif

   done:
      PD_TRACE1 ( SDB_DPSTRANSCB_ISVERSIONVISIBLE,
                  PD_PACK_UINT( visible ) ) ;
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_ISVERSIONVISIBLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ISVERSIONEXPIRED, "dpsTransCB::isVersionExpired" )
   BOOLEAN dpsTransCB::isVersionExpired( const DPS_TRANS_ID &transID )
   {
      BOOLEAN expired = FALSE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ISVERSIONEXPIRED ) ;

      DPS_TRANSID_SN expiredVersion = getExpiredVersion() ;
      DPS_TRANSID_SN transSN= transID.getGlobSN() ;

      // NOTE: if expired lowTran is invalid, means the global lowTrans had
      //       not been calculated yet, so any version is not expired at this
      //       time
      if ( DPS_INVALID_TRANSID_SN != expiredVersion )
      {
         // check if the version(represented by transaction ID) is expired.
         // Expired means it's older than system expired version
         expired = ( transSN < expiredVersion ) ;
      }

      PD_TRACE3( SDB_DPSTRANSCB_ISVERSIONEXPIRED,
                 PD_PACK_INT ( expired ),
                 PD_PACK_ULONG( transSN ),
                 PD_PACK_ULONG ( expiredVersion ) ) ;
      PD_TRACE_EXIT( SDB_DPSTRANSCB_ISVERSIONEXPIRED ) ;

      return expired ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBLOWTRAN, "dpsTransCB::getGlobLowTran" )
   DPS_TRANS_ID dpsTransCB::getGlobLowTran()
   {
      DPS_TRANS_ID lowTran ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBLOWTRAN ) ;

      // get global low transaction ID
      DPS_TRANSID_SN globLowTran = _getGlobLowTran() ;
      if ( DPS_INVALID_TRANSID_SN != globLowTran )
      {
         lowTran.setNodeID( _TransIDH16 ) ;
         lowTran.setSN( globLowTran ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETGLOBLOWTRAN ) ;

      return lowTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__GETGLOBLOWTRAN, "dpsTransCB::_getGlobLowTran" )
   DPS_TRANSID_SN dpsTransCB::_getGlobLowTran()
   {
      DPS_TRANSID_SN globLowTran = DPS_INVALID_TRANSID_SN ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__GETGLOBLOWTRAN ) ;

      // get global lowTran
      globLowTran = (DPS_TRANSID_SN)( _globLowTran.fetch() ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB__GETGLOBLOWTRAN ) ;

      return globLowTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBEXPIRETRAN, "dpsTransCB::getGlobExpireTran" )
   DPS_TRANS_ID dpsTransCB::getGlobExpireTran()
   {
      DPS_TRANS_ID expireTran ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBEXPIRETRAN ) ;

      // get global expire transaction ID
      DPS_TRANSID_SN globExpireTran = getExpiredVersion() ;
      if ( DPS_INVALID_TRANSID_SN != globExpireTran )
      {
         expireTran.setNodeID( _TransIDH16 ) ;
         expireTran.setSN( globExpireTran ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETGLOBEXPIRETRAN ) ;

      return expireTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SYNCUPDATEGLOBLOWTRAN, "dpsTransCB::syncUpdateGlobLowTran" )
   INT32 dpsTransCB::syncUpdateGlobLowTran( DPS_TRANS_ID &globalLowTran,
                                            INT64 timeout )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SYNCUPDATEGLOBLOWTRAN ) ;

      INT32 tmpRC = SDB_OK ;

      // reset wait event
      _waitLowTranEvent.reset() ;

      // signal lowTran job to update
      _updateLowTranEvent.signal() ;

      // lowTran is not critical, old lowTran will be OK for most cases
      // so just wait for one second
      rc = _waitLowTranEvent.wait( OSS_ONE_SEC, &tmpRC ) ;
      if ( SDB_OK != tmpRC )
      {
         rc = tmpRC ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to wait for lowTran event, rc: %d",
                   rc ) ;

      globalLowTran = getGlobLowTran() ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_SYNCUPDATEGLOBLOWTRAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SETGLOBLOWTRAN, "dpsTransCB::setGlobLowTran" )
   void dpsTransCB::setGlobLowTran( const DPS_TRANSID_SN &globLowTran )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SETGLOBLOWTRAN ) ;

      // for below 2 cases, we won't update global lowTran cache to keep
      // global lowTran in monotonic
      // - invalid value of transaction SN means global lowTran has not been
      //   calculated by CATALOG ( some node had not reported )
      // - max value of transaction SN means no global transaction in the
      //   cluster currently
      if ( DPS_INVALID_TRANSID_SN != globLowTran &&
           DPS_MAX_TRANSID_SN != globLowTran )
      {
         DPS_TRANSID_SN tempLowTran = DPS_INVALID_TRANSID_SN ;

         _globLowTran.swapGreaterThan( (UINT64)globLowTran ) ;

         tempLowTran = _globLowTran.fetch() ;
         PD_LOG( PDDEBUG, "Set global lowTran [%s]",
                 dpsTransSNToString( tempLowTran ).c_str() ) ;
      }
#if defined (_DEBUG)
      else
      {
         DPS_TRANSID_SN tempLowTran = _globLowTran.fetch() ;
         PD_LOG( PDDEBUG, "Got ignored global lowTran [%s], "
                 "current global lowTran [%s]",
                 dpsTransSNToString( globLowTran ).c_str(),
                 dpsTransSNToString( tempLowTran ).c_str() ) ;
      }
#endif

      PD_TRACE_EXIT( SDB_DPSTRANSCB_SETGLOBLOWTRAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SETGLOBEXPTRAN, "dpsTransCB::setGlobExpireTran" )
   void dpsTransCB::setGlobExpireTran( const DPS_TRANSID_SN &globExpireTran )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SETGLOBEXPTRAN ) ;

      // for below 2 cases, we won't update global expireTran cache to keep
      // global expireTran in monotonic
      // - invalid value of transaction SN means global expireTran has not been
      //   calculated by CATALOG ( some node had not reported )
      // - max value of transaction SN means no global transaction in the
      //   cluster currently
      if ( DPS_INVALID_TRANSID_SN != globExpireTran &&
           DPS_MAX_TRANSID_SN != globExpireTran )
      {
         DPS_TRANSID_SN tempExpireTran = DPS_INVALID_TRANSID_SN ;

         _globExpireTran.swapGreaterThan( (UINT64)globExpireTran ) ;

         tempExpireTran = _globExpireTran.fetch() ;
         PD_LOG( PDDEBUG, "Set global expireTran [%s]",
                 dpsTransSNToString( tempExpireTran ).c_str() ) ;
      }
#if defined (_DEBUG)
      else
      {
         DPS_TRANSID_SN tempExpireTran = _globExpireTran.fetch() ;
         PD_LOG( PDDEBUG, "Got ignored global expireTran [%s], "
                 "current global expireTran [%s]",
                 dpsTransSNToString( globExpireTran ).c_str(),
                 dpsTransSNToString( tempExpireTran ).c_str() ) ;
      }
#endif

      PD_TRACE_EXIT( SDB_DPSTRANSCB_SETGLOBEXPTRAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETLOCALLOWTRAN, "dpsTransCB::getLocalLowTran" )
   DPS_TRANS_ID dpsTransCB::getLocalLowTran()
   {
      DPS_TRANS_ID lowTran ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETLOCALLOWTRAN ) ;

      DPS_TRANS_ID minGlobTran = _dpsGetMinGlobTran() ;

      // NOTE: cbMap contains transactions between rtnTransBegin and
      //       rtnTransCommit / rtnTransRollback

      // for each element in concurrent map
      FOR_EACH_CMAP_BUCKET_S( TRANS_CB_MAP, _cbMap )
      {
         TRANS_CB_MAP::map_iterator iterCB =
                     bucket.getMap().upper_bound( minGlobTran ) ;
         if ( iterCB != bucket.end() &&
              ( lowTran.isInvalid() ||
                iterCB->first < lowTran ) )
         {
            lowTran = iterCB->first ;
         }
      }
      FOR_EACH_CMAP_BUCKET_END

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETLOCALLOWTRAN ) ;

      return lowTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETLOCALEXPTRAN, "dpsTransCB::getLocalExpireTran" )
   DPS_TRANS_ID dpsTransCB::getLocalExpireTran()
   {
      DPS_TRANS_ID expireTran ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETLOCALEXPTRAN ) ;

      // the expireTran is the maximum expired transaction ID, which
      // means transactions before expireTran are expired, whose transaction
      // objects, like RBS, old version, etc, could be cleared
      //
      //              +--------
      //              |
      //  +---+   +---+---+
      //  |   |   |   |   |
      // -+---+---+---+---+----
      //  T0      T1  T2     NOW
      //
      // T2 is minimum running transaction ( lowTran ), while T1 is the
      // minimum transaction started before T2, but committed after T2.
      //    start time of T1 < start time of T2 < commit time of T1
      // In this case, T2 should see old versions created by T1 ( old version's
      // owner transaction is T1 )
      // so the expired transaction should before T1, transaction objects
      // created by transactions before T1 ( e.g. T0 ) could be cleared
      // ( which means they are expired for given lowTran )

      DPS_TRANS_ID globLowTranID ;
      DPS_TRANSID_SN globLowTran = _getGlobLowTran() ;

      globLowTranID.setNodeID( _TransIDH16 ) ;
      globLowTranID.setSN( globLowTran ) ;

      // We need global lowTran to calculate local expireTran, so we need
      // at least 2 rounds of lowTran requests to calculate global expireTran
      // - the first round to report local lowTran and calculate global lowTran
      // - the second round to calculate local expireTran with global lowTran,
      //   report to CATALOG, and calculate global expire lowTran
      // if global lowTran is invalid, it means the global lowTran has not been
      // calculated yet
      if ( DPS_INVALID_TRANSID_SN != globLowTran )
      {
         UINT64 lowTranTime = globLowTranID.getLogicalTime() ;
         DPS_TRANS_ID lastGlobExpireTran = getGlobExpireTran() ;

         // find out the oldest transactions (among all buckets) that
         // committed after global lowTran
         FOR_EACH_CMAP_BUCKET_S( TRANS_HIST_MAP, _histGlobMap )
         {
            // iterate from last global expireTran to the global lowTran, find
            // transactions with commit time greater than global lowTran
            // ( with a maximum time error for network delay consideration ),
            // and assign the minimum one for local expireTran
            for ( TRANS_HIST_MAP::map_iterator iter =
                        bucket.getMap().lower_bound( lastGlobExpireTran ) ;
                  bucket.end() != iter ;
                  ++ iter )
            {
               const DPS_TRANS_ID &histTransID = iter->first ;
               UINT64 commitTime = iter->second._preCommitTime.getTime() ;

               if ( DPS_TRANS_COMMIT != iter->second._status )
               {
                  // we only check commit transaction
                  continue ;
               }
               else if ( globLowTranID < histTransID )
               {
                  // end of searching, this transaction is after global lowTran
                  break ;
               }
               else if ( expireTran.isValid() &&
                         !( histTransID < expireTran ) )
               {
                  // end of searching, this transaction is after current
                  // expireTran calculated from other buckets
                  break ;
               }
               else if ( commitTime + STP_MAX_TIME_ERROR_US >= lowTranTime &&
                         ( expireTran.isInvalid() ||
                           histTransID < expireTran ) )
               {
                  // end of searching, find the minimum one
                  expireTran = histTransID ;
                  break ;
               }
            }
         }
         FOR_EACH_CMAP_BUCKET_END
      }

      // if no matched transactions found,
      // use global lowTran as local expireTran
      if ( expireTran.isInvalid() )
      {
         expireTran = globLowTranID ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETLOCALEXPTRAN ) ;

      return expireTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETEXPIREDVERSION, "dpsTransCB::getExpiredVersion" )
   DPS_TRANSID_SN dpsTransCB::getExpiredVersion()
   {
      DPS_TRANSID_SN expiredVersion = DPS_INVALID_TRANSID_SN ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETEXPIREDVERSION ) ;

      // no need to add consideration of maximum time error, already done
      // by CATALOG
      expiredVersion = (DPS_TRANSID_SN)( _globExpireTran.fetch() ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETEXPIREDVERSION ) ;

      return expiredVersion ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBTRANSTIME, "dpsTransCB::getGlobTransTime" )
   INT32 dpsTransCB::getGlobTransTime( stpLogicalTimeUS &time,
                                       INT32 timeout,
                                       INT32 *pWaitedTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBTRANSTIME ) ;

      // try to get time in timeout
      // NOTE: it might be failed if STP is busy with synchronization
      //       we could retry within a given timeout
      rc = _stpAgent.getLogicalTimeUS( time, timeout, TRUE, pWaitedTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETGLOBTRANSTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBTRANSTIME_EXP, "dpsTransCB::getGlobTransTime" )
   INT32 dpsTransCB::getGlobTransTime( pmdEDUCB *eduCB,
                                       UINT64 expectTimeUS,
                                       stpLogicalTimeUS &time,
                                       INT32 timeout )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBTRANSTIME_EXP ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;

      INT32 totalTimeout = 0 ;
      INT32 waitedTime = 0 ;

   retry:
      waitedTime = 0 ;

      // check if interrupted
      PD_CHECK( !eduCB->isInterrupted(), SDB_APP_INTERRUPT, error, PDERROR,
                "Failed to get global logical time for pre-commit, "
                "it is interrupted" ) ;

      rc = getGlobTransTime( time, timeout, &waitedTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get global transaction time, "
                   "rc: %d", rc ) ;

      if ( 0LL == expectTimeUS )
      {
         // no need to wait
         goto done ;
      }
      else if ( expectTimeUS > time.getTime() )
      {
         totalTimeout += waitedTime ;
         if ( timeout >= 0 && totalTimeout > timeout )
         {
            PD_LOG( PDWARNING, "Failed to get global transaction time, "
                    "it is timeout" ) ;
            rc = SDB_TIMEOUT ;
            goto error ;
         }
         else
         {
            // there is an interval to reach expecting time, sleep and retry
            UINT32 sleepTimeUS = expectTimeUS - time.getTime() ;
            if ( sleepTimeUS > STP_MAX_TIME_ERROR_US )
            {
               sleepTimeUS = STP_MAX_TIME_ERROR_US ;
            }
            INT32 sleepTime = STP_MICROSEC_TO_MILLISEC( sleepTimeUS ) ;
            totalTimeout += sleepTime ;
            ossSleep( sleepTime ) ;
            goto retry ;
         }
      }
      else if ( expectTimeUS == time.getTime() )
      {
         // we are close to expecting time, retry immediately
         goto retry ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETGLOBTRANSTIME_EXP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBPRECOMMITTIME, "dpsTransCB::getGlobPreCommitTime" )
   INT32 dpsTransCB::getGlobPreCommitTime( pmdEDUCB *eduCB,
                                           stpLogicalTimeUS &preCommitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBPRECOMMITTIME ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;
      SDB_ASSERT( eduCB->isGlobTrans(), "should be in global transaction" ) ;

      UINT64 expectTimeUS = 0LL ;

      // we should commit transaction after an interval given by time error
      if ( !eduCB->isAutoCommitTrans() )
      {
         expectTimeUS = eduCB->getTransBeginTime().getUpperTime() ;
      }

      rc = getGlobTransTime( eduCB, expectTimeUS, preCommitTime,
                             (INT32)( eduCB->getTransTimeout() ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get global logical time for "
                   "pre-commit of transaction [%s], rc: %d",
                   dpsTransIDToString( eduCB->getTransID() ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETGLOBPRECOMMITTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETLOCALPRECOMMITTIME, "dpsTransCB::getLocalPreCommitTime" )
   INT32 dpsTransCB::getLocalPreCommitTime( const stpLogicalTimeUS &localTime,
                                            stpLogicalTimeUS &preCommitTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETLOCALPRECOMMITTIME ) ;

      // get upper bound of max read transaction ID
      UINT64 maxReadTran = _maxReadTran.fetch() ;

      if ( maxReadTran > localTime.getUpperTime() )
      {
         // In this case, if we don't defer the pre-commit time,
         // there could potentially be transaction (the one with maxReadTran)
         // eligible of reading the changes made by pre-committing transaction.
         // Need delay pre-commit time, make sure the new pre-commit time
         // must after the maxReadTran ( which indicates the latest started
         // transaction with upper time error )
         preCommitTime.setTime( maxReadTran ) ;
         preCommitTime.setTimeError( localTime.getTimeError() ) ;
      }
      else
      {
         preCommitTime = localTime ;
      }

      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETLOCALPRECOMMITTIME, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBCOMMITTIME, "dpsTransCB::getGlobCommitTime" )
   void dpsTransCB::getGlobCommitTime( pmdEDUCB *eduCB,
                                       stpLogicalTimeUS &commitTime )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBPRECOMMITTIME ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;
      SDB_ASSERT( eduCB->isGlobTrans(), "should be in global transaction" ) ;

      // global commit time should be >= global pre-commit time
      // NOTE: global pre-commit time might be delayed by DATA nodes, and
      //       conflict read transactions are waiting in DATA nodes, so we
      //       could use global pre-commit time directly
      commitTime = eduCB->getTransPreCommitTime() ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETGLOBPRECOMMITTIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETTRANSINFO_INFO, "dpsTransCB::getTransInfo" )
   BOOLEAN dpsTransCB::getTransInfo( const DPS_TRANS_ID &transID,
                                     dpsTransBackInfo &info )
   {
      BOOLEAN found = FALSE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETTRANSINFO_INFO ) ;

      DPS_TRANS_ID origTransID = transID.getOrigTransID() ;

      // firstly, find in running transactions, then, find in history
      // transactions
      if ( !_getTransInfo( origTransID, info ) )
      {
         dpsHisTransStatus histInfo ;
         if ( _getTransHistInfo( origTransID, histInfo ) )
         {
            info._lsn = histInfo._lsn ;
            info._status = histInfo._status ;
            info._beginTime = histInfo._beginTime ;
            info._preCommitTime = histInfo._preCommitTime ;
            info._commitTime = histInfo._commitTime ;
            found = TRUE ;
         }
         else
         {
            info._status = DPS_TRANS_UNKNOWN ;
         }
      }
      else
      {
         found = TRUE ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETTRANSINFO_INFO ) ;

      return found ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_CHECKGLOBTRANS, "dpsTransCB::checkGlobTrans" )
   INT32 dpsTransCB::checkGlobTrans( const DPS_TRANS_ID &transID,
                                     const stpLogicalTimeUS &beginTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_CHECKGLOBTRANS ) ;

      UINT64 activeTime = 0LL ;
      DPS_TRANS_ID globExpireTran = getGlobExpireTran() ;

      // only check with global transaction
      if ( !transID.isGlobTrans() )
      {
         goto done ;
      }

      // get primary active time
      activeTime = getPrimaryActiveTime() ;
      if ( DPS_INVALID_TRANS_TIME == activeTime )
      {
         // have a chance to retry if not set
         checkPrimaryActiveTime() ;
         activeTime = getPrimaryActiveTime() ;
      }
      // check if primary active time is valid
      PD_CHECK( DPS_INVALID_TRANS_TIME != activeTime,
                SDB_GLOB_TRANS_NOT_AVAILABLE, error, PDERROR,
                "Failed to get primary active time, it is invalid" ) ;

      // check transaction begin time against active time
      PD_CHECK( activeTime <= beginTime.getTime(),
                SDB_GLOB_TRANS_NOT_AVAILABLE, error, PDERROR,
                "Failed to check global transaction [%s], it is started "
                "on [%llu] which is before global transaction is activated "
                "in this node [%llu]", dpsTransIDToString( transID ).c_str(),
                beginTime.getTime(), activeTime ) ;

      // check global expireTran, make sure it is after global expireTran
      // NOTE: If the node to start transaction doesn't report expireTran for a
      //       while, it might be kicked out from global expireTran calculation.
      //       But if it can still send transaction requests to other nodes,
      //       we need to reject those requests
      PD_CHECK( globExpireTran.getLogicalTime() <= beginTime.getTime(),
                SDB_GLOB_TRANS_NOT_AVAILABLE, error, PDERROR,
                "Failed to check global transaction [%s], it had been passed "
                "by global expireTran [%s]",
                dpsTransIDToString( transID ).c_str(),
                dpsTransIDToString( globExpireTran ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_CHECKGLOBTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_DOARBITGLOBTRANS, "dpsTransCB::doArbitGlobTrans" )
   INT32 dpsTransCB::doArbitGlobTrans( _pmdEDUCB *eduCB,
                                       const DPS_TRANS_ID &readTransID,
                                       const DPS_TRANS_ID &writeTransID,
                                       DPS_TRANS_STATUS writeTransStatus,
                                       BOOLEAN forceLocal,
                                       BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_DOARBITGLOBTRANS ) ;

      SDB_ASSERT( NULL != eduCB, "EDUCB is invalid" ) ;

      visible = FALSE ;

      SDB_ASSERT( NULL != _gtsAgent, "GTS agent is invalid" ) ;
      PD_CHECK( NULL != _gtsAgent,
                SDB_GLOB_TRANS_NOT_AVAILABLE, error, PDERROR,
                "Failed to do arbitrate global transaction, "
                "GTS agent is invalid" ) ;

      rc = _gtsAgent->arbitGlobTrans( eduCB, readTransID, writeTransID,
                                      writeTransStatus, forceLocal,
                                      visible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do arbitrate for read "
                   "transaction [%s] against write transaction [%s] with "
                   "status [%s], rc: %d",
                   dpsTransIDToString( readTransID ).c_str(),
                   dpsTransIDToString( writeTransID ).c_str(),
                   dpsTransStatusToString( writeTransStatus ),
                   rc ) ;

      PD_LOG( PDDEBUG, "Arbitrate done for read transaction [%s] against "
              "write transaction [%s] with status [%s], visible: %s",
              dpsTransIDToString( readTransID ).c_str(),
              dpsTransIDToString( writeTransID ).c_str(),
              dpsTransStatusToString( writeTransStatus ),
              visible ? "TRUE" : "FALSE" ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_DOARBITGLOBTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ONARBITGLOBTRANS, "dpsTransCB::onArbitGlobTrans" )
   INT32 dpsTransCB::onArbitGlobTrans( const DPS_TRANS_ID &readTransID,
                                       const DPS_TRANS_ID &writeTransID,
                                       DPS_TRANS_STATUS writeTransStatus,
                                       BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ONARBITGLOBTRANS ) ;

      PD_LOG( PDDEBUG, "Begin arbitration: read transaction [%s] with "
              "write transaction [%s] status [%s]",
              dpsTransIDToString( readTransID ).c_str(),
              dpsTransIDToString( writeTransID ).c_str(),
              dpsTransStatusToString( writeTransStatus ) ) ;

      visible = FALSE ;

      if ( _TransIDH16 != readTransID.getNodeID() )
      {
         // read transaction is not from this node
         SDB_ASSERT( FALSE, "read transaction is not from this node" ) ;
         PD_LOG( PDWARNING, "Arbitrate read transaction [%s] is not from "
                 "this node [%u]", dpsTransIDToString( readTransID ).c_str(),
                 _TransIDH16 ) ;
      }
      else if ( !( readTransID.isGlobTrans() ) )
      {
         PD_LOG( PDWARNING, "Arbitrate read transaction [%s] is not global "
                 "transaction", dpsTransIDToString( readTransID ).c_str() ) ;
      }
      else if ( !( writeTransID.isGlobTrans() ) )
      {
         PD_LOG( PDWARNING, "Arbitrate write transaction [%s] is not global "
                 "transaction", dpsTransIDToString( writeTransID ).c_str() ) ;
      }
      else
      {
         // get read transaction's executor to do arbitration

         // get and lock bucket
         TRANS_CB_MAP::Bucket &bucket = _cbMap.getBucket( readTransID ) ;
         BUCKET_SLOCK( bucket ) ;

         // find EDU by transaction
         TRANS_CB_MAP::map_iterator iter = bucket.find( readTransID ) ;
         PD_CHECK( bucket.end() != iter,
                   SDB_DPS_TRANS_NO_TRANS, error, PDERROR,
                   "Failed to get EDUCB for transaction [%s], it is not found",
                   dpsTransIDToString( readTransID ).c_str() ) ;

         // do arbitrate with EDU ( need protected by bucket lock to avoid
         // ending transaction during arbitration
         rc = iter->second->getTransExecutor()->arbit( writeTransID,
                                                       writeTransStatus,
                                                       visible ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to arbitrate read transaction [%s] "
                      "with write transaction [%s], rc: %d",
                      dpsTransIDToString( readTransID ).c_str(),
                      dpsTransIDToString( writeTransID ).c_str(),
                      rc ) ;
      }

      PD_LOG( PDDEBUG, "Finish arbitration: read transaction [%s] with "
              "write transaction [%s] status [%s], visible: %s",
              dpsTransIDToString( readTransID ).c_str(),
              dpsTransIDToString( writeTransID ).c_str(),
              dpsTransStatusToString( writeTransStatus ),
              visible ? "TRUE" : "FALSE" ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_ONARBITGLOBTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__GETTRANSINFO_INFO, "dpsTransCB::_getTransInfo" )
   BOOLEAN dpsTransCB::_getTransInfo( const DPS_TRANS_ID &transID,
                                      dpsTransBackInfo &info )
   {
      BOOLEAN found = FALSE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__GETTRANSINFO_INFO ) ;

      // get and lock bucket
      TRANS_MAP::Bucket &bucket = _transMap.getBucket( transID ) ;
      BUCKET_SLOCK( bucket ) ;

      // find transaction
      TRANS_MAP::map_iterator iterTrans = bucket.find( transID ) ;
      if ( bucket.end() != iterTrans )
      {
         info._lsn = iterTrans->second._lsn ;
         info._status = (DPS_TRANS_STATUS)( iterTrans->second._status ) ;
         info._beginTime = iterTrans->second._beginTime ;
         info._preCommitTime = iterTrans->second._preCommitTime ;
         info._commitTime = iterTrans->second._commitTime ;
         found = TRUE ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB__GETTRANSINFO_INFO ) ;

      return found ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__GETTRANSHISTINFO_INFO, "dpsTransCB::_getTransHistInfo" )
   BOOLEAN dpsTransCB::_getTransHistInfo( const DPS_TRANS_ID &transID,
                                          dpsHisTransStatus &histInfo )
   {
      BOOLEAN found = FALSE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__GETTRANSHISTINFO_INFO ) ;

      // find and lock bucket
      // - for global transaction, we find from global history map
      // - for other transaction, we find from rollback history map
      TRANS_HIST_MAP::Bucket &bucket =
                  transID.isGlobTrans() ?
                              _histGlobMap.getBucket( transID ) :
                              _histRBMap.getBucket( transID ) ;

      BUCKET_SLOCK( bucket ) ;

      // find transaction
      TRANS_HIST_MAP::map_iterator iterHist = bucket.find( transID ) ;
      if ( iterHist != bucket.end() )
      {
         histInfo._status = iterHist->second._status ;
         histInfo._lsn = iterHist->second._lsn ;
         histInfo._beginTime = iterHist->second._beginTime ;
         histInfo._preCommitTime = iterHist->second._preCommitTime ;
         histInfo._commitTime = iterHist->second._commitTime ;
         found = TRUE ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB__GETTRANSHISTINFO_INFO ) ;

      return found ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SETPRIMARYACTIVETIME, "dpsTransCB::setPrimaryActiveTime" )
   void dpsTransCB::setPrimaryActiveTime()
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SETPRIMARYACTIVETIME ) ;

      INT32 rc = SDB_OK ;

      stpAgent timeAgent ;
      stpLogicalTimeUS activeTime ;

      // if I am not primary, or global transaction is not enabled,
      // we don't need to set primary active time
      if ( !pmdIsPrimary() ||
           !isGlobTransOn() )
      {
         goto done ;
      }

      // try get global logical time
      rc = timeAgent.getLogicalTimeUS( activeTime, 0, FALSE ) ;
      if ( SDB_OK == rc )
      {
         // set primary active time
         // NOTE: add max time error for network delay etc
         _primaryActiveTime.swap( activeTime.getTime() +
                                  STP_MAX_TIME_ERROR_US ) ;

         PD_LOG( PDEVENT, "Set primary active time: [%llu]",
                 activeTime.getTime() ) ;
      }
      else
      {
         PD_LOG( PDWARNING, "Failed to get logical time for primary active "
                 "time, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXIT( SDB_DPSTRANSCB_SETPRIMARYACTIVETIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_CHECKPRIMARYACTIVETIME, "dpsTransCB::checkPrimaryActiveTime" )
   void dpsTransCB::checkPrimaryActiveTime()
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_CHECKPRIMARYACTIVETIME ) ;

      INT32 rc = SDB_OK ;

      stpAgent timeAgent ;
      stpLogicalTimeUS activeTime ;

      // if I am not primary, or global transaction is not enabled,
      // we dont't need to check and set primary active time
      if ( !pmdIsPrimary() ||
           !isGlobTransOn() )
      {
         goto done ;
      }

      // if primary active time has already set,
      // no need to checking
      if ( isPrimaryActived() )
      {
         goto done ;
      }

      // try to get a global logical time in a short period
      // NOTE: if STP is unavailable temporarily, timeout > 0
      //       will trigger STP checking
      rc = timeAgent.getLogicalTimeUS( activeTime,
                                       STP_GET_TIME_RETRY_INTERVAL,
                                       FALSE ) ;
      if ( SDB_OK == rc )
      {
         // try set primary active time
         // NOTE: add max time error for network delay etc
         if ( _primaryActiveTime.compareAndSwap(
                     DPS_INVALID_TRANS_TIME,
                     ( activeTime.getTime() + STP_MAX_TIME_ERROR_US ) ) )
         {
            PD_LOG( PDEVENT, "Set primary active time: [%llu]",
                    activeTime.getTime() ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Failed to get logical time for primary active "
                 "time, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXIT( SDB_DPSTRANSCB_CHECKPRIMARYACTIVETIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_REGREADTRAN, "dpsTransCB::regReadTran" )
   void dpsTransCB::regReadTran( pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_REGREADTRAN ) ;

      if ( eduCB->isTransRR() )
      {
         dpsTransExecutor *executor = eduCB->getTransExecutor() ;
         if ( !( executor->hasRegReadTran() ) )
         {
            regReadTranTime( executor->getBeginTime().getUpperTime() ) ;
            executor->setRegReadTran( TRUE ) ;
         }

         // start a read operator, it is a good time to set expireTran cache
         executor->setExpireTranCache( getExpiredVersion() ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_REGREADTRAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETRESTOREWINDOW, "dpsTransCB::getRestoreWindow" )
   INT32 dpsTransCB::getRestoreWindow( UINT64 &minTime,
                                       UINT64 &maxTransCommitTime,
                                       UINT64 &restorePointTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETRESTOREWINDOW ) ;

      if ( SDB_ROLE_CATALOG == pmdGetDBRole() )
      {
         // CATALOG nodes do not have global transactions, so we need to
         // make the restore window from CATALOG covers full time interval
         minTime = DPS_MIN_TRANS_TIME ;
         maxTransCommitTime = DPS_MAX_TRANS_TIME ;
         restorePointTime = DPS_MAX_TRANS_TIME ;
      }
      else
      {
         dpsReplicaLogMgr *logMgr = sdbGetDPSCB()->getLogMgr() ;

         // block log writing
         ossScopedLock lock( logMgr->getWriteMutex() ) ;

         minTime = _minRecoverableTime ;
         maxTransCommitTime = _maxTransCommitTime ;
         restorePointTime = _restorePointTime ;
      }

      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETRESTOREWINDOW, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETMAXCOMMITTIMEBEFORE, "dpsTransCB::getMaxCommitTimeBefore" )
   INT32 dpsTransCB::getMaxCommitTimeBefore( DPS_LSN_OFFSET lsn,
                                             UINT64 &maxTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETMAXCOMMITTIMEBEFORE ) ;

      dpsReplicaLogMgr *logMgr = sdbGetDPSCB()->getLogMgr() ;
      dpsLogSummary summary ;
      BOOLEAN isValid = FALSE ;

      // block log writing
      ossScopedLock lock( logMgr->getWriteMutex() ) ;

      rc = logMgr->getCurrentSummary( lsn, summary, isValid ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get log summary for LSN [%llu], "
                   "rc: %d", lsn, rc ) ;

      if ( isValid )
      {
         // it is valid, get maximum transaction commit time from summary
         maxTime = summary._maxTransCommitTime ;
      }
      else
      {
         // invalid, use the maximum value of trnasaction time
         maxTime = DPS_MAX_TRANS_TIME ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETMAXCOMMITTIMEBEFORE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void dpsTransCB::onRegistered( const MsgRouteID &nodeID )
   {
      _TransIDH16 = (DPS_TRANSID_NODEID)( nodeID.columns.nodeID ) ;
   }

   void dpsTransCB::onPrimaryChange( BOOLEAN primary,
                                     SDB_EVENT_OCCUR_TYPE occurType )
   {
      // change to primary, start trans rollback
      if ( primary )
      {
         if ( SDB_EVT_OCCUR_BEFORE == occurType )
         {
            _doRollback = TRUE ;
            _rollbackEvent.reset() ;
         }
         else
         {
            startRollbackTask() ;

            // now is primary, ready to accept global transaction
            // need set the active time
            setPrimaryActiveTime() ;
         }
      }
      // change to secondary, stop trans rollback
      else
      {
         if ( SDB_EVT_OCCUR_AFTER == occurType )
         {
            stopRollbackTask( 0 ) ;
            termAllTrans() ;
         }
         else
         {
            // change to secondary, reset primary active time
            resetPrimaryActiveTime() ;
         }
      }
   }

   void dpsTransCB::setEventHandler( dpsTransEvent *pEventHandler )
   {
      _pEventHandler = pEventHandler ;
   }

   dpsTransEvent* dpsTransCB::getEventHandler()
   {
      return _pEventHandler ;
   }

   DPS_TRANS_ID dpsTransCB::getRollbackID( const DPS_TRANS_ID &transID )
   {
      return transID.getRollbackTransID() ;
   }

   DPS_TRANS_ID dpsTransCB::getTransID( const DPS_TRANS_ID &rollbackID )
   {
      return rollbackID.getOrigTransID() ;
   }

   BOOLEAN dpsTransCB::isHolding( _pmdEDUCB *eduCB, 
                                  INT8      &owningLockMode, 
                                  UINT32     logicCSID,
                                  UINT16     collectionID,
                                  const dmsRecordID *recordID )
   {
      BOOLEAN found = FALSE ;
      owningLockMode = DPS_TRANSLOCK_MAX ;
      if ( _isOn )
      {
         UINT32 refCount = 0 ;
         dpsTransLockId lockId( logicCSID, collectionID, recordID );
         found = _transLockMgr->isHolding( eduCB->getTransExecutor(),
                                           lockId, owningLockMode, 
                                           refCount ) ;
     
      }
      return found ;
   }

   BOOLEAN dpsTransCB::isRollback( const DPS_TRANS_ID &transID )
   {
      return transID.isRollback() ;
   }

   BOOLEAN dpsTransCB::isFirstOp( const DPS_TRANS_ID &transID )
   {
      return transID.isFirstOp() ;
   }

   void dpsTransCB::clearFirstOpTag( DPS_TRANS_ID &transID )
   {
      transID.clearFirstOp() ;
   }

   BOOLEAN dpsTransCB::isRBPending( const DPS_TRANS_ID &transID )
   {
      return transID.isRBPending() ;
   }

   BOOLEAN dpsTransCB::hasRBPendingTrans()
   {
      BOOLEAN hasRBPending = FALSE ;

      // iterate each elements in concurrent map to find rollback pending
      // transactions
      FOR_EACH_CMAP_ELEMENT_S( TRANS_MAP, _transMap )
      {
         if ( DPS_INVALID_LSN_OFFSET != it->second._curLSNWithRBPending )
         {
            hasRBPending = TRUE ;
            goto done ;
         }
      }
      FOR_EACH_CMAP_ELEMENT_END

   done:
      return hasRBPending ;
   }

   INT32 dpsTransCB::startRollbackTask()
   {
      INT32 rc = SDB_OK ;
      EDUID eduID = PMD_INVALID_EDUID ;
      pmdEDUMgr *pEduMgr = NULL ;
      _isNeedSyncTrans = FALSE ;
      _doRollback = TRUE ;
      _rollbackEvent.reset() ;
      pEduMgr = pmdGetKRCB()->getEDUMgr() ;
      eduID = pEduMgr->getSystemEDU( EDU_TYPE_DPSROLLBACK ) ;
      if ( PMD_INVALID_EDUID != eduID )
      {
         rc = pEduMgr->postEDUPost( eduID,
                                    PMD_EDU_EVENT_ACTIVE,
                                    PMD_EDU_MEM_NONE,
                                    NULL,
                                    _doRollbackID ) ;
      }
      else
      {
         rc = SDB_SYS ;
      }

      if ( rc )
      {
         _doRollback = FALSE ;
         _rollbackEvent.signalAll() ;
      }
      return rc ;
   }

   INT32 dpsTransCB::stopRollbackTask( UINT64 doRollbackID )
   {
      // only the same rollback ID can stop the task
      // or doRollback ID is 0, which means force to stop
      if ( 0 == doRollbackID || _doRollbackID == doRollbackID )
      {
         _doRollback = FALSE ;
         _rollbackEvent.signalAll() ;

         // if force to stop, increase the rollback ID
         if ( 0 == doRollbackID )
         {
            ++ _doRollbackID ;
         }
      }
      return SDB_OK ;
   }

   INT32 dpsTransCB::waitRollback( UINT64 millicSec )
   {
      return _rollbackEvent.wait( millicSec ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SVTRANSINFO, "dpsTransCB::updateTransInfo" )
   void dpsTransCB::updateTransInfo( const DPS_TRANS_ID &transID,
                                     DPS_LSN_OFFSET lsnOffset,
                                     INT32 status,
                                     const stpLogicalTimeUS &transTime,
                                     BOOLEAN checkRstPITWindow )
   {
      PD_TRACE_ENTRY ( SDB_DPSTRANSCB_SVTRANSINFO ) ;

      // we don't update transaction info in restore phase, it will be done
      // by initialization of dpsTransCB after restore
      if ( transID.isValid() &&
           !sdbGetDPSCB()->isInRestore() )
      {
         BOOLEAN transFinished = FALSE ;
         BOOLEAN rbPending = isRBPending( transID ) ;
         DPS_TRANS_ID origID = getTransID( transID ) ;
         dpsHisTransStatus histInfo ;

         // find and lock bucket
         TRANS_MAP::Bucket &bucket = _transMap.getBucket( origID ) ;
         BUCKET_XLOCK( bucket ) ;

         // get transaction
         TRANS_MAP::map_iterator iterTrans = bucket.find( origID ) ;

         if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
         {
            // invalid-lsn means the transaction is complete
            // need be moved to history map
            if ( iterTrans != bucket.end() )
            {
               // transaction is finished and need to be moved to history map
               transFinished = TRUE ;

               // prepare history transaction info
               histInfo._status = status ;
               histInfo._lsn = iterTrans->second._lsn ;
               histInfo._beginTime = iterTrans->second._beginTime ;
               if ( transID.isAutoCommit() )
               {
                  // auto-commit doesn't have pre-commit
                  // use given transaction time as pre-commit time
                  // use begin time error as pre-commit time error
                  histInfo._preCommitTime = transTime ;
                  histInfo._preCommitTime.setTimeError(
                                    histInfo._beginTime.getTimeError() ) ;
                  histInfo._commitTime = transTime ;
                  histInfo._commitTime.setTimeError(
                                    histInfo._beginTime.getTimeError() ) ;
               }
               else
               {
                  // must have pre-commit, pre-commit time had saved
                  histInfo._preCommitTime = iterTrans->second._preCommitTime ;
                  histInfo._preCommitTime.setTimeError(
                                    histInfo._beginTime.getTimeError() ) ;
                  // use given transaction time as commit time
                  histInfo._commitTime = transTime ;
                  histInfo._commitTime.setTimeError(
                                    histInfo._beginTime.getTimeError() ) ;
               }

               // check if still rollback pending
               if ( rbPending )
               {
                  // just check the tag, current pending LSN may not reset
                  // ( it should be reset by the tag )
                  PD_LOG( PDWARNING, "Transaction [%s] is still rollback "
                          "pending", dpsTransIDToString( origID ).c_str() ) ;
                  SDB_ASSERT( FALSE, "transaction is rollback pending" ) ;
               }

               // remove from transaction map
               bucket.erase( iterTrans ) ;
            }
         }
         else
         {
            // transaction is not completed yet
            if ( iterTrans != bucket.end() )
            {
               // found trans info
               updateTransInfo( iterTrans->second, status, lsnOffset,
                                rbPending ) ;

               if ( DPS_TRANS_WAIT_COMMIT == status &&
                    transID.isGlobTrans() )
               {
                  iterTrans->second._preCommitTime = transTime ;
                  // use begin time error as pre-commit time error
                  iterTrans->second._preCommitTime.setTimeError(
                              iterTrans->second._beginTime.getTimeError() ) ;
               }
            }
            else
            {
               // trans info is not found, create a new one
               SDB_ASSERT( !rbPending, "should not be rollback pending" ) ;

               try
               {
                  dpsTransBackInfo transInfo( lsnOffset, status ) ;
                  // the begin time is only valid for first operator of
                  // transaction
                  if ( transID.isFirstOp() && transID.isGlobTrans() )
                  {
                     transInfo._beginTime = transTime ;
                  }
                  bucket.getMap()[ origID ] = transInfo ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to add transaction info, "
                          "occur exception: %s", e.what() ) ;
               }
            }
         }

         /// add to his trans
         /// the transaction is finished, add this transaction into history
         // NOTE: must hold lock of transaction map, otherwise, during the gap,
         //       transaction will not be found from neither transaction map
         //       nor history map
         if ( transFinished )
         {
            addHisTrans( transID, histInfo, checkRstPITWindow ) ;
         }

         // update restore window if needed
         if ( checkRstPITWindow &&
              origID.isGlobTrans() &&
              DPS_TRANS_COMMIT == status )
         {
            updateRestoreWindow( transTime.getTime() ) ;
         }
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_SVTRANSINFO ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ADDTRANSINFO, "dpsTransCB::addTransInfo" )
   INT32 dpsTransCB::addTransInfo( DPS_TRANS_ID transID,
                                   DPS_LSN_OFFSET lsnOffset,
                                   INT32 status )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ADDTRANSINFO ) ;

      BOOLEAN rbPending = isRBPending( transID ) ;
      DPS_TRANS_ID origID = getTransID( transID ) ;

      // find and lock bucket
      TRANS_MAP::Bucket &bucket = _transMap.getBucket( origID ) ;
      BUCKET_XLOCK( bucket ) ;

      // check if transaction already exists
      TRANS_MAP::map_iterator iterTrans = bucket.find( origID ) ;
      if ( bucket.end() == iterTrans )
      {
         // not found in transaction map, insert a new one
         SDB_ASSERT( !rbPending, "should not be rollback pending" ) ;
         try
         {
            // it is means transaction is synchronous by log if transID
            // is exist
            bucket.getMap()[ origID ] = dpsTransBackInfo( lsnOffset,
                                                          status ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add transaction into transaction "
                    "map, occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      else
      {
         // found existing, update it
         updateTransInfo( iterTrans->second, status, lsnOffset, rbPending ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_ADDTRANSINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_UPDATETRANSINFO_INFO, "dpsTransCB::updateTransInfo" )
   void dpsTransCB::updateTransInfo( dpsTransBackInfo &transInfo,
                                     INT32 status,
                                     DPS_LSN_OFFSET lsn,
                                     BOOLEAN rbPending )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_UPDATETRANSINFO_INFO ) ;

      if ( rbPending )
      {
         // if it is rollback pending, save current relatedLSN as pendingLSN
         // and do not move transaction's LSN ( it is the last related LSN
         // which is not pending )
         // NOTE: in consult rollback mode, actually we don't know the last non
         // pending LSN to reset the transaction's LSN, but the consult
         // rollback will rollback to a non pending LSN, so it is safe to reset
         transInfo._curLSNWithRBPending = lsn ;
         transInfo._curNonPendingLSN.insert( lsn ) ;
      }
      else
      {
         // if it is not rollback pending, reset current pending LSN if needed
         transInfo._lsn = lsn ;
         if ( DPS_INVALID_LSN_OFFSET != transInfo._curLSNWithRBPending )
         {
            transInfo._curLSNWithRBPending = DPS_INVALID_LSN_OFFSET ;
            transInfo._curNonPendingLSN.clear() ;
         }
      }
      transInfo._status = status ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB_UPDATETRANSINFO_INFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_UPDATETRANSSTATUS, "dpsTransCB::updateTransStatus" )
   INT32 dpsTransCB::updateTransStatus( const DPS_TRANS_ID &transID,
                                        INT32 status )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_UPDATETRANSSTATUS ) ;

      DPS_TRANS_ID origID = getTransID( transID ) ;

      // find and lock bucket
      TRANS_MAP::Bucket &bucket = _transMap.getBucket( origID ) ;
      BUCKET_XLOCK( bucket ) ;

      // find transaction by original transaction ID
      TRANS_MAP::map_iterator iterTrans = bucket.find( origID ) ;

      // check if transaction is found
      PD_CHECK( bucket.end() != iterTrans,
                SDB_DPS_TRANS_NO_TRANS, error, PDERROR,
                "Failed to get status for transaction [%s], rc: %d",
                dpsTransIDToString( transID ).c_str(), rc ) ;

      // set status
      iterTrans->second._status = status ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_UPDATETRANSSTATUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ADDTRANSCB, "dpsTransCB::addTransCB" )
   BOOLEAN dpsTransCB::addTransCB( const DPS_TRANS_ID &transID,
                                   _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ADDTRANSCB ) ;
      BOOLEAN hasInsert = FALSE ;

      DPS_TRANS_ID origID = getTransID( transID ) ;

      // find and lock bucket
      TRANS_CB_MAP::Bucket &bucket = _cbMap.getBucket( origID ) ;
      BUCKET_XLOCK( bucket ) ;

      // try to insert new transaction
      try
      {
         hasInsert = bucket.insert( make_pair( origID, eduCB ) ).second ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add transaction into cb map, "
                 "occur exception: %s", e.what() ) ;
         hasInsert = FALSE ;
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_ADDTRANSCB ) ;
      return hasInsert ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_DELTRANSCB, "dpsTransCB::delTransCB" )
   void dpsTransCB::delTransCB( const DPS_TRANS_ID &transID )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_DELTRANSCB ) ;

      DPS_TRANS_ID origID = getTransID( transID ) ;

      // find and lock bucket
      TRANS_CB_MAP::Bucket &bucket = _cbMap.getBucket( origID ) ;
      BUCKET_XLOCK( bucket ) ;

      // get transaction by original transaction ID
      TRANS_CB_MAP::map_iterator it = bucket.find( origID ) ;
      if ( it != bucket.end() )
      {
         bucket.erase( it ) ;
      }

      // update archived lowTran
      if ( transID.isGlobTrans() )
      {
         _archivedLowTran.swapGreaterThan( (UINT64)( transID.getGlobSN() ) ) ;
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_DELTRANSCB ) ;
   }

   void dpsTransCB::dumpTransEDUList( TRANS_EDU_LIST & eduList )
   {
      // for each element in concurrent map
      FOR_EACH_CMAP_ELEMENT_S( TRANS_CB_MAP, _cbMap )
      {
         // try add into EDU list
         try
         {
            eduList.push( it->second->getID() ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add EDU list, occur exception: %s",
                    e.what() ) ;
         }
      }
      FOR_EACH_CMAP_ELEMENT_END
   }

   void dpsTransCB::snapTransLockWaiterLRB( DPS_TX_WAIT_LRB_SET & txWaiterLRBSet )
   {
      // for each element in concurrent map
      FOR_EACH_CMAP_ELEMENT_S( TRANS_CB_MAP, _cbMap )
      {
         dpsTransExecutor *exe = it->second->getTransExecutor();
         dpsTxWaitLRB waitInfo;

         waitInfo.eduID = it->second->getID() ;
         if ( exe && exe->getTransWaitingLRBInfo( waitInfo ) )
         {
            try
            {
               txWaiterLRBSet.insert( waitInfo );
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR,
                       "Failed to collect waiter LRB, exception captured: %s",
                       e.what() ) ;
               break ;
            }
         }
      }
      FOR_EACH_CMAP_ELEMENT_END
   }

   UINT32 dpsTransCB::getTransMapSize()
   {
      // need lock to get size
      return _transMap.size( TRUE ) ;
   }

   void dpsTransCB::removeTrans( const DPS_TRANS_ID &transID )
   {
      DPS_TRANS_ID origID = getTransID( transID ) ;

      // find and lock bucket
      TRANS_MAP::Bucket &bucket = _transMap.getBucket( origID ) ;
      BUCKET_XLOCK( bucket ) ;

      // remove transaction by original transaction ID
      bucket.erase( origID ) ;
   }

   void dpsTransCB::cloneTransMap( TRANS_DUMP_MAP &result )
   {
      // iterate each elements in concurrent map
      FOR_EACH_CMAP_ELEMENT_S( TRANS_MAP, _transMap )
      {
         result[ it->first ] = it->second ;
      }
      FOR_EACH_CMAP_ELEMENT_END
   }

   UINT32 dpsTransCB::getTransCBSize ()
   {
      // need lock to get size
      return _cbMap.size( TRUE ) ;
   }

   void dpsTransCB::clearTransInfo()
   {
      _beginLsnIdMap.clear();
      _idBeginLsnMap.clear();
      _transMap.clear( TRUE ) ;
      _cbMap.clear( TRUE ) ;
      clearHisTrans() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ROLLBACKTRANSINFO, "dpsTransCB::rollbackTransInfoFromLog" )
   INT32 dpsTransCB::rollbackTransInfoFromLog( _dpsLogWrapper *dpsCB,
                                               const DPS_LSN &expectLSN )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ROLLBACKTRANSINFO ) ;

      DPS_LSN currentLSN ;

      if ( !isTransOn() ||
           isNeedSyncTrans() ||
           NULL == dpsCB ||
           expectLSN.invalid() )
      {
         goto done ;
      }

      currentLSN.offset = dpsCB->getCurrentLsn().offset ;
      if ( DPS_INVALID_LSN_OFFSET == currentLSN.offset )
      {
         goto done ;
      }

      {
         dpsMessageBlock mb( DPS_MSG_BLOCK_DEF_LEN ) ;
         // reverse loop LSN until meets expected LSN
         while ( currentLSN.compareOffset( expectLSN ) >= 0 &&
                 !isNeedSyncTrans() )
         {
            dpsLogRecord record ;
            mb.clear() ;
            rc = dpsCB->search( currentLSN, &mb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to search LSN [ offset: %llu ], "
                         "rc: %d", expectLSN.offset, rc ) ;
            record.load( mb.startPtr() ) ;

            if ( !rollbackTransInfoFromLog( record ) )
            {
               setIsNeedSyncTrans( TRUE ) ;
            }

            currentLSN.offset = record.head()._preLsn ;
         }
      }

      PD_LOG( PDEVENT, "Finished rollback trans info, current LSN [%llu], "
              "expect LSN [%llu]", currentLSN.offset, expectLSN.offset ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_ROLLBACKTRANSINFO, rc ) ;
      return rc ;

   error:
      setIsNeedSyncTrans( TRUE ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ROLLBACKTRANSINFOFROMLOG_REC, "dpsTransCB::rollbackTransInfoFromLog" )
   BOOLEAN dpsTransCB::rollbackTransInfoFromLog( const dpsLogRecord &record )
   {
      BOOLEAN ret = TRUE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ROLLBACKTRANSINFOFROMLOG_REC ) ;

      DPS_LSN_OFFSET lsnOffset = DPS_INVALID_LSN_OFFSET ;
      DPS_TRANS_ID transID ;
      INT32 transStatus = DPS_TRANS_DOING ;
      stpLogicalTimeUS transTime ;

      if ( SDB_OK != dpsGetTransIDFromRecord( record, transID ) )
      {
         // Failed to get transaction ID from record
         // ( maybe it is not in transaction )
         goto done ;
      }

      if ( transID.isValid() )
      {
         // transaction ID is valid
         dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_PRETRANS ) ;
         if ( !itr.valid() )
         {
            lsnOffset = DPS_INVALID_LSN_OFFSET ;
         }
         else
         {
            lsnOffset = *((DPS_LSN_OFFSET *)itr.value()) ;
         }

         if ( LOG_TYPE_TS_COMMIT == record.head()._type )
         {
            DPS_LSN_OFFSET firstLsn = DPS_INVALID_LSN_OFFSET ;
            UINT8 attr = 0 ;

            itr = record.find( DPS_LOG_PUBLIC_FIRSTTRANS ) ;

            if ( DPS_INVALID_LSN_OFFSET == lsnOffset ||
                 !itr.valid() )
            {
               // In the old version, commit log have not pre trans lsn and
               // first trans lsn. So, we can't rollback trans info
               ret = FALSE ;
               goto done ;
            }
            firstLsn = *( ( DPS_LSN_OFFSET*)itr.value() ) ;

            itr = record.find( DPS_LOG_TSCOMMIT_ATTR ) ;
            if ( itr.valid() )
            {
               attr = *(UINT8*)itr.value() ;
            }

            if ( DPS_TS_COMMIT_ATTR_PRE != attr )
            {
               addBeginLsn( firstLsn, transID ) ;
               transStatus = DPS_TS_COMMIT_ATTR_SND == attr ?
                             DPS_TRANS_WAIT_COMMIT : DPS_TRANS_DOING ;
               delHisTrans( transID ) ;
            }
         }
         else if ( !isRollback( transID ) )
         {
            if ( isFirstOp( transID ) )
            {
               delBeginLsn( transID ) ;
            }
         }
         else // is rollback
         {
            DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
            itr = record.find( DPS_LOG_PUBLIC_RELATED_TRANS ) ;
            if ( !itr.valid() )
            {
               // In the old version, rollback log have not related trans lsn,
               // so, we can't rollback trans info correctly
               ret = FALSE ;
               goto done ;
            }
            relatedLsn = *( ( DPS_LSN_OFFSET*)itr.value() ) ;
            if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
            {
               // In the last rollback trans lsn, pre trans lsn is invalid
               addBeginLsn( relatedLsn, transID ) ;
               transStatus = DPS_TRANS_ROLLBACK ;
               delHisTrans( transID ) ;
            }
            lsnOffset = relatedLsn ;
         }

         updateTransInfo( transID, lsnOffset, transStatus, transTime, FALSE ) ;
      }

   done:
      PD_TRACE_EXIT( SDB_DPSTRANSCB_ROLLBACKTRANSINFOFROMLOG_REC ) ;

      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG, "dpsTransCB::saveTransInfoFromLog" )
   void dpsTransCB::saveTransInfoFromLog( const dpsLogRecord &record,
                                          BOOLEAN checkRstPITWindow )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG ) ;

      DPS_LSN_OFFSET lsnOffset = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID ;
      INT32 transStatus = DPS_TRANS_DOING ;
      stpLogicalTimeUS transTime ;

      if ( SDB_OK != dpsGetTransIDFromRecord( record, transID ) )
      {
         // Failed to get transaction ID from record
         // ( maybe it is not in transaction )
         if ( checkRstPITWindow &&
              LOG_TYPE_DUMMY != record.head()._type )
         {
            // for irreversible operators, push restore window
            // NOTE: dummy logs are meaningless, and only used to fullfill the
            // log files, so skip dummy logs
            pushRestoreWindow() ;
         }
         goto done ;
      }

      if ( checkRstPITWindow && !transID.isGlobTrans() )
      {
         // operators in non-global transactions are irreversible as well
         pushRestoreWindow() ;
      }

      if ( transID.isValid() )
      {
         if ( isRollback( transID ) )
         {
            transStatus = DPS_TRANS_ROLLBACK ;
            dpsLogRecord::iterator itr =
                                    record.find( DPS_LOG_PUBLIC_PRETRANS ) ;
            if ( !itr.valid() )
            {
               lsnOffset = DPS_INVALID_LSN_OFFSET ;
            }
            else
            {
               lsnOffset = *((DPS_LSN_OFFSET *)itr.value()) ;
            }
         }
         else if ( LOG_TYPE_TS_COMMIT == record.head()._type )
         {
            dpsLogRecord::iterator itr = record.find( DPS_LOG_TSCOMMIT_ATTR ) ;
            if ( !itr.valid() ||
                 DPS_TS_COMMIT_ATTR_PRE != *(UINT8*)itr.value() )
            {
               lsnOffset = DPS_INVALID_LSN_OFFSET ;
               transStatus = DPS_TRANS_COMMIT ;
            }
            else
            {
               lsnOffset = record.head()._lsn ;
               transStatus = DPS_TRANS_WAIT_COMMIT ;
            }

            if ( transID.isGlobTrans() )
            {
               dpsGetTransTimeFromRecord( record, transID, transTime ) ;
            }
         }
         else
         {
            lsnOffset = record.head()._lsn ;
            if ( isFirstOp( transID ) )
            {
               addBeginLsn( lsnOffset, transID ) ;

               if ( transID.isGlobTrans() )
               {
                  dpsGetTransTimeFromRecord( record, transID, transTime ) ;
               }
            }
         }

         if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
         {
            delBeginLsn( transID ) ;
         }
         updateTransInfo( transID, lsnOffset, transStatus, transTime,
                          checkRstPITWindow ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG ) ;
      return ;
   }

   void dpsTransCB::addHisTrans( const DPS_TRANS_ID &transID,
                                 const dpsHisTransStatus &histInfo,
                                 BOOLEAN checkRstPITWindow )
   {
      if ( transID.isGlobTrans() )
      {
         // transaction is global, save to global transaction map
         /// NOTE: will be gc by lowTran/expiredTran
         DPS_TRANS_ID origID = transID.getOrigTransID() ;

         // find and lock bucket
         TRANS_HIST_MAP::Bucket &bucket = _histGlobMap.getBucket( origID ) ;
         BUCKET_XLOCK( bucket ) ;

         // try insert transaction
         try
         {
            bucket.getMap()[ origID ] = histInfo ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add transaction info to history "
                    "map, error: %s", e.what() ) ;
         }
      }
      else if ( DPS_TRANS_COMMIT != histInfo._status &&
                !transID.isAutoCommit() )
      {
         // transaction is rollback and not global, save to rollback map
         DPS_TRANS_ID origID = transID.getOrigTransID() ;
         INT32 bucketIndex = _histRBMap.getIndex( origID ) ;

         TRANS_HIST_MAP::Bucket &bucket = _histRBMap.getBucketAt( bucketIndex ) ;
         BUCKET_XLOCK( bucket ) ;

         try
         {
            bucket.getMap()[ origID ] = histInfo ;
            _histRBLSNMap[ bucketIndex ][ histInfo._lsn ] = origID ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add transaction info to rollback "
                    "history map, error: %s", e.what() ) ;
         }
      }
   }

   void dpsTransCB::delHisTrans( const DPS_TRANS_ID &transID )
   {
      DPS_TRANS_ID origID = transID.getOrigTransID() ;

      if ( origID.isGlobTrans() )
      {
         // global transaction, remove from global history map

         // find and lock bucket
         TRANS_HIST_MAP::Bucket &bucket = _histGlobMap.getBucket( origID ) ;
         BUCKET_XLOCK( bucket ) ;

         // remove transaction
         bucket.erase( origID ) ;
      }
      else
      {
         // rollback transaction, remove from rollback transaction

         // find and lock bucket
         INT32 bucketIndex = _histRBMap.getIndex( origID ) ;
         TRANS_HIST_MAP::Bucket &bucket = _histRBMap.getBucketAt( bucketIndex ) ;
         BUCKET_XLOCK( bucket ) ;

         TRANS_HIST_MAP::map_iterator it = bucket.find( origID ) ;
         if ( it != bucket.end() )
         {
            // remove corresponding LSN item
            _histRBLSNMap[ bucketIndex ].erase( it->second._lsn ) ;
            // remove transaction
            bucket.erase( it ) ;
         }
      }
   }

   void dpsTransCB::clearHisTrans()
   {
      // clear rollback history map
      UINT32 bucketIndex = 0 ;
      // iterate each bucket in rollback history map
      FOR_EACH_CMAP_BUCKET_X( TRANS_HIST_MAP, _histRBMap )
      {
         bucket.clear() ;
         // clear corresponding LSN map
         _histRBLSNMap[ bucketIndex ].clear() ;
         ++ bucketIndex ;
      }
      FOR_EACH_CMAP_BUCKET_END

      if ( _isGlobTransOn )
      {
         // clear global history map
         // iterate each bucket in global history map
         FOR_EACH_CMAP_BUCKET_X( TRANS_HIST_MAP, _histGlobMap )
         {
            bucket.clear() ;
         }
         FOR_EACH_CMAP_BUCKET_END
      }
   }

   void dpsTransCB::clearOutDateHisTrans( DPS_LSN_OFFSET lsn )
   {
      if ( DPS_INVALID_LSN_OFFSET != lsn )
      {
         // clear expired rollback history transaction by given LSN
         UINT32 bucketIndex = 0 ;
         FOR_EACH_CMAP_BUCKET_X( TRANS_HIST_MAP, _histRBMap )
         {
            TRANS_LSN_ID_MAP &lsnMap = _histRBLSNMap[ bucketIndex ] ;
            TRANS_LSN_ID_MAP::iterator itLSN = lsnMap.begin() ;
            while( itLSN != lsnMap.end() )
            {
               // remove transactions whose DPS LSN is expired
               if ( itLSN->first < lsn )
               {
                  bucket.erase( itLSN->second ) ;
                  lsnMap.erase( itLSN++ ) ;
                  continue ;
               }
               break ;
            }
            ++ bucketIndex ;
         }
         FOR_EACH_CMAP_BUCKET_END
      }

      if ( _isGlobTransOn )
      {
         // clear expired global history transaction by expireTran
         DPS_TRANSID_SN expiredVersion = getExpiredVersion() ;
         if ( DPS_INVALID_TRANSID_SN != expiredVersion )
         {
            DPS_TRANS_ID expireTran ;
            expireTran.setSN( expiredVersion ) ;

            FOR_EACH_CMAP_BUCKET_X( TRANS_HIST_MAP, _histGlobMap )
            {
               // remove transactions who is expired against expireTran
               TRANS_HIST_MAP::map_iterator iterTrans =
                           bucket.getMap().lower_bound( expireTran ) ;
               if ( iterTrans != bucket.end() )
               {
                  // NOTE: erase range is [ begin, end )
                  bucket.erase( bucket.begin(), iterTrans ) ;
               }
            }
            FOR_EACH_CMAP_BUCKET_END
         }
      }
   }

   void dpsTransCB::addBeginLsn( DPS_LSN_OFFSET beginLsn,
                                 const DPS_TRANS_ID &transID )
   {
      SDB_ASSERT( beginLsn != DPS_INVALID_LSN_OFFSET, "invalid begin-lsn" ) ;
      SDB_ASSERT( transID.isValid(), "invalid transaction-ID" ) ;
      DPS_TRANS_ID origID = getTransID( transID );
      ossScopedLock _lock( &_lsnMapMutex, EXCLUSIVE ) ;
      try
      {
         _beginLsnIdMap[ beginLsn ] = origID ;
         _idBeginLsnMap[ origID ] = beginLsn ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add begin LSN to LSN map, error: %s",
                 e.what() ) ;
      }
   }

   void dpsTransCB::delBeginLsn( const DPS_TRANS_ID &transID )
   {
      DPS_TRANS_ID origID = getTransID( transID );
      ossScopedLock _lock( &_lsnMapMutex, EXCLUSIVE );
      DPS_LSN_OFFSET beginLsn;
      TRANS_ID_LSN_MAP::iterator iter = _idBeginLsnMap.find( origID ) ;
      if ( iter != _idBeginLsnMap.end() )
      {
         beginLsn = iter->second;
         _beginLsnIdMap.erase( beginLsn );
         _idBeginLsnMap.erase( origID );
      }
   }

   DPS_LSN_OFFSET dpsTransCB::getBeginLsn( const DPS_TRANS_ID &transID )
   {
      DPS_TRANS_ID origID = getTransID( transID ) ;
      ossScopedLock _lock( &_lsnMapMutex, SHARED ) ;
      TRANS_ID_LSN_MAP::iterator iter = _idBeginLsnMap.find( origID ) ;
      if ( iter != _idBeginLsnMap.end() )
      {
         return iter->second ;
      }
      return DPS_INVALID_LSN_OFFSET ;
   }

   DPS_LSN_OFFSET dpsTransCB::getOldestBeginLsn()
   {
      ossScopedLock _lock( &_lsnMapMutex, SHARED );
      if ( _beginLsnIdMap.size() > 0 )
      {
         return _beginLsnIdMap.begin()->first;
      }
      return DPS_INVALID_LSN_OFFSET;
   }

   BOOLEAN dpsTransCB::isNeedSyncTrans()
   {
      return _isNeedSyncTrans;
   }

   void dpsTransCB::setIsNeedSyncTrans( BOOLEAN isNeed )
   {
      _isNeedSyncTrans = isNeed;
   }

   INT32 dpsTransCB::syncTransInfoFromLocal( DPS_LSN_OFFSET beginLsn,
                                             BOOLEAN checkRestoreWindow )
   {
      INT32 rc = SDB_OK ;
      DPS_LSN curLsn ;
      curLsn.offset = beginLsn ;
      _dpsMessageBlock mb(DPS_MSG_BLOCK_DEF_LEN) ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      if ( !_isNeedSyncTrans || DPS_INVALID_LSN_OFFSET == beginLsn )
      {
         goto done;
      }

      // clear transaction info
      clearTransInfo() ;

      while ( curLsn.offset!= DPS_INVALID_LSN_OFFSET &&
              curLsn.compareOffset( dpsCB->expectLsn().offset ) < 0 )
      {
         mb.clear();
         rc = dpsCB->search( curLsn, &mb );
         PD_RC_CHECK( rc, PDERROR, "Failed to search %lld in dpsCB, rc=%d",
                      curLsn.offset, rc );
         _dpsLogRecord record ;
         rc = record.load( mb.readPtr() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load log record, rc=%d", rc );
         saveTransInfoFromLog( record, checkRestoreWindow ) ;
         curLsn.offset += record.head()._length;
      }
   done:
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_TERMALLTRANS, "dpsTransCB::termAllTrans" )
   void dpsTransCB::termAllTrans()
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_TERMALLTRANS ) ;

      // iterate each element in concurrent
      FOR_EACH_CMAP_ELEMENT_S( TRANS_CB_MAP, _cbMap )
      {
         it->second->postEvent( pmdEDUEvent( PMD_EDU_EVENT_TRANS_STOP ) ) ;
      }
      FOR_EACH_CMAP_ELEMENT_END

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_TERMALLTRANS );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_TRANSLOCKGETX, "dpsTransCB::transLockGetX" )
   INT32 dpsTransCB::transLockGetX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    _IContext *pContext,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_TRANSLOCKGETX ) ;

      PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKGETX, 
                 PD_PACK_UINT(logicCSID),
                 PD_PACK_UINT(collectionID) ) ;
      if ( recordID )
      {
         PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKGETX, 
                    PD_PACK_UINT(recordID->_extent),
                    PD_PACK_UINT(recordID->_offset) ) ;
      }

      if ( _isOn )
      {
         dpsTransLockId lockId( logicCSID, collectionID, recordID );
         rc =  _transLockMgr->acquire( eduCB->getTransExecutor(),
                                       lockId, DPS_TRANSLOCK_X,
                                       pContext, pdpsTxResInfo,
                                       callback );

         if ( eduCB->getTransExecutor()->hasLockWait() )
         {
            eduCB->getTransExecutor()->finishLockWait() ;

            if ( eduCB->getMonQueryCB() )
            {
               eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
            }
         }
      }

      PD_TRACE_EXITRC ( SDB_DPSTRANSCB_TRANSLOCKGETX, rc );
      return rc ;
   }


   INT32 dpsTransCB::transLockGetU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    _IContext *pContext,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return rc ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_U,
                                   pContext, pdpsTxResInfo,
                                   callback ) ;

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_TRANSLOCKGETS, "dpsTransCB::transLockGetS" )
   INT32 dpsTransCB::transLockGetS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    _IContext *pContext,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback,
                                    BOOLEAN useEscalation )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_TRANSLOCKGETS ) ;

      PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKGETS, 
                 PD_PACK_UINT(logicCSID),
                 PD_PACK_UINT(collectionID) ) ;

      if ( recordID )
      {
         PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKGETS, 
                    PD_PACK_UINT(recordID->_extent),
                    PD_PACK_UINT(recordID->_offset) ) ;
      }

      if ( _isOn )
      {
         dpsTransLockId lockId( logicCSID, collectionID, recordID );
         rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                      lockId, DPS_TRANSLOCK_S,
                                      pContext, pdpsTxResInfo,
                                      callback, NULL, useEscalation );

         if ( eduCB->getTransExecutor()->hasLockWait() )
         {
            eduCB->getTransExecutor()->finishLockWait() ;

            if ( eduCB->getMonQueryCB() )
            {
               eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB_DPSTRANSCB_TRANSLOCKGETS, rc ) ;
      return rc ;
   }


   INT32 dpsTransCB::transLockGetIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     _IContext *pContext,
                                     dpsTransRetInfo * pdpsTxResInfo )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return rc ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_IX,
                                   pContext, pdpsTxResInfo );

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }
      return rc ;
   }


   INT32 dpsTransCB::transLockGetIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     _IContext *pContext,
                                     dpsTransRetInfo * pdpsTxResInfo )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return rc ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_IS,
                                   pContext, pdpsTxResInfo );

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_TRANSLOCKRELEASE, "dpsTransCB::transLockRelease" )
   void dpsTransCB::transLockRelease( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      _dpsITransLockCallback * callback,
                                      BOOLEAN forceRelease,
                                      BOOLEAN releaseUpperLock )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_TRANSLOCKRELEASE ) ;

      PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKRELEASE,
                 PD_PACK_INT(logicCSID),
                 PD_PACK_INT(collectionID) ) ;
      if ( recordID )
      {
         PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKRELEASE,
                    PD_PACK_INT(recordID->_extent),
                    PD_PACK_INT(recordID->_offset) ) ;
      }

      if ( 0 != eduCB->getTransExecutor()->getLockCount( LOCKMGR_TRANS_LOCK ) )
      {
         dpsTransLockId lockId( logicCSID, collectionID, recordID );

         _transLockMgr->release( eduCB->getTransExecutor(),
                                 lockId, forceRelease, callback,
                                 releaseUpperLock ) ;
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_TRANSLOCKRELEASE );
      return ;
   }

   void dpsTransCB::transLockReleaseAll( _pmdEDUCB *eduCB,
                                         _dpsITransLockCallback * callback )
   {
      if ( 0 == eduCB->getTransExecutor()->getLockCount( LOCKMGR_TRANS_LOCK ) )
      {
         return ;
      }
      else
      {
         _transLockMgr->releaseAll( eduCB->getTransExecutor(), callback );
      }
   }

   BOOLEAN dpsTransCB::isTransOn() const
   {
      return _isOn ;
   }

   BOOLEAN dpsTransCB::isGlobTransOn() const
   {
      return _isGlobTransOn ;
   }

   BOOLEAN dpsTransCB::isGlobTransSyncCheck() const
   {
      return _isGlobTransSyncCheck ;
   }

   BOOLEAN dpsTransCB::isMVCCOn() const
   {
      return _isMVCCOn ;
   }

   BOOLEAN dpsTransCB::isRRSupported() const
   {
      return ( _isOn && _isGlobTransOn && _isMVCCOn ) ;
   }

   INT32 dpsTransCB::transLockTestS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID,
                                     dpsTransRetInfo * pdpsTxResInfo,
                                     _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_S,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo,
                                        callback );
   }


   INT32 dpsTransCB::transLockTestSPreempt( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                            UINT16 collectionID,
                                            const dmsRecordID *recordID,
                                            dpsTransRetInfo * pdpsTxResInfo,
                                            _dpsITransLockCallback * callback,
                                            BOOLEAN needUpperLock )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_S,
                                        TRUE, // preemptively test
                                        pdpsTxResInfo,
                                        callback,
                                        needUpperLock );
   }


   INT32 dpsTransCB::transLockTestIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      dpsTransRetInfo * pdpsTxResInfo )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_IS,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo );
   }

   INT32 dpsTransCB::transLockTestX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID,
                                     dpsTransRetInfo * pdpsTxResInfo,
                                     _dpsITransLockCallback * callback,
                                     BOOLEAN needUpperLock )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_X,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo,
                                        callback,
                                        needUpperLock ) ;
   }


   INT32 dpsTransCB::transLockTestU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID,
                                     dpsTransRetInfo * pdpsTxResInfo,
                                     _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_U,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo,
                                        callback ) ;
   }

   INT32 dpsTransCB::transLockTestZ( _pmdEDUCB *eduCB,
                                     UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID,
                                     dpsTransRetInfo *pdpsTxResInfo,
                                     _dpsITransLockCallback *callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID ) ;
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                         lockId,
                                         DPS_TRANSLOCK_Z,
                                         FALSE,
                                         pdpsTxResInfo,
                                         callback ) ;
   }

   INT32 dpsTransCB::transLockTestIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      dpsTransRetInfo * pdpsTxResInfo )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_IX,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo );
   }


   INT32 dpsTransCB::transLockTryX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->tryAcquire( eduCB->getTransExecutor(),
                                       lockId, DPS_TRANSLOCK_X,
                                       pdpsTxResInfo, callback );
   }

   INT32 dpsTransCB::transLockTryZ( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID ) ;
      return _transLockMgr->tryAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_Z,
                                        pdpsTxResInfo, callback ) ;
   }

   INT32 dpsTransCB::transLockTryU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->tryAcquire( eduCB->getTransExecutor(),
                                       lockId, DPS_TRANSLOCK_U,
                                       pdpsTxResInfo,
                                       callback );
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_TRANSLOCKTRYS, "dpsTransCB::transLockTryS" )
   INT32 dpsTransCB::transLockTryS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_TRANSLOCKTRYS ) ;

      PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKTRYS, 
                 PD_PACK_UINT(logicCSID),
                 PD_PACK_UINT(collectionID) ) ;
      if ( recordID )
      {
         PD_TRACE2( SDB_DPSTRANSCB_TRANSLOCKGETS, 
                    PD_PACK_UINT(recordID->_extent),
                    PD_PACK_UINT(recordID->_offset) ) ;
      }

      if ( _isOn )
      {
         dpsTransLockId lockId( logicCSID, collectionID, recordID );
         rc = _transLockMgr->tryAcquire( eduCB->getTransExecutor(),
                                          lockId, DPS_TRANSLOCK_S,
                                          pdpsTxResInfo,
                                          callback ) ;
      }
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_TRANSLOCKTRYS, rc ) ;
      return rc ;
   }

   INT32 dpsTransCB::transLockTrySAgainstWrite( _pmdEDUCB *eduCB,
                                                UINT32 logicCSID,
                                                UINT16 collectionID,
                                                const dmsRecordID *recordID,
                                                dpsTransRetInfo *pdpsTxResInfo,
                                                _dpsITransLockCallback *callback )
   {
      INT32 rc = SDB_OK ;

      dpsTransRetInfo localTxResInfo ;
      dpsTransRetInfo *tmpTxResInfo =
            NULL != pdpsTxResInfo ? pdpsTxResInfo : &localTxResInfo ;

      // test Z lock first, if no write locks are being hold, ( especially
      // for self transaction with SIX lock ), then try acquire S lock
      rc = transLockTestZ( eduCB, logicCSID, collectionID, recordID,
                           tmpTxResInfo, callback ) ;
      if ( SDB_OK != rc &&
           DPS_TRANSLOCK_IS != tmpTxResInfo->_lockType &&
           DPS_TRANSLOCK_S != tmpTxResInfo->_lockType &&
           DPS_TRANSLOCK_U != tmpTxResInfo->_lockType )
      {
         return rc ;
      }

      return transLockTryS( eduCB, logicCSID, collectionID, recordID,
                            pdpsTxResInfo, callback ) ;
   }

   BOOLEAN dpsTransCB::transLockKillWaiters( UINT32 logicCSID,
                                             UINT16 collectionID,
                                             const dmsRecordID *recordID,
                                             INT32 errorCode )
   {
      BOOLEAN killed = FALSE ;

      if ( _isOn )
      {
         dpsTransLockId lockID( logicCSID, collectionID, recordID ) ;
         killed = _transLockMgr->killWaiters( lockID, errorCode ) ;
      }

      return killed ;
   }

   BOOLEAN dpsTransCB::transIsHolding( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                        UINT16 collectionID,
                                        const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return FALSE ;
      }

      dpsTransLockId lockId( logicCSID, collectionID, recordID ) ;

      while ( lockId.isValid() )
      {
         INT8 holdingMode = DPS_TRANSLOCK_MAX ;
         UINT32 refCount = 0 ;

         if ( _transLockMgr->isHolding( eduCB->getTransExecutor(),
                                        lockId,
                                        holdingMode,
                                        refCount ) )
         {

            return TRUE ;
         }

         // lock is not holding in this level, check upper
         if ( !lockId.isRootLevel() )
         {
            lockId = lockId.upOneLevel() ;
            continue ;
         }

         break ;
      }

      return FALSE ;
   }

   BOOLEAN dpsTransCB::hasWait( UINT32 logicCSID, UINT16 collectionID,
                                const dmsRecordID *recordID)
   {
      if ( !_isOn )
      {
         return FALSE ;
      }
      SDB_ASSERT( collectionID != DMS_INVALID_MBID, "invalid collectionID" ) ;
      SDB_ASSERT( recordID, "recordID can't be NULL" ) ;
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->hasWait( lockId );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETINCOMPTRANS, "dpsTransCB::getIncompTrans" )
   INT32 dpsTransCB::getIncompTrans( _pmdEDUCB *               cb,
                                     const dpsTransLockId &    lockID,
                                     const DPS_TRANSLOCK_TYPE  lockMode,
                                     BOOLEAN                   canSelfIncomp,
                                     DPS_TRANS_ID_SET &        incompTrans )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETINCOMPTRANS ) ;

      if ( !_isOn )
      {
         goto done ;
      }

      rc = _transLockMgr->getIncompTrans( lockID, lockMode, incompTrans ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get incompatible transactions for "
                   "lock [%s], lock mode [%s], rc: %d",
                   lockID.toString().c_str(), lockModeToString( lockMode ),
                   rc ) ;

      if ( !canSelfIncomp && NULL != cb && cb->isTransaction() )
      {
         DPS_TRANS_ID selfTransID = cb->getTransID().getOrigTransID() ;
         PD_CHECK( 0 == incompTrans.count( cb->getTransID().getOrigTransID() ),
                   SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, error, PDERROR,
                   "Failed to get incompatible transactions, "
                   "self [%s] is incompatible with lock mode [%s]",
                   dpsTransIDToString( selfTransID ).c_str(),
                   lockModeToString( lockMode ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETINCOMPTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   BOOLEAN dpsTransCB::getOldVerRecordTransID( UINT32 logicCSID,
                                               UINT16 collectionID,
                                               const dmsRecordID *recordID,
                                               DPS_TRANS_ID &transID )
   {
      BOOLEAN               found     = FALSE ;
      oldVersionContainer * oldVerPtr = NULL ;
      dpsLRBExtData       * pExtData  = NULL ;

      SDB_ASSERT( collectionID != DMS_INVALID_MBID, "invalid collectionID" ) ;
      SDB_ASSERT( recordID, "recordID can't be NULL" ) ;

      dpsTransLockId lockId( logicCSID, collectionID, recordID ) ;
      pExtData = _transLockMgr->getExtDataHdlByLockId( lockId ) ;
      if ( pExtData )
      {
         oldVerPtr = (oldVersionContainer*)(pExtData->_data) ;
         if ( oldVerPtr )
         {
            if (  oldVerPtr->getRecord() )
            {
               found   = TRUE ;
               transID = oldVerPtr->getRecordTransID() ;
            }
         }
      }   
      return found ; 
   }


   // Agorithm to decide if we have sufficent log space for new LR:
   // Support archive logging for transaction is the ultimate goal, meanwhile,
   // we will use following method if circular logging is in use:
   // 1. we will track two largest record size on each node.  These two number
   // has the LR lsn associated with it.  We can guarantee the larger one is
   // always greater or equal than the largest uncommitted record within the
   // system by only reduce it after a full cycle of all logs
   // 2. we use a _reservedRBSpace to count all outstanding LR space required
   // including potential rollback LRs.  This can be implemented as each
   // transaction keep track of it's reservedSpace, and only release those
   // space at commit or rollbacktime.  non transactional LR reserved space
   // can be released after the LR is written
   // 3. we use a _reservedRBSpace to count all unwritten LR space
   INT32 dpsTransCB::reservedLogSpace( UINT32 length, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      // undo log size has extra field, need to add the delta
      UINT32 rblength = length + DPS_TRANS_LOG_UNDO_DELTA ;

      if ( !_isOn || ( cb && cb->isInTransRollback() ) )
      {
         goto done ;
      }

      {
         _reservedSpace.add( length ) ;
         _reservedRBSpace.add( rblength ) ;
      }

      if ( remainLogSpace() == 0 )
      {
#ifdef _DEBUG
         UINT64 logFileSize = pmdGetOptionCB()->getReplLogFileSz() ;
         UINT32 logFileNum = pmdGetOptionCB()->getReplLogFileNum() ;
         DPS_LSN_OFFSET curLsnOffset =
                             pmdGetKRCB()->getDPSCB()->expectLsn().offset;
         DPS_LSN_OFFSET beginLsnOffset = getOldestBeginLsn();

         PD_LOG( PDWARNING, "Running out of log space, please commit existing "
                 "transactions. (length=%u, usedSize=%u, maxLRSize=%u, "
                 "beginlsn=%u, curlsn=%u, logFileSize=%u, logFileNum=%u, "
                 "remainLogSpace=%u) " ,
                 length, usedLogSpace(), getMaxLRSize(), beginLsnOffset,
                 curLsnOffset, logFileSize, logFileNum, remainLogSpace() ) ;
         printCounters( ) ;
#endif

         rc = SDB_DPS_LOG_FILE_OUT_OF_SIZE ;
         _reservedRBSpace.sub( rblength ) ;
         _reservedSpace.sub( length ) ;
         goto error ;
      }

      if ( NULL != cb && cb->isTransaction() )
      {
         rc = cb->checkLogSpace( length, rblength ) ;
         if ( SDB_OK != rc )
         {
            _reservedRBSpace.sub( rblength ) ;
            _reservedSpace.sub( length ) ;

            PD_LOG( PDERROR, "Failed to check log space for "
                    "transaction [%s], rc: %d",
                    dpsTransIDToString( cb->getTransID() ).c_str(), rc ) ;

            goto error ;
         }

         cb->addUsedSpace( length ) ;
      }

      cb->addReservedSpace( rblength ) ;
   done:
      return rc;
   error:
      goto done;
   }

   void dpsTransCB::releaseLogSpace( UINT32 length, _pmdEDUCB *cb )
   {
      if ( !_isOn )
      {
         return ;
      }
      else if ( NULL != cb && cb->isInTransRollback() )
      {
         // check transaction rollback, return DPS log space reserved for
         // rollback
         if ( !cb->isTakeOverTransRB() )
         {
            // NOTE: transaction RB pending may create larger DPS log records
            if ( _reservedRBSpace.fetch() < (UINT64)length ||
                 cb->getReservedSpace() < (UINT64)length )
            {
               PD_LOG( PDWARNING, "Reserved log space is not enough "
                       "for rollback transaction [%s], total reserved [%llu], "
                       "cb reserved [%llu], need [%u]",
                       dpsTransIDToString( cb->getTransID() ).c_str(),
                       _reservedRBSpace.fetch(), cb->getReservedSpace(),
                       length ) ;
            }
            else
            {
               _reservedRBSpace.sub( length ) ;
               cb->decReservedSpace( length ) ;
            }
         }
         return ;
      }

      _reservedSpace.sub( length ) ;

      // for non-transactional LR, we can release the reserved space
      if ( !cb->isTransaction() )
      {
         releaseRBLogSpace( cb ) ;
      }
   }

   void dpsTransCB::releaseRBLogSpace( _pmdEDUCB *cb )
   {
#ifdef _DEBUG
      SDB_ASSERT( _reservedRBSpace.fetch() >= cb->getReservedSpace(),
                  "should have enough reserved log space" ) ;
#endif
      _reservedRBSpace.sub( cb->getReservedSpace() ) ;
      cb->resetLogSpace() ;
   }

   UINT64 dpsTransCB::usedLogSpace()
   {
      DPS_LSN_OFFSET beginLsnOffset;
      DPS_LSN_OFFSET curLsnOffset;
      DPS_LSN curLsn;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB();
      UINT64 usedSize = 0;

      beginLsnOffset = getOldestBeginLsn();
      if ( DPS_INVALID_LSN_OFFSET == beginLsnOffset )
      {
         goto done;
      }
      curLsn = dpsCB->expectLsn() ;
      curLsnOffset = curLsn.offset ;
      if ( DPS_INVALID_LSN_OFFSET == curLsnOffset )
      {
         goto done;
      }

      beginLsnOffset = beginLsnOffset % _logFileTotalSize ;
      curLsnOffset = curLsnOffset % _logFileTotalSize ;
      usedSize = ( curLsnOffset + _logFileTotalSize - beginLsnOffset ) %
                   _logFileTotalSize ;
   done:
      return usedSize ;
   }

   // Calculate remaining log space based on current usage and predicted
   // waste space
   UINT64 dpsTransCB::remainLogSpace()
   {
      UINT64 remainSize = _logFileTotalSize ;
      UINT64 totalSpace = 0 ;
      UINT64 usedSize = 0 ;
      UINT64 logFileSize = 0 ;
      UINT32 logFileNum = 0 ;
      UINT32 adjust = 0 ;

      if ( !_isOn )
      {
         goto done ;
      }

      usedSize = usedLogSpace() ;
      if ( 0 == usedSize )
      {
         // no transaction
         goto done ;
      }

      // culation: based on current logspace usage to decide how much
      // space left. The rule of thumb: we can't overwrite
      // any logs from the oldest uncommitted transaction (lowtran).
      // usedSize: include LRs from uncommitted transaction, LRs from committed
      // transactions but held by lowtran, also LRs from nontransaction
      // _reservedRBSpace: contain space for undo LRs(rollback purpose) for all
      // the uncommitted transaction
      // _reservedSpace: contain space for all unwritten LRs
      // logFileSize: leave one log file as the beginLSN could be in the middle
      // of a log file, but we can't reuse the whole file
      // For the unused file, we should expect waste space for each of them,
      // the waste space is predicated base on current max LR size.
      // Free log space logic:
      // _logFileTotalSize - ( usedSize + logfilesize + length +
      //                      _reservedSpace + _reservedRBSpace +
      //                      (#totalfile - usedfile -1 )*max_record_size )
      logFileSize = pmdGetOptionCB()->getReplLogFileSz() ;
      logFileNum = pmdGetOptionCB()->getReplLogFileNum() ;
      // need a round down to guarantee enough waste space
      adjust = ( usedSize > logFileSize ) ?
               ( usedSize - logFileSize ) / logFileSize : 0 ;
      totalSpace =  usedSize + logFileSize + _reservedRBSpace.peek() +
                    _reservedSpace.peek() +
                    ( logFileNum - adjust - 1 ) * getMaxLRSize() ;

      if ( totalSpace < _logFileTotalSize )
      {
         remainSize = _logFileTotalSize - totalSpace ;
      }
      else
      {
         remainSize = 0 ;
      }

   done:
      return remainSize ;
   }

   UINT32 dpsTransCB::getMaxLRSize()
   {
      return ( 0 != _maxLRSize1 ) ? _maxLRSize1 : DMS_RECORD_MAX_SZ ;
   }

   // based on the input log record size and lsn, update the stored
   // maxLRSize as needed. Caller need to guarantee serialization.
   void  dpsTransCB::updateMaxLRSize( UINT32 recordSize,
                                      DPS_LSN_OFFSET recordLSN )
   {
      if ( _maxLRSize1 < recordSize )
      {
         _maxLRSize2 = 0 ;
         _maxLRLSN2 = DPS_INVALID_LSN_OFFSET ;

         _maxLRSize1 = recordSize ;
         _maxLRLSN1 = recordLSN ;
      }
      else
      {
         if ( _maxLRSize2 < recordSize )
         {
            _maxLRSize2 = recordSize ;
            _maxLRLSN2 = recordLSN ;
         }

         // when the recordLSN wrapped whole log files, we guarantee safe to
         // update the _maxLRSize1, reduce it to _maxLRSize2, and set its lsn
         // to the recordLSN
         if ( ( recordLSN - _maxLRLSN1 ) > _logFileTotalSize )
         {
#if SDB_INTERNAL_DEBUG
            PD_LOG( PDDEBUG, "Going to reduce maxLRSize1 size, "
                    "_maxLRLSN1=%u, recordLSN=%u ",
                    _maxLRLSN1, recordLSN );
            printCounters( ) ;
#endif
            _maxLRSize1 = _maxLRSize2 ;
            // this LR LSN is very close to curLSN
            _maxLRLSN1 = recordLSN ;
            _maxLRSize2 = 0 ;
            _maxLRLSN2 = DPS_INVALID_LSN_OFFSET ;
         }
      }
   }

   void  dpsTransCB::printCounters()
   {
#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Dumping dpsTransCB Log related counters: "
                       "_logFileTotalSize=%u, _reservedRBSpace=%u, "
                       "_reservedSpace=%u, "
                       "_maxLRSize1=%u, _maxLRLSN1=%u, "
                       "_maxLRSize2=%u, _maxLRLSN2=%u, ",
                       _logFileTotalSize,
                       _reservedRBSpace.peek(), _reservedSpace.peek(),
                       _maxLRSize1, _maxLRLSN1, _maxLRSize2, _maxLRLSN2 ) ;
#endif
   }

   dpsTransLockManager * dpsTransCB::getLockMgrHandle()
   {
      return ( _transLockMgr->isInitialized() ? ( _transLockMgr ) : NULL ) ;
   }

   ixmIndexLockManager * dpsTransCB::getIndexLockMgrHandle()
   {
      return ( _indexLockMgr->isInitialized() ? ( _indexLockMgr ) : NULL ) ;
   }

   void dpsTransCB::registerGTSAgent( _dpsGTSAgent *gtsAgent )
   {
      SDB_ASSERT( NULL != gtsAgent, "GTS agent is invalid" ) ;
      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must register in main thread" ) ;

      _gtsAgent = gtsAgent ;
   }

   void dpsTransCB::unregisterGTSAgent()
   {
      // should be unregister in main thread ( only assert here )
      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must unregister in main thread" ) ;
      _gtsAgent = NULL ;
   }

   // Set the rollback log limit cache
   void dpsTransCB::setLogLimitTime( UINT64 tim, UINT64 lim )
   {
      _rollbackLogTime  = tim ;
      _rollbackLogLimit = lim ;
   }

   // Reset the values for the rollback log limit cache
   void dpsTransCB::clearLogLimitTime()
   {
      _rollbackLogTime  = DPS_MAX_TRANSID_SN ;
      _rollbackLogLimit = DPS_MAX_TRANSID_SN ;
   }

   // Get the rollback log limit for the target time
   UINT64 dpsTransCB::getLogLimitTime( UINT64 tim )
   {
      if ( _rollbackLogTime <= tim )
      {
         // Cache only valid if the cached Time is earlier than the input,
         // indicating a previous scan went further than this one
         return _rollbackLogLimit ;
      }
      return DPS_MAX_TRANSID_SN ;
   }

   /*
      get global trans cb
   */
   dpsTransCB* sdbGetTransCB ()
   {
      static dpsTransCB s_transCB ;
      return &s_transCB ;
   }

   /*
      helper functions for dpsTransPendingKey
    */
   // check pending key and value
   // 1. should have OID
   // 2. should be owned
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSCHECKPENDING, "_dpsCheckTransPending" )
   static INT32 _dpsCheckTransPending( dpsTransPendingKey &pendingKey,
                                       dpsTransPendingValue &pendingValue )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSCHECKPENDING ) ;

      try
      {
         BSONElement element ;

         element = pendingKey._obj.getField( DMS_ID_KEY_NAME ) ;
         PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                   "Failed to check pending key, "
                   "OID is not found in BSON [%s]",
                   pendingKey._obj.toPoolString().c_str() ) ;
         pendingKey._obj = pendingKey._obj.getOwned() ;

         if ( !( pendingValue._obj.isEmpty() ) )
         {
            element = pendingValue._obj.getField( DMS_ID_KEY_NAME ) ;
            PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                      "Failed to check pending value, "
                      "OID is not found in BSON [%s]",
                      pendingValue._obj.toPoolString().c_str() ) ;
         }
         pendingValue._obj = pendingValue._obj.getOwned() ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Failed to check pending key, exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
      }

   done :
      PD_TRACE_EXITRC( SDB__DPSCHECKPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSADDTRANSPENDING, "dpsAddTransPending" )
   INT32 dpsAddTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                             dpsTransPendingKey &pendingKey,
                             dpsTransPendingValue &pendingValue,
                             BOOLEAN &added )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSADDTRANSPENDING ) ;

      MAP_TRANS_PENDING_OBJ::iterator iter ;

      added = FALSE ;

      rc = _dpsCheckTransPending( pendingKey, pendingValue ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check transaction rollback pending "
                   "key [ collection %s, key %s ], value [ %s ], rc: %d",
                   pendingKey._collection.c_str(),
                   pendingKey._obj.toPoolString().c_str(),
                   pendingValue._obj.toPoolString().c_str(), rc ) ;

      iter = pendingMap.find( pendingKey ) ;
      if ( iter != pendingMap.end() )
      {
         PD_LOG( PDDEBUG, "Find duplicated transaction rollback pending "
                 "object [ collection %s, key %s ],"
                 "old value [ %s ], "
                 "new value [ %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str(),
                 iter->second._obj.toPoolString().c_str(),
                 pendingValue._obj.toPoolString().c_str() ) ;
         // replace
         iter->second = pendingValue ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Insert transaction rollback pending "
                 "object [ collection %s, key %s ], value [ %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str(),
                 pendingValue._obj.toPoolString().c_str() ) ;
         pendingMap.insert( make_pair( pendingKey, pendingValue ) ) ;
      }

      added = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB__DPSADDTRANSPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRMTRANSPENDING, "dpsRemoveTransPending" )
   INT32 dpsRemoveTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                                const dpsTransPendingKey &pendingKey,
                                BSONObj *oldKey,
                                dpsTransPendingValue *oldValue,
                                BOOLEAN &removed )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSRMTRANSPENDING ) ;

      removed = FALSE ;

      MAP_TRANS_PENDING_OBJ::iterator iter =
                                       pendingMap.find( pendingKey ) ;
      if ( iter != pendingMap.end() )
      {
         PD_LOG( PDDEBUG, "Delete transaction rollback pending "
                 "object [ collection %s, key %s ], value [ %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str(),
                 iter->second._obj.toPoolString().c_str() ) ;

         if ( NULL != oldKey )
         {
            (*oldKey) = iter->first._obj ;
         }
         if ( NULL != oldValue )
         {
            oldValue->setValue( iter->second._obj, iter->second._opType ) ;
         }

         pendingMap.erase( iter ) ;
         removed = TRUE ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Failed to find transaction rollback pending "
                 "object for delete [ collection %s, key %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str() ) ;
      }

      PD_TRACE_EXITRC( SDB__DPSRMTRANSPENDING, rc ) ;

      return rc ;
   }

}
