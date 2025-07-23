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

   Source File Name = dpsTransLockMgr.hpp

   Descriptive Name = DPS Lock manager header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/28/2018  JT  Initial Draft, locking performance improvement

   Last Changed =

*******************************************************************************/
#ifndef DPSTRANSLOCKMANAGER_HPP_
#define DPSTRANSLOCKMANAGER_HPP_

#include "utilSegment.hpp"
#include "dpsTransLRB.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransDef.hpp"
#include "monDps.hpp"
#include "ossAtomic.hpp"
#include "ossRWMutex.hpp"

namespace engine
{
   class _dpsTransExecutor ;
   class _dpsITransLockCallback ;
   class _IContext ;
 
   // lock bucket,  524287 ( a prime number close to 524288  ) for now
   #define DPS_TRANS_LOCKBUCKET_SLOTS_MAX ( (UINT32) 524287 ) 
   #define DPS_TRANS_INVALID_BUCKET_SLOT DPS_TRANS_LOCKBUCKET_SLOTS_MAX

   class dpsTransLockManager : public SDBObject
   {
      friend class _dpsTransExecutor ;
   public:
      dpsTransLockManager();

      virtual ~dpsTransLockManager();

      // initialization,
      //   . init _LockHdrBkt
      //   . allocate segment for LRB and LRB Header
      INT32 init( UINT32 lrbInitNum = DPS_TRANS_LRB_INIT_DFT,
                  UINT32 lrbTotalNum = DPS_TRANS_LRB_TOTAL_DFT ) ;

      // free allocated LRB and LRB Header segments
      void fini() ;

      // if a lock has any waiters ( upgrade and waiter list )
      BOOLEAN hasWait( const dpsTransLockId &lockId );

      // if lock manager has been initialized
      BOOLEAN isInitialized() { return _initialized ; } ; 

      // acquire a lock with given mode, the higher level lock ( CL, CS )
      // will be also acquired respectively
      // If a lock can't be acquired right away, the request will be added
      // to waiter/upgrade queue and wait for the lock till it be waken up,
      // lock waiting timeout elapsed, or be interrupted. When it is woken up,
      // it will try to acquire the lock again.
      INT32 acquire
      (
         _dpsTransExecutor        * dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         _IContext                * pContext      = NULL,
         dpsTransRetInfo          * pdpsTxResInfo = NULL,
         _dpsITransLockCallback   * callback = NULL
      ) ;

      // release a lock. The higher level intent lock will be also released
      // respectively. It will also wake up the first one in upgrade or
      // waiter queue if it is necessary.
      void release
      (
         _dpsTransExecutor      * dpsTxExectr,
         const dpsTransLockId   & lockId,
         const BOOLEAN            bForceRelease = FALSE,
         _dpsITransLockCallback * callback = NULL
      ) ;

      // release all locks an executor ( EDU ) holding. The executor ( EDU )
      // walks through its EDU LRB chain ( all locks acquired within a tx ),
      // and release all locks its holding and wake up the first one in waiter
      // or upgrade queue if it is necessary.
      void releaseAll
      (
         _dpsTransExecutor      * dpsTxExectr, 
         _dpsITransLockCallback * callback = NULL
      ) ;

      // try to acquire a lock with given mode and will try to acquire higher
      // level intent lock respectively.
      // If a lock can not be acquired right away, it will NOT put itself in
      // wait/upgrade queue, simply return error SDB_DPS_TRANS_LOCK_INCOMPATIBLE
      INT32 tryAcquire
      (
         _dpsTransExecutor        * dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         dpsTransRetInfo          * pdpsTxResInfo = NULL,
         _dpsITransLockCallback   * callback = NULL
      ) ;

      // test if a lock with give lock mode can be acquired, higher level intent
      // lock will also be tested.  It will not acquire the lock nor wait
      // the lock.
      // If the test fails, error code SDB_DPS_TRANS_LOCK_INCOMPATIBLE will be
      // returned.
      INT32 testAcquire
      (
         _dpsTransExecutor        * dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         dpsTransRetInfo          * pdpsTxResInfo = NULL,
         _dpsITransLockCallback   * callback = NULL
      ) ;

