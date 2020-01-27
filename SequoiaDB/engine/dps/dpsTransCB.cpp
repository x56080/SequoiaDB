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
#include "dpsUtil.hpp"

namespace engine
{

   static const DPS_TRANS_ID &_dpsGetMinGlobTran()
   {
      static DPS_TRANS_ID s_minGlobTran( DPS_TRANSID_MIN_GLOB_SN,
                                         DPS_INVALID_TRANSID_NODEID ) ;
      return s_minGlobTran ;
   }

   dpsTransCB::dpsTransCB()
   :_TransIDL56Cur( 1 ) ,
    _MapMutex( MON_LATCH_DPSTRANSCB_MAPMUTEX ),
    _CBMapMutex( MON_LATCH_DPSTRANSCB_CBMAPMUTEX ),
    _lsnMapMutex( MON_LATCH_DPSTRANSCB_LSNMAPMUTEX ),
    _hisMutex( MON_LATCH_DPSTRANSCB_HISMUTEX ),
    _maxFileSizeMutex( MON_LATCH_DPSTRANSCB_MAXFILESIZEMUTEX ),
    _reservedRBSpace( 0 ) ,
    _reservedSpace( 0 ),
    _sucCount( 0LL ),
    _errCount( 0LL ),
    _primaryActiveTime( DPS_INVALID_TRANS_TIME ),
    _globLowTran( DPS_INVALID_TRANSID_SN ),
    _globExpireTran( DPS_INVALID_TRANSID_SN ),
    _archivedLowTran( DPS_INVALID_TRANSID_SN ),
    _numTransIDConflict( 0LL )
   {
      _TransIDH16          = DPS_INVALID_TRANSID_NODEID ;
      _isOn                = FALSE ;
      _isGlobTransOn       = FALSE ;
      _isMVCCOn            = FALSE ;
      _doRollback          = FALSE ;
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
      DPS_LSN_OFFSET startLsnOffset = DPS_INVALID_LSN_OFFSET ;

      _isOn = pmdGetOptionCB()->transactionOn() ;
      _isGlobTransOn = pmdGetOptionCB()->globTransOn() ;
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
         UINT64 logFileSize = pmdGetOptionCB()->getReplLogFileSz() ;
         UINT32 logFileNum = pmdGetOptionCB()->getReplLogFileNum() ;
         _logFileTotalSize = logFileSize * logFileNum ;

         startLsnOffset = sdbGetDPSCB()->readOldestBeginLsnOffset() ;
         if ( _isOn && startLsnOffset != DPS_INVALID_LSN_OFFSET &&
              SDB_ROLE_STANDALONE != pmdGetDBRole() )
         {
            rc = syncTransInfoFromLocal( startLsnOffset ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to sync trans info from local, rc: %d",
                       rc ) ;
               goto error ;
            }
         }
         setIsNeedSyncTrans( FALSE ) ;

