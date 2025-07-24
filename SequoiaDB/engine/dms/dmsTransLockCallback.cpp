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
#include "dmsOprHandler.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "rtnIXScanner.hpp"
#include "rtnTBScanner.hpp"
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
         _dmsMemRecordRW( const _dmsRecordRW &recordRW, dpsOldRecordPtr ptr )
         :_dmsRecordRW( recordRW )
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

      result = UTIL_LJOB_DO_FINISH ;

<<<<<<< HEAD
=======
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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
                 pExtData ? pExtData->_data : 0 ) ;
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

   dmsTransLockCallback::dmsTransLockCallback( IDmsOprHandler *handler )
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
<<<<<<< HEAD
      _latchedIdxLid = DMS_INVALID_EXTENT ;
      _pScanner      = NULL ;
      _opHandler     = handler ;
=======
      _latchedIdxLid  = DMS_INVALID_EXTENT ;
      _transIsolation = TRANS_ISOLATION_MAX ;
      _nonTransNeedCleanup = FALSE ;
      _useLatestVersion = FALSE ;
      _pScanner    = NULL ;
      _opHandler   = handler ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

      clearStatus() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK, "dmsTransLockCallback::dmsTransLockCallback" )
   dmsTransLockCallback::dmsTransLockCallback( dpsTransCB *transCB,
                                               _pmdEDUCB *eduCB,
                                               IDmsOprHandler *handler )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK ) ;

      _transCB    = transCB ;
      _oldVer     = NULL ;
      _eduCB      = eduCB ;
      _recordRW   = NULL ;
      _rbsRecordData = NULL ;
      _nonTransNeedCleanup = FALSE ;
      _useLatestVersion = FALSE ;
      _oldVerCB   = transCB->getOldVCB() ;
      _rbsMgr     = pmdGetKRCB()->getDMSCB()->getRBSSUMgr() ;

      _csLID      = ~0 ;
      _clLID      = ~0 ;
      _csID       = DMS_INVALID_SUID ;
      _clID       = DMS_INVALID_MBID ;
      _latchedIdxLid = DMS_INVALID_EXTENT ;
      _pScanner      = NULL ;
      _opHandler     = handler ;

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

   void dmsTransLockCallback::setScanner( _rtnScanner *pScanner )
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
      _needPostAction      = FALSE ;
      _recordOnDiskVisible = FALSE ;
      _indexBitmap.resetBitmap() ;

      _diskRecordTransID.reset() ;
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
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE, "dmsTransLockCallback::afterLockAcquire" )
   void dmsTransLockCallback::afterLockAcquire
   (
      const dpsTransLockId       &lockId,
      INT32                       irc,
      DPS_TRANSLOCK_TYPE          requestLockMode,
      UINT32                      refCounter,
      DPS_TRANSLOCK_OP_MODE_TYPE  opMode,
      dpsLRBExtData              *pExtData
   )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE ) ;

      /// when not leaf level, do nothing
      if ( !lockId.isLeafLevel() )
      {
         goto done ;
      }

      // In test lock mode, pExtData( pLRBHdr ) could be NULL, but we would
      // still have actions (RC/RR handling) to do. However, in other
      // acquiring mode, if LRBHdr does not exist, we do not have further
      // handling at this moment. Early exit.
      if ( ( NULL == pExtData ) &&
           ( DPS_TRANSLOCK_OP_MODE_TEST != opMode ) )
      {
         goto done ;
      }

      // S lock and isolation RR
      if (  ( TRANS_ISOLATION_RR == _transIsolation ) &&
            ( DPS_TRANSLOCK_S == requestLockMode ) &&
            ( !_useLatestVersion )  )

      {
         _afterAcquireSLockRRread( lockId,
                                   irc,
                                   requestLockMode,
                                   refCounter,
                                   opMode,
                                   pExtData ) ;
      }
      // X,U lock or isolation RU, RC, RS
      else
      {
         _afterAcquireUXLockOrNonRRread( lockId,
                                         irc,
                                         requestLockMode,
                                         refCounter,
                                         opMode,
                                         pExtData ) ;
      }
   done :
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE ) ;
      return ;
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
   void dmsTransLockCallback::_afterAcquireUXLockOrNonRRread
   (
      const dpsTransLockId       &lockId,
      INT32                       irc,
      DPS_TRANSLOCK_TYPE          requestLockMode,
      UINT32                      refCounter,
      DPS_TRANSLOCK_OP_MODE_TYPE  opMode,
      dpsLRBExtData              *pExtData
   )
   {
      BOOLEAN notTransOrRollback = FALSE  ;
      dmsRecordID rid( lockId.extentID(), lockId.offset() ) ;
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIREUXLOCKORNONRRREAD );

      /// not in transaction
      if ( _eduCB->getTransID().isInvalid() || _eduCB->isInTransRollback() )
      {
         notTransOrRollback = TRUE ;
      }

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
          (DPS_TRANSLOCK_OP_MODE_TEST == opMode) )
      {
         if ( ( NULL == pExtData ) || ( 0 == pExtData->_data ) )
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
            if ( _pScanner &&
                 SCANNER_TYPE_INDEX == _pScanner->getStorageType() &&
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
            SDB_ASSERT( NULL != _recordRW, "record should be attached" ) ;
            *_recordRW = dmsMemRecordRW( *_recordRW, _recordPtr ) ;

            // set the return info if we successfully used old version
            _useOldVersion = TRUE ;
         }
      } // end of case 1

      // Handle case 2 mentioned above
      // X record lock request from a transaction need to prepare to set
      // up old copy if the copy is not already there
      else if ( SDB_OK == irc )
      {
#ifdef _DEBUG
         if ( DPS_TRANSLOCK_OP_MODE_TEST != opMode )
         {
            SDB_ASSERT( refCounter > 0, "Ref count must > 0" ) ;
         }
#endif
         _recordInfo._refCount = refCounter ;

         /// from memory tree
         if ( _pScanner &&
              SCANNER_TYPE_MEM_TREE == _pScanner->getCurScanType() )
         {
            _skipRecord = TRUE ;
            /// remove the duplicate rid
            _pScanner->removeDuplicatRID( rid ) ;
            _oldVer = NULL ;
            goto done ;
         }

         // when test lock the pExtData (LRB Header)
         // could be NULL.
         if ( NULL == pExtData )
         {
            // For isolation RC, test S lock successfully and
            // there is no LRB header, so no old ver, we may
            // read from disk directly
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
<<<<<<< HEAD
                  BOOLEAN hasLock = -1 != _getLatchedIdxMode() ? TRUE : FALSE ;
                  _oldVer->releaseRecord( _latchedIdxLid, hasLock ) ;
=======
                  _oldVer->releaseRecord( this ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

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

      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIREUXLOCKORNONRRREAD ) ;
      return ;
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
         SDB_ASSERT( NULL != _recordRW, "record should be attached" ) ;
         *_recordRW = dmsMemRecordRW( *_recordRW, _recordPtr ) ;
      }

      return rc ;
   error:
     goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIRESLOCKRRREAD, "dmsTransLockCallback::_afterAcquireSLockRRread" )
   void dmsTransLockCallback::_afterAcquireSLockRRread
   (
      const dpsTransLockId       &lockId,
      INT32                       irc,
      DPS_TRANSLOCK_TYPE          requestLockMode,
      UINT32                      refCounter,
      DPS_TRANSLOCK_OP_MODE_TYPE  opMode,
      dpsLRBExtData              *pExtData
   )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIRESLOCKRRREAD ) ;

      INT32        rc            = SDB_OK ;
      DPS_TRANS_ID transID       = _eduCB->getTransID() ;
      BOOLEAN      visible       = FALSE ;
      BOOLEAN notTransOrRollback = FALSE ;
      dmsRecordID rid( lockId.extentID(), lockId.offset() ) ;


      if ( transID.isInvalid() || _eduCB->isInTransRollback() )
      {
         // not in transaction
         notTransOrRollback = TRUE ;
      }

      // when roll back or transaction is not avaiable
      if ( notTransOrRollback )
      {
         if ( SDB_OK == irc )
         {
            _oldVer = NULL ;
         }
         goto done ;
      }

      // RR read normally won't fail, only if there is something wrong with
      // version check or stp. In this case, we will try to tolerant those
      // by getS.
      // When getS returns error other than SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
      // say SDB_DPS_TRANS_APPEND_TO_WAIT, that is, about to be added into lock
      // waiter list. Nothing need to be done in this case, since it will
      // come back when it acquires the lock.
      if ( ( SDB_OK != irc ) &&
           ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != irc ) ||
             ( DPS_TRANSLOCK_OP_MODE_TEST != opMode ) ) )
      {
         goto done ;
      }
      else if ( SDB_OK == irc )
      {
#ifdef _DEBUG
         if ( DPS_TRANSLOCK_OP_MODE_TEST != opMode )
         {
            SDB_ASSERT( refCounter > 0, "Ref count must > 0" ) ;
         }
#endif
         _recordInfo._refCount = refCounter ;
      }

      // some cases we can skip the record quickly
      if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE == irc ) &&
           ( DPS_TRANSLOCK_OP_MODE_TEST == opMode )  &&
           ( pExtData && ( 0 != pExtData->_data ) ) )
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
         // old version container is available
         if ( pExtData && ( 0 != pExtData->_data ) )
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
            _needPostAction = TRUE ;
         }
      }
      /// TBScan or merge scan from disk index
      else
      {
         if ( _recordOnDiskVisible )
         {
            goto done ;
         }
         // old version container is available
         if ( pExtData && ( 0 != pExtData->_data ) )
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
#ifdef  _DEBUG
                  PD_LOG( PDDEBUG,
                          "skipping rid(%d, %d) because can't use disk version",
                          rid._extent, rid._offset ) ;
#endif
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
            _needPostAction = TRUE ;
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

#if SDB_INTERNAL_DEBUG
      PD_LOG( PDDEBUG,
             "oldVer[%x] for rid[%s] in memory, lockmod=%s, visible=%d, "
             "_useOldVersion=%d, _skipRecord=%d, _needPostAction=%d, "
             "_rbsRecordData->isEmpty()=%d, rc=%d, transID(%s)",
             _oldVer, lockId.toString().c_str(),
             lockModeToString( requestLockMode ), visible,
             _useOldVersion, _skipRecord, _needPostAction,
             (_rbsRecordData ? _rbsRecordData->isEmpty() : -1 ), rc,
             dpsTransIDToString( transID ).c_str() ) ;
#endif
      _result = rc ;
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK__AFTERACQUIRESLOCKRRREAD ) ;
      return ;
   error :
      goto done ;
   }

   // Description:
   //    Once "afterLockAcquire" was invoked, there could be move heavy work
   // want to do, but can be done outside of bucket latch. This is the place
   // to execute them.
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIREPOSTACTION, "dmsTransLockCallback::afterLockAcquirePostAction" )
   void dmsTransLockCallback::afterLockAcquirePostAction
   (
      const dpsTransLockId &lockId
   )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIREPOSTACTION ) ;
      SINT32        rc      = SDB_OK ;
      BOOLEAN       found   = FALSE ;
      preIdxTreePtr dummy ;
      dmsRBSOffset  startPos, endPos ;
      DPS_TRANS_ID  transID = _eduCB->getTransID() ;
      dmsRecordID   rid( lockId.extentID(), lockId.offset() ) ;

      // decide if need to do any work
      if ( !isPostActionRequired() )
      {
         goto done ;
      }

      // setup search RBS range if it is index scan.
      // As for TBScan, it can directly hash and use RBS following
      // the chain.
      if ( _pScanner )
      {
         preIdxTreePtr dummy ;
         _pScanner->getRBSPositions(startPos, endPos, rid, dummy) ;
      }

      // TB scanner and IXdisk scanner can have invalid start/end Pos, at which
      // time was scan all RBS record. IXmemTree scanner must have one pos
      // valid to scan the RBS chain.
      if ( !_pScanner ||
           ( SCANNER_TYPE_DISK == _pScanner->getCurScanType() ) ||
           ( startPos.isValid() || endPos.isValid() ) )
      {
         rc = _rbsMgr->rbsGetRecord( _csLID, _clID, _clLID,
                                     rid, transID, found,
                                     *_rbsRecordData,
                                     startPos, endPos,
                                     _diskRecordTransID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to read record from RBS, rc: %d", rc ) ;
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

#if SDB_INTERNAL_DEBUG
      PD_LOG( PDDEBUG,
             "postaction for rid[%s], found=%d, "
             "_useOldVersion=%d, _skipRecord=%d, _needPostAction=%d, "
             "_rbsRecordData->isEmpty()=%d, rc=%d",
             lockId.toString().c_str(), found,
             _useOldVersion, _skipRecord, _needPostAction,
             (_rbsRecordData ? _rbsRecordData->isEmpty() : -1 ), rc ) ;
#endif
   done :
      _result = rc ;
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIREPOSTACTION ) ;
      return ;
   error :
      goto done ;
   }

   // Description:
   //   reads record on disk and verify the record trans version visibility
   //   before acquiring a record lock.
   // Note:
   //   It doesn't need the bucket latch / record lock, but it must be
   //   protected by mblatch latch to make sure no one can update/change
   //   the record it is going to read.
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_CHKRECVISIBLE, "dmsTransLockCallback::checkRecordVisible" )
   INT32 dmsTransLockCallback::checkRecordVisible( dmsMBContext *context, BOOLEAN *needData )
   {
      INT32        rc      = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_CHKRECVISIBLE ) ;

      DPS_TRANS_ID transID = _eduCB->getTransID() ;
      BOOLEAN      visible = FALSE ;

      // TBScan or disk index scan
      if ( ( transID.isValid() ) &&
           ( !_eduCB->isInTransRollback() ) &&
           ( TRANS_ISOLATION_RR == _transIsolation ) &&
           ( !_useLatestVersion ) &&
           ( ( !_pScanner ) ||
             ( SCANNER_TYPE_DISK == _pScanner->getCurScanType() ) ) )
      {
         // if no matter we have the record lock or not, always verify disk
         // version first. Following scenario described the case we could
         // endup using the disk version:
         // T1 start, T2 start, T3 start;
         // T2 did update on record(r1), hold record lock in X;
         // T3 tries to delete r1, wait on lock X;
         // As soon as T2 rollback, T1 tries to read r1 through idx scan,
         // coming from disk scanner. Note that T1 will fail on record lock
         // because there is a X waiter. But rollback will remove the old
         // version from the in memory tree, and put back the original
         // record. So the record should be visiable. The oldver is still
         // exist in LRBHdr. If we don't return the record, we could end up
         // skipping the record because _oldVer->idxLidExist() could be true.
         // We won't read partial page because we hold mbLatch in S.

         if ( context->mbStat()->getMaxGlobTransID() <
              _eduCB->getExpireTranCache() )
         {
            _recordOnDiskVisible = TRUE ;
            goto done ;
         }
         else if ( context->mbStat()->getMaxGlobTransID() <
                   _transCB->getExpiredVersion() )
         {
            _recordOnDiskVisible = TRUE ;
            _eduCB->setExpireTranCache( _transCB->getExpiredVersion() ) ;
            goto done ;
         }

         if ( NULL != needData && _recordRW->isEmpty() )
         {
            *needData = TRUE ;
            goto done ;
         }

         const dmsRecord* record = _recordRW->readPtr( 0 ) ;

         // record doesn't have glob trans, might come from
         // non-transactional update, visible
         if ( ! record->hasGlobTransID() )
         {
            _recordOnDiskVisible = TRUE ;
            goto done ;
         }
         DPS_TRANS_ID recTransID = record->getGlobTransID() ;
         stpLogicalTimeUS visibleTime( DPS_MAX_TRANS_TIME,
                                       STP_MAX_TIME_ERROR ) ;
         rc = _transCB->isVersionVisible( _eduCB,
                                          recTransID,
                                          transID,
                                          _eduCB->getTransBeginTime(),
                                          TRANS_ISOLATION_RR,
                                          FALSE,
                                          visible,
                                          &visibleTime ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check visibility for "
                      "read transaction [%s] against record"
                      "transaction [%s], rc: %d",
                      dpsTransIDToString( transID ).c_str(),
                      dpsTransIDToString( recTransID ).c_str(), rc ) ;

         if ( !visible )
         {
            // it is invisible, need to check older versions
            // check with global transaction available time on collection
            SDB_ASSERT( recTransID.isGlobTrans(),
                        "should be global transaction of record" ) ;
            UINT64 globTransAvailTime =
                  context->mbStat()->_globTransAvailTime.peek() ;
            if ( DPS_MAX_TRANS_TIME == globTransAvailTime )
            {
               stpAgent timeAgent ;
               stpLogicalTimeUS curTime ;
               // get global logical time
               PD_LOG( PDDEBUG, "Global transaction time is unvailable. "
                                "Try to get STP logical time" ) ;
               rc = timeAgent.getLogicalTimeUS( curTime,
                                                OSS_ONE_SEC,
                                                FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get STP logical time, rc:%d",
                            rc ) ;
               context->mbStat()
                      ->_globTransAvailTime.compareAndSwap( DPS_MAX_TRANS_TIME,
                                                            curTime.getTime() ) ;
               globTransAvailTime =
                     context->mbStat()->_globTransAvailTime.peek() ;
            }
            PD_CHECK( ( 0 == globTransAvailTime ) ||
                      ( DPS_MAX_TRANS_TIME == visibleTime.getTime() ) ||
                      ( visibleTime.getTime() > globTransAvailTime ),
                      SDB_GLOB_TRANS_NOT_AVAILABLE, error, PDERROR,
                      "Failed to check global transaction, available "
                      "timestamp on collection [%s] is [%llu], "
                      "current transaction [%s] is start from [%llu],"
                      "invisible record from transaction [%s] "
                      "will be visible on [%llu]",
                      context->mb()->_collectionName,
                      globTransAvailTime,
                      dpsTransIDToString( transID ).c_str(),
                      transID.getGlobSN(),
                      dpsTransIDToString( recTransID ).c_str(),
                      visibleTime.getTime() ) ;
         }

         _recordOnDiskVisible = visible ;
         _diskRecordTransID = recTransID ;
      }
      else
      {
         if ( NULL != needData && _recordRW->isEmpty() )
         {
            *needData = TRUE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSTRANSLOCKCALLBACK_CHKRECVISIBLE, rc ) ;
      return rc ;

   error:
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
                                                 dpsLRBExtData *pExtData )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE ) ;

