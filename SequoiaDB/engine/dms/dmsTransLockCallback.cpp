/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = dmsTransLockCallback.cpp

   Descriptive Name = Data Management Service Lock Callback Functions

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/02/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsTransLockCallback.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "rtnIXScanner.hpp"
#include "utilLightJobBase.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "pmd.hpp"
#include "dpsUtil.hpp"
#include "dmsRBSSUMgr.hpp"

using namespace bson ;


namespace engine
{

   /*
      _dmsMemRecordRW implement
   */
   class _dmsMemRecordRW : public _dmsRecordRW
   {
      public:
         _dmsMemRecordRW( dpsOldRecordPtr ptr )
         :_dmsRecordRW()
         {
            if ( ptr.get() )
            {
               _isDirectMem = TRUE ;
               _ptr = ( const dmsRecord* )ptr.get() ;
            }
         }
   } ;
   typedef _dmsMemRecordRW dmsMemRecordRW ;

   /*
      _dmsReleaseLockJob define and implement
   */
   class _dmsReleaseLockJob : public _utilLightJob
   {
      public:
         _dmsReleaseLockJob( const dpsTransLockId &id,
                             oldVersionContainer *oldVer,
                             BOOLEAN isDiskDeleting )
         {
            _pOldVer = oldVer ;
            _isDiskDeleting = isDiskDeleting ;
         }
         virtual ~_dmsReleaseLockJob()
         {
            SDB_OSS_DEL _pOldVer ;
            _pOldVer = NULL ;
         }

         virtual const CHAR*     name() const
         {
            return "ReleaseLockJob" ;
         }
         virtual INT32           doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) ;