         // if have trans info, need log
         if ( getTransMap()->size() > 0 )
         {
            PD_LOG( PDEVENT, "Restored trans info, have %d trans not "
                    "be complete, the oldest lsn offset is %lld",
                    getTransMap()->size(), getOldestBeginLsn() ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsTransCB::active ()
   {
      return SDB_OK ;
   }

   INT32 dpsTransCB::deactive ()
   {
      PD_LOG( PDEVENT, "Counts of transID conflicts [%llu]",
              _numTransIDConflict ) ;
      return SDB_OK ;
   }

   INT32 dpsTransCB::fini ()
   {
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ISVERSIONVISIBLE, "dpsTransCB::isVersionVisible" )
   BOOLEAN dpsTransCB::isVersionVisible( const DPS_TRANS_ID &recTransID,
                                         const DPS_TRANS_ID &transID,
                                         const stpLogicalTimeUS &transBeginTime,
                                         TRANS_ISOLATION_LEVEL transIsolation )
   {
      BOOLEAN visible = FALSE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ISVERSIONVISIBLE ) ;

      if ( TRANS_ISOLATION_RR != transIsolation )
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
         visible = TRUE ;
      }
      else if ( recTransID.getOrigTransID() == transID.getOrigTransID() )
      {
         // the same transaction
         visible = TRUE ;
      }
      else if ( recTransID.getGlobSN() < transID.getGlobSN() )
      {
         DPS_TRANS_STATUS status ;
         stpLogicalTimeUS recBeginTime ;
         stpLogicalTimeUS recCommitTime ;

         getGlobTransInfo( recTransID, status, recBeginTime, recCommitTime ) ;

         if ( DPS_TRANS_WAIT_COMMIT == status ||
              DPS_TRANS_COMMIT == status )
         {
            // the transaction of record is committed, check with commit time
            // if current transaction started after record's transaction had
            // been committed, record is visible to current transaction
            // TODO: add time error in consideration
            if ( transBeginTime.getTime() > recCommitTime.getTime() )
            {
               visible = TRUE ;
            }
         }
         else if ( DPS_TRANS_UNKNOWN == status )
         {
            // if status is unknown, means transaction of record is before
            // lowTran, and status has been cleared
            visible = TRUE ;
         }
         else if ( DPS_TRANS_ROLLBACK == status )
         {
            // if rollbacked, record is actually restored previous version
            // which must contain a value older than current transaction
            visible = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_ISVERSIONVISIBLE ) ;

      return visible ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ISVERSIONEXPIRED, "dpsTransCB::isVersionExpired" )
   BOOLEAN dpsTransCB::isVersionExpired( const DPS_TRANS_ID &transID )
   {
      BOOLEAN expired = FALSE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ISVERSIONEXPIRED ) ;

      DPS_TRANSID_SN expiredVersion = getExpiredVersion() ;

      // NOTE: if expired lowTran is invalid, means the global lowTrans had
      //       not been calculated yet, so any version is not expired at this
      //       time
      if ( DPS_INVALID_TRANSID_SN != expiredVersion )
      {
         // check if the version(represented by transaction ID) is expired.
         // Expired means it's older than system expired version
         expired = ( transID.getGlobSN() < expiredVersion ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_ISVERSIONEXPIRED ) ;

      return expired ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBLOWTRAN, "dpsTransCB::getGlobLowTran" )
   DPS_TRANS_ID dpsTransCB::getGlobLowTran()
   {
      DPS_TRANS_ID lowTran ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBLOWTRAN ) ;

      // get global low transaction ID
      DPS_TRANSID_SN globTransID = _getGlobLowTran( 0 ) ;
      if ( DPS_INVALID_TRANSID_SN != globTransID )
      {
         lowTran.setNodeID( _TransIDH16 ) ;
         lowTran.setSN( globTransID ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETGLOBLOWTRAN ) ;

      return lowTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__GETGLOBLOWTRAN, "dpsTransCB::_getGlobLowTran" )
   DPS_TRANSID_SN dpsTransCB::_getGlobLowTran( INT32 timeError )
   {
      DPS_TRANSID_SN globLowTran = DPS_INVALID_TRANSID_SN ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__GETGLOBLOWTRAN ) ;

      // get global lowTran
      globLowTran = (DPS_TRANSID_SN)( _globLowTran.fetch() ) ;

      DPS_ADJUST_TRANSID_SN( globLowTran, timeError ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB__GETGLOBLOWTRAN ) ;

      return globLowTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBEXPIRETRAN, "dpsTransCB::getGlobExpireTran" )
   DPS_TRANS_ID dpsTransCB::getGlobExpireTran()
   {
      DPS_TRANS_ID expireTran ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBEXPIRETRAN ) ;

      // get global expire transaction ID
      DPS_TRANSID_SN globTransID = _getGlobExpireTran( 0 ) ;
      if ( DPS_INVALID_TRANSID_SN != globTransID )
      {
         expireTran.setNodeID( _TransIDH16 ) ;
         expireTran.setSN( globTransID ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETGLOBEXPIRETRAN ) ;

      return expireTran ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB__GETGLOBEXPTRAN, "dpsTransCB::_getGlobExpireTran" )
   DPS_TRANSID_SN dpsTransCB::_getGlobExpireTran( INT32 timeError )
   {
      DPS_TRANSID_SN globExpireTran = DPS_INVALID_TRANSID_SN ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB__GETGLOBEXPTRAN ) ;

      // get global expireTran
      globExpireTran = (DPS_TRANSID_SN)( _globExpireTran.fetch() ) ;

      DPS_ADJUST_TRANSID_SN( globExpireTran, timeError ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB__GETGLOBEXPTRAN ) ;