<<<<<<< HEAD
      BOOLEAN hasLock = -1 != _getLatchedIdxMode() ? TRUE : FALSE ;
      dmsOnTransLockRelease( lockId, lockMode, refCounter, pExtData,
                             _latchedIdxLid, hasLock ) ;
=======
      dmsOnTransLockRelease( lockId, lockMode, refCounter,
                             _nonTransNeedCleanup, _transCB, _eduCB,
                             pExtData ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKESCALATED, "dmsTransLockCallback::afterLockEscalated" )
<<<<<<< HEAD
   INT32 dmsTransLockCallback::afterLockEscalated( const dpsTransLockId &lockId,
                                                   DPS_TRANSLOCK_OP_MODE_TYPE opMode )
   {
      INT32 rc = SDB_OK ;

=======
   void dmsTransLockCallback::afterLockEscalated( const dpsTransLockId &lockId,
                                                  DPS_TRANSLOCK_OP_MODE_TYPE opMode )
   {
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKESCALATED ) ;

      _recordInfo._transLockEscalated = TRUE ;

<<<<<<< HEAD
      if ( _pScanner )
      {
         // we are lock escalated now, we should see the disk records
         // if current RID is from memory tree, we should skip it
         if ( SCANNER_TYPE_MEM_TREE == _pScanner->getCurScanType() )
         {
            _skipRecord = TRUE ;
            _oldVer = NULL ;
            _pScanner->removeDuplicatRID( dmsRecordID( lockId.extentID(),
                                                       lockId.offset() ) ) ;
         }

         if ( _pScanner->isTypeEnabled( SCANNER_TYPE_MEM_TREE ) )
         {
            BOOLEAN isCursorSame = FALSE ;

            // pause scanner to disabled memory index scanner
            rc = _pScanner->pauseScan() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pause scanner, rc: %d", rc ) ;

            _pScanner->disableByType( SCANNER_TYPE_MEM_TREE ) ;

            rc = _pScanner->resumeScan( isCursorSame ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to resume scanner, rc: %d", rc ) ;

            if ( !isCursorSame && !_skipRecord )
            {
               // cursor changed after resume, skip record
               _skipRecord = TRUE ;
               _oldVer = NULL ;
               _pScanner->removeDuplicatRID( dmsRecordID( lockId.extentID(),
                                                          lockId.offset() ) ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKESCALATED, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // Description
   // Dependency: Caller must hold the mbLcok
   INT32 dmsTransLockCallback::saveOldVersionRecord( const dmsRecordID &rid,
                                                     const BSONObj &obj,
                                                     UINT32 ownnerTID,
                                                     BOOLEAN isDeleting )
=======
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKESCALATED ) ;
   }

   // Description
   // Dependency: Caller must hold the recordlock and mbLcok
   INT32 dmsTransLockCallback::saveOldVersionRecord( const _dmsRecordRW *pRecordRW,
                                                     const dmsRecordID  &rid,
                                                     const UINT32        clLID,
                                                     const BSONObj      &obj,
                                                     const UINT32        ownerTID )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
<<<<<<< HEAD
            // save record
            rc = _oldVer->saveRecord( obj, ownnerTID ) ;
=======
            const dmsRecord *pRecord= pRecordRW->readPtr( 0 ) ;
#if SDB_INTERNAL_DEBUG

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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
         // hang the old version container to the linked list
         if ( !_unitPtr.get() )
         {
            oldVersionCB *oldCB = _transCB->getOldVCB() ;
            rc = oldCB->getOrCreateOldVersionUnit( _csID, _clID, _unitPtr ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         rc = _unitPtr->addToChain( _oldVer, isDeleting ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else if ( _oldVer &&
                _oldVer->isOnChain() &&
                isDeleting )
      {
         if ( !_unitPtr.get() )
         {
            oldVersionCB *oldCB = _transCB->getOldVCB() ;
            _unitPtr = oldCB->getOldVersionUnit( _csID, _clID ) ;
            // old version is on chain, should be valid
            SDB_ASSERT( _unitPtr, "should be valid" ) ;
            PD_CHECK( _unitPtr, SDB_SYS, error, PDERROR, "Failed to get old verion unit " ) ;
         }
         rc = _unitPtr->addToDeleting( rid ) ;
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

   INT32 dmsTransLockCallback::_getLatchedIdxMode()
   {
      if ( NULL != _pScanner )
      {
         return _pScanner->getIdxLockModeByType( SCANNER_TYPE_MEM_TREE ) ;
      }

      return -1 ;
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
<<<<<<< HEAD
              -1 == _getLatchedIdxMode() )
=======
              -1 == idxTreeLatchMode() )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
      INT32 rc = SDB_OK ;
      if ( _opHandler )
      {
         rc = _opHandler->onInsertRecord( context,  object, rid,
                                          pRecordRW, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( _oldVer )
      {
         _oldVer->setRecordNew( cb->getTID() ) ;
      }

      // mark insert by self
<<<<<<< HEAD
=======
      context->mbStat()->updateGlobTransIDWithComp( _eduCB->getTransID() ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      _recordInfo._transInsert = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onDeleteRecord( _dmsMBContext *context,
                                               const BSONObj &object,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               BOOLEAN markDeleting,
                                               _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
<<<<<<< HEAD
=======

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      if ( _opHandler )
      {
         rc = _opHandler->onDeleteRecord( context, object, rid, pRecordRW,
                                          markDeleting, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

<<<<<<< HEAD
      rc = saveOldVersionRecord( rid, object, cb->getTID(), TRUE ) ;
=======
      rc = saveOldVersionRecord( pRecordRW, rid, context->clLID(),
                                 object, cb->getTID() ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      if ( SDB_OK == rc && markDeleting && _oldVer )
      {
         _oldVer->setDiskDeleting() ;
      }
      if ( !markDeleting &&
           ( _recordInfo._transInsert ||
             ( cb->isInTransRollback() &&
               !cb->isTakeOverTransRB() ) ) )
      {
         // if the record is deleted from disk during transaction,
         // mark it in record info, so the scanner can release
         // transaction lock for this record later
         _recordInfo._transInsertDeleted = TRUE ;
      }
      else
      {
         _recordInfo._transInsertDeleted = FALSE ;
      }
<<<<<<< HEAD
=======
      context->mbStat()->updateGlobTransIDWithComp( _eduCB->getTransID() ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onUpdateRecord( _dmsMBContext *context,
                                               const BSONObj &orignalObj,
                                               const BSONObj &newObj,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      if ( _opHandler )
      {
         rc = _opHandler->onUpdateRecord( context, orignalObj, newObj, rid,
                                          pRecordRW, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

<<<<<<< HEAD
      rc = saveOldVersionRecord( rid, orignalObj, cb->getTID(), FALSE ) ;
=======
      context->mbStat()->updateGlobTransIDWithComp( _eduCB->getTransID() ) ;
      rc = saveOldVersionRecord( pRecordRW, rid, context->clLID(),
                                 orignalObj, cb->getTID() ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   done:
      return rc ;
   error:
      goto done ;
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

      if ( _opHandler )
      {
         rc = _opHandler->onInsertIndex( context, indexCB, isUnique, isEnforce,
                                         keySet, rid, cb, pResult ) ;
         if ( rc )
         {
            goto error ;
         }
      }

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

      if ( _opHandler )
      {
         rc = _opHandler->onInsertIndex( context, indexCB, isUnique, isEnforce,
                                         keyObj, rid, cb, pResult ) ;
         if ( rc )
         {
            goto error ;
         }
      }

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
                                                  INT32 indexID,
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

<<<<<<< HEAD
      if ( _latchedIdxLid == gid._idxLID && _getLatchedIdxMode() != -1 )
      {
         if ( EXCLUSIVE == _getLatchedIdxMode() )
=======
      if ( _latchedIdxLid == gid._idxLID && idxTreeLatchMode() != -1 )
      {
         if ( EXCLUSIVE == idxTreeLatchMode() )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         {
            hasLocked = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock mode(%d) is not EXCLUSIVE(%d)",
<<<<<<< HEAD
                    _getLatchedIdxMode(), EXCLUSIVE ) ;
=======
                    idxTreeLatchMode(), EXCLUSIVE ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
            SDB_ASSERT( FALSE, "Lock mode is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      // use owner transID to insert into the mem tree
      rc = treePtr->insertWithOldVer( &keyObj, rid, _oldVer, hasLocked,
                                      this->getOwnerTransID(), this,
                                      indexID ) ;
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

      if ( _opHandler )
      {
         rc = _opHandler->onDeleteIndex( context, indexCB, isUnique, keySet,
                                         rid, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }

      /// insert key to mem tree
      for ( BSONObjSet::const_iterator cit = keySet.begin() ;
            cit != keySet.end() ;
            ++cit )
      {
         rc = _checkDeleteIndex( treePtr,deleteCursor, -1, indexCB,
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
                                              INT32 indexID,
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

      if ( _opHandler )
      {
<<<<<<< HEAD
         rc = _opHandler->onUpdateIndex( context, indexCB, isUnique, isEnforce,
                                         oldKeySet, newKeySet, rid, isRollback,
                                         cb, pResult ) ;
=======
         rc = _opHandler->onUpdateIndex( context, indexID, indexCB, isUnique,
                                         isEnforce, oldKeySet, newKeySet, rid,
                                         isRollback, cb, pResult ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         if ( rc )
         {
            goto error ;
         }
      }

      /// not use transaction
      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }
      /// rollback
      else if ( isRollback )
      {
         // check rollback on index
         rc = _checkRollbackIndex( indexID, indexCB, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to undo index [%s], rc: %d",
                      indexCB->getName(), rc ) ;
         goto done ;
      }

      if ( oldKeySet.size() != newKeySet.size() )
      {
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
            rc = _checkDeleteIndex( treePtr, deleteCursor, indexID, indexCB,
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
      INT32 rc = SDB_OK ;

      if ( _opHandler )
      {
         rc = _opHandler->onDropIndex( context, indexCB, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         pOldVCB->delIdxTree( gid, FALSE ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
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

      if ( _opHandler )
      {
         rc = _opHandler->onRebuildIndex( context, indexCB, cb, pResult ) ;
         if ( rc )
         {
            goto error ;
         }
      }

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

                  rc = _checkDeleteIndex( treePtr,deleteCursor, -1, indexCB,
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

      if ( _opHandler )
      {
         rc = _opHandler->onCreateIndex( context, indexCB, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

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
      if ( _opHandler )
      {
         _opHandler->onCSClosed( csID ) ;
      }

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
      if ( _opHandler )
      {
         _opHandler->onCLTruncated( csID, clID ) ;
      }

      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         pOldVCB->clearIdxTreeByCLID( csID, clID, FALSE ) ;
         pOldVCB->delOldVersionUnit( csID, clID ) ;
      }
   }
<<<<<<< HEAD
}
=======

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK__CHKROLLBACKIDX, "dmsTransLockCallback::_checkRollbackIndex" )
   INT32 dmsTransLockCallback::_checkRollbackIndex( INT32 indexID,
                                                    const ixmIndexCB *indexCB,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK__CHKROLLBACKIDX ) ;

      SDB_ASSERT( NULL != indexCB, "index is invalid" ) ;

      // test if we need to rollback on given index
      if ( NULL != _oldVer &&
           NULL != cb &&
           isIndexUpdated( indexID ) &&
           _oldVer->idxLidExist( indexCB->getLogicalID() ) )
      {
         oldVersionCB *oldVerCB = _transCB->getOldVCB() ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         preIdxTreePtr treePtr = oldVerCB->getIdxTree( gid, FALSE ) ;
         BOOLEAN hasLocked = FALSE ;

         // check if mem-tree already has locked
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

         if ( NULL != treePtr.get() )
         {
            // remove index item from given index tree
            _oldVer->releaseIndex( this, treePtr, hasLocked ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSTRANSLOCKCALLBACK__CHKROLLBACKIDX, rc ) ;
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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

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