      private:
         oldVersionContainer    *_pOldVer ;
         BOOLEAN                 _isDiskDeleting ;
   } ;
   typedef _dmsReleaseLockJob dmsReleaseLockJob ;

   static void _dmsRemoveOldVerFromChain( oldVersionContainer *oldVer )
   {
      static dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      static oldVersionCB *oldCB = transCB->getOldVCB() ;

      oldVersionUnitPtr unitPtr ;
      unitPtr = oldCB->getOldVersionUnit( oldVer->getCSID(),
                                          oldVer->getCLID() ) ;
      /// when unitPtr.get() is null, means collection is dropped or truncated
      if ( unitPtr.get() )
      {
         unitPtr->removeFromChain( oldVer ) ;
      }
   }

   INT32 _dmsReleaseLockJob::doit( IExecutor *pExe,
                                   UTIL_LJOB_DO_RESULT &result,
                                   UINT64 &sleepTime )
   {
      static dpsTransCB *pTransCB = sdbGetTransCB() ;
      static SDB_DMSCB  *pDmsCB = sdbGetDMSCB() ;

      INT32 rcTmp = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *pContext = NULL ;
      dpsTransRetInfo transRetInfo ;
      sleepTime = 2000000 ;

      // This backgroud job, _dmsReleaseLockJob, is started by
      // dmsOnTransLockRelease() after it removed this old version
      // container from the chain.
      SDB_ASSERT( ( ! _pOldVer->isOnChain() ),
                  "This old version container must have been "
                  "removed from chain." ) ;

      /// release
      if ( _pOldVer->isRecordDeleted() )
      {
#ifdef _DEBUG
         const dmsRecordID &rid = _pOldVer->getRecordID() ;
         PD_LOG( PDDEBUG, "Removing rid(%d, %d) in from memory tree",
                 rid._extent, rid._offset ) ;
#endif

         _pOldVer->releaseRecord() ;
      }

      if ( !_isDiskDeleting || !pmdGetOptionCB()->recycleRecord() )
      {
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      /// lock collectionspace
      su = pDmsCB->suLock( _pOldVer->getCSID() ) ;
      if ( !su || su->LogicalCSID() != _pOldVer->getCSLID() )
      {
         /// collectionspace has dropped
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      /// lock collection
      if ( SDB_OK == su->data()->getMBContext( &pContext,
                                               _pOldVer->getCLID(),
                                               _pOldVer->getCLLID(),
                                               _pOldVer->getCLLID() ) )
      {
         rcTmp = pContext->mbTryLock( EXCLUSIVE ) ;
         if ( SDB_TIMEOUT == rcTmp )
         {
            result = UTIL_LJOB_DO_CONT ;
            goto done ;
         }
         else if ( rcTmp )
         {
            /// collection has dropped or truncated
            result = UTIL_LJOB_DO_FINISH ;
            goto done ;
         }
      }
      else
      {
         /// out-of-memory
         result = UTIL_LJOB_DO_CONT ;
         goto done ;
      }

      rcTmp = pTransCB->transLockTestX( (_pmdEDUCB*)pExe,
                                        _pOldVer->getCSLID(),
                                        _pOldVer->getCLID(),
                                        &_pOldVer->getRecordID(),
                                        &transRetInfo,
                                        NULL ) ;
      if ( SDB_OK == rcTmp )
      {
         result = UTIL_LJOB_DO_FINISH ;

         dmsRecordRW recordRW ;
         const dmsRecord *pRecord = NULL ;
         const dmsRecordID &rid = _pOldVer->getRecordID() ;

         recordRW = su->data()->record2RW( rid,
                                           _pOldVer->getCLID() ) ;
         recordRW.setNothrow( TRUE ) ;
         pRecord = recordRW.readPtr<dmsRecord>() ;
         if ( !pRecord || pRecord->getMyOffset() != rid._offset )
         {
            /// record not exist
            goto done ;
         }

         if ( !pRecord->isDeleting() )
         {
            /// record not deleting
            goto done ;
         }

         // record is not expired in mvcc
         if ( pmdGetOptionCB()->mvccOn() &&
              !pTransCB->isVersionExpired( pRecord->getGlobTransID() ) )
         {
            goto done ;
         }

         /// delete record
         su->data()->deleteRecord( pContext, rid,
                                   (ossValuePtr)pRecord,
                                   (pmdEDUCB*)pExe, NULL, NULL, NULL ) ;
      }
      else if ( DPS_TRANSLOCK_X == transRetInfo._lockType )
      {
         /// other trans locked it, will clear
         result = UTIL_LJOB_DO_FINISH ;
      }
      else
      {
         result = UTIL_LJOB_DO_CONT ;
      }

   done:
      if ( pContext )
      {
         su->data()->releaseMBContext( pContext ) ;
      }
      if ( su )
      {
         pDmsCB->suUnlock( su->CSID(), SHARED ) ;
      }
      return SDB_OK ;
   }

   static INT32 _dmsStartReleaseLockJob( const dpsTransLockId &lockId,
                                         oldVersionContainer *oldVer,
                                         BOOLEAN isDiskDeleting )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != oldVer, "old version is invalid" ) ;

      // oldVer might be destroyed after job submit
      // copy fields if we want to print into logs later
      INT32 csID = oldVer->getCSID() ;
      UINT16 clID = oldVer->getCLID() ;
      UINT32 csLID = oldVer->getCSLID() ;
      UINT32 clLID = oldVer->getCLLID() ;

      dmsReleaseLockJob *pJob = NULL ;
      pJob = SDB_OSS_NEW dmsReleaseLockJob( lockId, oldVer, isDiskDeleting ) ;
      SDB_ASSERT( pJob, "Job is NULL" ) ;
      if ( pJob )
      {
         UINT64 jobID = 0 ;
         rc = pJob->submit( TRUE, UTIL_LJOB_PRI_MID, UTIL_LJOB_DFT_AVG_COST,
                            &jobID ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Submit dmsReleaseLockJob(ID:%llu, CSID:%u, "
                    "CLID:%u, CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) "
                    "failed, rc: %d", jobID, csID, clID, csLID, clLID,
                    lockId.extentID(), lockId.offset(), rc) ;
         }
         else
         {
            PD_LOG( PDDEBUG, "Submit dmsReleaseLockJob(ID:%llu, CSID:%u, "
                    "CLID:%u, CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) "
                    "succeed", jobID, csID, clID, csLID, clLID,
                    lockId.extentID(), lockId.offset() ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Alloc dmsReleaseLockJob(CSID:%u, CLID:%u, "
                 "CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) failed",
                 csID, clID, csLID, clLID,
                 lockId.extentID(), lockId.offset() ) ;
         rc = SDB_OOM ;
      }

      return rc ;
   }

   /*
      Extend data function
   */
   BOOLEAN dmsIsExtendDataValid( const dpsLRBExtData *pExtData )
   {
      return 0 != pExtData->_data ? TRUE : FALSE ;
   }

   BOOLEAN dmsExtendDataReleaseCheck( const dpsLRBExtData *pExtData )
   {
      const oldVersionContainer *pOldVer = NULL ;
      pOldVer = ( const oldVersionContainer * )( pExtData->_data ) ;
      if ( pOldVer &&
           ( !pOldVer->isIndexObjEmpty() || pOldVer->isOnChain() ) )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   void dmsReleaseExtendData( dpsLRBExtData *pExtData )
   {
      oldVersionContainer *pOldVer = NULL ;
      pOldVer = ( oldVersionContainer * )( pExtData->_data ) ;
      if ( pOldVer )
      {
         SDB_OSS_DEL pOldVer ;
         pExtData->_data = 0 ;
      }
   }

   // Callback function pointer main body. This is actual callback
   // is done. There are two part of handling here:
   // 1. special case for nontransactional changes when mvcc is on
   // 2. general case to handle in memory old versions
   void dmsOnTransLockRelease( const dpsTransLockId &lockId,
                               DPS_TRANSLOCK_TYPE    lockMode,
                               UINT32                refCounter,
                               BOOLEAN               nonTransNeedCleanup,
                               dpsTransCB           *transCB,
                               pmdEDUCB             *eduCB,
                               dpsLRBExtData        *pExtData )
   {
      oldVersionContainer *oldVer   = NULL ;
      BOOLEAN isDiskDeleting = FALSE ;

      // When mvcc is enabled, transactions will store old version indexes
      // in the in memory idxTree. However, if a non transactional change
      // comes in, the result is immediately avaliable. Thus at the time
      // those changes were made, we need to get rid of all old version
      // tree nodes associate with the rid so that index scan would not
      // pick up any of those versions. 
      if( pmdGetOptionCB()->mvccOn()        &&
          eduCB->getTransID().isInvalid()  &&
          ( lockId.isLeafLevel() )          &&
          ( DPS_TRANSLOCK_X == lockMode )   &&
          nonTransNeedCleanup )   
      {
#ifdef _DEBUG
         PD_LOG( PDDEBUG, 
                 "Cleanup old indexes for rid[%s] in mvcc for nontrans change"
                 "refCounter=%d, pExtData=%p, _data=%p",
                 lockId.toString().c_str(), refCounter, pExtData,
                 pExtData ? pExtData->_data : NULL ) ;
#endif
         transCB->getOldVCB()->cleanIdxNodesForRecord( lockId.csID(),
                                                       lockId.clID(),
                                                       lockId.extentID(),
                                                       lockId.offset() ) ;
      }
      // early exit if this is not record lock OR not in X mode OR
      // there is no old record setup.
      if ( ( !lockId.isLeafLevel() )               ||
           ( DPS_TRANSLOCK_X != lockMode )         ||
           ( 0 != refCounter )                     ||
           ( NULL == pExtData )                    ||
           ( 0 == pExtData->_data ) )
      {

         goto done ;
      }

      oldVer = ( oldVersionContainer* )( pExtData->_data ) ;
      SDB_ASSERT( oldVer->getRecordID() ==
                  dmsRecordID(lockId.extentID(), lockId.offset()),
                  "LockID is not the same" ) ;

      if ( eduCB->isInTransRollback() )
      {
         // notify releaseRecord guy (could be here or LJ) to remove the old
         // version index from tree if the transaction is rolledback
         oldVer->setRolledback() ;
      }

      // Previously when creates index, takes old version container
      // from chain first, then inserts into index LID set of that
      // old version container and inserts index tree; while
      // tryReleaseRecord deletes from index tree and index LID set,
      // then removes old version container from chain. In case of
      // both create/rebuild index and releaseRecord running at the
      // same time, we may see unexpect status of index LID set
      // So, we remove old version container from the chain first
      // then releaseRecord when release a lock( dmsOnTransLockRelease,
      // _dmsReleaseLockJob::doit ); when creates an index, walks through
      // the chain and checks if the index LID is in that old version
      // container's index LID set and do insert index LID and index
      // tree ( insertIdxTree and insertWithOldVer ) if it is not there.
      // When traverse the chain, add or remove an old version container
      // to or from the chain, proper latch on oldVersionUnit will apply
      // to protect synchronized accessing.
      if ( oldVer->isOnChain() )
      {
         _dmsRemoveOldVerFromChain( oldVer ) ;
      }

      /// Should save isDiskDeleting before call tryReleaseRecord()
      isDiskDeleting = oldVer->isDiskDeleting() ;

      /// try relerase record, because maybe should wait index tree's lock,
      /// so use try
      if ( oldVer->tryReleaseRecord() )
      {
         // FIXME: remove after stable
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Delete old record for rid[%s] from memory",
                 lockId.toString().c_str() ) ;
#endif

         if ( !isDiskDeleting || !pmdGetOptionCB()->recycleRecord() )
         {
            goto done ;
         }
      }
      else
      {
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Set old record for rid[%s] in memory to deleted",
                 lockId.toString().c_str() ) ;
#endif
         oldVer->setRecordDeleted() ;
      }

      /// PUT the rid to backgroud task to recycle
      if ( SDB_OK == _dmsStartReleaseLockJob( lockId, oldVer, isDiskDeleting ) )
      {
         pExtData->_data = 0 ;
      }

   done:
      return ;
   }

   static BOOLEAN _dmsIsKeyUndefined( const BSONObj &keyObj )
   {
      BSONObjIterator itr( keyObj ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( Undefined != e.type() )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   dmsTransLockCallback::dmsTransLockCallback()
   {
      _transCB    = NULL ;
      _oldVer     = NULL ;
      _eduCB      = NULL ;
      _recordRW   = NULL ;
      _oldVerCB   = NULL ;
      _rbsMgr     = pmdGetKRCB()->getDMSCB()->getRBSSUMgr() ;

      _csLID      = ~0 ;
      _clLID      = ~0 ;
      _csID       = DMS_INVALID_SUID ;
      _clID       = DMS_INVALID_MBID ;
      _latchedIdxLid  = DMS_INVALID_EXTENT ;
      _transIsolation = TRANS_ISOLATION_MAX ;
      _nonTransNeedCleanup = FALSE ;
      _pScanner    = NULL ;

      clearStatus() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK, "dmsTransLockCallback::dmsTransLockCallback" )
   dmsTransLockCallback::dmsTransLockCallback( dpsTransCB *transCB,
                                               _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK ) ;

      _transCB    = transCB ;
      _oldVer     = NULL ;
      _eduCB      = eduCB ;
      _recordRW   = NULL ;
      _rbsRecordData = NULL ;
      _nonTransNeedCleanup = FALSE ;
      _oldVerCB   = transCB->getOldVCB() ;
      _rbsMgr     = pmdGetKRCB()->getDMSCB()->getRBSSUMgr() ;

      _csLID      = ~0 ;
      _clLID      = ~0 ;
      _csID       = DMS_INVALID_SUID ;
      _clID       = DMS_INVALID_MBID ;
      _latchedIdxLid = DMS_INVALID_EXTENT ;
      _pScanner      = NULL ;

      clearStatus() ;

      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK );
   }

   dmsTransLockCallback::~dmsTransLockCallback()
   {
   }

   void dmsTransLockCallback::setBaseInfo( dpsTransCB *transCB,
                                           _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( eduCB, "EDUCB can't be NULL" ) ;
      _transCB = transCB ;
      _eduCB   = eduCB ;
      if ( transCB )
      {
         _transIsolation = eduCB->getTransExecutor()->getTransIsolation() ;
      }
   }

   void dmsTransLockCallback::setIDInfo( INT32 csID, UINT16 clID,
                                         UINT32 csLID, UINT32 clLID )
   {
      _csID = csID ;
      _clID = clID ;
      _csLID = csLID ;
      _clLID = clLID ;
   }

   void dmsTransLockCallback::setIXScanner( _rtnIXScanner *pScanner )
   {
      _latchedIdxLid = pScanner->getIdxLID() ;
      _pScanner = pScanner ;
   }

   void dmsTransLockCallback::detachRecordRW()
   {
      _recordRW = NULL ;
      _rbsRecordData = NULL ;
   }

   void dmsTransLockCallback::attachRecordRW( _dmsRecordRW *recordRW,
                                              dmsRecordData * recordData )
   {
      _recordRW = recordRW ;
      _rbsRecordData = recordData ;
   }

   void dmsTransLockCallback::clearStatus()
   {
      _oldVer           = NULL ;
      _skipRecord       = FALSE ;
      _result           = SDB_OK ;
      _useOldVersion    = FALSE ;
      _nonTransNeedCleanup = FALSE ;
      _recordPtr        = dpsOldRecordPtr() ;
      _recordInfo.reset() ;
      _rbsRecordOffset.reset() ;
   }

   const dmsRBSOffset & dmsTransLockCallback::getRBSRecordOffset() 
   {
      return _rbsRecordOffset ; 
   }

   const dmsTransRecordInfo* dmsTransLockCallback::getTransRecordInfo() const
   {
      return &_recordInfo ;
   }

   DPS_TRANS_ID dmsTransLockCallback::getRecordTransID() 
   {
      DPS_TRANS_ID rv ;
      if ( _oldVer )
      {
         rv = _oldVer->getRecordTransID() ;
      }
      return rv ;
   }

   DPS_TRANS_ID dmsTransLockCallback::getOwnerTransID() 
   {
      DPS_TRANS_ID rv ;
      if ( _oldVer )
      {
         rv = _oldVer->getOwnerTransID().getOrigTransID() ;
      }
      return rv ;
   }

   // Description:
   //    Function called after lock acquirement.
   //
   // Input:
   //    lockId: lock id to operate on
   //    rc:  rc from the lock acquire
   //    requestLockMode: lock mode requested (IS/IX/S/U/X)
   //    opMode: lock operation mode (TRY/ACQUIRE/TEST)
   // Output:
   //    pdpsTxResInfo: return information about the lock
   // Note that _rbsRecordData is only setup from RBS, not the in memory version
   // Dependency:
   //    caller must hold lrb bucket latch
   // TODO: there could be some code cleanup in this function, including:
   //  duplicated code, logic maybe simplified, remove debug code...
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE, "dmsTransLockCallback::afterLockAcquire" )
   INT32 dmsTransLockCallback::afterLockAcquire
   (
      const dpsTransLockId       &lockId,
      INT32                       irc,
      DPS_TRANSLOCK_TYPE          requestLockMode,
      UINT32                      refCounter,
      DPS_TRANSLOCK_OP_MODE_TYPE  opMode,
      const dpsTransLRBHeader    *pLRBHeader,
      dpsLRBExtData              *pExtData
   )
   {
      INT32   rc                 = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE ) ;

      /// when not leaf level, do nothing
      if ( !lockId.isLeafLevel() )
      {
         goto done ;
      }

      if ( !pExtData )
      {
         goto done ;
      }

      SDB_ASSERT( ( DPS_TRANSLOCK_OP_MODE_TEST != opMode ),
                  "Test mode shouldn't have call back" ) ;

      if ( DPS_TRANSLOCK_OP_MODE_TEST == opMode )
      {
         goto done ;
      }

      // S lock and isolation RR
      if (  ( TRANS_ISOLATION_RR == _transIsolation ) &&
            ( DPS_TRANSLOCK_S == requestLockMode ) )

      {
         rc = _afterAcquireSLockRRread( lockId,
                                                irc,
                                                requestLockMode,
                                                refCounter,
                                                opMode,
                                                pLRBHeader,
                                                pExtData ) ;
      }
      // X,U lock or isolation RU, RC, RS
      else
      {
         rc = _afterAcquireUXLockOrNonRRread( lockId,
                                              irc,
                                              requestLockMode,
                                              refCounter,
                                              opMode,
                                              pLRBHeader,
                                              pExtData ) ;
      }
   done :
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE ) ;
      return rc ;
   }

   // Description:
   //    Function called after lock acquirement. There are two cases to handle:
   //
   // CASE 1: Under RC, TB scanner failed to get record lock in S mode due to
   // incompatibility, we'll try to use the saved old copy if it exist.
   // Note that normally tb scanner or idx scanner will wait on record lock
   // unless transaction isolation level is RC and translockwait is set to NO
   //
   // CASE 2: Update successfully acquire X record lock within a transaction,
   // we'll try to copy the data to lrbHrd if it's not already there
   // Note that we have decided to defer the copy to the actual update time,
   // because there are cases where X lock was acquired, but no update will
   // be made due to other critieras. However, we will do some preparation
   // work at this time.
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIREUXLOCKORNONRRREAD, "dmsTransLockCallback::_afterAcquireUXLockOrNonRRread" )
   INT32 dmsTransLockCallback::_afterAcquireUXLockOrNonRRread
   (
      const dpsTransLockId       &lockId,
      INT32                       irc,
      DPS_TRANSLOCK_TYPE          requestLockMode,
      UINT32                      refCounter,
      DPS_TRANSLOCK_OP_MODE_TYPE  opMode,
      const dpsTransLRBHeader    *pLRBHeader,
      dpsLRBExtData              *pExtData
   )
   {
      INT32   rc                 = SDB_OK ;
      BOOLEAN notTransOrRollback = FALSE  ;
      dmsRecordID rid( lockId.extentID(), lockId.offset() ) ;
#ifdef _DEBUG
      DPS_TRANS_ID transID       = _eduCB->getTransID() ;
#endif
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIREUXLOCKORNONRRREAD );

      clearStatus() ;

      /// not in transaction
      if ( _eduCB->getTransID().isInvalid() || _eduCB->isInTransRollback() )
      {
         notTransOrRollback = TRUE ;
      }

      //FIXME remove