      // Latching for monitoring / dumping locking info of an EDU.
      //   . latch _rwMutext in exclusive mode
      OSS_INLINE void acquireMonLatch() { _rwMutex.lock_w() ; }

      // release monitoring / dumping latch
      OSS_INLINE void releaseMonLatch() { _rwMutex.release_w() ; }

      // dump specific lock info to a file for debugging purpose
      void dumpLockInfo
      (
         const dpsTransLockId & lockId,
         const CHAR           * fileName,
         BOOLEAN                bOutputInPlainMode = FALSE
      ) ;

      // dump all holding locks of an executor( EDU ) to a file for debugging.
      // The caller shall acquire the monitoring( dump ) latch before
      // calling this function and make sure the dpsTxExectr is still valid
      void dumpLockInfo
      (
         _dpsTransExecutor * dpsTxExectr,
         const CHAR        * fileName,
         BOOLEAN             bOutputInPlainMode = FALSE
      ) ;

      // dump all holding locks of an executor ( EDU )
      // the caller shall acquire the monitoring( dump ) latch before
      // calling this function and make sure the dpsTxExectr is still valid
      void dumpLockInfo
      (
         dpsTransLRB       * lastLRB,
         VEC_TRANSLOCKCUR  & vecLocks
      ) ;
     
      // dump the waiting lock info of an executor ( EDU ) 
      // the caller shall acquire the monitoring( dump ) latch before
      // calling this function and make sure the dpsTxExectr is still valid
      void dumpLockInfo
      (
         dpsTransLRB     * lrb,
         monTransLockCur & lockCur
      ) ;

      // dump a specific lock info ( waiter, owners, upgrade list etc )
      void dumpLockInfo
      (
         const dpsTransLockId & lockId,
         monTransLockInfo     & monLockInfo
      ) ;

      // get LRB Header ( index and its pointer ) of a given lock from
      // LRB Header bucket. Wrapper of _getLRBHdrByLockId
      BOOLEAN getLRBHdrByLockId
      (
         const dpsTransLockId & lockId,
         dpsTransLRBHeader *  & pLRBHdr
      ) ;

      // try to free LRB and LRB header segments higher current high water mark
      // this function is desgined to be called by background thread
      // periodically
      void tryToShrinkLRBAndLRBHeaderPool() ;

   private:
      // Latch for normal lock operation ( acquire, tryAcquire,
      // testAcquire, release, releaseAll, hasWait etc on ) :
      //     . latch _rwMutext in shared mode
      //     . latch a bucket slot in exclusively
      OSS_INLINE void _acquireOpLatch ( const UINT32 bucketIndex )
      {
         _rwMutex.lock_r() ;
         _LockHdrBkt[ bucketIndex ].hashHdrLatch.get() ;
      }

      // release latches for normal lock operation
      OSS_INLINE void _releaseOpLatch ( const UINT32 bucketIndex )
      {
         _LockHdrBkt[ bucketIndex ].hashHdrLatch.release() ;
         _rwMutex.release_r() ;
      }

      // release/return a LRB Header to LRB Header manager
      INT32 _releaseLRBHdr( dpsTransLRBHeader * hdrLRB ) ;

      // release/return a LRB to LRB manager 
      INT32 _releaseLRB( dpsTransLRB * lrb );

      // search LRB list ( owner, waiter or upgrade list ) and find
      // the one with given eduId
      BOOLEAN _getLRBByEDUId
      (
         const EDUID     eduId,
         dpsTransLRB *   lrbBegin,
         dpsTransLRB * & pLRBEduId
      ) ;

      // search the EDU LRB list and find the LRB associated with same lock Id
      dpsTransLRB * _getLRBFromEDULRBList
      (
         _dpsTransExecutor    * dpsTxExectr,
         const dpsTransLockId & lockId
      ) ;