      return globExpireTran ;
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
         PD_LOG( PDDEBUG, "Set global lowTran [%llu(0x%llX)]",
                 tempLowTran, tempLowTran ) ;
      }
#if defined (_DEBUG)
      else
      {
         DPS_TRANSID_SN tempLowTran = _globLowTran.fetch() ;
         PD_LOG( PDDEBUG, "Got ignored global lowTran [%llu(0x%llX)], "
                 "current global lowTran [%llu(0x%llX)]",
                 globLowTran, globLowTran, tempLowTran, tempLowTran ) ;
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
         PD_LOG( PDDEBUG, "Set global expireTran [%llu(0x%llX)]",
                 tempExpireTran, tempExpireTran ) ;
      }
#if defined (_DEBUG)
      else
      {
         DPS_TRANSID_SN tempExpireTran = _globExpireTran.fetch() ;
         PD_LOG( PDDEBUG, "Got ignored global expireTran [%llu(0x%llX)], "
                 "current global expireTran [%llu(0x%llX)]",
                 globExpireTran, globExpireTran, tempExpireTran, tempExpireTran ) ;
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
      ossScopedLock _lock( &_CBMapMutex, SHARED ) ;

      // get first transaction
      TRANS_CB_MAP::iterator iterCB = _cbMap.upper_bound( minGlobTran ) ;
      if ( iterCB != _cbMap.end() )
      {
         lowTran = iterCB->first ;
      }

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

      // get global lowTran, minus maximum time error for network delay
      // consideration
      DPS_TRANS_ID globLowTranID ;
      DPS_TRANSID_SN globLowTran = _getGlobLowTran( -STP_MAX_TIME_ERROR_US ) ;

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
         DPS_TRANS_ID minGlobTran = _dpsGetMinGlobTran() ;

         ossScopedLock lock( &_hisMutex, SHARED ) ;

         // iterate from beginning of history with global transaction tag to
         // the global lowTran, find transactions with commit time greater
         // than global lowTran, and assign the minimum one for local
         // expireTran
         for ( TRANS_ID_2_STATUS::iterator iter =
                                 _hisTransStatus.upper_bound( minGlobTran ) ;
               _hisTransStatus.end() != iter ;
               ++ iter )
         {
            const DPS_TRANS_ID &histTransID = iter->first ;
            UINT64 commitTime = iter->second._commitTime.getTime() ;

            if ( globLowTranID < histTransID )
            {
               // end of searching, this transaction is after global lowTran
               break ;
            }
            else if ( commitTime - STP_MAX_TIME_ERROR_US > lowTranTime )
            {
               // end of searching, find the minimum one
               expireTran = histTransID ;
               break ;
            }
         }
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

      // NOTE: add consideration of maximum time error due to network delay etc
      expiredVersion = _getGlobExpireTran( -STP_MAX_TIME_ERROR_US ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETEXPIREDVERSION ) ;

      return expiredVersion ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBTRANSTIME, "dpsTransCB::getGlobTransTime" )
   INT32 dpsTransCB::getGlobTransTime( stpLogicalTimeUS &time,
                                       INT32 timeout )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBTRANSTIME ) ;

      stpAgent *agent = sdbGetRTNCB()->getSTPAgent() ;
      PD_CHECK( NULL != agent, SDB_GLOB_TRANS_NOT_AVAILABLE, error, PDERROR,
                "Failed to allocate transaction ID for global transaction, "
                "STP agent is not available" ) ;

      // try to get time in timeout
      // NOTE: it might be failed if STP is busy with synchronization
      //       we could retry within a given timeout
      rc = agent->getLogicalTimeUS( time, timeout ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_GETGLOBTRANSTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_GETGLOBTRANSINFO, "dpsTransCB::getGlobTransInfo" )
   void dpsTransCB::getGlobTransInfo( const DPS_TRANS_ID &transID,
                                      DPS_TRANS_STATUS &status,
                                      stpLogicalTimeUS &beginTime,
                                      stpLogicalTimeUS &commitTime )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_GETGLOBTRANSINFO ) ;

      DPS_TRANS_ID origID = transID.getOrigTransID() ;
      BOOLEAN found = FALSE ;

      // try to get from running transaction map
      _MapMutex.get_shared() ;

      TRANS_MAP::iterator iterTrans = _TransMap.find( origID ) ;
      if ( iterTrans != _TransMap.end() )
      {
         status = (DPS_TRANS_STATUS)( iterTrans->second._status ) ;
         beginTime = iterTrans->second._beginTime ;
         commitTime = iterTrans->second._commitTime ;
         found = TRUE ;
      }

      _MapMutex.release_shared() ;

      if ( !found )
      {
         // try to get from history transaction map
         _hisMutex.get_shared() ;

         TRANS_ID_2_STATUS::iterator iterHis = _hisTransStatus.find( origID ) ;
         if ( iterHis != _hisTransStatus.end() )
         {
            status = (DPS_TRANS_STATUS)( iterHis->second._status ) ;
            beginTime = iterHis->second._beginTime ;
            commitTime = iterHis->second._commitTime ;
            found = TRUE ;
         }

         _hisMutex.release_shared() ;
      }

      // not found, just report UNKNOWN
      if ( !found )
      {
         status = DPS_TRANS_UNKNOWN ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_GETGLOBTRANSINFO ) ;
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
                "Failed to check global transaction  [%s], it had been passed "
                "by global expireTran [%llu(0x%llX)]",
                dpsTransIDToString( transID ).c_str(),
                globExpireTran.getGlobSN(), globExpireTran.getGlobSN() ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_CHECKGLOBTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SETPRIMARYACTIVETIME, "dpsTransCB::setPrimaryActiveTime" )
   void dpsTransCB::setPrimaryActiveTime()
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SETPRIMARYACTIVETIME ) ;

      INT32 rc = SDB_OK ;

      stpLogicalTimeUS activeTime ;

      // if I am not primary, or global transaction is not enabled,
      // we don't need to set primary active time
      if ( !pmdIsPrimary() ||
           !isGlobTransOn() )
      {
         goto done ;
      }

      // try get global transaction time
      rc = getGlobTransTime( activeTime, 0 ) ;
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

      // try to get a global time in a short period
      // NOTE: if STP is unavailable temporarily, timeout > 0
      //       will trigger STP checking
      rc = getGlobTransTime( activeTime, 100 ) ;
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
            stopRollbackTask() ;
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
      ossScopedLock _lock( &_MapMutex, SHARED ) ;
      for ( TRANS_MAP::iterator iter = _TransMap.begin() ;
            iter != _TransMap.end() ;
            ++ iter )
      {
         if ( DPS_INVALID_LSN_OFFSET != iter->second._curLSNWithRBPending )
         {
            return TRUE ;
         }
      }
      return FALSE ;
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
         rc = pEduMgr->postEDUPost( eduID, PMD_EDU_EVENT_ACTIVE,
                                    PMD_EDU_MEM_NONE, NULL ) ;
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

   INT32 dpsTransCB::stopRollbackTask()
   {
      _doRollback = FALSE ;
      _rollbackEvent.signalAll() ;
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
                                     const stpLogicalTimeUS &transTime )
   {
      PD_TRACE_ENTRY ( SDB_DPSTRANSCB_SVTRANSINFO ) ;

      // we don't update transaction info in restore phase, it will be done
      // by initialization of dpsTransCB after restore
      if ( transID.isValid() &&
           !sdbGetDPSCB()->isInRestore() )
      {
         DPS_LSN_OFFSET lastLsn = DPS_INVALID_LSN_OFFSET ;
         BOOLEAN rbPending = isRBPending( transID ) ;
         DPS_TRANS_ID origID = getTransID( transID ) ;
         stpLogicalTimeUS beginTime, commitTime ;
         TRANS_MAP::iterator it ;

         ossScopedLock _lock( &_MapMutex, EXCLUSIVE ) ;

         it = _TransMap.find( origID ) ;

         if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
         {
            // invalid-lsn means the transaction is complete
            // need be moved to history map
            if ( it != _TransMap.end() )
            {
               lastLsn = it->second._lsn ;
               beginTime = it->second._beginTime ;
               if ( transID.isAutoCommit() )
               {
                  // auto-commit doesn't have pre-commit
                  commitTime = transTime ;
               }
               else
               {
                  // must have pre-commit, commit time had saved
                  commitTime = it->second._commitTime ;
               }

               if ( rbPending )
               {
                  // just check the tag, current pending LSN may not reset
                  // ( it should be reset by the tag )
                  PD_LOG( PDWARNING, "Transaction [%s] is still rollback "
                          "pending", dpsTransIDToString( origID ).c_str() ) ;
                  SDB_ASSERT( FALSE, "transaction is rollback pending" ) ;
               }
               _TransMap.erase( it ) ;
            }
         }
         else
         {
            if ( it != _TransMap.end() )
            {
               updateTransInfo( it->second, status, lsnOffset, rbPending ) ;

               if ( DPS_TRANS_WAIT_COMMIT == status &&
                    transID.isGlobTrans() )
               {
                  it->second._commitTime = transTime ;
                  // use begin time error as pre-commit time error
                  it->second._commitTime.setTimeError(
                        it->second._beginTime.getTimeError() ) ;
               }
            }
            else
            {
               SDB_ASSERT( !rbPending, "should not be rollback pending" ) ;
               _TransMap[ origID ] = dpsTransBackInfo( lsnOffset, status ) ;

               // the begin time is only valid for first operator of
               // transaction
               if ( transID.isFirstOp() && transID.isGlobTrans() )
               {
                  _TransMap[ origID ]._beginTime = transTime ;
               }
            }
         }

         /// add to his trans
         /// if we found last LSN, means the transaction is finished,
         /// add this transaction into history
         if ( DPS_INVALID_LSN_OFFSET != lastLsn )
         {
            addHisTrans( origID, status, lastLsn, beginTime, commitTime ) ;
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

      TRANS_MAP::iterator it ;

      BOOLEAN rbPending = isRBPending( transID ) ;
      DPS_TRANS_ID origID = getTransID( transID ) ;

      ossScopedLock _lock( &_MapMutex, EXCLUSIVE ) ;

      it = _TransMap.find( origID ) ;
      if ( it == _TransMap.end() )
      {
         SDB_ASSERT( !rbPending, "should not be rollback pending" ) ;
         try
         {
            // it is means transaction is synchronous by log if transID
            // is exist
            _TransMap[ origID ] = dpsTransBackInfo( lsnOffset, status ) ;
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
         updateTransInfo( it->second, status, lsnOffset, rbPending ) ;
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
   void dpsTransCB::updateTransStatus( DPS_TRANS_ID transID,
                                       INT32 status )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_UPDATETRANSSTATUS ) ;

      transID = getTransID( transID ) ;

      ossScopedLock _lock( &_MapMutex ) ;

      TRANS_MAP::iterator iter = _TransMap.find( transID ) ;
      if ( _TransMap.end() != iter )
      {
         iter->second._status = status ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_UPDATETRANSSTATUS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ADDTRANSCB, "dpsTransCB::addTransCB" )
   BOOLEAN dpsTransCB::addTransCB( const DPS_TRANS_ID &transID,
                                   _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ADDTRANSCB ) ;
      BOOLEAN hasInsert = FALSE ;
      {
         DPS_TRANS_ID origID = getTransID( transID ) ;
         ossScopedLock _lock( &_CBMapMutex, EXCLUSIVE ) ;
         hasInsert = _cbMap.insert( std::make_pair( origID, eduCB ) ).second ;
      }
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_ADDTRANSCB ) ;
      return hasInsert ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_DELTRANSCB, "dpsTransCB::delTransCB" )
   void dpsTransCB::delTransCB( const DPS_TRANS_ID &transID )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_DELTRANSCB ) ;

      TRANS_CB_MAP::iterator it ;
      DPS_TRANS_ID origID = getTransID( transID ) ;

      ossScopedLock _lock( &_CBMapMutex, EXCLUSIVE ) ;
      it = _cbMap.find( origID ) ;
      if ( it != _cbMap.end() )
      {
         _cbMap.erase( it ) ;
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
      ossScopedLock _lock( &_CBMapMutex, SHARED ) ;
      TRANS_CB_MAP::iterator iter = _cbMap.begin() ;
      while( iter != _cbMap.end() )
      {
         eduList.push( iter->second->getID() ) ;
         ++iter ;
      }
   }

   TRANS_MAP *dpsTransCB::getTransMap()
   {
      return &_TransMap;
   }

   void dpsTransCB::cloneTransMap( TRANS_MAP &result )
   {
      TRANS_MAP::iterator it = _TransMap.begin() ;
      while ( it != _TransMap.end() )
      {
         result[ it->first ] = it->second ;
         ++it ;
      }
   }

   UINT32 dpsTransCB::getTransCBSize ()
   {
      ossScopedLock _lock( &_CBMapMutex, SHARED ) ;
      return _cbMap.size() ;
   }

   void dpsTransCB::clearTransInfo()
   {
      _TransMap.clear();
      _cbMap.clear();
      _beginLsnIdMap.clear();
      _idBeginLsnMap.clear();

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

      // reverse loop LSN until meets expected LSN
      while ( currentLSN.compareOffset( expectLSN ) >= 0 &&
              !isNeedSyncTrans() )
      {
         dpsMessageBlock mb ;
         dpsLogRecord record ;

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

         updateTransInfo( transID, lsnOffset, transStatus, transTime ) ;
      }

   done:
      PD_TRACE_EXIT( SDB_DPSTRANSCB_ROLLBACKTRANSINFOFROMLOG_REC ) ;

      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG, "dpsTransCB::saveTransInfoFromLog" )
   void dpsTransCB::saveTransInfoFromLog( const dpsLogRecord &record )
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
         goto done ;
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

            if ( transID.isGlobTrans() &&
                 ( DPS_TRANS_WAIT_COMMIT == transStatus ||
                   transID.isAutoCommit() ) )
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
         updateTransInfo( transID, lsnOffset, transStatus, transTime ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG ) ;
      return ;
   }

   void dpsTransCB::addHisTrans( const DPS_TRANS_ID &transID,
                                 INT32 status,
                                 DPS_LSN_OFFSET lsn,
                                 const stpLogicalTimeUS &beginTime,
                                 const stpLogicalTimeUS &commitTime )
   {
      /// non-global auto-commit transaction don't need add to history list
      /// NOTE: we added committed and rollbacked transactions into history
      /// TODO: we need to clear with lowTran
      if ( !transID.isAutoCommit() || transID.isGlobTrans() )
      {
         DPS_TRANS_ID origID = transID.getOrigTransID() ;

         ossScopedLock lock( &_hisMutex, EXCLUSIVE ) ;
         _hisTransStatus[ origID ] = dpsHisTransStatus( status,
                                                        lsn,
                                                        beginTime,
                                                        commitTime ) ;
         _hisLsnTrans[ lsn ] = origID ;
      }
   }

   void dpsTransCB::delHisTrans( const DPS_TRANS_ID &transID )
   {
      DPS_TRANS_ID origID = transID.getOrigTransID() ;

      ossScopedLock lock( &_hisMutex, EXCLUSIVE ) ;
      TRANS_ID_2_STATUS::iterator it = _hisTransStatus.find( origID ) ;
      if ( it != _hisTransStatus.end() )
      {
         _hisLsnTrans.erase( it->second._lsn ) ;
         _hisTransStatus.erase( it ) ;
      }
   }

   void dpsTransCB::clearHisTrans()
   {
      ossScopedLock lock( &_hisMutex, EXCLUSIVE ) ;

      _hisLsnTrans.clear() ;
      _hisTransStatus.clear() ;
   }

   void dpsTransCB::clearOutDateHisTrans( DPS_LSN_OFFSET lsn )
   {
      TRANS_LSN_ID_MAP::iterator it ;
      DPS_TRANSID_SN expiredVersion = getExpiredVersion() ;

      if ( DPS_INVALID_LSN_OFFSET != lsn )
      {
         ossScopedLock lock( &_hisMutex, EXCLUSIVE ) ;

         it = _hisLsnTrans.begin() ;
         while( it != _hisLsnTrans.end() )
         {
            // history is expired in below cases
            // - DPS LSN is expired
            // - global transaction is older than expired global lowTran
            if ( it->first < lsn ||
                 ( it->second.isGlobTrans() &&
                   it->second.getGlobSN() < expiredVersion ) )
            {
               _hisTransStatus.erase( it->second ) ;
               _hisLsnTrans.erase( it++ ) ;
               continue ;
            }
            break ;
         }
      }
   }

   INT32 dpsTransCB::checkTransStatus( const DPS_TRANS_ID &transID,
                                       DPS_LSN_OFFSET & lsn )
   {
      /// first check in-line trans, then check history trans
      INT32 transStatus = DPS_TRANS_UNKNOWN ;
      // should use origin transaction ID
      DPS_TRANS_ID origID = transID.getOrigTransID() ;

      lsn = DPS_INVALID_LSN_OFFSET ;

      {
         ossScopedLock _lock( &_MapMutex, SHARED ) ;
         TRANS_MAP::iterator it = _TransMap.find( origID ) ;
         if ( it != _TransMap.end() )
         {
            transStatus = it->second._status ;
            lsn = it->second._lsn ;
            goto done ;
         }
      }

      {
         ossScopedLock lock( &_hisMutex, SHARED ) ;
         TRANS_ID_2_STATUS::iterator it = _hisTransStatus.find( origID ) ;
         if ( it != _hisTransStatus.end() )
         {
            transStatus = it->second._status ;
            lsn = it->second._lsn ;
            goto done ;
         }
      }

   done:
      return transStatus ;
   }

   void dpsTransCB::addBeginLsn( DPS_LSN_OFFSET beginLsn,
                                 const DPS_TRANS_ID &transID )
   {
      SDB_ASSERT( beginLsn != DPS_INVALID_LSN_OFFSET, "invalid begin-lsn" ) ;
      SDB_ASSERT( transID.isValid(), "invalid transaction-ID" ) ;
      DPS_TRANS_ID origID = getTransID( transID );
      ossScopedLock _lock( &_lsnMapMutex ) ;
      _beginLsnIdMap[ beginLsn ] = origID ;
      _idBeginLsnMap[ origID ] = beginLsn ;
   }

   void dpsTransCB::delBeginLsn( const DPS_TRANS_ID &transID )
   {
      DPS_TRANS_ID origID = getTransID( transID );
      ossScopedLock _lock( &_lsnMapMutex );
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
      ossScopedLock _lock( &_lsnMapMutex ) ;
      TRANS_ID_LSN_MAP::iterator iter = _idBeginLsnMap.find( origID ) ;
      if ( iter != _idBeginLsnMap.end() )
      {
         return iter->second ;
      }
      return DPS_INVALID_LSN_OFFSET ;
   }

   DPS_LSN_OFFSET dpsTransCB::getOldestBeginLsn()
   {
      ossScopedLock _lock( &_lsnMapMutex );
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

   INT32 dpsTransCB::syncTransInfoFromLocal( DPS_LSN_OFFSET beginLsn )
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
         saveTransInfoFromLog( record ) ;
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

      ossScopedLock _lock( &_CBMapMutex, SHARED );
      for ( TRANS_CB_MAP::iterator iterMap = _cbMap.begin() ;
            iterMap != _cbMap.end() ;
            ++ iterMap )
      {
         iterMap->second->postEvent( pmdEDUEvent(
                                     PMD_EDU_EVENT_TRANS_STOP ) ) ;
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_TERMALLTRANS );
   }


   INT32 dpsTransCB::transLockGetX( _pmdEDUCB *eduCB, UINT32 logicCSID,
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


   INT32 dpsTransCB::transLockGetS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    _IContext *pContext,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_S,
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


   void dpsTransCB::transLockRelease( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      _dpsITransLockCallback * callback )
   {
      if ( 0 == eduCB->getTransExecutor()->getLockCount( LOCKMGR_TRANS_LOCK ) )
      {
         return ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );

      return _transLockMgr->release( eduCB->getTransExecutor(),
                                     lockId, FALSE, callback );
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
                                            _dpsITransLockCallback * callback )
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
                                        callback );
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
                                     _dpsITransLockCallback * callback )
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
                                        callback ) ;
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


   INT32 dpsTransCB::transLockTryS( _pmdEDUCB *eduCB, UINT32 logicCSID,
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
                                       lockId, DPS_TRANSLOCK_S,
                                       pdpsTxResInfo,
                                       callback ) ;
   }


   BOOLEAN dpsTransCB::hasWait( UINT32 logicCSID, UINT16 collectionID,
                                const dmsRecordID *recordID)
   {
      if ( !_isOn )
      {
         return FALSE ;
      }
      SDB_ASSERT( collectionID!=DMS_INVALID_MBID, "invalid collectionID" ) ;
      SDB_ASSERT( recordID, "recordID can't be NULL" ) ;
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->hasWait( lockId );
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
      cb->addReservedSpace( rblength ) ;
   done:
      return rc;
   error:
      goto done;
   }

   void dpsTransCB::releaseLogSpace( UINT32 length, _pmdEDUCB *cb )
   {
      if ( !_isOn || ( cb && cb->isInTransRollback() ) )
      {
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