#ifdef _DEBUG
      PD_LOG( PDDEBUG,
              "Begin check for rid(%d, %d), transid(%s), clLID(%d), irc(%d), "
              "requestLockMode(%s), notTransOrRollback(%d), ISO:%d, scanner:%s",
              rid._extent, rid._offset,
              dpsTransIDToString( transID ).c_str(),
              _clLID, irc, lockModeToString( requestLockMode ),
              notTransOrRollback,
              _transIsolation,
              ( (!_pScanner)
                ? "TBScan"
                : ( (SCANNER_TYPE_MEM_TREE == _pScanner->getCurScanType())
                    ? "Memory tree"
                    : "Disk index" ) ) ) ;
#endif

      // when roll back or transaction is not avaiable
      if ( notTransOrRollback )
      {
         if ( SDB_OK == irc )
         {
            _oldVer = NULL ;
         }
         goto done ;
      }

      // Handle case 1 mentioned above
      // When the update was done by non transaction session, it's
      // possible that the oldRecord does not exist. We do nothing
      // in that case
      if( (SDB_DPS_TRANS_LOCK_INCOMPATIBLE == irc ) &&
          (DPS_TRANSLOCK_S == requestLockMode)     &&
          (DPS_TRANSLOCK_OP_MODE_TRY == opMode) )
      {
         SDB_ASSERT( pExtData, "ExtData is invalid " ) ;

         if ( 0 == pExtData->_data )
         {
            goto done ;
         }

         _oldVer = (oldVersionContainer*)(pExtData->_data) ;
         SDB_ASSERT( _oldVer->getRecordID() == rid, "LockID is not the same" ) ;

         if ( _oldVer->isRecordDeleted() )
         {
            _oldVer = NULL ;
            // Don't set skip record here as the caller would wait on
            // lock to get the latest version
            goto done ;
         }

         _recordPtr = _oldVer->getRecordPtr() ;

         if ( _oldVer->isRecordNew() )
         {
            _skipRecord = TRUE ;
         }
         else if ( _recordPtr.get() && !_oldVer->isRecordDummy() )
         {
            // We need to re-verify the record with the
            // index again. Here is how this could happen:
            // Session 1 did update, changed index from 1 to 2, paused;
            // session 2 does index scan, searching for record with
            // index 2. It found the index on disk. Did get record lock
            // and ended up using the old version from memory. But
            // the old version record contain the index 1. We must verify
            // and skip this record.
            if ( _pScanner && _latchedIdxLid != DMS_INVALID_EXTENT &&
                 SCANNER_TYPE_DISK == _pScanner->getCurScanType() &&
                 _oldVer->idxLidExist( _latchedIdxLid ) )
            {
               _skipRecord = TRUE ;
               /// remove the duplicate rid
               _pScanner->removeDuplicatRID( _oldVer->getRecordID() ) ;
               goto done ;
            }

#ifdef _DEBUG
            PD_LOG( PDDEBUG, "Use old copy for rid[%s] from memory",
                    lockId.toString().c_str() ) ;
#endif
            // setup the buffer pointer in dmsRecordRW
            *_recordRW = dmsMemRecordRW( _recordPtr ) ;

            // set the return info if we successfully used old version
            _useOldVersion = TRUE ;
         }
      } // end of case 1

      // Handle case 2 mentioned above
      // X record lock request from a transaction need to prepare to set
      // up old copy if the copy is not already there
      else if ( SDB_OK == irc )
      {
         SDB_ASSERT( refCounter > 0, "Ref count must > 0" ) ;

         _recordInfo._refCount = refCounter ;

         /// from memory tree
         if ( _pScanner && _latchedIdxLid != DMS_INVALID_EXTENT &&
              SCANNER_TYPE_MEM_TREE == _pScanner->getCurScanType() )
         {
            _skipRecord = TRUE ;
            /// remove the duplicate rid
            _pScanner->removeDuplicatRID( rid ) ;
            _oldVer = NULL ;
            goto done ;
         }

         /// when pExtData->_data is 0, should create oldver
         if ( 0 == pExtData->_data )
         {
            if ( DPS_TRANSLOCK_X == requestLockMode &&
                 DPS_TRANSLOCK_OP_MODE_TEST != opMode &&
                 !notTransOrRollback )
            {
               _oldVer = SDB_OSS_NEW oldVersionContainer( rid, _csID, _clID,
                                                          _csLID, _clLID ) ;
               if ( !_oldVer )
               {
                  _result = SDB_OOM ;
                  PD_LOG( PDERROR, "Alloc oldVersionContainer faild" ) ;
                  goto done ;
               }
               pExtData->_data = (UINT64)_oldVer ;

               /// set callback
               pExtData->setValidFunc(
                  (DPS_EXTDATA_VALID_FUNC)dmsIsExtendDataValid ) ;
               pExtData->setReleaseCheckFunc(
                  (DPS_EXTDATA_RELEASE_CHECK)dmsExtendDataReleaseCheck ) ;
               pExtData->setReleaseFunc(
                  (DPS_EXTDATA_RELEASE)dmsReleaseExtendData ) ;
               pExtData->setOnLockReleaseFunc(
                  (DPS_EXTDATA_ON_LOCKRELEASE)dmsOnTransLockRelease ) ;
            }
         }
         else
         {
            _oldVer = (oldVersionContainer*)pExtData->_data ;
            SDB_ASSERT( _oldVer->getRecordID() == rid,
                        "LockID is not the same" ) ;

            /// check the record whether is deleted
            if ( DPS_TRANSLOCK_X == requestLockMode )
            {
               if( _oldVer->isRecordDeleted() )
               {
                  _oldVer->releaseRecord( this ) ;

                  PD_LOG( PDDEBUG, "Delete old record for rid[%s] from memory",
                          lockId.toString().c_str() ) ;
               }
            }

            if ( _oldVer->isRecordNew() )
            {
               // Special case is the new record created within the same
               // transaction, we should not setup oldVer for it because
               // there is no older verion for it.
               if ( _oldVer->getOwnerTID() == _eduCB->getTID() )
               {
                  _recordInfo._transInsert = TRUE ;
               }
               _oldVer = NULL ;
            }
         }
      }

   done :

     //FIXME remove