      // get LRB Header poiner of a given lock from LRB Header bucket
      BOOLEAN _getLRBHdrByLockId
      (
         const dpsTransLockId & lockId,
         dpsTransLRBHeader *  & pLRBHdr
      ) ;

      // walk through the LRB list check if the input
      // lockMode is compatible with others, and find
      // first incompatible
      BOOLEAN _checkLockModeWithOthers
      (
         const dpsTransLRB *  lrbBegin,
         const dpsTransLRB *  pLRBToBeChecked,
         dpsTransLRB *     &  pLRBIncompatible
      ) ;

      // add a LRB at the end of the queue ( waiter or upgrade list )
      void _addToLRBListTail
      (
         dpsTransLRB * & lrbBegin,
         dpsTransLRB *   idxNew
      ) ;

      // add a LRB at the beginning of the queue ( owner list )
      void _addToLRBListHead
      (
         dpsTransLRB * & lrbBegin,
         dpsTransLRB *   lrbNew
      ) ;

      // add a LRB at the given position in owner list ( the owner list is
      // sorted on lock mode in descending order )
      void _addToOwnerLRBList
      (
         dpsTransLRB * insPos,
         dpsTransLRB * idxNew
      ) ; 

      // search owner LRB list, and find
      //  . if the edu is in owner list
      //  . the position where the new LRB shall be inserted after
      //  . the pointer of first incompatible LRB
      void _searchOwnerLRBList
      (
         const _dpsTransExecutor * dpsTxExectr,
         const DPS_TRANSLOCK_TYPE  lockMode,
         dpsTransLRB *             lrbBegin,
         dpsTransLRB *           & pLRBToInsert,
         dpsTransLRB *           & pLRBIncompatible,
         dpsTransLRB *           & pLRBOwner
      ) ;

      // search owner LRB list, and find
      //  . the position where the new LRB shall be inserted after
      //  . the pointer of first incompatible LRB
      void _searchOwnerLRBListForInsertAndIncompatible
      (
         const _dpsTransExecutor * dpsTxExectr,
         const DPS_TRANSLOCK_TYPE  lockMode,
         dpsTransLRB             * lrbBegin,
         dpsTransLRB *           & pLRBToInsert,
         dpsTransLRB *           & pLRBIncompatible
      ) ;

      void _moveToEDULRBListTail
      (
         _dpsTransExecutor    * dpsTxExectr,
         dpsTransLRB          * insLRB
      ) ;

      // add a LRB at the end of the EDU LRB chain,
      // which is a list of all locks acquired within a session/tx
      void _addToEDULRBListTail
      (
         _dpsTransExecutor    * dpsTxExectr,
         dpsTransLRB          * lrb,
         const dpsTransLockId & lockId
      ) ;

      // remove a LRB from a LRB list ( owner, waiter, upgrade list )
      void _removeFromLRBList
      (
         dpsTransLRB * & idxBegin,
         dpsTransLRB *   lrb
      ) ;

      // remove LRB from waiter or upgrade queue/list,
      // and wake up the next one in the queue if necessary
      void _removeFromUpgradeOrWaitList
      (
         _dpsTransExecutor    * dpsTxExectr,
         const dpsTransLockId & lockId,
         const UINT32           bktIdx,
         const BOOLEAN          removeLRBHeader
      ) ;

  
      // remove a LRB Header from a LRB Header list
      void _removeFromLRBHeaderList
      (
         dpsTransLRBHeader * & lrbBegin,
         dpsTransLRBHeader *   lrbDel
      ) ;


      // remove a LRB from the EDU LRB list
      void _removeFromEDULRBList
      (
         _dpsTransExecutor    * dpsTxExectr,
         dpsTransLRB          * lrb,
         const dpsTransLockId & lockId
      ) ;