#ifdef _DEBUG
     PD_LOG( PDDEBUG,
             "oldVer[%x] for rid[%s] in memory, lockmod=%s, "
             "_useOldVersion=%d, _skipRecord=%d, "
             "_rbsRecordData->isEmpty()=%d, rc=%d, transID(%s)",
             _oldVer, lockId.toString().c_str(),
             lockModeToString( requestLockMode ),
             _useOldVersion, _skipRecord,
             (_rbsRecordData ? _rbsRecordData->isEmpty() : -1 ), rc,
             dpsTransIDToString( transID ).c_str() ) ;
#endif
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIREUXLOCKORNONRRREAD ) ;
      return rc ;
   }

   //
   // read record from old version container and do following validations:
   //  1. if oldVer recordPtr is NULL, return found = FALSE
   //  2. if record doesn't have glob trans ID, i.e., the record comes
   //     non-transactional update, set _useOldVersion = TRUE, _recordRW
   //     return found = TRUE,
   //  3. check record owner trans version visibility,
   //     if yes set _oldVer = NULL, found = FALSE, let the caller
   //     to decide if skip the record( normally when get S lock in hand ),
   //     or retry and wait on S lock( normally try S lock fails while calling
   //     afterLockAcquire ).
   //  4. when record owner trans version is invisible, check
   //     record trans version visibility. if invisible set found = FALSE,
   //     else set _useOldVersion = TRUE, _recordRW and return found = TRUE
   INT32 dmsTransLockCallback::_validateRecordFromOldVer
   (
      pmdEDUCB               *eduCB,
      const DPS_TRANS_ID     &transID,
      BOOLEAN                &found
   )
   {
      INT32   rc        = SDB_OK ;
      BOOLEAN isVisible = FALSE ;
      SDB_ASSERT( _oldVer, "Invalid old version container" ) ;

      if ( !_oldVer->getRecordPtr().get() || _oldVer->isRecordDummy() )
      {
         found = FALSE ;
         goto done ;
      }
      else
      {
         _recordPtr              = _oldVer->getRecordPtr() ;
         const dmsRecord* record = (const dmsRecord*)_recordPtr.get() ;

         if ( record->hasGlobTransID() )
         {
            DPS_TRANS_ID writingTransID     = _oldVer->getOwnerTransID() ;
            DPS_TRANS_ID recTransID         = record->getGlobTransID() ;
            stpLogicalTimeUS transBeginTime = _eduCB->getTransBeginTime() ;

            // if both read transaction ( current transaction ) and owner
            // transaction (who did the original change) are global RR trans,
            // we need to check if the write transaction could be committed
            // before read transaction started ( compare global logical time
            // with time error ). If that's the case, we will skip the record
            // because there will be a newer verion of the record which is
            // visiable to this read transaction.

            // check owner trans version
            rc = _transCB->isVersionVisible( _eduCB,
                                             writingTransID,
                                             transID,
                                             transBeginTime,
                                             TRANS_ISOLATION_RR,
                                             FALSE,
                                             isVisible ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to check visibility for "
                         "read transaction [%s] against owner "
                         "transaction [%s], rc: %d",
                         dpsTransIDToString( transID ).c_str(),
                         dpsTransIDToString( recTransID ).c_str(), rc ) ;
            if ( ! isVisible )
            {
               rc = _transCB->isVersionVisible( _eduCB,
                                                recTransID,
                                                transID,
                                                transBeginTime,
                                                TRANS_ISOLATION_RR,
                                                FALSE,
                                                found ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to check visibility for "
                            "read transaction [%s] against record "
                            "transaction [%s], rc: %d",
                            dpsTransIDToString( transID ).c_str(),
                            dpsTransIDToString( recTransID ).c_str(), rc ) ;
#ifdef _DEBUG
               if ( ! found )
               {
                  PD_LOG( PDDEBUG,
                          "In memory record version not visiable, "
                          "transid(%s) vs recordTransid(%s), clLID(%d)",
                          dpsTransIDToString( transID ).c_str(),
                          dpsTransIDToString( recTransID ).c_str(), _clLID ) ;
               }
#endif
            }
            else
            {
               // when get here means record owner trans version visibile,
               // set _oldVer = NULL and found = FALSE, let the caller
               // to decide if skip the record or retry and wait on S lock
#ifdef _DEBUG
               PD_LOG( PDDEBUG,
                       "skip this record after comparing curTransID(%s) against"
                       " owner transID(%s), clLID(%d)",
                       dpsTransIDToString( transID ).c_str(),
                       dpsTransIDToString( writingTransID ).c_str(), _clLID ) ;
#endif
               _oldVer = NULL ;
               found   = FALSE ;
            }
         }
         // record doesn't have glob trans, may come from non-transactional
         // update, so it is visible
         else
         {
            found = TRUE ;
         }
      }
   done:
      if ( found && _oldVer )
      {
         _useOldVersion = TRUE ;
         // setup the buffer pointer in dmsRecordRW
         *_recordRW = dmsMemRecordRW( _recordPtr ) ;
      }

      return rc ;
   error:
     goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIRESLOCKRRREAD, "dmsTransLockCallback::_afterAcquireSLockRRread" )
   INT32 dmsTransLockCallback::_afterAcquireSLockRRread
   (
      const dpsTransLockId       &lockId,
      INT32                       irc,
      DPS_TRANSLOCK_TYPE          requestLockMode,
      UINT32                      refCounter,
      DPS_TRANSLOCK_OP_MODE_TYPE  opMode,
      const dpsTransLRBHeader    *pLRBHeader,
      dpsLRBExtData              *pExtData
   )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIRESLOCKRRREAD ) ;

      INT32        rc            = SDB_OK ;
      DPS_TRANS_ID transID       = _eduCB->getTransID() ;
      BOOLEAN      found         = FALSE ;
      BOOLEAN notTransOrRollback = FALSE ;
      dmsRecordID rid( lockId.extentID(), lockId.offset() ) ;

      clearStatus() ;

      if ( transID.isInvalid() || _eduCB->isInTransRollback() )
      {
         // not in transaction
         notTransOrRollback = TRUE ;
      }

     //FIXME remove
#ifdef _DEBUG
      PD_LOG( PDDEBUG,
              "Begin check for rid(%d, %d), transid(%s), clLID(%d), irc(%d), "
              "requestLockMode(%s), notTransOrRollback(%d), ISO:RR, scanner:%s",
              rid._extent, rid._offset,
              dpsTransIDToString( transID ).c_str(),
              _clLID, irc, lockModeToString( requestLockMode ),
              notTransOrRollback,
              ( (!_pScanner)
                ? "TBScan"
                : ( (SCANNER_TYPE_MEM_TREE == _pScanner->getCurScanType())
                    ? "Memory tree"
                    : "Disk index" ) ) ) ;