      // core logic of acquire, try or test to get a lock with given mode
      INT32 _tryAcquireOrTest
      (
         _dpsTransExecutor                * dpsTxExectr,
         const dpsTransLockId             & lockId,
         const DPS_TRANSLOCK_TYPE           requestLockMode,
         const DPS_TRANSLOCK_OP_MODE_TYPE   opMode,
         UINT32                             bktIdx,
         const BOOLEAN                      bktLatched,
         dpsTransRetInfo                  * pdpsTxResInfo,
         _dpsITransLockCallback           * callback = NULL
      ) ;

      // core logic of release a lock
      void _release
      (
         _dpsTransExecutor       * dpsTxExectr,
         const dpsTransLockId    & lockId,
         dpsTransLRB             * pOwnerLRB,
         const BOOLEAN             bForceRelease,
         UINT32                  & refCountToDecrease,
         _dpsITransLockCallback  * callback = NULL
      ) ;

      // acquire and setup a new LRB header and a new LRB
      INT32 _prepareNewLRBAndHeader
      (
         _dpsTransExecutor *        dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         const UINT32               bktIdx,
         dpsTransLRBHeader *      & pLRBHdrNew,
         dpsTransLRB       *      & pLRBNew
      ) ;

      // calculate the index to LRB Header bucket
      UINT32 _getBucketNo( const dpsTransLockId &lockId );

      // wakeup a lock waiting exectuor ( EDU )
      void _wakeUp( _dpsTransExecutor * dpsTxExectr ) ;

      // wait a lock till be woken up, lock timeout elapsed, or be interrupted
      INT32 _waitLock( _dpsTransExecutor * dpsTxExectr ) ;

      // format LRB to string, flat one line
      CHAR * _LRBToString ( dpsTransLRB * lrb, CHAR * pBuf, UINT32 bufSz ) ;

      // format LRB to string, one field/member per line, with optional prefix
      CHAR * _LRBToString 
      (
         dpsTransLRB * lrb,
         CHAR *            pBuf,
         UINT32            bufSz,
         CHAR *            prefix
      ) ;

      // format LRB Header to string, flat one line
      CHAR * _LRBHdrToString 
      (
         dpsTransLRBHeader * lrbHdr,
         CHAR              * pBuf,
         UINT32              bufSz
      ) ;

      // format LRB Header, one field/member per line, with optional prefix
      CHAR * _LRBHdrToString
      (
         dpsTransLRBHeader * lrbHdr,
         CHAR              * pBuf,
         UINT32              bufSz,
         CHAR              * prefix
      ) ;

   private:
      // LRB manager
      _utilSegmentManager< dpsTransLRB       > * _pLRBMgr ;

      // LRB Header manager
      _utilSegmentManager< dpsTransLRBHeader > * _pLRBHdrMgr ;

      // LRB Header bucket :
      //   class dpsTransLRBHeaderHash : public SDBObject
      //   {
      //   public :
      //      dpsTransLRBHeader    lrbHdr ;  -- LRB Header
      //      ossSpinXLatch  hashHdrLatch ;  -- bucket slot latch
      //   } ;
      dpsTransLRBHeaderHash  _LockHdrBkt[ DPS_TRANS_LOCKBUCKET_SLOTS_MAX ] ;

      // flag mark if lock manager has been initialized
      BOOLEAN                _initialized ;

      //
      // monitor/dump EDU locking info latch
      //
      // Latching protocol :
      // Monitoring/dumping locking info of a specific EDU is mutually
      // exclusive with normal lock operation( acquire, tryAcquire,
      // testAcquire, release, releaseAll, hasWait etc on )
      //   Normal lock operation :
      //     . latch _rwMutext in shared mode
      //     . latch a bucket slot in exclusively
      //   Monitoring/dump EDU locking info :
      //     . latch _rwMutext in exclusive mode
      ossRWMutex             _rwMutex ;
   } ;
}

#endif // DPSTRANSLOCKMANAGER_HPP_