#endif

      // when roll back or transaction is not avaiable
      if ( notTransOrRollback )
      {
         if ( SDB_OK == irc )
         {
            _oldVer = NULL ;
         }
         goto done ;
      }

      // In case of RR read, if tryS fails and neither skipping the record
      // nor using old version, it will end up with getS, i.e., wait on
      // the S lock.
      // When getS returns error other than SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
      // say SDB_DPS_TRANS_APPEND_TO_WAIT, that is, about to be added into lock
      // waiter list. Nothing need to be done in this case, since it will
      // come back when it acquires the lock.
      if ( ( SDB_OK != irc ) &&
           ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != irc ) ||
             ( DPS_TRANSLOCK_OP_MODE_TRY != opMode ) ) )
      {
         goto done ;
      }
      else if ( SDB_OK == irc )
      {
         SDB_ASSERT( refCounter > 0, "Ref count must > 0" ) ;
         _recordInfo._refCount = refCounter ;
      }

      // some cases we can skip the record quickly
      if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE == irc ) &&
           ( DPS_TRANSLOCK_OP_MODE_TRY == opMode )  &&
           ( 0 != pExtData->_data ) )
      {
         _oldVer = (oldVersionContainer*)(pExtData->_data) ;
         SDB_ASSERT( _oldVer->getRecordID() == rid, "LockID is not the same" ) ;
         if ( _oldVer->isRecordNew() )
         {
            // Since we guarantee that a record is not physically deleted,
            // we can guarantee that a new record is not visiable,
            // so skip the record.
            _skipRecord = TRUE ;
           goto done ;
         }
      }

      /// merge scan from memory index tree
      if ( _pScanner &&
           ( SCANNER_TYPE_MEM_TREE == _pScanner->getCurScanType() ) )
      {
         BOOLEAN visible = FALSE ;

         // old version container is available
         if ( 0 != pExtData->_data )
         {
            _oldVer = (oldVersionContainer*)(pExtData->_data) ;
            SDB_ASSERT( _oldVer->getRecordID() == rid,
                        "LockID is not the same" ) ;

            rc = _validateRecordFromOldVer( _eduCB, transID, visible ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            // when oldVer record owner version is visible,
            // _validateRecordFromOldVer set _oldVer = NULL visible = FALSE
            //
            // if both read transaction ( current transaction ) and write
            // transaction ( who is holding the lock ) are global RR
            // transactions, we need to check if the write transaction could be
            // committed before read transaction started ( compare global
            // logical time with time error ).
            // if the visible is TRUE returned by checking, the write
            // transaction must be committed in other groups
            // to access this record updated by the write transaction, we need
            // to wait for write transaction to commit to release locks
            // instead of reading the old version
            if ( ( NULL == _oldVer ) && ( !visible ) )
            {
               // only set _skipRecord when have the record lock
               // if we don't have the record lock, DO NOT set _skipRecord
               // because we are going to wait on the lock after coming out of
               // this function
               if ( SDB_OK == irc )
               {
                  _skipRecord = TRUE ;
               }
               goto done ;
            }
         }
         if ( !visible )
         {
            // setup search RBS range
            preIdxTreePtr dummy ;
            dmsRBSOffset  startPos, endPos ;
            _pScanner->getRBSPositions( startPos, endPos, rid, dummy ) ;
            // search RBS if we have a proper range
            if ( startPos.isValid() || endPos.isValid() )
            {
               rc = _rbsMgr->rbsGetRecord( _csLID, _clID, _clLID,
                                           rid, transID, found,
                                           *_rbsRecordData,
                                           startPos, endPos ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "idxScan failed to read record from RBS, rc:%d",
                            rc ) ;
            }
            if ( !found )
            {
               _skipRecord = TRUE ;
               _useOldVersion = FALSE ;
            }
            else
            {
               _useOldVersion = TRUE ;
            }
         }
      }
      /// TBScan or merge scan from disk index
      else
      {
         BOOLEAN visible = FALSE ;

         // if have the record lock, read from disk first
         if ( SDB_OK == irc )
         {
            const dmsRecord* record = _recordRW->readPtr( 0 ) ;
            // record doesn't have glob trans, might come from
            // non-transactional update, visible
            if ( ! record->hasGlobTransID() )
            {
               goto done ;
            }
            DPS_TRANS_ID recTransID = record->getGlobTransID() ;
            rc = _transCB->isVersionVisible( _eduCB,
                                             recTransID,
                                             transID,
                                             _eduCB->getTransBeginTime(),
                                             TRANS_ISOLATION_RR,
                                             FALSE,
                                             visible ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to check visibility for "
                         "read transaction [%s] against record"
                         "transaction [%s], rc: %d",
                         dpsTransIDToString( transID ).c_str(),
                         dpsTransIDToString( recTransID ).c_str(), rc ) ;
         }
         if ( !visible )
         {
            // old version container is available
            if ( 0 != pExtData->_data )
            {
               _oldVer = (oldVersionContainer*)(pExtData->_data) ;
               SDB_ASSERT( _oldVer->getRecordID() == rid,
                           "LockID is not the same" ) ;

               _recordPtr = _oldVer->getRecordPtr() ;
               if ( _recordPtr.get() && !_oldVer->isRecordDummy() )
               {
                  // We need to re-verify the record with the index again.
                  // Here is how this could happen:
                  // Session 1 did update, changed index from 1 to 2, paused;
                  // session 2 does index scan, searching for record with
                  // index 2. It found the index on disk. It ended up using
                  // the old version from memory. But the old version record
                  // contain the index 1. We must verify this case and skip
                  // this record.
                  if ( _pScanner && _latchedIdxLid != DMS_INVALID_EXTENT &&
                       SCANNER_TYPE_DISK == _pScanner->getCurScanType() &&
                       _oldVer->idxLidExist( _latchedIdxLid ) )
                  {
                     _skipRecord = TRUE ;
                     /// remove the duplicate rid
                     _pScanner->removeDuplicatRID( _oldVer->getRecordID() ) ;
                     goto done ;
                  }
               }

               rc = _validateRecordFromOldVer( _eduCB, transID, visible ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               // when oldVer record owner version is visible,
               // _validateRecordFromOldVer set _oldVer = NULL visible = FALSE
               //
               // if both read transaction ( current transaction ) and write
               // transaction ( who is holding the lock ) are global RR
               // transactions, we need to check if the write transaction
               // could be committed before read transaction started ( compare
               // global logical time with time error ).
               // if the visible is TRUE returned by checking, the write
               // transaction must be committed in other groups to access
               // this record updated by the write transaction, we need to
               // wait for write transaction to commit to release locks
               // instead of reading the old version
               if ( ( NULL == _oldVer ) && ( !visible ) )
               {
                  // only set _skipRecord when have the record lock
                  // if we don't have the record lock, DO NOT set _skipRecord
                  // because we are going to wait on the lock after coming
                  // out of this function
                  if ( SDB_OK == irc )
                  {
                     _skipRecord = TRUE ;
                  }
                  goto done ;
               }
            }
            // try to get record from RBS
            if ( !visible )
            {
               dmsRBSOffset  startPos, endPos ;
               // setup search RBS range if it is merge scan from disk index.
               // As for TBScan, it can directly hash and use RBS based on
               // the chain.
               if ( _pScanner )
               {
                  SDB_ASSERT((SCANNER_TYPE_DISK == _pScanner->getCurScanType()),
                             "Invalid scanner type!" ) ;
                  preIdxTreePtr dummy ;
                  _pScanner->getRBSPositions(startPos, endPos, rid, dummy) ;
               }
               rc = _rbsMgr->rbsGetRecord( _csLID, _clID, _clLID,
                                           rid, transID, found,
                                           *_rbsRecordData,
                                           startPos, endPos ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to read record from RBS, rc: %d", rc ) ;

               if ( !found )
               {
                  _skipRecord = TRUE ;
                  _useOldVersion = FALSE ;
               }
               else
               {
                  _useOldVersion = TRUE ;
               }
            }
         }
      }

   done :
      if ( ( SDB_OK == irc ) && ( _oldVer && _oldVer->isRecordNew() ) )
      {
         // Special case is the new record created within the same
         // transaction, we should not setup oldVer for it because
         // there is no older verion for it.
         if ( _oldVer->getOwnerTID() == _eduCB->getTID() )
         {
            _recordInfo._transInsert = TRUE ;
         }
         _oldVer = NULL ;
      }

     //FIXME remove
#ifdef _DEBUG
     PD_LOG( PDDEBUG,
             "oldVer[%x] for rid[%s] in memory, lockmod=%s, "
             "_useOldVersion=%d, _skipRecord=%d, "
             "_rbsRecordData->isEmpty()=%d, rc=%d, transID(%s)",
             _oldVer, lockId.toString().c_str(),
             lockModeToString( requestLockMode ),
             _useOldVersion, _skipRecord,
             (_rbsRecordData ? _rbsRecordData->isEmpty() : -1 ), rc,
             dpsTransIDToString( transID ).c_str() ) ;
#endif
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIRESLOCKRRREAD ) ;
      return  rc ;
   error :
      goto done ;
   }

   // Description:
   //    Function called before lock release(in dpsTransLockManager::_release)
   // Under RC, before X lock is released, the update is responsible
   // to free up the kept old version record and clean up all related indexes
   // from the in-memory index tree. All information were kept in lrbHdr->oldVer
   // The latching protocal has to be:
   // 1. LRB hash bkt latch must be held(X) to tranverse/update lrbHdrs/LRBs
   // 2. preIdxTree latch must be held in X to insert/delete node in the tree,
   //    oldVersionCB(_oldVersionCBLatch) need to be held in S before
   //    accessing individual index tree.
   // 3. Request preIdxTree latch while holding LRB hash bkt latch is forbidden,
   //    But reverse order is OK. Note that the scanner will hold tree latch
   //    and acquire lock.
   //
   // Input:
   //    lockId: lock id to operate on
   //    lockMode: lock mode requested (IS/IX/S/U/X)
   //    refCounter: current reference counter of the lock
   //    oldVer: pointer to oldVersionContainer
   // Output:
   //    oldVer: pointer to oldVersionContainer
   // Dependency:
   //    caller MUST hold the record lock and LRB hash bkt latch in X
   // which protects all the update on oldVer
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE, "dmsTransLockCallback::beforeLockRelease" )
   void dmsTransLockCallback::beforeLockRelease( const dpsTransLockId &lockId,
                                                 DPS_TRANSLOCK_TYPE lockMode,
                                                 UINT32 refCounter,
                                                 const dpsTransLRBHeader *pLRBHeader,
                                                 dpsLRBExtData *pExtData )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE ) ;

      dmsOnTransLockRelease( lockId, lockMode, refCounter,
                             _nonTransNeedCleanup, _transCB, _eduCB,
                             pExtData ) ;

      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE );
   }

   // Description
   // Dependency: Caller must hold the recordlock and mbLcok
   INT32 dmsTransLockCallback::saveOldVersionRecord( const _dmsRecordRW *pRecordRW,
                                                     const dmsRecordID  &rid,
                                                     const UINT32        clLID,
                                                     const BSONObj      &obj,
                                                     const UINT32        ownerTID )
   {
      INT32        rc      = SDB_OK ;
      DPS_TRANS_ID transID ;
      SDB_ASSERT( _eduCB && pRecordRW,
                  "eduCB or recordRW is not properly setup " ) ;
      // get the owner transaction id
      transID = _eduCB->getTransID() ;

      // if the oldRecord does not exist, we will create one
      if ( _oldVer && _oldVer->isRecordEmpty() )
      {
         /// when not use rollback segment
         if ( ( !_eduCB->getTransExecutor()->useRollbackSegment() ) &&
              ( ! pmdGetOptionCB()->mvccOn() ) )
         {
            // mvccon will overwrite transuserbs
            _oldVer->setRecordDummy( ownerTID ) ;
         }
         else
         {
            const dmsRecord *pRecord= pRecordRW->readPtr( 0 ) ;
#ifdef _DEBUG
            // TODO: for record from V0, we do not have LSN on page header.
            // and we haven't done inflight migration yet. 
            // we must either force export/import all the records during
            // migration OR only support mvcc in newly created record(CS/CL)
            // OR use same method to generate a create LSN for hash purpose.

        // FIXME: remove
      PD_LOG( PDDEBUG, "saving old record to memory and RBS:"
              "rid(%d, %d), ownertransid(%s), "
              "recordTransID(%s)",
              rid._extent, rid._offset, 
              dpsTransIDToString( transID ).c_str(),
              dpsTransIDToString( pRecord->getGlobTransID() ).c_str() ) ;
#endif

            // 1. get to overflow record if needed
            if ( pRecord->isOvf() )
            {
               dmsRecordID ovfRID = pRecord->getOvfRID() ;
               dmsRecordRW ovfRW = pRecordRW->derive( ovfRID );
               ovfRW.setNothrow( pRecordRW->isNothrow() ) ;
               pRecord = ovfRW.readPtr( 0 ) ;
            }

            // 2. save record
            rc = _oldVer->saveRecord( pRecord, obj, ownerTID, transID ) ;
            if ( rc )
            {
               goto error ;
            }

            // 3. write version to RBS at the same if mvccon
            if ( pmdGetOptionCB()->mvccOn() )
            {
               DPS_TRANS_ID recTransID = pRecord->getGlobTransID() ;
               rc = pmdGetKRCB()->getDMSCB()->getRBSSUMgr()
                      ->rbsAppendRecord( _oldVer->getCSID(), 
                                         _oldVer->getCLID(),
                                         clLID,
                                         rid,
                                         recTransID,
                                         transID,
                                         obj, this ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, 
                          "Failed to save to RBS  :rid(%d, %d), tid(%s), "
                          "obj(%s)",
                          rid._extent, rid._offset, 
                          dpsTransIDToString( transID ).c_str(),
                          _oldVer->getRecordObj().toString().c_str() ) ;
                  goto error ;
               }
            }
         }
         // 3. hang the old version container to the linked list
         if ( !_unitPtr.get() )
         {
            oldVersionCB *oldCB = _transCB->getOldVCB() ;
            rc = oldCB->getOrCreateOldVersionUnit( _csID, _clID, _unitPtr ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         _unitPtr->addToChain( _oldVer ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::_checkInsertIndex( preIdxTreePtr &treePtr,
                                                  _INSERT_CURSOR &insertCursor,
                                                  const ixmIndexCB *indexCB,
                                                  BOOLEAN isUnique,
                                                  BOOLEAN isEnforce,
                                                  const BSONObj &keyObj,
                                                  const dmsRecordID &rid,
                                                  _pmdEDUCB *cb,
                                                  BOOLEAN allowSelfDup,
                                                  utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      // If there is an old verion of the index in our in memory tree, we
      // should not insert the index because the update/delete transaction
      // has not committed yet.
      if ( isUnique && !cb->isInTransRollback() )
      {
         preIdxTreeNodeValue idxValue ;

         if ( !treePtr.get() && _INSERT_NONE == insertCursor )
         {
            oldVersionCB *pOVCB = _transCB->getOldVCB() ;
            globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
            treePtr = pOVCB->getIdxTree( gid, FALSE ) ;
         }
         insertCursor = _INSERT_CHECK ;

         if ( !treePtr.get() )
         {
            goto done ;
         }

         if ( _latchedIdxLid != indexCB->getLogicalID() ||
              -1 == idxTreeLatchMode() )
         {
            treePtr->lockS() ;
            locked = TRUE ;
         }

         if ( treePtr->isKeyExist( keyObj, idxValue ) )
         {
            // Internally will test X lock on the record to make sure key
            // does not exist and avoid duplicate key false alarm
            // for example:
            //   db.cs.createCL( "c1" )
            //   db.cs.c1.createIndex( "a", {a:1})
            //   db.cs.c1.insert( {a:1, b:1} )
            //   db.transBegin()
            //   db.cs.c1.remove() // or delete { a:1, b:1 }
            //   db.cs.c1.insert({a:1, b:1}) ==> fails due to dupblicate key
            //
            if ( allowSelfDup && idxValue.getOwnerTID() == cb->getTID() )
            {
               goto done ;
            }
            else if ( !isEnforce && _dmsIsKeyUndefined( keyObj ) )
            {
               goto done ;
            }

            rc = SDB_IXM_DUP_KEY ;
            if ( NULL != pResult )
            {
               INT32 rcTmp = pResult->setIndexErrInfo( indexCB->getName(),
                                                       indexCB->keyPattern(),
                                                       keyObj ) ;
               if ( rcTmp )
               {
                  rc = rcTmp ;
               }
               else
               {
                  pResult->setPeerID( idxValue.getRecordObj() ) ;
               }
            }
            PD_LOG ( PDERROR, "Insert index(%s) key(%s) with rid(%d, %d) "
                     "found key(%s), failed, rc: %d", 
                     indexCB->getDef().toString().c_str(),
                     keyObj.toString().c_str(),
                     rid._extent, rid._offset, 
                     idxValue.toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      if ( locked )
      {
         treePtr->unlockS() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onInsertRecord( _dmsMBContext *context,
                                               const BSONObj &object,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               _pmdEDUCB* cb )
   {
      if ( _oldVer )
      {
         _oldVer->setRecordNew( cb->getTID() ) ;
      }
      return SDB_OK ;
   }

   INT32 dmsTransLockCallback::onDeleteRecord( _dmsMBContext *context,
                                               const BSONObj &object,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               BOOLEAN markDeleting,
                                               _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      rc = saveOldVersionRecord( pRecordRW, rid, context->clLID(),
                                 object, cb->getTID() ) ;
      if ( SDB_OK == rc && markDeleting && _oldVer )
      {
         _oldVer->setDiskDeleting() ;
      }
      return rc ;
   }

   INT32 dmsTransLockCallback::onUpdateRecord( _dmsMBContext *context,
                                               const BSONObj &orignalObj,
                                               const BSONObj &newObj,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               _pmdEDUCB *cb )
   {
      return saveOldVersionRecord( pRecordRW, rid, context->clLID(),
                                   orignalObj, cb->getTID() ) ;
   }

   INT32 dmsTransLockCallback::onInsertIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              BOOLEAN isEnforce,
                                              const BSONObjSet &keySet,
                                              const dmsRecordID &rid,
                                              _pmdEDUCB *cb,
                                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      preIdxTreePtr treePtr ;
      _INSERT_CURSOR insertCursor = _INSERT_NONE ;

      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }

      for ( BSONObjSet::const_iterator cit = keySet.begin() ;
            cit != keySet.end() ;
            ++cit )
      {
         rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                 isUnique, isEnforce, *cit,
                                 rid, cb, TRUE, pResult ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      PD_LOG ( PDDEBUG, "onInsertIndex with rid(%d, %d) failed, rc=%d  ",
               rid._extent, rid._offset, rc ) ;
      goto done ;
   }

   INT32 dmsTransLockCallback::onInsertIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              BOOLEAN isEnforce,
                                              const BSONObj &keyObj,
                                              const dmsRecordID &rid,
                                              _pmdEDUCB* cb,
                                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      preIdxTreePtr treePtr ;
      _INSERT_CURSOR insertCursor = _INSERT_NONE ;

      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }

      /// create index, don't allow self duplicate
      rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                              isUnique, isEnforce, keyObj,
                              rid, cb, FALSE, pResult ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      PD_LOG ( PDERROR, "onInsertIndex with rid(%d, %d) failed, rc=%d  ",
               rid._extent, rid._offset, rc ) ;
      goto done ;
   }

   INT32 dmsTransLockCallback::_checkDeleteIndex( preIdxTreePtr &treePtr,
                                                  _DELETE_CURSOR &deleteCursor,
                                                  const ixmIndexCB *indexCB,
                                                  BOOLEAN isUnique,
                                                  const BSONObj &keyObj,
                                                  const dmsRecordID &rid,
                                                  _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasLocked = FALSE ;
      globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;

      if ( !_oldVer || cb->isInTransRollback() )
      {
         /// when not use rollback segment
         /// or in trans rollback
         goto done ;
      }
      else if ( _oldVer->isRecordNew() )
      {
         goto done ;
      }
      else if ( _oldVer->isRecordDummy() && !isUnique )
      {
         goto done ;
      }
      else if ( _DELETE_NONE == deleteCursor )
      {
         //    check if oldVer has the index, Note that if it exist in
         //    the set, the index must exist in the tree as well.
         //    This is valid scenario because one transaction can do multiple
         //    update of the same record. But we only need to keep the last
         //    committed version which happened to be the first copy.
         if ( _oldVer->idxLidExist( gid._idxLID ) )
         {
            deleteCursor = _DELETE_IGNORE ;
            goto done ;
         }
         else
         {
            BOOLEAN bInsertResult = FALSE ;
            if ( !treePtr.get() )
            {
               oldVersionCB *pVerCB = _transCB->getOldVCB() ;
               rc = pVerCB->getOrCreateIdxTree( gid, indexCB,
                                                treePtr, FALSE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Create memory index tree(%s) "
                          "failed, rc: %d", gid.toString().c_str(), rc ) ;
                  goto error ;
               }
            }

            deleteCursor = _DELETE_SAVE ;
            /// insert into oldVer's indexSet
            rc = _oldVer->insertIdxTree( treePtr, &bInsertResult ) ;
            if ( rc )
            {
               goto error ;
            }
#ifdef _DEBUG
            PD_LOG( PDDEBUG, "Insert tree[%d] into _oldIdxLid %s",
                    gid._idxLID,
                    ( bInsertResult ? "succeeded" : "failed" ) ) ;
            SDB_ASSERT( bInsertResult, "Failed to insert into _oldIdxLid" ) ;
#endif
         }
      }
      else if ( _DELETE_IGNORE == deleteCursor )
      {
         goto done ;
      }

      if ( _latchedIdxLid == gid._idxLID && idxTreeLatchMode() != -1 )
      {
         if ( EXCLUSIVE == idxTreeLatchMode() )
         {
            hasLocked = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock mode(%d) is not EXCLUSIVE(%d)",
                    idxTreeLatchMode(), EXCLUSIVE ) ;
            SDB_ASSERT( FALSE, "Lock mode is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      // use owner transID to insert into the mem tree
      rc = treePtr->insertWithOldVer( &keyObj, rid, _oldVer, hasLocked,
                                      this->getOwnerTransID(), this ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Insert index keys(%s) with rid(%d, %d) "
                  "failed, rc: %d", keyObj.toString().c_str(),
                  rid._extent, rid._offset, rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onDeleteIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              const BSONObjSet &keySet,
                                              const dmsRecordID &rid,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      _DELETE_CURSOR deleteCursor = _DELETE_NONE ;
      preIdxTreePtr treePtr ;

      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }

      /// insert key to mem tree
      for ( BSONObjSet::const_iterator cit = keySet.begin() ;
            cit != keySet.end() ;
            ++cit )
      {
         rc = _checkDeleteIndex( treePtr,deleteCursor, indexCB,
                                 isUnique, *cit, rid, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onUpdateIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              BOOLEAN isEnforce,
                                              const BSONObjSet &oldKeySet,
                                              const BSONObjSet &newKeySet,
                                              const dmsRecordID &rid,
                                              BOOLEAN isRollback,
                                              _pmdEDUCB* cb,
                                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      BSONObjSet::const_iterator itori ;
      BSONObjSet::const_iterator itnew ;
      preIdxTreePtr treePtr ;
      _DELETE_CURSOR deleteCursor = _DELETE_NONE ;
      _INSERT_CURSOR insertCursor = _INSERT_NONE ;
      BOOLEAN hasChanged = FALSE ;
      // check ID index for normal update
      // NOTE: for sequoiadb upgrade, if the old data before upgrade
      //       contains invalid _id field, we could not report error,
      //       we need to allow update if _id field is not changed
      BOOLEAN checkIDIndex = indexCB->isSysIndex() &&
                             !cb->isInTransRollback() &&
                             !cb->isDoRollback() ;

      /// not use transaction
      if ( !_transCB || !_transCB->isTransOn() )
      {
         if ( checkIDIndex )
         {
            // for no transaction, we need to check _id field
            if ( oldKeySet.size() != newKeySet.size() )
            {
               // check length of _id field
               PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to update $id index, "
                         "_id field can't be array or empty" ) ;
            }

            itori = oldKeySet.begin() ;
            itnew = newKeySet.begin() ;
            while ( oldKeySet.end() != itori && newKeySet.end() != itnew )
            {
               if ( 0 != (*itori).woCompare((*itnew), BSONObj(), FALSE ) )
               {
                  PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                            "Failed to update $id index, "
                            "_id field can't be array" ) ;
                  // check _id field
                  rc = _checkIDIndexUpdate( rid, (*itnew).firstElement(), cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to update $id index, "
                               "rc: %d", rc ) ;
               }
               itori++ ;
               itnew++ ;
            }
         }
         goto done ;
      }
      /// rollback
      else if ( isRollback )
      {
         goto done ;
      }

      if ( oldKeySet.size() != newKeySet.size() )
      {
         if ( checkIDIndex )
         {
            // check length of _id field
            PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to update $id index, "
                      "_id field can't be array or empty" ) ;
         }
         hasChanged = TRUE ;
      }

      itori = oldKeySet.begin() ;
      itnew = newKeySet.begin() ;
      while ( oldKeySet.end() != itori && newKeySet.end() != itnew )
      {
         INT32 result = (*itori).woCompare((*itnew), BSONObj(), FALSE ) ;
         if ( 0 == result )
         {
            // new and original are the same, we don't need to change
            // anything in the index
            itori++ ;
            itnew++ ;
         }
         else if ( result < 0 )
         {
            // original smaller than new, that means the original doesn't
            // appear in the new list anymore, let's delete it
            hasChanged = TRUE ;
            itori++ ;
         }
         else
         {
            if ( checkIDIndex )
            {
               PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to update $id index, "
                         "_id field can't be array" ) ;
               // check _id field
               rc = _checkIDIndexUpdate( rid, (*itnew).firstElement(), cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to update $id index, "
                            "rc: %d", rc ) ;
            }

            hasChanged = TRUE ;
            rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                    isUnique, isEnforce, *itnew,
                                    rid, cb, TRUE, pResult ) ;
            if ( rc )
            {
               goto error ;
            }
            itnew++ ;
         }
      }

      // insert rest of itnew
      while ( newKeySet.end() != itnew )
      {
         if ( checkIDIndex )
         {
            PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to update $id index, "
                      "_id field can't be array" ) ;
            // check _id field
            rc = _checkIDIndexUpdate( rid, (*itnew).firstElement(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update $id index, "
                         "rc: %d", rc ) ;
         }
         rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                 isUnique, isEnforce, *itnew,
                                 rid, cb, TRUE, pResult ) ;
         if ( rc )
         {
            goto error ;
         }
         ++itnew ;
      }

      if ( hasChanged )
      {
         // insert the original index into the in-memory old version tree
         itori = oldKeySet.begin() ;
         while ( oldKeySet.end() != itori )
         {
            rc = _checkDeleteIndex( treePtr, deleteCursor, indexCB,
                                    isUnique, *itori, rid, cb ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, 
                        "checkDelete index keys(%s) failed, rc: %d",
                        itori->toString().c_str(), rc ) ;
               goto error ;
            }
            ++itori ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onDropIndex( _dmsMBContext *context,
                                            const ixmIndexCB *indexCB,
                                            _pmdEDUCB *cb )
   {
      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         pOldVCB->delIdxTree( gid, FALSE ) ;
      }

      return SDB_OK ;
   }

   // when rebuild index, we need to build in memory old version index
   // tree based on all old version record on the collection.
   // Input:
   //    context:  dms MB context
   //    indexCB:  index control block
   //    cb:  edu control block
   // return:
   //    rc:
   // Dependency:
   //    The context and indexCB must be setup.
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_ONREBUILDINDEX, "dmsTransLockCallback::onRebuildIndex" )
   INT32 dmsTransLockCallback::onRebuildIndex( _dmsMBContext *context,
                                               const ixmIndexCB *indexCB,
                                               _pmdEDUCB *cb,
                                               utilWriteResult *pResult )
   {
      INT32   rc         = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_ONREBUILDINDEX ) ;

      // Input parameter validation ASSERTS
      SDB_ASSERT( context && context->isMBLock(),
                  "Caller should hold mb lock" ) ;
      SDB_ASSERT( indexCB, "indexCB is invalid " ) ;

      if ( _transCB && _transCB->isTransOn() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         oldVersionContainer *oldVer = NULL ;
         preIdxTreePtr treePtr ;
         oldVersionUnitPtr unitPtr ;
         oldVersionUnit::iterator itChain ;

         unitPtr = pOldVCB->getOldVersionUnit( _csID, _clID ) ;
         if ( !unitPtr.get() )
         {
            /// collection chain is not exist
            goto done ;
         }
         itChain = unitPtr->itr() ;

         treePtr = pOldVCB->getIdxTree( gid, FALSE ) ;
         if ( !treePtr.get() || !treePtr->isValid() )
         {
            PD_LOG( PDERROR, "Memory index tree(%s) is not exist",
                    gid.toString().c_str(), rc ) ;
            rc = SDB_DMS_INVALID_INDEXCB ;
            goto error ;
         }
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "RebuildIndex got memtree,gid(%s)",
                 gid.toString().c_str() ) ;
#endif
         // go through the old version link list and insert index to tree
         oldVer = itChain.next() ;
         while ( oldVer )
         {
            if ( oldVer->isRecordDeleted() )
            {
               /// do nothing
            }
            else if ( oldVer->isRecordDummy() ||
                      ( ! cb->getTransExecutor()->useRollbackSegment() ) )
            {
               if ( indexCB->unique() )
               {
                  /// We can't allow unique index if there are transactions
                  /// with RB segment disable configuration. otherwise
                  /// rollback of those transaction could fail
                  rc = SDB_OPERATION_CONFLICT ;
                  PD_LOG_MSG( PDERROR, "Can't allow create unique index "
                              "when doing some transactions with "
                              "\'transuserbs=false\'" ) ;
                  goto error ;
               }
            }
            else if ( !oldVer->isRecordNew() && oldVer->getRecord() )
            {
               BSONObjSet  keySet ;
               _DELETE_CURSOR deleteCursor = _DELETE_NONE ;
               _INSERT_CURSOR insertCursor = _INSERT_NONE ;
               _oldVer = oldVer ;

               // get the keyset
               rc = indexCB->getKeysFromObject ( oldVer->getRecordObj(),
                                                 keySet ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to get keys from object %s",
                           oldVer->getRecordObj().toString().c_str() ) ;
                  goto error ;
               }

               // We remove old version container from the chain first
               // then releaseRecord when release a lock( dmsOnTransLockRelease,
               // _dmsReleaseLockJob::doit ); when creates an index, walks
               // through the chain and checks if the index LID is in that
               // old version container's index LID set and do insert index
               // LID and index tree ( insertIdxTree and insertWithOldVer )
               // if it is not there.
               // When traverse the chain, add or remove an old version
               // container to or from the chain, proper latch on oldVersionUnit
               // will apply to protect synchronized accessing.

               // insert to the mem tree
               for ( BSONObjSet::const_iterator cit = keySet.begin() ;
                     cit != keySet.end() ;
                     ++cit )
               {
                  rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                          indexCB->unique(),
                                          indexCB->enforced(), *cit,
                                          oldVer->getRecordID(), cb,
                                          FALSE, pResult ) ;
                  if ( rc )
                  {
                     goto error ;
                  }

                  rc = _checkDeleteIndex( treePtr,deleteCursor, indexCB,
                                          indexCB->unique(), *cit,
                                          oldVer->getRecordID(), cb ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to insert the key(%s) for obj(%s)",
                              cit->toString().c_str(),
                              oldVer->getRecordObj().toString().c_str() ) ;
                     goto error ;
                  }
               }
#ifdef _DEBUG
               PD_LOG( PDDEBUG, "Inserted key to mem tree for obj(%s)",
                       oldVer->getRecordObj().toString().c_str() ) ;
#endif
            }

            oldVer = itChain.next() ;
         }  // while (oldVer)
         PD_LOG( PDDEBUG, "Rebuild index callback done successfully(%s)",
                 gid.toString().c_str() ) ;
      }

   done :
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_ONREBUILDINDEX ) ;
      return rc ;

   error :
      PD_LOG( PDERROR, "Rebuild index callback failed, lid=%d, rc=%d",
              indexCB->getLogicalID(), rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_ONCREATEINDEX, "dmsTransLockCallback::onCreateIndex" )
   INT32 dmsTransLockCallback::onCreateIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_ONCREATEINDEX ) ;
      INT32   rc         = SDB_OK ;

      // create in memory tree
      if ( _transCB && _transCB->isTransOn() )
      {
         preIdxTreePtr treePtr ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         oldVersionCB *pVerCB = _transCB->getOldVCB() ;

#ifdef _DEBUG
         SDB_ASSERT( !(pVerCB->getIdxTree(gid, FALSE).get()),
                     "Index tree already exist " ) ;
#endif

         rc = pVerCB->addIdxTree( gid, indexCB, treePtr, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create memory index tree(%s) "
                    "failed, rc: %d", gid.toString().c_str(), rc ) ;
            goto error ;
         }
         PD_LOG( PDDEBUG, "Create index callback done successfully(%s)",
                 gid.toString().c_str() ) ;
      }

   done :
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_ONCREATEINDEX ) ;
      return rc ;
   error :
      goto done ;
   }

   void dmsTransLockCallback::onCSClosed( INT32 csID )
   {
      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         pOldVCB->clearIdxTreeByCSID( csID, FALSE ) ;
         pOldVCB->clearOldVersionUnitByCS( csID ) ;
         PD_LOG( PDDEBUG, "CS close callback done successfully(%d)",
                 csID ) ;
      }
   }

   void dmsTransLockCallback::onCLTruncated( INT32 csID, UINT16 clID )
   {
      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         pOldVCB->clearIdxTreeByCLID( csID, clID, FALSE ) ;
         pOldVCB->delOldVersionUnit( csID, clID ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK__CHKIDIDXUPDATE, "dmsTransLockCallback::_checkIDIndexUpdate" )
   INT32 dmsTransLockCallback::_checkIDIndexUpdate( const dmsRecordID &rid,
                                                    const BSONElement &idEle,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK__CHKIDIDXUPDATE ) ;

      const CHAR *errStr = "" ;

      // check _id field
      if ( !dmsIsRecordIDValid( idEle, FALSE, &errStr ) )
      {
         PD_LOG( PDERROR, "Failed to update $id index, "
                 "_id is error: %s", errStr ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSTRANSLOCKCALLBACK__CHKIDIDXUPDATE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::idxTreeLatchMode () const
   {
      if ( _pScanner )
      {
         return _pScanner->getLockModeByType( SCANNER_TYPE_MEM_TREE ) ;
      }
      else
      {
         return -1 ;
      }
   }

   BOOLEAN dmsTransLockCallback::isIndexProtectionRequired()
   {
      BOOLEAN bResult = FALSE ;
      if ( _pScanner && ( SCANNER_TYPE_MERGE == _pScanner->getType() ) )
      {
         bResult = TRUE ;
      }
      return bResult ;
   }

   BOOLEAN dmsTransLockCallback::isIndexProtected
   (
      INT32 idxTreeId,
      INT32 latchMode
   )
   {
      BOOLEAN bResult = FALSE ;
      if ( idxTreeId == _latchedIdxLid )
      {
         if ( -1 != latchMode )
         {
            if ( idxTreeLatchMode() == latchMode )
            {
               bResult = TRUE ;
            }
         }
         else
         {
            bResult = TRUE ;
         }
      }
      return bResult ;
   }

}
