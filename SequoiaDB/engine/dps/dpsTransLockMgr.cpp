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

   Source File Name = dpsTransLockMgr.cpp

   Descriptive Name = DPS lock manager

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     JT  10/28/2018, locking performance improvement

*******************************************************************************/
#include "dpsTransLockMgr.hpp"
#include "dpsTransExecutor.hpp"
#include "dpsTransLockCallback.hpp"
#include "dpsTransDef.hpp"
#include "dpsTrace.hpp"
#include "pd.hpp"
#include "dpsTrace.hpp"
#include "pdTrace.hpp"
#include "sdbInterface.hpp"   // IContext

#include <stdio.h>
#if 0
//#ifdef _DEBUG    // once we upgrade the boost, we might want to have this
#include <boost/stacktrace.hpp>
#endif


namespace engine
{
   #define DPS_LOCKID_STRING_MAX_SIZE      ( 128 )
   #define DPS_TRANSLOCK_DUMP_SLICE_SIZE   ( 1000 )

   // thredshold of EDU LRB list length when search whether owning a lock
   #define DPS_LOCK_PRE_SEARCH_EDULIST_THRESHOLD ( 8 )

   dpsTransLockManager::dpsTransLockManager( LOCKMGR_TYPE managerType )
   : _LockHdrBkt( NULL ),
     _bktSlotMax( 0 ) ,
     _initialized( FALSE ),
     _lockMgrType( managerType ),
     _autoUpperLockOp( TRUE )
   {
   }

   dpsTransLockManager::~dpsTransLockManager()
   {
      if ( _initialized )
      {
         fini() ;
      }
   }


   //
   // Description: free allocated LRBs, LRB Headers and reset buckets
   // Input:       none
   // Output:      none
   // Return:      none
   // Dependency:  this function is called during system shutdown,
   //              the caller shall make sure no threads are accessing locking
   //              resource.
   void dpsTransLockManager::fini()
   {
      if ( _initialized )
      {
         _initialized = FALSE ;
         if ( _LockHdrBkt )
         {
            SDB_OSS_DEL [] _LockHdrBkt ;
            _LockHdrBkt = NULL ;
         }
      }
   }

   //
   // Description: Initialize lock manager
   //              . initialize bucket
   // Input:
   //    bucketSize             -- hash bucket size, better be a prime number
   //    autoOperateOnUpperLock -- whether automatically operate on upper
   //                              level lock
   // Output:      none
   // Return:      SDB_OK  :  lock manager successefully initialized
   //              SDB_OOM :  failed to initialize lock manager due to
   //                         lack of memory
   // Dependency:  this function is called during system starting up,
   //              the caller shall make sure no thread is trying to access
   //              lock resource before lock manager is fully initialized
   //
   INT32 dpsTransLockManager::init
   (
      UINT32 bucketSize,
      BOOLEAN autoOperateOnUpperLock
   )
   {
      _bktSlotMax      = bucketSize ;
      _autoUpperLockOp = autoOperateOnUpperLock ;

      _LockHdrBkt = SDB_OSS_NEW dpsTransLRBHeaderHash[ _bktSlotMax + 1 ] ;
      if ( NULL == _LockHdrBkt )
      {
         PD_LOG( PDERROR,
                 "Failed to allocate memory for lock bucket, bucket size:%d",
                 _bktSlotMax + 1 ) ;
         return SDB_OOM ;
      }

      // set initialized flag
      _initialized = TRUE ;
      return SDB_OK ;
   }

   //
   //
   // Description: search the LRB Header chain and find the one with same lockId
   // Function:    walk through LRB Header list/chain, find the one with same
   //              lockId.
   // Input:
   //    lockId   -- lock Id
   //    pLRBHdr  -- the first LRB Header object in the chain
   // Output:
   //    pLRBHdr  -- the pointer of first LRB Header object matches
   //                the lockId if it is found. If not, it shall be the
   //                pointer of the last LRB Header object in the list
   //
   // Return:     true  -- found the LRB Header object with same lockId
   //             false -- not found
   //
   // Dependency:  the lock bucket latch shall be acquired
   //
   BOOLEAN dpsTransLockManager::_getLRBHdrByLockId
   (
      const dpsTransLockId & lockId,
      dpsTransLRBHeader *  & pLRBHdr
   )
   {
      BOOLEAN found = FALSE ;
      dpsTransLRBHeader * pLocal = pLRBHdr;
      while ( NULL != pLocal )
      {
         pLRBHdr = pLocal;
         if ( lockId == pLocal->lockId )
         {
            found = TRUE ;
            break ;
         }
         pLocal = pLocal->nextLRBHdr ;
      }
      return found ;
   }


   //
   // Description: walk through the LRB list check if the input
   //              lockMode is compatible with others in the queue,
   //              and find the first incompatible
   // Input:
   //    lrbBegin -- the LRB in the queue to start searching
   //    dpsTxExectr --  the request dpsTxExectr
   //    requestLockMode -- the request lock mode
   // Output:
   //    pLRBIncompatible -- the first incompatible LRB
   // Return:
   //    TRUE     -- if an incompatible one is found in the queue
   //    FALSE    -- compatible with owners
   // Dependency:  the lock bucket latch shall be acquired
   //
   BOOLEAN dpsTransLockManager::_checkLockModeWithOthers
   (
      const dpsTransLRB *         lrbBegin,
      _dpsTransExecutor        *  dpsTxExectr,
      const DPS_TRANSLOCK_TYPE    requestLockMode,
      dpsTransLRB *            &  pLRBIncompatible,
      BOOLEAN                     bSortedAndSUXModeList
   )
   {
      dpsTransLRB *plrb = (dpsTransLRB *)lrbBegin ;
      BOOLEAN foundIncomp = FALSE ;

      pLRBIncompatible = NULL ;
      if ( NULL == lrbBegin )
      {
         goto exit ;
      }

      // the owner list is sorted on lockMode in descending order
      // so we may have chance of early exit
      if ( bSortedAndSUXModeList )
      {
         // if first owner mode is X or request mode is X
         // and the owner is not the requestor, then
         // the first incompatbile owner LRB is found
         if ( ( ( DPS_TRANSLOCK_X == plrb->lockMode ) ||
                ( DPS_TRANSLOCK_X == requestLockMode ) ) &&
              ( dpsTxExectr != plrb->dpsTxExectr ) )
         {
            // save the address/pointer of first incompatible LRB
            pLRBIncompatible = plrb ;
            foundIncomp = TRUE ;
            goto exit ;
         }
      }

      while ( plrb )
      {
         if ( bSortedAndSUXModeList )
         {
            // as the owner list is sorted on lockMode in
            // descending order, if current owner is S
            // and it is compatible with request lockMode,
            // then others in the list will be compatible with
            // the request lockMode.
            if ( DPS_TRANSLOCK_S == plrb->lockMode )
            {
               if ( dpsIsLockCompatible( plrb->lockMode, requestLockMode ) )
               {
                  break ;
               }
               else if ( dpsTxExectr != plrb->dpsTxExectr )
               {
                  // save the address/pointer of first incompatible LRB
                  pLRBIncompatible = plrb ;
                  foundIncomp = TRUE ;
                  break ;
               }
            }
         }
         if ( ( dpsTxExectr != plrb->dpsTxExectr ) &&
              ( ! dpsIsLockCompatible( plrb->lockMode, requestLockMode ) ) )
         {
            pLRBIncompatible = plrb ;
            foundIncomp = TRUE ;
            break ;
         }
         plrb = plrb->nextLRB ;
      }

   exit :
      return foundIncomp ;
   }


   //
   // Description: walk through the upgrade list check if the request
   //              LRB might be dead-lock with others, and
   //              and find the first incompatible one
   // Input:
   //    lrbBegin -- the LRB in the uprade queue to start searching
   //    pLRBTobeChecked -- the input LRB to be checked with others
   // Output:
   //    pLRBIncompatible -- the first incompatible LRB
   // Return:
   //    TRUE     -- if a potential dead-lock issue is detected`
   //    FALSE    -- no dead-lock deteced
   // Dependency:  the lock bucket latch shall be acquired
   //
   BOOLEAN dpsTransLockManager::_deadlockDetectedWhenUpgrade
   (
      const dpsTransLRB *  lrbBegin,
      const dpsTransLRB *  pLRBTobeChecked,
      dpsTransLRB *     &  pLRBIncompatible
   )
   {
      dpsTransLRB *plrb = (dpsTransLRB *)lrbBegin ;
      BOOLEAN foundIncomp = FALSE ;

      pLRBIncompatible = NULL ;
      if ( ( NULL == pLRBTobeChecked ) || ( NULL == lrbBegin ) )
      {
         goto exit ;
      }

      while ( plrb )
      {
         if ( ( ! dpsIsLockCompatible( plrb->originMode,
                                       pLRBTobeChecked->lockMode ) ) ||
              ( ! dpsIsLockCompatible( plrb->lockMode,
                                       pLRBTobeChecked->originMode ) ) )
         {
            pLRBIncompatible = plrb ;
            foundIncomp = TRUE ;
            break ;
         }
         plrb = plrb->nextLRB ;
      }

   exit :
      return foundIncomp ;
   }


   //
   // Description: add a LRB at the end of the LRB chain/list
   // Function:    walk through LRB list/chain, add the LRB at the end of
   //              list( waiter or upgrade )
   // Input:
   //    queueType -- queue type
   //                    LOCKMGR_LRB_QUE_WAITER ,  waiter list
   //                    LOCKMGR_LRB_QUE_UPGRADE,  upgrade list
   //    lrbBegin  -- the first LRB in the chain( owner, waiter or upgrade
   //                 queue )
   //    lrbNew    -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToLRBListTail
   (
      LOCKMGR_LRB_QUE_TYPE   queueType,
      dpsTransLRB*         & lrbBegin,
      dpsTransLRB*           lrbNew
   )
   {
      dpsTransLRBHeader * pLRBHdr = lrbNew->lrbHdr ;

      if ( lrbBegin )
      {
#ifdef _DEBUG
         SDB_ASSERT( (pLRBHdr == lrbBegin->lrbHdr), "Invalid pLRBHdr" ) ;
         SDB_ASSERT( ( ( LOCKMGR_LRB_QUE_WAITER  == queueType ) ||
                       ( LOCKMGR_LRB_QUE_UPGRADE == queueType ) ),
                     "Invalid queueType" ) ;
#endif
         dpsTransLRB * plrb ;

         // waiter list
         if ( LOCKMGR_LRB_QUE_WAITER == queueType )
         {
#ifdef _DEBUG
            SDB_ASSERT( (pLRBHdr->waiterTail), "Invalid waiterTail" ) ;
#endif
            // get last LRB
            plrb = pLRBHdr->waiterTail;
            // set waitTail
            pLRBHdr->waiterTail = lrbNew ;
         }
         // upgrade list
         else
         {
#ifdef _DEBUG
            SDB_ASSERT( (pLRBHdr->upgradeTail), "Invalid upgradeTail" ) ;
#endif
            // get last LRB
            plrb = pLRBHdr->upgradeTail ;
            // set upgradeTail
            pLRBHdr->upgradeTail = lrbNew ;
         }

         // add this new LRB behind the last LRB
         plrb->nextLRB = lrbNew ;
         lrbNew->prevLRB = plrb ;
      }
      else
      {
         // waiter list
         if ( LOCKMGR_LRB_QUE_WAITER == queueType )
         {
            // set waitTail
            pLRBHdr->waiterTail = lrbNew ;
         }
         // upgrade list
         else
         {
            // set upgradeTail
            pLRBHdr->upgradeTail = lrbNew ;
         }

         // add this new LRB behind the last LRB
         lrbBegin = lrbNew ;
         lrbNew->prevLRB = NULL ;
      }

      lrbNew->nextLRB = NULL ;
   }


   //
   // Description: add a LRB at the head of the LRB chain/list
   // Function:    add the LRB at the head of list
   // Input:
   //    lrbBegin -- the first LRB in the chain
   //    lrbNew   -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToLRBListHead
   (
      dpsTransLRB * & lrbBegin,
      dpsTransLRB *   lrbNew
   )
   {
      if ( lrbBegin )
      {
         lrbBegin->prevLRB  = lrbNew ;
      }
      if ( lrbNew )
      {
         lrbNew->nextLRB = lrbBegin ;
         lrbNew->prevLRB = NULL ;
      }
      lrbBegin = lrbNew ;
   }


   //
   // Description: add a LRB to owner list right after a given LRB
   // Function:    the owner list is sorted on lock mode in descending order,
   //              add a LRB right after the given LRB
   // Input:
   //    lrbPos -- the LRB where the new LRB is being inserted after
   //
   //    lrbNew -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToOwnerLRBList
   (
      dpsTransLRB *insPos,
      dpsTransLRB *lrbNew
   )
   {
      if ( insPos && lrbNew )
      {
         lrbNew->nextLRB = insPos->nextLRB ;
         lrbNew->prevLRB = insPos ;
         if ( insPos->nextLRB )
         {
            insPos->nextLRB->prevLRB = lrbNew ;
         }
         insPos->nextLRB = lrbNew ;
      }
   }


   //
   // Description: after add an IX or IS LRB to owner list,
   //              update the newestIXOwnerList or newestISOwnerList in LRBHdr
   // Input:
   //    lrbNew -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_setNewestISIXOwner
   (
      dpsTransLRB *lrbNew
   )
   {
      if ( lrbNew )
      {
         if ( DPS_TRANSLOCK_IS == lrbNew->lockMode )
         {
            lrbNew->lrbHdr->newestISOwner = lrbNew ;
         }
         else if ( DPS_TRANSLOCK_IX == lrbNew->lockMode )
         {
            lrbNew->lrbHdr->newestIXOwner = lrbNew ;
         }
      }
   }


   //
   // Description: search owner LRB list and find the expected LRB
   // Function:    walk through owner LRB list ( it is sorted on lockMode in
   //              descending order ) and find out :
   //              . if the edu is in owner list
   //              . the pointer which the new LRB shall be inserted after
   //              . the pointer of last compatible and pointer of first
   //                incompatible LRB
   // Input:
   //    dpsTxExectr -- pointer to _dpsTransExecutor
   //    opMode      -- opreation mode, try, acquire or test
   //    lockMode    -- lock mode
   //    lrbBegin    -- the first LRB pointer in the owner list
   // Output:
   //    pLRBToInsert      -- the lrb which the new LRB shall be inserted after
   //    pLRBIncompatible  -- pointer of first incompatible LRB
   //    pLRBOwner         -- the LRB owned by same dpsTxExectr ( eduId )
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired and
   //              all output parameters are initialized as NULL
   void dpsTransLockManager::_searchOwnerLRBList
   (
      const _dpsTransExecutor    * dpsTxExectr,
      DPS_TRANSLOCK_OP_MODE_TYPE   opMode,
      const DPS_TRANSLOCK_TYPE     lockMode,
      dpsTransLRB                * lrbBegin,
      dpsTransLRB *              & pLRBToInsert,
      dpsTransLRB *              & pLRBIncompatible,
      dpsTransLRB *              & pLRBOwner
   )
   {
#ifdef _DEBUG
      SDB_ASSERT( ( NULL == pLRBToInsert ),     "Invalid pLRBToInsert" ) ;
      SDB_ASSERT( ( NULL == pLRBIncompatible ), "Invalid pLRBIncompatible" ) ;
#endif
      dpsTransLRB *plrb = lrbBegin, *plrbPrev = NULL;
      BOOLEAN searchInsertDone = FALSE ;
      BOOLEAN searchIncompDone = FALSE ;
      BOOLEAN searchOwnerDone  = FALSE ;

      // no need to search inserting position for test
      if ( ( DPS_TRANSLOCK_OP_MODE_TEST == opMode ) ||
           ( DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT == opMode ) )
      {
         searchInsertDone = TRUE ;
      }

      // if the owner LRB is not NULL means the owner
      // is already found
      if ( pLRBOwner )
      {
         searchOwnerDone = TRUE ;
      }
      // for non-transaction lock, it searches EDU LRB
      // list before calling this function. If the pLRBOwner
      // is NULL, means it doesn't own this lock, thus no need
      // to check if itself is in owner list.
      else if ( FALSE == _autoUpperLockOp )
      {
         searchOwnerDone = TRUE ;
      }

      // if first owner mode is X or request mode is X
      // and the owner is not the requestor, then
      // the first incompatbile owner LRB is found
      if ( plrb &&
           ( ( DPS_TRANSLOCK_X == plrb->lockMode ) ||
             ( DPS_TRANSLOCK_X == lockMode ) ) &&
           ( dpsTxExectr != plrb->dpsTxExectr ) )
      {
         // save the address/pointer of first incompatible LRB
         pLRBIncompatible = plrb ;
         searchIncompDone = TRUE ;
      }

      while ( plrb )
      {
         // check if this LRB owned by the requested EDU, i.e., owner LRB
         if ( ! searchOwnerDone )
         {
            if ( dpsTxExectr == plrb->dpsTxExectr )
            {
               // save the pointer if the given eduId is found
               // in the owner list
               pLRBOwner = plrb;
               searchOwnerDone = TRUE ;
            }
         }

         // to find the insert position of this LRB
         // the owner list is sorted on lock mode in descending order
         if ( ! searchInsertDone )
         {
            if ( lockMode >= plrb->lockMode )
            {
               // save the LRB pointer where it shall be inserted after
               pLRBToInsert     = plrb->prevLRB ;
               searchInsertDone = TRUE ;
            }
         }

         // check if the requested lock mode is compatible other owners.
         // If not, remember the first incompatible one.
         if ( ! searchIncompDone )
         {
            // as the owner list is sorted on lockMode in
            // descending order, if current owner is S
            // and it is compatible with request lockMode,
            // then others in the list will be compatible with
            // the request lockMode.
            if ( DPS_TRANSLOCK_S == plrb->lockMode )
            {
               if ( dpsIsLockCompatible( plrb->lockMode, lockMode ) )
               {
                  searchIncompDone = TRUE ;
               }
               else if ( dpsTxExectr != plrb->dpsTxExectr )
               {
                  // save the address/pointer of first incompatible LRB
                  pLRBIncompatible = plrb ;
                  searchIncompDone = TRUE ;
               }
            }
            if (    ( ! searchIncompDone )
                 && ( dpsTxExectr != plrb->dpsTxExectr )
                 && ( ! dpsIsLockCompatible( plrb->lockMode, lockMode ) ) )
            {
               // save the address/pointer of first incompatible LRB
               pLRBIncompatible = plrb ;
               searchIncompDone = TRUE ;
            }
         }

         // early exit if all jobs are done
         if ( searchOwnerDone &&
              searchInsertDone &&
              searchIncompDone )
         {
            break ;
         }

         // move to next
         plrbPrev = plrb ;
         plrb = plrb->nextLRB ;
      }

      if ( ( NULL != lrbBegin ) && ( ! searchInsertDone ) )
      {
         // if the request lock mode is smaller than all owners,
         // the insert position is the end of the list
         pLRBToInsert = plrbPrev ;
      }
   }


   //
   // Description: search owner LRB list and find the expected LRB
   // Function:    walk through owner LRB list ( it is sorted on lockMode in
   //              descending order ) and find out :
   //              . the pointer which the new LRB shall be inserted after
   //              . the pointer of last compatible and pointer of first
   //                incompatible LRB
   // Input:
   //    lockMode -- lock mode
   //    lrbBegin -- the first LRB pointer in the owner list
   //    dpsTxExectr -- current requester _dpsTransExecutor pointer
   // Output:
   //    pLRBToInsert      -- the lrb which the new LRB shall be inserted after
   //    pLRBIncompatible  -- pointer of first incompatible LRB
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired and
   //              all output parameters are initialized as NULL
   void dpsTransLockManager::_searchOwnerLRBListForInsertAndIncompatible
   (
      const _dpsTransExecutor * dpsTxExectr,
      const DPS_TRANSLOCK_TYPE  lockMode,
      dpsTransLRB             * lrbBegin,
      dpsTransLRB *           & pLRBToInsert,
      dpsTransLRB *           & pLRBIncompatible
   )
   {
#ifdef _DEBUG
      SDB_ASSERT( ( NULL == pLRBToInsert ),     "Invalid pLRBToInsert" ) ;
      SDB_ASSERT( ( NULL == pLRBIncompatible ), "Invalid pLRBIncompatible" ) ;
#endif
      dpsTransLRB *plrb = lrbBegin, *plrbPrev = NULL;
      BOOLEAN foundIns = FALSE ;

      while ( plrb )
      {
         // to find the insert position of this LRB
         // the owner list is sorted on lock mode in descending order
         if ( ! foundIns )
         {
            // if request mode is IS or IX
            if ( DPS_TRANSLOCK_IX >= lockMode )
            {
               dpsTransLRBHeader * pLRBHdr = plrb->lrbHdr ;

               if ( DPS_TRANSLOCK_IS == lockMode )
               {
                  if ( pLRBHdr->newestISOwner )
                  {
                     // when pLRBHdr->newestISOwner->prevLRB is NULL,
                     // it implies that is the first one in the list.
                     // The later code ( add to the new LRB after
                     // this pLRBToInsert into owner list ) shall
                     // be able to handle this case.
                     pLRBToInsert = pLRBHdr->newestISOwner->prevLRB ;
                     foundIns     = TRUE ;
                  }
               }
               else
               {
                  if ( pLRBHdr->newestIXOwner )
                  {
                     // ditto, when pLRBHdr->newestIXOwner->prevLRB is NULL,
                     // implies that is the first one in the list.
                     pLRBToInsert = pLRBHdr->newestIXOwner->prevLRB ;
                     foundIns     = TRUE ;
                  }
               }
            }
            if ( ( !foundIns ) && ( lockMode >= plrb->lockMode ) )
            {
               // save the LRB pointer where it shall be inserted after
               pLRBToInsert = plrb->prevLRB ;
               foundIns     = TRUE ;
            }
         }

         if ( ( DPS_TRANSLOCK_IX >= lockMode ) && foundIns )
         {
            // as the owner list is sorted on lockMode in
            // descending order, when IS/IX lock request found
            // the position to insert, no need to walk through
            // rest of the list to search for incompatible,
            // beause IX/IS lock is the lowest lock mode value,
            // i.e., stays at the tail end of the owner list,
            // if there is an incompatible one in the list,
            // it must be found already since it has greater
            // lock mode value than IX/IS lock
            break ;
         }

         // check if the requested lock mode is compatible other owners.
         // If not, remember the first incompatible one.
         if ( NULL == pLRBIncompatible )
         {
            if (    ( dpsTxExectr != plrb->dpsTxExectr )
                 && ( ! dpsIsLockCompatible( plrb->lockMode, lockMode ) ) )
            {
               pLRBIncompatible = plrb ;
            }
         }

         // early exit if all jobs are done (insert position, incompatible LRB)
         if ( foundIns && pLRBIncompatible )
         {
            break ;
         }

         // move to next
         plrbPrev = plrb ;
         plrb = plrb->nextLRB ;
      }

      if ( ( NULL != lrbBegin ) && ( ! foundIns ) )
      {
         // if the request lock mode is smaller than all owners,
         // the insert position is the end of the list
         pLRBToInsert = plrbPrev ;
      }
   }


   //
   // Description: add a LRB at the end of the EDU LRB chain, which is the list
   //              of all locks acquired within a session/tx
   // Function:    walk through EDU LRB chain( doubly linked list ),
   //              add the LRB at the end of list.
   //
   //              the dpsTxExectr::_lastLRB is the latest LRB
   //              acquired within the same tx
   // Input:
   //    dpsTxExectr -- _dpsTransExecutor ptr
   //    insLRB      -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToEDULRBListTail
   (
      _dpsTransExecutor    * dpsTxExectr,
      dpsTransLRB          * insLRB,
      const dpsTransLockId & lockId
   )
   {
      if ( insLRB )
      {
         // get the pointer of last LRB in the EDU LRB chain
         // and add the new LRB into the chain
         dpsTransLRB *plrb = dpsTxExectr->getLastLRB( _lockMgrType ) ;
         if ( plrb )
         {
            plrb->eduLrbNext   = insLRB ;
            insLRB->eduLrbPrev = plrb ;
            insLRB->eduLrbNext = NULL ;
         }
         else
         {
            insLRB->eduLrbPrev = NULL ;
            insLRB->eduLrbNext = NULL ;
         }

         // set this newly inserted lrb as the last LRB
         dpsTxExectr->setLastLRB( insLRB, _lockMgrType ) ;

         // add to executor non-leaf lock map if automatically operating on
         // upper level lock flag is turned on
         if ( _autoUpperLockOp )
         {
            dpsTxExectr->addLock( lockId, insLRB, _lockMgrType ) ;
         }

         // increase the lock count
         dpsTxExectr->incLockCount( _lockMgrType ) ;

         // clear the wait info in dpsTxExectr
         dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
      }
   }

   //
   // Description: move a LRB to the end of the EDU LRB list
   // Function:    move a LRB to the end of the EDU LRB list
   //
   //              The reason for this is to ensure that during
   //              lock release, it can search the correct LRB
   //              through EDU LRB list in constant time
   // Input:
   //    dpsTxExectr -- _dpsTransExecutor ptr
   //    insLRB      -- the LRB to be moved
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //              insLRB is not NULL
   //
   void dpsTransLockManager::_moveToEDULRBListTail
   (
      _dpsTransExecutor    * dpsTxExectr,
      dpsTransLRB          * insLRB
   )
   {
      dpsTransLRB *plrb = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      if ( plrb == insLRB)
      {
         return ;
      }

      dpsTxExectr->acquireLRBAccessingLock( _lockMgrType ) ;
      dpsTransLRB *accesingLRB = dpsTxExectr->getAccessingLRB( _lockMgrType ) ;
      if ( accesingLRB != NULL && accesingLRB == insLRB )
      {
         // ignore recycling the lrb
         dpsTxExectr->setAccessingLRB( _lockMgrType, insLRB->eduLrbPrev ) ;
      }
      dpsTxExectr->releaseLRBAccessingLock( _lockMgrType ) ;

      if ( insLRB->eduLrbPrev )
      {
         insLRB->eduLrbPrev->eduLrbNext = insLRB->eduLrbNext ;
      }
      if ( insLRB->eduLrbNext )
      {
         insLRB->eduLrbNext->eduLrbPrev = insLRB->eduLrbPrev ;
      }

      plrb->eduLrbNext   = insLRB ;
      insLRB->eduLrbPrev = plrb ;
      insLRB->eduLrbNext = NULL ;

      // set this newly inserted lrb as the last LRB
      dpsTxExectr->setLastLRB( insLRB, _lockMgrType ) ;
   }

   //
   // Description: remove a LRB from a LRB chain
   // Function:    walk through the LRB chain(linked list, it can be lock owner
   //              list, lock waiter list or upgrade list ) and remove the LRB
   //              from the chain. Please note, it doesn't release/return the
   //              LRB to the LRB manager.
   // Input:
   //    beginLRB -- the LRB in the list that begin to search
   //    delLRB   -- the LRB object to be removed
   //
   // Output:
   //    nextLRB  -- the next LRB to the delLRB
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_removeFromLRBList
   (
      LOCKMGR_LRB_QUE_TYPE   queueType,
      dpsTransLRB*         & beginLRB,
      dpsTransLRB*           delLRB
   )
   {
      if ( beginLRB && delLRB )
      {
         dpsTransLRBHeader * pLRBHdr = delLRB->lrbHdr ;

         // if remove from owner list
         if ( LOCKMGR_LRB_QUE_OWNER == queueType )
         {
            // if delLRB is the newestISOwner or newestIXOwner
            if ( delLRB == pLRBHdr->newestISOwner )
            {
               if ( ( delLRB->nextLRB ) &&
                    ( DPS_TRANSLOCK_IS == delLRB->nextLRB->lockMode ) )
               {
                  pLRBHdr->newestISOwner = delLRB->nextLRB ;
               }
               else
               {
                  pLRBHdr->newestISOwner = NULL ;
               }
            }
            else if ( delLRB == pLRBHdr->newestIXOwner )
            {
               if ( ( delLRB->nextLRB ) &&
                    ( DPS_TRANSLOCK_IX == delLRB->nextLRB->lockMode ) )
               {
                  pLRBHdr->newestIXOwner = delLRB->nextLRB ;
               }
               else
               {
                  pLRBHdr->newestIXOwner = NULL ;
               }
            }
         }

         // if the first one is the one to be removed
         if ( delLRB == beginLRB )
         {
            beginLRB = delLRB->nextLRB ;
            if ( delLRB->nextLRB )
            {
               delLRB->nextLRB->prevLRB = NULL ;
            }
            // if it is the last one to be removed
            else
            {
               // set waiterTail
               if ( LOCKMGR_LRB_QUE_WAITER == queueType )
               {
                  pLRBHdr->waiterTail = delLRB->prevLRB ;
               }
               // set upgradeTail
               else if ( LOCKMGR_LRB_QUE_UPGRADE == queueType )
               {
                  pLRBHdr->upgradeTail = delLRB->prevLRB ;
               }

            }
         }
         else
         {
            SDB_ASSERT( delLRB->prevLRB, "Invalid LRB, prevLRB is NULL" ) ;
            {
               delLRB->prevLRB->nextLRB = delLRB->nextLRB ;
            }
            if ( delLRB->nextLRB )
            {
               delLRB->nextLRB->prevLRB = delLRB->prevLRB ;
            }
            // if it is the last one to be removed
            else
            {
               // set waiterTail
               if ( LOCKMGR_LRB_QUE_WAITER == queueType )
               {
                  pLRBHdr->waiterTail = delLRB->prevLRB ;
               }
               // set upgradeTail
               else if ( LOCKMGR_LRB_QUE_UPGRADE == queueType )
               {
                  pLRBHdr->upgradeTail = delLRB->prevLRB ;
               }
            }
         }
         delLRB->nextLRB = NULL ;
         delLRB->prevLRB = NULL ;
      }
   }


   //
   // Description: remove waiter from waiter or upgrade queue/list
   // Function:    remove the waiter LRB ( dpsTxExectr->getWaiterLRBIdx() )
   //              from upgrade or waiter queue/list, and wakeup the next
   //              one if necessary
   // REVISIT:
   // Here are two cases :
   // 1. an EDU was waken up ( _waitLock returned SDB_OK )
   //    it checks next waiters lock mode whether compatible with itself
   //    as itself will retry acquire the lock. Only wake up next waiter
   //    if the lock mode is compatbile with current waiter.
   //
   // 2. an EDU was timeout / interrupted from _waitLock
   //    if the owner list is empty, we may wake up the next waiter, as
   //    the current one will not retry acquire the lock.
   //
   //    alternatives :
   //      a. treat this EDU same as been waken up case and retry acquire.
   //      b. do nothing, next waiter(s) can timeout same as this EDU.
   //
   // Input:
   //    dpsTxExectr     -- pointer to _dpsTransExecutor
   //    removeLRBHeader -- whether remove LRB Header when owner, waiter,
   //                       upgrade queue are all empty
   //                       when it is true, it means _waitLock returned
   //                       non SDB_OK value, due to either lock waiting
   //                       timeout elapsed or be interrupted.
   // Output:  none
   // Return:  none
   // Dependency:  the lock bucket latch shall be acquired
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST, "dpsTransLockManager::_removeFromUpgradeOrWaitList" )
   void dpsTransLockManager::_removeFromUpgradeOrWaitList
   (
      _dpsTransExecutor *    dpsTxExectr,
      const dpsTransLockId & lockId,
      const UINT32           bktIdx,
      const BOOLEAN          removeLRBHeader,
      const BOOLEAN          bSortedAndSUXModeList,
      const BOOLEAN          bTryAndWait
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST ) ;

      dpsTransLRB *pLRB = dpsTxExectr->getWaiterLRB( _lockMgrType ) ;
      dpsTransLRB *pLRBNext = NULL;
      dpsTransLRB *pLRBIncompatible = NULL ;
      dpsTransLRBHeader * pLRBHdr   = NULL ;

      if ( pLRB )
      {
#ifdef _DEBUG
         SDB_ASSERT( pLRB->lrbHdr != NULL, "Invalid LRB Header." );
#endif
         pLRBHdr  = pLRB->lrbHdr ;
         pLRBNext = pLRB->nextLRB ;

         // sanity check, panic if fails.
         if ( ! ( pLRBHdr->lockId == lockId ) )
         {
            PD_LOG( PDSEVERE,
                    "Fatal error, requested lockId doesn't match LRB Header."
                    "Requested lockId:%s, lockId in LRB Header %s",
                    lockId.toString().c_str(),
                    pLRBHdr->lockId.toString().c_str() ) ;
            ossPanic() ;
         }

         if ( DPS_QUE_UPGRADE == dpsTxExectr->getWaiterQueType( _lockMgrType ) )
         {
            // remove from upgrade list
            _removeFromLRBList( LOCKMGR_LRB_QUE_UPGRADE, pLRBHdr->upgradeLRB,
                                pLRB ) ;
         }
         else if ( DPS_QUE_WAITER ==
                   dpsTxExectr->getWaiterQueType( _lockMgrType ) )
         {
            // remove from waiter list
            _removeFromLRBList( LOCKMGR_LRB_QUE_WAITER, pLRBHdr->waiterLRB,
                                pLRB ) ;
         }

         // no need to clear the status of current pLRB
         // since it will be released
         // OSS_BIT_CLEAR( pLRB->status, DPS_LRB_STATUS_AWAKE ) ;

         // clear the wait info in dpsTxExectr
         dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;

         // get the waiter LRB pointer
         if ( pLRBHdr->upgradeLRB )
         {
            pLRBNext = pLRBHdr->upgradeLRB ;
         }
         else if ( pLRBHdr->waiterLRB  )
         {
            pLRBNext = pLRBHdr->waiterLRB  ;
         }

         // wake up the next waiting one if necessary.
         // Here are two cases :
         // 1. an EDU was waken up ( _waitLock returned SDB_OK )
         //    it checks next waiters lock mode whether compatible with itself
         //    as itself will retry acquire the lock. Only wake up next waiter
         //    if the lock mode is compatbile with current waiter.
         //
         // REVISIT
         //
         // 2. an EDU was timeout / interrupted from _waitLock
         //    if the owner list is empty, we may wake up the next waiter, as
         //    the current one will not retry acquire the lock.
         //
         //    another approach is treat this EDU same as been waken up case
         //    and retry acquire. Or, do nothing let other waiters timeout.
         //
         if ( pLRBNext &&
              ( ! OSS_BIT_TEST( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ) )
         {
#ifdef _DEBUG
            SDB_ASSERT( pLRB->lrbHdr == pLRBNext->lrbHdr, "Invalid LRB" ) ;
#endif
            // the EDU was waken up ( _waitLock returned SDB_OK )
            if ( FALSE == removeLRBHeader )
            {
               if ( dpsIsLockCompatible( pLRB->lockMode, pLRBNext->lockMode ) )
               {
                  // wake up next waiter if the lock mode is compatible
                  OSS_BIT_SET( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ;
                  _wakeUp( pLRBNext->dpsTxExectr ) ;
               }
               else
               {
                  // if try-and-wait mode
                  if ( bTryAndWait )
                  {
                     if ( ! pLRBHdr->ownerLRB )
                     {
                        // wake up next waiter if owner list is empty
                        OSS_BIT_SET( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ;
                        _wakeUp( pLRBNext->dpsTxExectr ) ;
                     }
                  }
               }
            }
            else
            {
               // the _waitLock() returned timeout or interrupted

               if ( ! pLRBHdr->ownerLRB )
               {
                  // wake up next waiter if owner list is empty
                  OSS_BIT_SET( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ;
                  _wakeUp( pLRBNext->dpsTxExectr ) ;
               }
               else if ( ! _checkLockModeWithOthers( pLRBHdr->ownerLRB,
                                                     pLRBNext->dpsTxExectr,
                                                     pLRBNext->lockMode,
                                                     pLRBIncompatible,
                                                     bSortedAndSUXModeList ) )
               {
                  // wake up next waiter if it is compatible with all owners :
                  //  . A is holding U lock
                  //  . B requests U and is in waiter queue
                  //  . C requests S and is put in waiter queue
                  //  B timed out, it shall try to wake up C if C is compatible
                  //  with current owners.
                  OSS_BIT_SET( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ;
                  _wakeUp( pLRBNext->dpsTxExectr ) ;
               }
            }
         }

         // release the waiter LRB
         _releaseLRB( pLRB ) ;
         pLRB = NULL ;

         // when LRB Header is empty, release it if necessary
#ifdef _DEBUG
         SDB_ASSERT( ( pLRBHdr ), "Invalid LRB Header." ) ;
#endif
         // if extData hasn't been setup/initialized,
         // canRelease() will return TRUE
         if (    removeLRBHeader
              && ( !  pLRBHdr->ownerLRB   )
              && ( !  pLRBHdr->upgradeLRB )
              && ( !  pLRBHdr->waiterLRB  )
              && ( pLRBHdr->extData.canRelease() ) )
         {
            // remove the LRB Header from the list
            _removeFromLRBHeaderList( _LockHdrBkt[bktIdx].lrbHdr, pLRBHdr ) ;
            // release the LRB Header
            //
            _releaseLRBHdr( pLRBHdr ) ;
            pLRBHdr = NULL ;
         }
      }
      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST ) ;
   }


   //
   // Description: remove a LRB Header from a LRB Header chain
   // Function:    walk through the LRB Header chain( linked list ) and remove
   //              the LRB Header from the chain. Please note, it doesn't
   //              release/return the LRB Header to the LRB Header manager.
   // Input:
   //    lrbBegin -- the first LRB Header in the list
   //    lrbDel   -- the LRB Header object to be removed
   //
   // Output:
   //    lrbBegin -- if the lrbBegin is same as lrbDel, it will be updated with
   //                the LRB Header next to lrbDel
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_removeFromLRBHeaderList
   (
      dpsTransLRBHeader* & lrbBegin,
      dpsTransLRBHeader*   lrbDel
   )
   {
      if ( ( NULL != lrbBegin ) && ( NULL != lrbDel ) )
      {
         dpsTransLRBHeader *plrbHdr = lrbBegin;

         // if the first one is the one to be removed
         if ( lrbDel == lrbBegin )
         {
            lrbBegin = lrbDel->nextLRBHdr ;
         }
         else
         {
            while ( NULL != plrbHdr )
            {
               if ( lrbDel == plrbHdr->nextLRBHdr )
               {
                  plrbHdr->nextLRBHdr = lrbDel->nextLRBHdr ;
                  break ;
               }
               plrbHdr = plrbHdr->nextLRBHdr ;
            }
         }
         lrbDel->nextLRBHdr = NULL ;
      }
   }


   //
   // Description: remove a LRB from the EDU LRB chain
   // Function:    walk through the EDU LRB chain( doubly linked list ),
   //              and remove the LRB Header from the chain.
   //              Please note, it doesn't release/return the LRB to
   //              the LRB Header manager.
   // Input:
   //    dpsTxExectr  -- dpsTxExectr
   //    delLrb       -- the LRB object to be removed
   //
   // Output:
   //    dpsTxExectr->_lastLRB -- the last LRB object
   //                             in the EDU LRB chain.
   //                             If it is equal to delLrb, it will be
   //                             updated with the LRB previous
   //                             to delLrb
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_removeFromEDULRBList
   (
      _dpsTransExecutor    * dpsTxExectr,
      dpsTransLRB          * delLRB,
      const dpsTransLockId & lockId
   )
   {
      if ( dpsTxExectr && delLRB && dpsTxExectr->getLastLRB( _lockMgrType ) )
      {
         dpsTxExectr->acquireLRBAccessingLock( _lockMgrType ) ;
         dpsTransLRB *accesingLRB = dpsTxExectr->getAccessingLRB(
                                                                _lockMgrType ) ;
         if ( accesingLRB != NULL && accesingLRB == delLRB )
         {
            dpsTxExectr->setAccessingLRB( _lockMgrType,
                                          delLRB->eduLrbPrev ) ;
         }
         dpsTxExectr->releaseLRBAccessingLock( _lockMgrType ) ;

         if ( delLRB->eduLrbPrev )
         {
            delLRB->eduLrbPrev->eduLrbNext = delLRB->eduLrbNext ;
         }
         if ( delLRB->eduLrbNext )
         {
            delLRB->eduLrbNext->eduLrbPrev = delLRB->eduLrbPrev ;
         }
         // set new last LRB if I am the last one
         if ( dpsTxExectr->getLastLRB( _lockMgrType ) == delLRB )
         {
            dpsTxExectr->setLastLRB( delLRB->eduLrbPrev, _lockMgrType ) ;
         }
         // remove it from executor non-leaf lock map
         // if automatically operating on upper level lock flag is turned on
         if ( _autoUpperLockOp )
         {
            dpsTxExectr->removeLock( lockId, _lockMgrType ) ;
         }

         // decrease the lock count
         dpsTxExectr->decLockCount( _lockMgrType ) ;
         delLRB->eduLrbPrev = NULL ;
         delLRB->eduLrbNext = NULL ;
      }
   }


   //
   // Description: acquire, try or test to get a lock with given mode
   // Function: core of acquire, try or test operation, behaviour varies
   //   depending operation mode :
   //   1 DPS_TRANSLOCK_OP_MODE_ACQUIRE
   //     return SDB_OK
   //       . lock acquired, new LRB is added in owner list
   //       . if holing higher level lock, no need to add new LRB in owner list
   //     return SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST
   //       . can't upgrade to requested lock mode
   //     return SDB_DPS_TRANS_APPEND_TO_WAIT
   //       . need to upgrade, new LRB is added to upgrade list
   //       . need to wait, new LRB is added to waiter list
   //   2 DPS_TRANSLOCK_OP_MODE_TRY
   //     try mode will not add LRB to waiter or upgrade list
   //     return SDB_OK
   //       . lock acquired, new LRB is added in owner list
   //       . holing higher level lock, no need to add new LRB in owner list
   //     return SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST
   //       . can't upgrade to requested lock mode
   //     return SDB_DPS_TRANS_LOCK_INCOMPATIBLE
   //       . request lock mode can't be acquired
   //   3 DPS_TRANSLOCK_OP_MODE_TEST, DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT
   //     return SDB_OK
   //       . request lock can be acquired
   //     return SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST
   //       . can't upgrade to requested lock mode
   //     return SDB_DPS_TRANS_LOCK_INCOMPATIBLE
   //       . request lock mode can't be acquired
   //
   // Input:
   //    dpsTxExectr     -- dpsTxExectr
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   //    opMode          -- try     ( DPS_TRANSLOCK_OP_MODE_TRY )
   //                       acquire ( DPS_TRANSLOCK_OP_MODE_ACQUIRE )
   //                       test    ( DPS_TRANSLOCK_OP_MODE_TEST )
   //                       testPreempt( DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT )
   //    bktIdx          -- bucket index
   //    bktLatched      -- if bucket is already latched
   //
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo
   // Return:
   //     SDB_OK,
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,
   //     SDB_DPS_TRANS_APPEND_TO_WAIT,
   //     SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__TRYACQUIREORTEST, "dpsTransLockManager::_tryAcquireOrTest" )
   INT32 dpsTransLockManager::_tryAcquireOrTest
   (
      _dpsTransExecutor                * dpsTxExectr,
      const dpsTransLockId             & lockId,
      const DPS_TRANSLOCK_TYPE           requestLockMode,
      const DPS_TRANSLOCK_OP_MODE_TYPE   opMode,
      UINT32                             bktIdx,
      const BOOLEAN                      bktLatched,
      dpsTransRetInfo                  * pdpsTxResInfo,
      _dpsITransLockCallback           * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__TRYACQUIREORTEST ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      INT32 rc = SDB_OK ;
      dpsTransLRB *pLRBNew          = NULL ,
                  *pLRBIncompatible = NULL ,
                  *pLRBDeadlock     = NULL ,
                  *pLRBToInsert     = NULL ,
                  *pLRBOwner        = NULL ,
                  *pLRB             = NULL ;

      dpsTransLRBHeader *pLRBHdrNew = NULL ,
                        *pLRBHdr    = NULL ;

      BOOLEAN bFreeLRB       = FALSE ,
              bLatched       = FALSE ;

      BOOLEAN testMode = ( DPS_TRANSLOCK_OP_MODE_TEST == opMode ||
                           DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT == opMode ) ;

      BOOLEAN bIsLockLeafLevel        =  lockId.isLeafLevel() ;
      BOOLEAN bLockModeUpgradeChecked = FALSE ;

#ifdef _DEBUG
      SDB_ASSERT( _initialized, "dpsTransLockManager is not initialized." ) ;
#endif
      if ( bktLatched )
      {
         bLatched = TRUE ;
      }

      //
      // try to take a shortcut if it is less expensive to find out
      // whether the lock is already acquired
      //
      // transaction CS/CL lock
      if ( _autoUpperLockOp && ( ! bIsLockLeafLevel ) )
      {
         // lookup the executor _mapLockID map, if it is found and current
         // lock mode covers the requesting mode then increase refCounter,
         // and job is done. Otherwise, still need to go through the normal
         // routine.
         // we actually don't need bkt latch for looking up CS,CL lock
         // in the executor _mapLockID map
         //
         // findLock works for non-leaf lock only
         if ( ! dpsTxExectr->findLock( lockId, pLRB, _lockMgrType ) )
         {
            pLRB = NULL ;
         }
      }
      // record lock or non-transaction lock,
      // e.g., index page lock, extent lock
      else
      {
         // record lock
         if ( _autoUpperLockOp )
         {
            // if the EDU doesn't hold many locks ( i.e., the EDU
            // LRB list is short ), it would be less expensive
            // to search on EDU LRB list without bkt latch.
            // Otherwise, still need to go through the normal
            // routine.
            if ( DPS_LOCK_PRE_SEARCH_EDULIST_THRESHOLD >=
                 dpsTxExectr->getLockCount( _lockMgrType ) )
            {
               pLRB = _getLRBFromEDULRBList( dpsTxExectr, lockId ) ;
            }
         }
         // non-transaction lock
         else
         {
            pLRB = _getLRBFromEDULRBList( dpsTxExectr, lockId ) ;
         }
      }

      if ( pLRB )
      {
         pLRBOwner = pLRB ;
         pLRBHdr   = pLRB->lrbHdr ;
         if ( dpsLockCoverage( pLRB->lockMode, requestLockMode ) )
         {
            if ( !testMode )
            {
               pLRB->refCounter++ ;

               // clear the wait info in dpsTxExectr
               dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
            }
            goto done ;
         }
         // if dpsUpgradeCheck is OK
         rc = dpsUpgradeCheck( pLRB->lockMode, requestLockMode ) ;
         if ( SDB_OK != rc )
         {
            // can't do upgrade, job done with error rc set

            // constrct conflict lock info
            if ( pdpsTxResInfo )
            {
               pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
               pdpsTxResInfo->_lockType = pLRB->lockMode ;
               pdpsTxResInfo->_eduID    = pLRB->dpsTxExectr->getEDUID();
               pdpsTxResInfo->_tid      = pLRB->dpsTxExectr->getTID() ;
            }
            goto done ;
         }
         bLockModeUpgradeChecked = TRUE ;
      }

      /// normal lock acquire/try get/test routine

      // allocate new LRB
      if ( !testMode )
      {
         // no need to allocate LRB for test mode
         pLRBNew = SDB_OSS_NEW dpsTransLRB( dpsTxExectr, requestLockMode, NULL ) ;
         if ( ! pLRBNew )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc a LRB (rc=%d)", rc ) ;
            goto error ;
         }
         bFreeLRB = TRUE ;
      }

      if ( bktIdx == DPS_LOCK_INVALID_BUCKET_SLOT )
      {
         bktIdx = _getBucketNo( lockId );
      }

      // latch bucket
      // for test mode, we could quickly test the header
      // if header is empty, it means no one has lock any records in this
      // bucket, so it should be safe to pass the test lock request
      // NOTE: this is based on the rule that before test locks we have
      // acquired the mblatch of collection.
      if ( _autoUpperLockOp &&
           testMode &&
          ( NULL == _LockHdrBkt[ bktIdx ].lrbHdr ) )
      {
         goto done ;
      }
      if ( ! bktLatched )
      {
         _acquireOpLatch( bktIdx ) ;
      }
      bLatched = TRUE ;

      // if LRB header is not found by above shortcut
      if ( NULL == pLRBHdr )
      {
         // if no LRB Header
         if ( NULL == _LockHdrBkt[ bktIdx ].lrbHdr )
         {
            if ( !testMode )
            {
               // allocate LRB header prepare new LRB and LRB Header
               rc = _prepareNewLRBAndHeader( dpsTxExectr,
                                             lockId,
                                             requestLockMode,
                                             bktIdx,
                                             pLRBHdrNew,
                                             pLRBNew ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               // add new LRB header to the link
               _LockHdrBkt[ bktIdx ].lrbHdr = pLRBHdrNew;

               // sample tick before adding to edulist or setting
               // waiter info to make sure snapshot trans is correct.
               pLRBNew->beginTick.sample() ;

               // add new LRB to EDU LRB list
               _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

               // mark the new LRB and LRB Header are used
               bFreeLRB       = FALSE ;
               pLRBHdr        = pLRBHdrNew ;
               pLRB           = pLRBNew ;

               // clear the wait info in dpsTxExectr
               dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
            }
            // job done
            goto done;
         }

         // LRB header exists,
         // lookup the LRB header list and find the one with same lockId
         pLRBHdr = _LockHdrBkt[ bktIdx ].lrbHdr ;
         if ( ! _getLRBHdrByLockId( lockId, pLRBHdr ) )
         {
            // no LRB header with same lockId is found,
            // add the new LRB Header in the lrb header list
            if ( !testMode )
            {
               // allocate LRB header prepare new LRB and LRB Header
               rc = _prepareNewLRBAndHeader( dpsTxExectr,
                                             lockId,
                                             requestLockMode,
                                             bktIdx,
                                             pLRBHdrNew,
                                             pLRBNew ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               // at this time, pLRBHdr shall be the tail of LRB header list.
               // add the new LRB header to LRB Header list ;
               pLRBHdr->nextLRBHdr = pLRBHdrNew ;

               // sample tick before adding to edulist or setting
               // waiter info to make sure snapshot trans is correct.
               pLRBNew->beginTick.sample() ;

               // add the new LRB to EDU LRB list
               _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

               // mark the new LRB and new LRB Header are used
               bFreeLRB       = FALSE ;
               pLRBHdr        = pLRBHdrNew ;
               pLRB           = pLRBNew ;

               // clear the wait info in dpsTxExectr
               dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
            }
            else
            {
               pLRBHdr = NULL ;
            }
            // job done
            goto done ;
         }
      }

#ifdef _DEBUG
      SDB_ASSERT( ( NULL != pLRBHdr ), "Invalid LRB Header" ) ;
#endif
      // found the LRB header with same lockId

      // update the lrbHdrIdx of new LRB to current LRB Header
      if ( !testMode )
      {
         pLRBNew->lrbHdr = pLRBHdr;
      }

      // leaf level lock ( e.g., record lock or index page lock )
      if ( bIsLockLeafLevel || ( FALSE == _autoUpperLockOp ) )
      {
         // search owner LRB list, which is sorted on lock mode
         // in descending order, to find
         //  . if the edu is in owner list
         //  . the pointer which the new LRB shall be inserted after
         //  . the pointer of first incompatible LRB
         //
         // pLRBToInsert     -- lrb to insert after
         // pLRBIncompatible -- lrb of first incompatible
         // pLRBOwner        -- lrb owned by same EDU
         _searchOwnerLRBList( dpsTxExectr,
                              opMode,
                              requestLockMode,
                              pLRBHdr->ownerLRB,
                              pLRBToInsert,
                              pLRBIncompatible,
                              pLRBOwner ) ;
      }
      // non-leaf Lock
      else
      {
         // the owner LRB should already checked by findLock, result is saved
         // in pLRBOwner. now search owner LRB list to find
         //  . the position where the new LRB shall be inserted after
         //  . the pointer of first incompatible LRB
         //
         // pLRBToInsert     -- lrb to insert after
         // pLRBIncompatible -- lrb of first incompatible
         _searchOwnerLRBListForInsertAndIncompatible( dpsTxExectr,
                                                      requestLockMode,
                                                      pLRBHdr->ownerLRB,
                                                      pLRBToInsert,
                                                      pLRBIncompatible ) ;
      }

      if ( pLRBOwner )
      {
         //
         // in owner list
         //
         pLRB = pLRBOwner ;

#ifdef _DEBUG
         SDB_ASSERT( pLRB && ( pLRB->lrbHdr == pLRBHdr ),
                     "Invalid LRB or the lrbHdr doesn't match "
                     "the LRB Header" ) ;
#endif
         // if current holding lock mode covers the requesting mode,
         // then job is done
         if ( dpsLockCoverage( pLRB->lockMode, requestLockMode ) )
         {
            if ( !testMode )
            {
               pLRB->refCounter ++ ;

               _moveToEDULRBListTail( dpsTxExectr, pLRB ) ;

               // clear the wait info in dpsTxExectr
               dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
            }
            goto done ;
         }

         // if dpsUpgradeCheck is OK
         if ( FALSE == bLockModeUpgradeChecked )
         {
            rc = dpsUpgradeCheck( pLRB->lockMode, requestLockMode ) ;
            if ( SDB_OK != rc )
            {
               // can't do upgrade, job done with error rc set

               // constrct conflict lock info
               if ( pdpsTxResInfo )
               {
                  pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
                  pdpsTxResInfo->_lockType = pLRB->lockMode ;
                  pdpsTxResInfo->_eduID    = pLRB->dpsTxExectr->getEDUID();
                  pdpsTxResInfo->_tid      = pLRB->dpsTxExectr->getTID() ;
               }
               goto done ;
            }
         }

         // try to do upgrade
         //
         // check if the requested mode is compatible with other owners
         if ( NULL != pLRBIncompatible )
         {
            // valid pLRBIncompatible means an incompatible LRB is found,
            // i.e., not compatible with others

            if ( DPS_TRANSLOCK_OP_MODE_ACQUIRE == opMode )
            {
               // save current owining lock mode
               pLRBNew->originMode = pLRBOwner->lockMode ;

               // do dead-lock detection before put in upgrade list.
               // For example, when two S owners both want to upgrade,
               // if allow both of them go in upgrade queue, there will
               // be a dead-lock, and eventually timed out.
               // scenarios:
               //  Owner          Upgrade
               //  1. S     --->  X
               //  2. S     --->  X <--- dead lock with owner 1
               //
               //  Owner          Uprade
               //  1. S     --->  U
               //  2. S     --->  U <--- compatible, no dead-lock
               //  3. U     --->  X <--- dead lock with owner 1 and 2
               if ( FALSE == _deadlockDetectedWhenUpgrade( pLRBHdr->upgradeLRB,
                                                           pLRBNew,
                                                           pLRBDeadlock ) )
               {
                  // add the new LRB to upgrade list
                  _addToLRBListTail( LOCKMGR_LRB_QUE_UPGRADE, pLRBHdr->upgradeLRB,
                                     pLRBNew ) ;

                  // sample tick before adding to edulist or setting
                  // waiter info to make sure snapshot trans is correct.
                  pLRBNew->beginTick.sample() ;

                  // set the wait info in dpsTxExectr
                  dpsTxExectr->setWaiterInfo( pLRBNew, DPS_QUE_UPGRADE,
                                              _lockMgrType ) ;

                  // mark the new LRB is used
                  bFreeLRB = FALSE ;

                  // set return code to SDB_DPS_TRANS_APPEND_TO_WAIT
                  rc = SDB_DPS_TRANS_APPEND_TO_WAIT ;
               }
               else
               {
                  // may cause dead-lock, can't do upgrade
                  pLRBIncompatible = pLRBDeadlock ;
                  rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
               }
            }
            else
            {
               // for try or test mode
               // . set return code to SDB_DPS_TRANS_LOCK_INCOMPATIBLE
               // . no need to add to upgrade/waiter list
               rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
            }

            // construct conflcit lock info ( representative )
            if ( pdpsTxResInfo )
            {
               pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
               pdpsTxResInfo->_lockType = pLRBIncompatible->lockMode ;
               pdpsTxResInfo->_eduID    =
                  pLRBIncompatible->dpsTxExectr->getEDUID() ;
               pdpsTxResInfo->_tid      =
                  pLRBIncompatible->dpsTxExectr->getTID() ;
            }

            // job done
            goto done ;
         }
         else
         {
            // compatible with all others
            if ( !testMode )
            {
               // upgrade/convert to request mode.
               //   when upgrade, it implies the request mode is greater
               //   than current mode. Since the owner list is sorted on
               //   the lock mode in acsent order, we will need to move
               //   the owner LRB to the right place by following two steps :
               //     . remove from current place
               //     . move it to the place right after the pLRBToInsert

               // remove from current place
               _removeFromLRBList( LOCKMGR_LRB_QUE_OWNER, pLRBHdr->ownerLRB,
                                   pLRB ) ;

               // update current lock mode to the request mode
               pLRB->lockMode = requestLockMode ;

               // insert it to the new position
               if ( pLRBToInsert )
               {
                  _addToOwnerLRBList( pLRBToInsert, pLRB ) ;
               }
               else
               {
                  // add it at the beginning of owner list
                  _addToLRBListHead( pLRBHdr->ownerLRB, pLRB ) ;
               }

               // set newestISOwner or newestIXOwner in lrbhdr
               _setNewestISIXOwner( pLRB ) ;

               _moveToEDULRBListTail( dpsTxExectr, pLRB ) ;

               // clear the wait info in dpsTxExectr
               dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;

               pLRB->refCounter ++ ;
            }
            // job done
            goto done ;
         }
      }
      else
      {
         //
         // not in owner list
         //

         // check if lock is compatible with all owners
         if ( NULL != pLRBIncompatible )
         {
            // found an incompatible one, i.e., not compatible with others

            if ( DPS_TRANSLOCK_OP_MODE_ACQUIRE == opMode )
            {
               // add it at the end of waiter list
               _addToLRBListTail( LOCKMGR_LRB_QUE_WAITER, pLRBHdr->waiterLRB,
                                  pLRBNew ) ;

               // sample tick before adding to edulist or setting
               // waiter info to make sure snapshot trans is correct.
               pLRBNew->beginTick.sample() ;

               // set the wait info in dpsTxExectr
               dpsTxExectr->setWaiterInfo( pLRBNew, DPS_QUE_WAITER,
                                           _lockMgrType ) ;

               // mark the new LRB is used
               bFreeLRB = FALSE ;

               // set return code to SDB_DPS_TRANS_APPEND_TO_WAIT
               rc = SDB_DPS_TRANS_APPEND_TO_WAIT ;
            }
            else
            {
               rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
            }

            // construct the conflict lock info
            if ( pdpsTxResInfo )
            {
               pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
               pdpsTxResInfo->_lockType = pLRBIncompatible->lockMode ;
               pdpsTxResInfo->_eduID    =
                  pLRBIncompatible->dpsTxExectr->getEDUID() ;
               pdpsTxResInfo->_tid      =
                  pLRBIncompatible->dpsTxExectr->getTID() ;
            }

            // job done
            goto done ;

         }
         else
         {
            //
            // no incompatible found, i.e., compatible with other owners

            // add to owner list when satisfy following conditions :
            // a. if both upgrade and waiter list are empty
            // b. if it is doing retry-acquire after been woken up.( when input
            //    parameter, bktLatched, is true, means retry acquiring )
            if (    ( ( ! pLRBHdr->upgradeLRB ) && ( ! pLRBHdr->waiterLRB ) )
                 || ( bktLatched ) )
            {
               if ( !testMode )
               {
                  // add the owner list
                  if ( pLRBToInsert )
                  {
                     _addToOwnerLRBList( pLRBToInsert, pLRBNew ) ;
                  }
                  else
                  {
                     _addToLRBListHead( pLRBHdr->ownerLRB, pLRBNew ) ;
                  }

                  // set newestISOwner or newestIXOwner in lrbhdr
                  _setNewestISIXOwner( pLRBNew ) ;

                  // sample tick before adding to edulist or setting
                  // waiter info to make sure snapshot trans is correct.
                  pLRBNew->beginTick.sample() ;

                  // add the new LRB to EDU LRB list
                  _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

                  // mark the new LRB is used
                  bFreeLRB = FALSE ;
                  pLRB     = pLRBNew ;
               }

               // job done
               goto done ;
            }
            else
            {
               // when test lock with preemptive mode, return succeess
               // if there is no conflict with current owners regardless
               // the upgrade or wait list.
               if ( DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT == opMode )
               {
                  goto done ;
               }

               // if the requested locked mode is compabile with all members
               // in both upgrade and waiter list, then add it into owner list
               if ( FALSE == _checkLockModeWithOthers( pLRBHdr->upgradeLRB,
                                                       dpsTxExectr,
                                                       requestLockMode,
                                                       pLRBIncompatible,
                                                       FALSE ) )
               {
                  if ( FALSE == _checkLockModeWithOthers( pLRBHdr->waiterLRB,
                                                          dpsTxExectr,
                                                          requestLockMode,
                                                          pLRBIncompatible,
                                                          FALSE ) )
                  {
                     // add to owner list
                     if ( DPS_TRANSLOCK_OP_MODE_TEST != opMode )
                     {
                        if ( pLRBToInsert )
                        {
                           _addToOwnerLRBList( pLRBToInsert, pLRBNew ) ;
                        }
                        else
                        {
                           _addToLRBListHead( pLRBHdr->ownerLRB, pLRBNew ) ;
                        }

                        // set newestISOwner or newestIXOwner in lrbhdr
                        _setNewestISIXOwner( pLRBNew ) ;

                        // sample tick before adding to edulist or setting
                        // waiter info to make sure snapshot trans is correct.
                        pLRBNew->beginTick.sample() ;

                        // add the new LRB to EDU LRB list
                        _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

                        // mark the new LRB is used
                        bFreeLRB = FALSE ;
                        pLRB     = pLRBNew ;
                     }
                     // job done
                     goto done ;
                  }
               }

               // if the requested lock mode is not compabile with all
               // members in upgrade and waiter list, add it to waiter list
               if ( DPS_TRANSLOCK_OP_MODE_ACQUIRE == opMode )
               {

                  // add to the end of waiter list
                  _addToLRBListTail( LOCKMGR_LRB_QUE_WAITER, pLRBHdr->waiterLRB,
                                     pLRBNew ) ;

                  // sample tick before adding to edulist or setting waiter info
                  // to make sure snapshot trans is correct.
                  pLRBNew->beginTick.sample() ;

                  // set the wait info in dpsTxExectr
                  dpsTxExectr->setWaiterInfo( pLRBNew, DPS_QUE_WAITER,
                                              _lockMgrType ) ;

                  // set return code to SDB_DPS_TRANS_APPEND_TO_WAIT
                  rc = SDB_DPS_TRANS_APPEND_TO_WAIT ;

                  // mark the new LRB is used
                  bFreeLRB = FALSE ;
               }
               else
               {
                  rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
               }

               // construct the conflict lock info ( representative )
               if ( pdpsTxResInfo )
               {
                  pLRB = pLRBIncompatible ;

                  pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
                  pdpsTxResInfo->_lockType = pLRB->lockMode ;
                  pdpsTxResInfo->_eduID    = pLRB->dpsTxExectr->getEDUID() ;
                  pdpsTxResInfo->_tid      = pLRB->dpsTxExectr->getTID() ;
               }

               // job done
               goto done ;

            }  // if both upgrade and waiter queue/list are empty
         }  // if request lock mode is compatible with other owners
      }  // if in owner list
   done:
      {
         if( callback )
         {
            // need to call this under bktlatch to make sure we are safe to
            // lookup information in LRBHdr
            callback->afterLockAcquire(
                         lockId, rc,
                         requestLockMode,
                         pLRB ? pLRB->refCounter : 0,
                         ( testMode ? DPS_TRANSLOCK_OP_MODE_TEST : opMode ),
                         pLRBHdr,
                         pLRBHdr ? &(pLRBHdr->extData) : NULL ) ;
         }
         // there is a scenario, using testX to clean up 'old version'
         // hanging off LRB header. Release LRB header if it is possible
         // when test opreation succeeded.
         if ( ( DPS_TRANSLOCK_OP_MODE_TEST == opMode ) &&
              ( DPS_TRANSLOCK_X == requestLockMode ) &&
              ( SDB_OK == rc ) )
         {
            if (    ( NULL != pLRBHdr )
                 && ( NULL == pLRBHdr->ownerLRB )
                 && ( NULL == pLRBHdr->upgradeLRB )
                 && ( NULL == pLRBHdr->waiterLRB )
                 && ( pLRBHdr->extData.canRelease() ) )
            {
               _removeFromLRBHeaderList( _LockHdrBkt[bktIdx].lrbHdr, pLRBHdr ) ;
               _releaseLRBHdr( pLRBHdr ) ;
            }
         }
      }

      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }
      if ( bFreeLRB )
      {
         _releaseLRB( pLRBNew ) ;
         bFreeLRB = FALSE ;
      }
      else
      {
         // sample lock owning( first time ) or waiting timestamp ( ossTick )
         if ( !testMode && pLRBNew )
         {
            if ( !(BOOLEAN) (pLRBNew->beginTick) )
            {
               pLRBNew->beginTick.sample() ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER__TRYACQUIREORTEST, rc ) ;
      return rc;
   error:
      goto done ;
   }


   //
   // Description: acquire and setup a new LRB header and a new LRB
   // Function: acquire a new LRB Header and a new LRB, and initialize these
   //           two new object with given input parameters.
   //           the new LRB will be linked to the new LRB Header
   // Input:
   //    eduId           -- edu Id
   //    lockId          -- lock id
   //    requestLockMode -- requested lock mode
   // Output:
   //    pLRBHdrNew      -- pointer of the new LRB header object
   //    pLRBNew         -- pointer of the new LRB object
   // Return:  SDB_OK or any error returned from _utilSegmentManager::acquire
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRBANDHEADER, "dpsTransLockManager::_prepareNewLRBAndHeader" )
   INT32 dpsTransLockManager::_prepareNewLRBAndHeader
   (
      _dpsTransExecutor *        dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      const UINT32               bktIdx,
      dpsTransLRBHeader *      & pLRBHdrNew,
      dpsTransLRB       *      & pLRBNew
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRBANDHEADER ) ;

      INT32   rc           = SDB_OK ;
      BOOLEAN bAllocLRBHdr = TRUE ;
      BOOLEAN bAllocLRB    = TRUE ;

      // acquire a free LRB Header
      if ( NULL == pLRBHdrNew )
      {
         pLRBHdrNew = SDB_OSS_NEW dpsTransLRBHeader( lockId, bktIdx ) ;
         if ( ! pLRBHdrNew )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc a LRBHeader (rc=%d)", rc ) ;
            goto error ;
         }
      }
      else
      {
         bAllocLRBHdr = FALSE ;
      }

      // acquire a lrb
      if ( NULL == pLRBNew )
      {
         pLRBNew = SDB_OSS_NEW dpsTransLRB( dpsTxExectr,
                                            requestLockMode,
                                            pLRBHdrNew ) ;
         if ( ! pLRBNew )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc a LRB (rc=%d)", rc ) ;
            goto error ;
         }
      }
      else
      {
         bAllocLRB = FALSE ;
         pLRBNew->lrbHdr = pLRBHdrNew ;
      }

      if ( bAllocLRBHdr )
      {
         pLRBHdrNew->ownerLRB = pLRBNew ;

         if ( DPS_TRANSLOCK_IS == requestLockMode )
         {
            pLRBHdrNew->newestISOwner = pLRBNew ;
         }
         else if ( DPS_TRANSLOCK_IX == requestLockMode )
         {
            pLRBHdrNew->newestIXOwner = pLRBNew ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRBANDHEADER, rc ) ;
      return rc ;
   error :
      if( pLRBNew && bAllocLRB )
      {
         _releaseLRB( pLRBNew ) ;
      }
      if( pLRBHdrNew && bAllocLRBHdr )
      {
         _releaseLRBHdr( pLRBHdrNew ) ;
      }
      goto done;
   }


   //
   // Description: acquire a lock with given mode
   // Function:    acquire a lock with requested mode
   //              . if the request is fulfilled, LRB is added to owner list
   //                and EDU LRB chain ( all locks in same TX ).
   //              . if the lock is record lock, intent lock on collection
   //                and collection space will be also acquired.
   //              . if the lock is collection lock, an intention lock on
   //                collection space will be also acquired.
   //              . if lock is not applicable at that time, the LRB will be
   //                added to waiter or upgrade list and wait.
   //              . while lock waiting it could be either woken up or
   //                lock waiting time out ( return an error )
   //              . when it is woken up from lock waiting,
   //                it will try to acquire the lock again
   // Input:
   //    dpsTxExectr     -- dpsTxExectr ( per EDU, similar to eduCB,
   //                                     isolate pmd from dps )
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   //    pContext        -- pointer to context :
   //                         dmsTBTransContext
   //                         dmsIXTransContext
   //    callback        -- pointer to trans lock callback
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo, a structure used to
   //                       save the conflict lock info
   // Return:
   //     SDB_OK,                                 -- lock is acquired
   //     SDB_DPS_TRANS_APPEND_TO_WAIT,           -- put on wait/upgrade list
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,   -- invalid upgrade request
   //     SDB_INTERRUPT,                          -- lock wait interrrupted
   //     SDB_TIMEOUT,                            -- lock wait timeout elapsed
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_ACQUIRE, "dpsTransLockManager::acquire" )
   INT32 dpsTransLockManager::acquire
   (
      _dpsTransExecutor        * dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      IContext                 * pContext,
      dpsTransRetInfo          * pdpsTxResInfo,
      _dpsITransLockCallback   * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_ACQUIRE ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif

      INT32 rc  = SDB_OK ,
            rc2 = SDB_OK ; // context pause or resume return code
      dpsTransLockId     iLockId ;
      DPS_TRANSLOCK_TYPE iLockMode = DPS_TRANSLOCK_MAX ;
      BOOLEAN isIntentLockAcquired = FALSE ;

      UINT32  bktIdx   = DPS_LOCK_INVALID_BUCKET_SLOT ;
      BOOLEAN bLatched = FALSE ;
      BOOLEAN bSortedAndSUXModeList = ( ( ! _autoUpperLockOp ) ||
                                        lockId.isLeafLevel() ) ;

      if ( ! lockId.isValid() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // for trans lock, get intent lock at first
      if ( _autoUpperLockOp && ( ! lockId.isRootLevel()) )
      {
         iLockId   = lockId.upOneLevel() ;
         iLockMode = dpsIntentLockMode( requestLockMode ) ;
         rc = acquire( dpsTxExectr, iLockId, iLockMode,
                       pContext, pdpsTxResInfo );
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         isIntentLockAcquired = TRUE ;
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

   acquireRetry:
      // check if EDU is intrrupted first
      if ( dpsTxExectr->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }

      //
      // acquire the lock
      //
      rc = _tryAcquireOrTest( dpsTxExectr, lockId, requestLockMode,
                              DPS_TRANSLOCK_OP_MODE_ACQUIRE,
                              bktIdx,
                              bLatched,
                              pdpsTxResInfo,
                              callback ) ;
      // _tryAcquireOrTest acquires bucket latch by default unless the input
      // parameter, bLatched, is set to TRUE; and it always releases the latch
      // before returns
      bLatched = FALSE ;

      if ( SDB_OK == rc )
      {
         // lock acquired sucessfully, job done
         goto done ;
      }
      else if ( SDB_DPS_TRANS_APPEND_TO_WAIT == rc )
      {
         // checks if need to process lock waiting logic
         goto LockWaiting ;
      }
      else
      {
         // lock request is neither fulfilled nor put on wait
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_ACQUIRE, rc ) ;
      return rc ;

   LockWaiting:
      //
      // Processing lock waiting.
      //   at this time the lock request is put on upgrade or wait queue.
      //

      // init rc2. it is used to mark whether the context has
      // been successfully paused / resumed
      rc2 = SDB_OK ;

      // pause the context before waiting the lock
      if ( pContext )
      {
         rc2 = pContext->pause() ;
#ifdef _DEBUG
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING("context pause rc:"),
                    PD_PACK_INT( rc2 ) )  ;
#endif
      }
      if ( SDB_OK != rc2 )
      {
         goto postLockWaiting ;
      }

      // wait for the lock
      rc = _waitLock( dpsTxExectr ) ;
      if ( SDB_OK != rc )
      {
#ifdef _DEBUG
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING("waitLock rc:"),
                    PD_PACK_INT( rc ) )  ;
#endif
         dpsTxExectr->getExecutor()->resetInfo( EDU_INFO_ERROR ) ;
         dpsTxExectr->getExecutor()->printInfo( EDU_INFO_ERROR,
                     "Acquire transaction lock(%s)(%s) failed",
                     lockId.toString().c_str(),
                     lockModeToString( requestLockMode ) ) ;
         goto postLockWaiting ;
      }

      // NOTE:
      // IContext interface has two classes, dmsMBContext and dmsIXTransContext
      // dmsMBContext resume will regain the mblatch, while dmsIXTransContext
      // will reaquire memory index tree latch. 
      // The latching protocol / order is
      //    mblatch,
      //    bkt latch, 
      //    memory tree latch

      // resume context
      if ( pContext )
      {
         rc2 = pContext->resume( ICTX_RESUME_CONTEXT ) ;
#ifdef _DEBUG
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING("context resume rc:"),
                    PD_PACK_INT( rc2 ) )  ;
#endif
      }

   postLockWaiting:
      // remove the LRB from upgrade or waiter list and remove the empty
      // LRB Header if it is needed
      if ( ! bLatched )
      {
         // need latch bucket before remove it from upgrade or waiter list
         _acquireOpLatch( bktIdx ) ;
         bLatched = TRUE ;
      }

      // resume scanner
      if ( pContext && ( SDB_OK == rc2 ) )
      {
         // acquire the memory tree latch
         rc2 = pContext->resume( ICTX_RESUME_SCANNER ) ;
      }

      // remove the LRB from upgrade or waiter list and wakeup next waiter
      // if necessary. The empty LRB Header will be removed only when it is
      // needed, i.e., when _waitLock() fails( either timeout duration elapsed
      // or be interrupted )
      // The reason removing the empty LRB Header only when _waitLock() fails
      // is if _waitLock returns success, when retry acquiring the lock,
      // _tryAcquireOrTest(), the LRB Header will be added back again.
      _removeFromUpgradeOrWaitList( dpsTxExectr,
                                    lockId, bktIdx,
                                    ( SDB_OK != rc ),
                                    bSortedAndSUXModeList,
                                    FALSE ) ;

      // retry acquire the lock when following conditions are satisfied:
      //   . SDB_OK == rc = _waitLock(), it has been woken up
      //   . SDB_OK == rc2, context resumed successfully
      //   . bLatched, holding the bucket latch, to avoid race condition
      //     between itself and the one is woken up
      if ( ( SDB_OK == rc ) && ( SDB_OK == rc2 ) && bLatched )
      {
         goto acquireRetry ;
      }

      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }

      // set recode to rc2 when _waitLock() succeeded but resume()
      // or pause() failed
      if ( ( SDB_OK != rc2 ) && ( SDB_OK == rc ) )
      {
         rc = rc2 ;
      }

   error:
      // when _waitLock() fails ( timeout or be interrupted ), or pause/resume
      // context fails, we will need to release upper level intent lock
      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }
      if ( isIntentLockAcquired && _autoUpperLockOp )
      {
         release( dpsTxExectr, iLockId, FALSE ) ;
         isIntentLockAcquired = FALSE ;
      }
      goto done ;
   }


   //
   // Description: try to acquire a lock with given mode
   // Function:    try to acquire a lock with given mode
   //              . if the request is fulfilled, LRB is added to owner list
   //                and EDU LRB chain ( all locks in same TX ).
   //              . if lock is not applicable at that time, the LRB will be
   //                added to waiter or upgrade list and wait.
   //              . while lock waiting it could be either woken up or
   //                lock waiting time out ( return an error )
   //              . when it is woken up from lock waiting,
   //                it will NOT try to acquire the lock again,
   //                just removes the request from wait or upgrade list,
   //                and returns the error code.
   // NOTE: this function will always pause context when the lock is not acquired
   //                 
   // Input:
   //    dpsTxExectr     -- dpsTxExectr ( per EDU, similar to eduCB,
   //                                     isolate pmd from dps )
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   //    pContext        -- pointer to context :
   //                         dmsTBTransContext
   //                         dmsIXTransContext
   //    callback        -- pointer to trans lock callback
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo, a structure used to
   //                       save the conflict lock info
   // Return:
   //     SDB_OK,                                 -- lock is acquired
   //     SDB_DPS_TRANS_APPEND_TO_WAIT,           -- put on wait/upgrade list
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,   -- invalid upgrade request
   //     SDB_INTERRUPT,                          -- lock wait interrrupted
   //     SDB_TIMEOUT,                            -- lock wait timeout elapsed
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   INT32 dpsTransLockManager::tryAcquireAndWait
   (
      _dpsTransExecutor        * dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      _IContext                * pContext,
      dpsTransRetInfo          * pdpsTxResInfo,
      _dpsITransLockCallback   * callback
   )
   {
      INT32 rc  = SDB_OK ,
            rc2 = SDB_OK ; // context pause or resume return code
      dpsTransLockId     iLockId ;
      DPS_TRANSLOCK_TYPE iLockMode = DPS_TRANSLOCK_MAX ;
      BOOLEAN isIntentLockAcquired = FALSE ;

      UINT32  bktIdx   = DPS_LOCK_INVALID_BUCKET_SLOT ;
      BOOLEAN bLatched = FALSE ;

      BOOLEAN bContextPaused  = FALSE ;
      BOOLEAN bSortedAndSUXModeList = ( ( ! _autoUpperLockOp ) ||
                                        lockId.isLeafLevel() ) ;
      if ( ! lockId.isValid() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // for trans lock, get intent lock at first
      if ( _autoUpperLockOp && ( ! lockId.isRootLevel()) )
      {
         iLockId   = lockId.upOneLevel() ;
         iLockMode = dpsIntentLockMode( requestLockMode ) ;
         rc = tryAcquire( dpsTxExectr, iLockId, iLockMode, pdpsTxResInfo );
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         isIntentLockAcquired = TRUE ;
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // check if EDU is intrrupted first
      if ( dpsTxExectr->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }

      //
      // acquire the lock
      //
      rc = _tryAcquireOrTest( dpsTxExectr, lockId, requestLockMode,
                              DPS_TRANSLOCK_OP_MODE_ACQUIRE,
                              bktIdx,
                              bLatched,
                              pdpsTxResInfo,
                              callback ) ;
      // _tryAcquireOrTest acquires bucket latch by default unless the input
      // parameter, bLatched, is set to TRUE; and it always releases the latch
      // before returns
      bLatched = FALSE ;

      if ( SDB_OK == rc )
      {
         // lock acquired sucessfully, job done
         goto done ;
      }
      else if ( SDB_DPS_TRANS_APPEND_TO_WAIT == rc )
      {
         // checks if need to process lock waiting logic
         goto LockWaiting ;
      }
      else
      {
         // lock request is neither fulfilled nor put on wait
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_ACQUIRE, rc ) ;
      return rc ;

   LockWaiting:
      //
      // Processing lock waiting.
      //   at this time the lock request is put on upgrade or wait queue.
      //

      // init rc2. it is used to mark whether the context has
      // been successfully paused / resumed
      rc2 = SDB_OK ;

      // pause the context before waiting the lock
      if ( pContext )
      {
         rc2 = pContext->pause() ;
         bContextPaused = TRUE ;
      }
      if ( SDB_OK != rc2 )
      {
         goto postLockWaiting ;
      }

      // wait for the lock
      rc = _waitLock( dpsTxExectr ) ;

   postLockWaiting:
      // remove the LRB from upgrade or waiter list and remove the empty
      // LRB Header if it is needed
      if ( ! bLatched )
      {
         // need latch bucket before remove it from upgrade or waiter list
         _acquireOpLatch( bktIdx ) ;
         bLatched = TRUE ;
      }

      // remove the LRB from upgrade or waiter list and wakeup next waiter
      // if necessary. The empty LRB Header will be removed only when it is
      // needed, i.e., when _waitLock() fails( either timeout duration elapsed
      // or be interrupted )
      // The reason removing the empty LRB Header only when _waitLock() fails
      // is if _waitLock returns success, when retry acquiring the lock,
      // _tryAcquireOrTest(), the LRB Header will be added back again.
      _removeFromUpgradeOrWaitList( dpsTxExectr,
                                    lockId, bktIdx,
                                    ( SDB_OK != rc ),
                                    bSortedAndSUXModeList,
                                    TRUE ) ;

      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }

      if ( SDB_OK == rc )
      {
         if ( SDB_OK != rc2 )
         {
            // rc2 means error occurred in pause()
            rc = rc2 ;
         }
         else
         {
            rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
         }
      }
   error:
      // when _waitLock() fails ( timeout or be interrupted ), or pause/resume
      // context fails, we will need to release upper level intent lock
      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }

      // pause the context
      if ( pContext )
      {
         if ( FALSE == bContextPaused )
         {
            rc2 = pContext->pause() ;
            bContextPaused = TRUE ;
         }
      }

      // release upper level lock
      if ( isIntentLockAcquired && _autoUpperLockOp )
      {
         release( dpsTxExectr, iLockId, FALSE ) ;
         isIntentLockAcquired = FALSE ;
      }
      goto done ;
   }


   //
   // Description: search EDU LRB chain and find the LRB has same lock ID
   // Function:    walk through the EDU LRB chain( doubly linked list ),
   //              and search for the LRB with the LRB Header it associated to
   //              containing the same lockId.
   // Input:
   //    dpsTxExectr  -- dpsTxExectr
   //    lockId       -- the lock ID
   // Output:
   //    None
   // Return:      the LRB pointer when it is found
   //              NULL when it is not found
   // Dependency:  None
   //
   dpsTransLRB * dpsTransLockManager::_getLRBFromEDULRBList
   (
      _dpsTransExecutor    * dpsTxExectr,
      const dpsTransLockId & lockId
   )
   {
      dpsTransLRB * pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      while ( pLRB )
      {
         if ( ( NULL != pLRB->lrbHdr ) && ( lockId == pLRB->lrbHdr->lockId ) )
         {
            break ;
         }
         pLRB = pLRB->eduLrbPrev ;
      }
      return pLRB ;
   }

   //
   // Description: search EDU LRB chain and find the LRB has same lock ID
   // Function:    walk through the EDU LRB chain( doubly linked list ),
   //              and search for the LRB with the LRB Header it associated to
   //              containing the same lockId.
   // Input:
   //    dpsTxExectr  -- dpsTxExectr
   //    lockId       -- the lock ID
   // Output:
   //    owningLockMode -- the lock mode it is owning when found
   // Return:      TRUE  the caller thread is owning this lock
   //              FALSE the caller thread is not ownning this lock
   // Dependency:  None
   //
   BOOLEAN dpsTransLockManager::isHolding
   (
      _dpsTransExecutor    * dpsTxExectr,
      const dpsTransLockId & lockId,
      INT8                 & owningLockMode
   )
   {
      BOOLEAN found = FALSE ;
      dpsTransLRB * pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;

      while ( pLRB )
      {
#ifdef _DEBUG
         SDB_ASSERT ( ( NULL != pLRB->lrbHdr ) , "Invalid LRB header" ) ;
#endif
         if ( lockId == pLRB->lrbHdr->lockId )
         {
            owningLockMode = pLRB->lockMode ;
            found          = TRUE ;
            break ;
         }
         pLRB = pLRB->eduLrbPrev ;
      }
      return found ;
   }

   //
   // Description: core logic of release a lock
   // Function: decrease lock reference counter, do following when the counter
   //           comes zero :
   //           .  remove from the edu ( caller ) LRB chain
   //           .  remove it from owner list
   //           .  wake up a waiter( upgrade or waiter list ) when necessary,
   //           .  remove the LRB Header if it is empty ( owner, waiter,
   //              upgrade list are all empty )
   // Input:
   //    dpsTxExectr         -- dpsTransExecutor
   //    lockId              -- lock id
   //    bForceRelease       -- force release flag
   //    refCountToDecrease  -- value to be decreased from lock refCounter
   //                           when release CL,CS lock in 'force' mode ;
   //                           if this value is zero, then set lock refCounter
   //                           to zero when release CL,CS lock in 'force'
   //
   //    callback            -- pointer to translock callback
   // Output :
   //    bForceRelease       -- value of lock refCounter if release
   //                           a record lock with 'force' mode
   //
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__RELEASE, "dpsTransLockManager::_release" )
   void dpsTransLockManager::_release
   (
      _dpsTransExecutor       * dpsTxExectr,
      const dpsTransLockId    & lockId,
      dpsTransLRB             * pOwnerLRB,
      const BOOLEAN             bForceRelease,
      UINT32                  & refCountToDecrease,
      _dpsITransLockCallback  * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__RELEASE ) ;

      UINT32       bktIdx        = DPS_LOCK_INVALID_BUCKET_SLOT ;
      dpsTransLRB *pMyLRB        = pOwnerLRB,
                  *lrbToRelease  = NULL ,
                  *pWaiterLRB    = NULL ;
      dpsTransLRB *pLRBIncompatible = NULL ;
      dpsTransLRBHeader *pLRBHdr = NULL,
                        *pLRBHdrToRelease = NULL;
      BOOLEAN bLatched = FALSE ;
      BOOLEAN foundIncomp = FALSE ;
      BOOLEAN bisLeafLevel = lockId.isLeafLevel() ;
      BOOLEAN bSortedAndSUXModeList = (( ! _autoUpperLockOp ) || bisLeafLevel );

      // short cut that we may do without bkt latch.
      // If we have the LRB, not doing "force release" and refCounter > 1,
      // we may simply decrease the refCounter. Otherwise, we shall go through
      // the normal routine.
      if ( ( ! bForceRelease ) && pMyLRB && ( pMyLRB->refCounter > 1 ) )
      {
         pMyLRB->refCounter-- ;
         goto done ;
      }

      // some quick ways to get LRB
      if ( NULL == pMyLRB )
      {
         if ( _autoUpperLockOp )
         {
            // for non-leaf lock ( CS, CL ), lookup the executor _mapLockID map
            if ( ! bisLeafLevel )
            {
               dpsTxExectr->findLock( lockId, pMyLRB, _lockMgrType ) ;
            }
            else
            {
               // for record lock, find the LRB from the EDU LRB list
               // in a single lock release scenario, the lock ID's LRB should
               // be at the end of the EDU LRB list. This should be guaranteed
               // through _moveToEDULRBListTail and _addToEDULRBListTail
               pMyLRB = _getLRBFromEDULRBList( dpsTxExectr, lockId ) ;
            }
         }
         else
         {
            pMyLRB = _getLRBFromEDULRBList( dpsTxExectr, lockId ) ;
         }

         if ( ( ! bForceRelease ) && pMyLRB && ( pMyLRB->refCounter > 1) )
         {
            pMyLRB->refCounter-- ;
            goto done ;
         }
      }

      // We must have a LRB by now
      SDB_ASSERT ( NULL != pMyLRB, "lrb cannot be NULL" ) ;
      // normal lock release routine

      bktIdx = pMyLRB->lrbHdr->bktIdx ;

      // latch the bucket
      _acquireOpLatch( bktIdx ) ;
      bLatched = TRUE ;

      pLRBHdr = pMyLRB->lrbHdr ;

      SDB_ASSERT( ( NULL != pLRBHdr ),
                  "Trying to release a non-exist lock" ) ;

      if ( bForceRelease )
      {
         if ( _autoUpperLockOp )
         {
            if ( bisLeafLevel )
            {
               // save the record lock refCounter to output parameter
               // so can we decrease that value for CL,CS lock later
               refCountToDecrease = pMyLRB->refCounter ;
               pMyLRB->refCounter = 0 ;
            }
            else
            {
               if ( 0 == refCountToDecrease )
               {
                  // when release CL, CL lock with 'refoce' mode,
                  // if the refCounter passed in is zero,
                  // then set lock refCounter to zero
                  pMyLRB->refCounter = 0 ;
               }
               else
               {
                  // when release CL, CL lock with 'refoce' mode,
                  // if the refCounter passed in is non-zero value,
                  // then substract that value from current lock refCounter
                  pMyLRB->refCounter -= refCountToDecrease ;
               }
            }
         }
         else
         {
            pMyLRB->refCounter = 0 ;
         }
      }
      else
      {
         SDB_ASSERT( pMyLRB->refCounter > 0, "refCounter is negative");

         pMyLRB->refCounter -- ;
      }

      {
         // invoke call back function before release
         if( callback )
         {
            callback->beforeLockRelease( lockId,
                                         pMyLRB->lockMode,
                                         pMyLRB->refCounter,
                                         pLRBHdr,
                                         pLRBHdr ? &(pLRBHdr->extData) :
                                                   NULL ) ;
         }
         else if ( pLRBHdr && pLRBHdr->extData._onLockReleaseFunc )
         {
            pLRBHdr->extData._onLockReleaseFunc( lockId,
                                                 pMyLRB->lockMode,
                                                 pMyLRB->refCounter,
                                                 &(pLRBHdr->extData),
                                                 -1,
                                                 FALSE ) ;
         }
      }

      if ( 0 == pMyLRB->refCounter )
      {
         // remove it from EDU LRB list
         _removeFromEDULRBList( dpsTxExectr, pMyLRB, lockId ) ;

         // remove it from lock owner list
         _removeFromLRBList( LOCKMGR_LRB_QUE_OWNER, pLRBHdr->ownerLRB, pMyLRB );

         // get the waiter LRB pointer
         if ( pLRBHdr->upgradeLRB )
         {
            pWaiterLRB = pLRBHdr->upgradeLRB ;
         }
         else if ( pLRBHdr->waiterLRB  )
         {
            pWaiterLRB = pLRBHdr->waiterLRB  ;
         }
         if ( pWaiterLRB &&
              ( ! OSS_BIT_TEST( pWaiterLRB->status, DPS_LRB_STATUS_AWAKE ) ) )
         {
            // lookup owner list check if the waiter lockMode is compabile
            // with other owners
            foundIncomp = _checkLockModeWithOthers( pLRBHdr->ownerLRB,
                                                    pWaiterLRB->dpsTxExectr,
                                                    pWaiterLRB->lockMode,
                                                    pLRBIncompatible,
                                                    bSortedAndSUXModeList ) ;

            // if the owner queue is empty ( after remove current owner ),
            // or if the waiter lockMode is compabile with other owners
            // wake it up
            if ( FALSE == foundIncomp )
            {
               // wake up the edu by posting an event
               OSS_BIT_SET( pWaiterLRB->status, DPS_LRB_STATUS_AWAKE ) ;
               _wakeUp( pWaiterLRB->dpsTxExectr ) ;
            }
         }

         // save the pointer of owner LRB to be released
         lrbToRelease = pMyLRB;

         // remove the LRB Header if owner, waiter,
         // and upgrade list are all empty
         if (    ( NULL == pLRBHdr->ownerLRB )
              && ( NULL == pLRBHdr->upgradeLRB )
              && ( NULL == pLRBHdr->waiterLRB )
              && ( pLRBHdr->extData.canRelease() ) )
         {
            _removeFromLRBHeaderList( _LockHdrBkt[bktIdx].lrbHdr, pLRBHdr ) ;
            pLRBHdrToRelease = pLRBHdr;
         }
      }  // end of if pMyLRB->refCounter is zero
   done:
      // release the bucket latch
      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }
      if ( lrbToRelease )
      {
         _releaseLRB( lrbToRelease ) ;
      }
      if ( pLRBHdrToRelease )
      {
         _releaseLRBHdr( pLRBHdrToRelease ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER__RELEASE ) ;
      return ;
   }


   //
   // Description: release a lock
   // Function: release a lock, remove from the edu ( caller ) LRB chain
   //           and remove it from owner list if the refference counter is zero.
   //           Wake up a waiter when necessary, it will also release the upper
   //           level intent lock
   // Input:
   //    dpsTxExectr     -- pointer to dpsTransExecutor
   //    lockId          -- lock id
   //    bForceRelease   -- requested lock mode
   // Output:
   //    none
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_RELEASE, "dpsTransLockManager::release" )
   void dpsTransLockManager::release
   (
      _dpsTransExecutor      * dpsTxExectr,
      const dpsTransLockId   & lockId,
      const BOOLEAN            bForceRelease,
      _dpsITransLockCallback * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_RELEASE ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      if ( lockId.isValid() )
      {
         dpsTransLockId myLockId = lockId ;
         BOOLEAN done = FALSE ;
         UINT32  refCountToBeDecreased = 0 ;
         do
         {
            // main logic of release by lockId
            _release( dpsTxExectr, myLockId,
                      NULL, bForceRelease, refCountToBeDecreased,
                      callback ) ;

            if ( _autoUpperLockOp )
            {
               // release the intent lock
               if ( ! myLockId.isRootLevel() )
               {
                  myLockId = myLockId.upOneLevel() ;
               }
               else
               {
                  done = TRUE ;
                  break ;
               }
            }
            else
            {
               done = TRUE ;
               break ;
            }
         } while ( ! done ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER_RELEASE ) ;
      return ;
   }

   //
   // Description: Force release all locks an EDU is holding
   // Function: walk though the EDU LRB chain, force release all locks
   //           an EDU is holding.
   //           . remove them from the edu ( caller ) LRB chain
   //           . remove it from owner list
   //           . wake up a waiter when necessary,
   //           . release the upper level intent lock
   // Input:
   //    dpsTxExectr     -- pointer to _dpsTransExecutor
   //    callback        -- pointer to call back function
   // Output:
   //    none
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_RELEASEALL, "dpsTransLockManager::releaseAll" )
   void dpsTransLockManager::releaseAll
   (
      _dpsTransExecutor      * dpsTxExectr,
      _dpsITransLockCallback * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_RELEASEALL ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      dpsTransLRB       * pLRB     = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      dpsTransLRB       * pNextOne = NULL ;
      dpsTransLRBHeader * pLRBHdr  = NULL ;
      dpsTransLockId      lockId ;
      UINT32              refCount = 0 ;

      if ( NULL != pLRB )
      {
         if ( _autoUpperLockOp )
         {
            // walk through EDU LRB list,
            //   first loop, release all record locks with force mode.
            //   second loop, release all CL locks with force mode.
            //   last loopi, release all CS locks
            for ( UINT32 loop = 0 ; loop < 3; loop++ )
            {
               pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
               while ( pLRB )
               {
                  // save the next LRB address, pLRB->eduLrbPrev,
                  // before release
                  pNextOne = pLRB->eduLrbPrev ;

                  // peek LRB Header
                  pLRBHdr = pLRB->lrbHdr ;
#ifdef _DEBUG
                  SDB_ASSERT( pLRBHdr, "LRB Header can't be null" ) ;
#endif
                  {
                     lockId  = pLRBHdr->lockId ;
#ifdef _DEBUG
                     SDB_ASSERT( lockId.isValid(), "Invalid lockId" ) ;
#endif
                     // first loop, release all record locks
                     if ( ( 0 == loop ) && ( ! lockId.isLeafLevel() ) )
                     {
                        goto nextLock ;
                     }
                     // second loop, release all CL locks
                     if ( ( 1 == loop ) && lockId.isRootLevel() )
                     {
                        goto nextLock ;
                     }
                     refCount = 0 ;
                     _release( dpsTxExectr, lockId, pLRB, TRUE, refCount,
                               callback ) ;
                  }
nextLock:
                  // move to next lock attempt to be released
                  pLRB = pNextOne ;
               } // end while
            }
         }
         // the _autoUpperLockOp flag is off
         else
         {
            // walk through EDU LRB list
            while ( pLRB )
            {
               // save the next LRB address, pLRB->eduLrbPrev,
               // before release, because pLRB may be freed by _release
               pNextOne = pLRB->eduLrbPrev ;

               pLRBHdr = pLRB->lrbHdr ;
#ifdef _DEBUG
               SDB_ASSERT( pLRBHdr, "LRB Header can't be null" ) ;
#endif
               {
                  lockId  = pLRBHdr->lockId ;
#ifdef _DEBUG
                  SDB_ASSERT( lockId.isValid(), "Invalid lockId" ) ;
#endif
                  refCount = 0 ;
                  _release( dpsTxExectr, lockId, pLRB, TRUE, refCount,
                            callback ) ;
               }
               // move to next LRB
               pLRB = pNextOne;
            }
         }
      }

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER_RELEASEALL ) ;
      return ;
   }


   UINT32 dpsTransLockManager::countAllLocks
   (
      _dpsTransExecutor * dpsTxExectr,
      BOOLEAN             bPrintLog,
      CHAR              * memoStr
   )
   {
      dpsTransLRB * pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      dpsTransLRBHeader * pLRBHdr ;
      dpsTransLockId      lockId ;
      UINT32              lockCount = 0 ;

      std::stringstream lockInfo ;

      // walk through EDU LRB list,
      while ( pLRB )
      {
         pLRBHdr = pLRB->lrbHdr ;
         // get LRB Header
         if ( NULL != pLRBHdr )
         {
            lockId  = pLRBHdr->lockId ;
#ifdef _DEBUG
            SDB_ASSERT( lockId.isValid(), "Invalid lockId" ) ;
#endif
            lockCount++ ;
            if ( bPrintLog )
            {
               lockInfo << "LockId[" << lockId.toString() << "] "
                        << "lockMode:" << lockModeToString( pLRB->lockMode )
                        << std::endl ;
            }
         }
         // move to next LRB
         pLRB = pLRB->eduLrbPrev ;
      }
      if ( bPrintLog )
      {
         PD_LOG( PDINFO,
                 "%s"OSS_NEWLINE
                 "Number of locks:%d"OSS_NEWLINE"%s",
                 ( memoStr ? memoStr : "" ),
                 lockCount,
                 lockInfo.str().c_str() ) ;
      }
      return lockCount ;
   }


   //
   // Description: Wakeup a lock waiting EDU
   // Function: wake up an EDU by post an event
   //
   // Input:
   //    dpsTxExectr -- pointer to _dpsTransExecutor
   // Output:
   //    none
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__WAKEUP, "dpsTransLockManager::_wakeUp" )
   void dpsTransLockManager::_wakeUp( _dpsTransExecutor *dpsTxExectr )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__WAKEUP ) ;
#ifdef _DEBUG
      // SDB_ASSERT( dpsTxExectr, "dpsTransExecutor can't be NULL" ) ;
      // dpsTransLRB *pLRB = dpsTxExectr->getWaiterLRB( _lockMgrType ) ;
      // dpsTransLRBHeader * pLRBHdr = pLRB->lrbHdr ;
      // PD_LOG( PDDEBUG, "Waking up TID:%d for lock(%s)",
      //         dpsTxExectr->getTID(), pLRBHdr->lockId.toString().c_str() ) ;
#endif
      dpsTxExectr->wakeup() ;
      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER__WAKEUP ) ;
   }


   //
   // Description: Wait a lock
   // Function: wait a lock by waiting on an event
   //
   // Input:
   //    dpsTxExectr -- pointer to _dpsTransExecutor
   // Output:
   //    none
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__WAITLOCK, "dpsTransLockManager::_waitLock" )
   INT32 dpsTransLockManager::_waitLock ( _dpsTransExecutor *dpsTxExectr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__WAITLOCK ) ;
#ifdef _DEBUG
      // dpsTransLRB *pLRB = dpsTxExectr->getWaiterLRB( _lockMgrType ) ;

      // SDB_ASSERT( dpsTxExectr, "dpsTransExecutor can't be NULL" ) ;
      // PD_LOG( PDDEBUG, "Wait on lock, TID:%d, lockId:( %s ), mode:%s",
      //         dpsTxExectr->getTID(),
      //         pLRB->lrbHdr->lockId.toString().c_str(),
      //         lockModeToString( pLRB->lockMode ) ) ;
#endif
      rc = dpsTxExectr->wait( dpsTxExectr->getTransTimeout() ) ;

      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER__WAITLOCK, rc ) ;
      return rc ;
   }


   //
   // Description: try to acquire a lock with given mode
   // Function:    try to acquire a lock with given mode
   //              . if the request is fulfilled, LRB is added to owner list
   //                and EDU LRB chain ( all locks in same TX ).
   //              . if the lock is record lock, intent lock on collection
   //                and collection space will be also acquired.
   //              . if the lock is collection lock, an intention lock on
   //                collection space will be also acquired.
   //              . if lock is not applicable at that time,
   //                the LRB will NOT be put into waiter / upgrade list,
   //                SDB_DPS_TRANS_LOCK_INCOMPATIBLE will be returned.
   // Input:
   //    dpsTxExectr     -- dpsTxExectr
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo
   // Return:
   //     SDB_OK,
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,
   //     SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE, "dpsTransLockManager::tryAcquire" )
   INT32 dpsTransLockManager::tryAcquire
   (
      _dpsTransExecutor        * dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      dpsTransRetInfo          * pdpsTxResInfo,
      _dpsITransLockCallback   * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif

      INT32 rc = SDB_OK ;
      dpsTransLockId iLockId;
      DPS_TRANSLOCK_TYPE iLockMode = DPS_TRANSLOCK_MAX ;
      BOOLEAN isIntentLockAcquired = FALSE;

      if ( ! lockId.isValid() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // get intent lock at first
      // it is not need to get intent lock while lock space
      if ( _autoUpperLockOp && ( ! lockId.isRootLevel()) )
      {
         iLockId = lockId.upOneLevel() ;
         iLockMode = dpsIntentLockMode( requestLockMode ) ;
         rc = tryAcquire( dpsTxExectr, iLockId, iLockMode, pdpsTxResInfo );
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         isIntentLockAcquired = TRUE;
      }

      // check if EDU is intrrupted first
      if ( dpsTxExectr->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }

      // try to acquire the lock
      // when tryAcquire will not add LRB to either upgrade or waiter list
      rc = _tryAcquireOrTest( dpsTxExectr, lockId, requestLockMode,
                              DPS_TRANSLOCK_OP_MODE_TRY,
                              DPS_LOCK_INVALID_BUCKET_SLOT,
                              FALSE,
                              pdpsTxResInfo,
                              callback ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE, rc ) ;
      return rc;

   error:
      if ( ( isIntentLockAcquired ) && _autoUpperLockOp )
      {
         release( dpsTxExectr, iLockId, FALSE ) ;
         isIntentLockAcquired = FALSE ;
      }
      goto done;
   }


   //
   // Description: test whether a lock can be acquired with given mode
   // Function:    test whether a lock can be acquired with given mode
   //              . if the request can be fulfilled, returns SDB_OK,
   //                the request will not be added to either owner list
   //                or EDU LRB chain, as it doesn't really acquire the lock.
   //              . if lock is not applicable at that time,
   //                SDB_DPS_TRANS_LOCK_INCOMPATIBLE will be returned.
   // Input:
   //    dpsTxExectr     -- dpsTxExectr
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   //    isPreemptMode   -- if do test with preemptive mode
   //    needIntentLock  -- whether to acquire intent lock in upper level
   //                       WARNING: no need to acquire intent lock only when
   //                       we have acquired earlier
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo
   // Return:
   //     SDB_OK,
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,
   //     SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE, "dpsTransLockManager::testAcquire" )
   INT32 dpsTransLockManager::testAcquire
   (
      _dpsTransExecutor        * dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      const BOOLEAN              isPreemptMode,
      dpsTransRetInfo          * pdpsTxResInfo,
      _dpsITransLockCallback   * callback,
      BOOLEAN                    needIntentLock
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      INT32 rc = SDB_OK;
      dpsTransLockId iLockId;
      DPS_TRANSLOCK_TYPE iLockMode = DPS_TRANSLOCK_MAX ;
      UINT32 bktIdx = DPS_LOCK_INVALID_BUCKET_SLOT ;
      const DPS_TRANSLOCK_OP_MODE_TYPE testOpMode =
               ( isPreemptMode ? DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT
                               : DPS_TRANSLOCK_OP_MODE_TEST ) ;

      if ( ! lockId.isValid() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // get intent lock at first
      // it is not need to get intent lock while lock space
      if ( needIntentLock && _autoUpperLockOp && ( ! lockId.isRootLevel()) )
      {
         iLockId = lockId.upOneLevel() ;
         iLockMode = dpsIntentLockMode( requestLockMode ) ;
         rc = testAcquire( dpsTxExectr, iLockId, iLockMode,
                           isPreemptMode, pdpsTxResInfo, callback, TRUE );
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // test if the request lock mode can be acquired
      // it will not acquire the lock, the LRB will not be added to
      // owner, upgrade or waiter list
      rc = _tryAcquireOrTest( dpsTxExectr, lockId, requestLockMode,
                              testOpMode,
                              bktIdx,
                              FALSE,
                              pdpsTxResInfo,
                              callback ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE, rc ) ;
      return rc;
   error:
      goto done;
   }


   //
   // Description: whether a lock is being waited
   // Function:    test whether a lock is being waited by checking if
   //              the waiter list and upgrade list are empty.
   // Input:
   //    lockId -- lock Id
   // Output:
   //    none
   // Return:
   //    True   -- the lock has waiter(s)
   //    False  -- the lock has no waiter(s)
   // Dependency:  the lock manager must be initialized
   //
   BOOLEAN dpsTransLockManager::hasWait( const dpsTransLockId &lockId )
   {
      BOOLEAN result = FALSE;
      UINT32 bktIdx  = DPS_LOCK_INVALID_BUCKET_SLOT ;
      dpsTransLRBHeader *pLRBHdr = NULL ;

      if ( ! lockId.isValid() )
      {
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // latch the LRB Header list
      _acquireOpLatch( bktIdx ) ;

      pLRBHdr  = _LockHdrBkt[bktIdx].lrbHdr ;
      if ( _getLRBHdrByLockId( lockId, pLRBHdr ) )
      {
         SDB_ASSERT( pLRBHdr, "Invalid LRB Header" ) ;
         if ( pLRBHdr && ( pLRBHdr->waiterLRB || pLRBHdr->upgradeLRB ) )
         {
            result = TRUE ;
         }
      }

      // free LRB Header list latch
      _releaseOpLatch( bktIdx ) ;

   done:
      return result ;
   error:
      goto done ;
   }


   #define DPS_STRING_LEN_MAX ( 512 )
   //
   // format LRB to string, flat one line
   //
   CHAR * dpsTransLockManager::_LRBToString
   (
      dpsTransLRB *pLRB,
      CHAR * pBuf,
      UINT32 bufSz
   )
   {
      if ( pLRB )
      {
         UINT32 seconds = 0, microseconds = 0 ;
         ossTickConversionFactor factor ;
         ossTick endTick ;
         endTick.sample() ;
         ossTickDelta delta = endTick - pLRB->beginTick ;
         delta.convertToTime( factor, seconds, microseconds ) ;

         ossSnprintf( pBuf, bufSz,
            "LRB: %p, EDU: %llu, dpsTxExectr: %p, "
            "eduLrbNext: %p, eduLrbPrev: %p, "
            "lrbHdr: %p, nextLRB: %p ,prevLRB: %p, "
            "refCounter: %llu, lockMode: %s, duration: %llu",
            pLRB,
            pLRB->dpsTxExectr->getEDUID(), pLRB->dpsTxExectr,
            pLRB->eduLrbNext,
            pLRB->eduLrbPrev,
            pLRB->lrbHdr,
            pLRB->nextLRB,
            pLRB->prevLRB,
            pLRB->refCounter,
            lockModeToString( pLRB->lockMode ),
            (UINT64)(seconds*1000 + microseconds / 1000 ) ) ;
      }
      return pBuf ;
   }



   //
   // format LRB to string, each field/member per line, with optional prefix
   //
   CHAR * dpsTransLockManager::_LRBToString
   (
      dpsTransLRB *pLRB,
      CHAR * pBuf,
      UINT32 bufSz,
      CHAR * prefix
   )
   {
      CHAR * pBuff = pBuf;
      CHAR * pDummy= "" ;
      CHAR * pStr  = ( prefix ? prefix : pDummy ) ;
      if ( pLRB )
      {

         UINT32 seconds = 0, microseconds = 0 ;
         ossTickConversionFactor factor ;
         ossTick endTick ;
         endTick.sample() ;
         ossTickDelta delta = endTick - pLRB->beginTick ;
         delta.convertToTime( factor, seconds, microseconds ) ;

         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sEDU          : %llu"OSS_NEWLINE, pStr,
                               pLRB->dpsTxExectr->getEDUID() ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sLRB          : %p"OSS_NEWLINE, pStr,
                               pLRB ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sdpsTxExectr  : %p"OSS_NEWLINE, pStr,
                               pLRB->dpsTxExectr ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%seduLrbNext   : %p"OSS_NEWLINE, pStr,
                               pLRB->eduLrbNext ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%seduLrbPrev   : %p"OSS_NEWLINE, pStr,
                               pLRB->eduLrbPrev ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%slrbHdr       : %p"OSS_NEWLINE, pStr,
                               pLRB->lrbHdr ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%snextLRB      : %p"OSS_NEWLINE, pStr,
                               pLRB->nextLRB ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sprevLRB      : %p"OSS_NEWLINE, pStr,
                               pLRB->prevLRB ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%srefCounter   : %llu"OSS_NEWLINE, pStr,
                               pLRB->refCounter ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%slockMode     : %s"OSS_NEWLINE, pStr,
                               lockModeToString( pLRB->lockMode ) ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sduration     : %llu"OSS_NEWLINE, pStr,
                               (UINT64)(seconds*1000 + microseconds / 1000 ) ) ;
      }
      return pBuf ;
   }


   //
   // format LRB Header to flat one line string
   //
   CHAR * dpsTransLockManager::_LRBHdrToString
   (
      dpsTransLRBHeader *pLRBHdr,
      CHAR * pBuf,
      UINT32 bufSz
   )
   {
      if ( pLRBHdr )
      {
         ossSnprintf( pBuf, bufSz,
          "LRB Header: %p, nextLRBHdr: %p, "
          "ownerLRB: %p, waiterLRB : %p, upgradeLRB: %p, "
          "waiterTail : %p, upgradeTail: %p, "
          "newestISOwner: %p, newestIXOwner: %p,"
          "lockId: ( %s )",
          pLRBHdr,
          pLRBHdr->nextLRBHdr,
          pLRBHdr->ownerLRB,
          pLRBHdr->waiterLRB,
          pLRBHdr->upgradeLRB,
          pLRBHdr->waiterTail,
          pLRBHdr->upgradeTail,
          pLRBHdr->newestISOwner,
          pLRBHdr->newestIXOwner,
          pLRBHdr->lockId.toString().c_str() ) ;
      }
      return pBuf;
   }


   //
   // format LRB Header to string, one field/member per line, w/ optional prefix
   //
   CHAR * dpsTransLockManager::_LRBHdrToString
   (
      dpsTransLRBHeader *pLRBHdr,
      CHAR * pBuf,
      UINT32 bufSz,
      CHAR * prefix
   )
   {
      CHAR * pBuff = pBuf;
      CHAR * pDummy= "" ;
      CHAR * pStr  = ( prefix ? prefix : pDummy ) ;
      if ( pLRBHdr )
      {
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sLRB Header : %p"OSS_NEWLINE, pStr,
                               pLRBHdr ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%snextLRBHdr : %p"OSS_NEWLINE, pStr,
                              pLRBHdr->nextLRBHdr );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sownerLRB   : %p"OSS_NEWLINE, pStr,
                               pLRBHdr->ownerLRB );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%swaiterLRB  : %p"OSS_NEWLINE, pStr,
                               pLRBHdr->waiterLRB );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%supgradeLRB : %p"OSS_NEWLINE, pStr,
                               pLRBHdr->upgradeLRB );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%swaiterTail : %p"OSS_NEWLINE, pStr,
                               pLRBHdr->waiterTail );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%supgradeTail: %p"OSS_NEWLINE, pStr,
                               pLRBHdr->upgradeTail );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sISOwner    : %p"OSS_NEWLINE, pStr,
                               pLRBHdr->newestISOwner );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sIXOwner    : %p"OSS_NEWLINE, pStr,
                               pLRBHdr->newestIXOwner );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%slockId     : ( %s )"OSS_NEWLINE, pStr,
                               pLRBHdr->lockId.toString().c_str() ) ;
      }
      return pBuf;
   }


   //
   // dump all LRB in EDU LRB chain to specified the file ( full path name )
   // for debugging purpose .
   // It walks through EDU LRB chain, the caller shall acquire the monitoring(
   // dump ) latch, acquireMonLatch(), and make sure the executor is still
   // available
   //
   void dpsTransLockManager::dumpLockInfo
   (
      _dpsTransExecutor * dpsTxExectr,
      const CHAR        * fileName,
      BOOLEAN             bOutputInPlainMode
   )
   {
      dpsTransLRB *pLRB  = NULL ;
      CHAR * pStr = NULL ;
      CHAR * prefixStr = (CHAR*)"   " ;
      CHAR szBuffer[ DPS_STRING_LEN_MAX ] = { '\0' } ;
      FILE * fp   = NULL;

      if ( dpsTxExectr )
      {
         // open output file
         if ( NULL == ( fp = fopen( fileName, "ab+" ) ) )
         {
            goto error ;
         }

         pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
         while ( pLRB )
         {
            SDB_ASSERT( pLRB->lrbHdr != NULL ,
                        "Invalid LRB Header." ) ;

            if ( bOutputInPlainMode )
            {
               pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                            sizeof(szBuffer) ) ;
            }
            else
            {
               pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                            sizeof(szBuffer), prefixStr ) ;
            }
            fprintf( fp, "%s"OSS_NEWLINE, pStr ) ;
            pLRB = pLRB->eduLrbPrev ;
         }

         // close output file
         if ( fp )
         {
            fclose( fp ) ;
         }
      }
   error:
      return ;
   }


   //
   // dump LRB Header, owner/waiter/upgrade list info
   // to specified file( full path name ), for debugging purpose only
   // This function doesn't need to acquire monitoring/dump
   // latch ( acquireMonLatch() ), it will get the bucket latch
   void dpsTransLockManager::dumpLockInfo
   (
      const dpsTransLockId & lockId,
      const CHAR           * fileName,
      BOOLEAN                bOutputInPlainMode
   )
   {
      dpsTransLockId iLockId;
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;
      UINT32             bktIdx;
      CHAR * pStr = NULL ;
      CHAR * prefixStr = (CHAR*)"   " ;
      CHAR szBuffer[ DPS_STRING_LEN_MAX ] = { '\0' } ;

      FILE * fp = NULL ;


      // dump intent lock at first
      if ( ! lockId.isRootLevel() )
      {
         iLockId = lockId.upOneLevel() ;
         dumpLockInfo( iLockId, fileName, bOutputInPlainMode );
      }

      // open output file
      if ( NULL == ( fp = fopen( fileName, "ab+" ) ) )
      {
         goto error ;
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // latch the bucket
      _acquireOpLatch( bktIdx ) ;

      pLRBHdr  = _LockHdrBkt[bktIdx].lrbHdr ;
      if ( _getLRBHdrByLockId( lockId, pLRBHdr ))
      {
         fprintf( fp, "%s",  "LRB Header " ) ;
         if ( lockId.isRootLevel() )
         {
            pStr = "( Container Space Lock )" ;
         }
         else if ( lockId.isLeafLevel() )
         {
            pStr = "( Record Lock )" ;
         }
         else
         {
            pStr = "( Container Lock )" ;
         }

         fprintf( fp, " %s"OSS_NEWLINE, pStr ) ;
         fprintf( fp, "%s", "-------------------------------"OSS_NEWLINE ) ;
         if ( bOutputInPlainMode )
         {
            pStr = (CHAR*) _LRBHdrToString(pLRBHdr, szBuffer, sizeof(szBuffer));
         }
         else
         {
            pStr = (CHAR*) _LRBHdrToString(pLRBHdr, szBuffer, sizeof(szBuffer),
                                           NULL );
         }
         fprintf( fp, "%s"OSS_NEWLINE OSS_NEWLINE, pStr ) ;
         if ( pLRBHdr->ownerLRB )
         {
            if ( bOutputInPlainMode )
            {
               fprintf( fp, "%s", "Owner list:"OSS_NEWLINE ) ;
               fprintf( fp, "%s", "-----------"OSS_NEWLINE ) ;
            }
            else
            {
               fprintf( fp, "%sOwner list:"OSS_NEWLINE, prefixStr ) ;
               fprintf( fp, "%s-----------"OSS_NEWLINE, prefixStr ) ;
            }
            pLRB = pLRBHdr->ownerLRB ;
            while ( pLRB )
            {
               if ( bOutputInPlainMode )
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer) ) ;
               }
               else
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer), prefixStr ) ;
               }
               fprintf( fp, "%s"OSS_NEWLINE, pStr ) ;
               pLRB = pLRB->nextLRB ;
            }
            fprintf( fp, "%s", OSS_NEWLINE ) ;
         }
         if ( pLRBHdr->upgradeLRB )
         {
            fprintf( fp, "%sUpgrade list:"OSS_NEWLINE, prefixStr ) ;
            fprintf( fp, "%s-------------"OSS_NEWLINE, prefixStr ) ;
            pLRB = pLRBHdr->upgradeLRB ;
            while ( pLRB )
            {
               if ( bOutputInPlainMode )
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer) ) ;
               }
               else
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer), prefixStr );
               }
               fprintf( fp, "%s"OSS_NEWLINE, pStr ) ;
               pLRB = pLRB->nextLRB ;
            }
            fprintf( fp, "%s", OSS_NEWLINE ) ;
         }
         if ( pLRBHdr->waiterLRB != NULL )
         {
            fprintf( fp, "%sWaiter list:"OSS_NEWLINE, prefixStr ) ;
            fprintf( fp, "%s------------"OSS_NEWLINE, prefixStr ) ;
            pLRB = pLRBHdr->waiterLRB ;
            while ( pLRB )
            {
               if ( bOutputInPlainMode )
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer) ) ;
               }
               else
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer), prefixStr );
               }
               fprintf( fp, "%s"OSS_NEWLINE, pStr ) ;
               pLRB = pLRB->nextLRB ;
            }
            fprintf( fp, "%s", OSS_NEWLINE ) ;
         }
         fprintf( fp, "%s", OSS_NEWLINE ) ;
      }

      // free bucket latch
      _releaseOpLatch( bktIdx ) ;

      // close file
      if ( fp )
      {
         fclose( fp ) ;
      }
   error:
      return ;
   }

   //
   // dump EDU LRB info into VEC_TRANSLOCKCUR
   // It walks through the EDU LRB chain, the caller shall acquire
   // the monitoring( dump ) latch, acquireMonLatch(),
   // and make sure the executor is still available
   //
   void dpsTransLockManager::dumpLockInfo
   (
      dpsTransLRB       * lastLRB,
      VEC_TRANSLOCKCUR  & vecLocks
   )
   {
      monTransLockCur monLock ;
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;

      pLRB = lastLRB ;
      while ( pLRB )
      {
         pLRBHdr  = pLRB->lrbHdr ;

         monLock._id    = pLRBHdr->lockId ;
         monLock._mode  = pLRB->lockMode ;
         monLock._count = pLRB->refCounter ;
         monLock._beginTick = pLRB->beginTick ;

         vecLocks.push_back( monLock ) ;

         pLRB = pLRB->eduLrbPrev ;
      }
   }

   INT32 dpsTransLockManager::dumpEDUTransInfo( _dpsTransExecutor *executor,
                                                monTransLockCur &waitLock,
                                                VEC_TRANSLOCKCUR  &eduLocks )
   {
      SDB_ASSERT( NULL != executor, "Should not be null" ) ;
      INT32 rc = SDB_OK ;
      dpsTransLRB *lrb = NULL ;
      INT32 count = 0 ;

      executor->acquireLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
      lrb = executor->getWaiterLRB( LOCKMGR_TRANS_LOCK ) ;
      if ( NULL != lrb )
      {
         dumpLockInfo( lrb, waitLock ) ;
      }

      SDB_ASSERT( executor->getAccessingLRB( LOCKMGR_TRANS_LOCK ) == NULL,
                  "should be NULL" ) ;
      lrb = executor->getLastLRB( LOCKMGR_TRANS_LOCK ) ;
      while ( NULL != lrb )
      {
         monTransLockCur tmpLock ;
         dumpLockInfo( lrb, tmpLock ) ;

         try
         {
            eduLocks.push_back( tmpLock ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Occur exception when dump trans info: %s",
                    e.what() ) ;
            executor->releaseLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
            goto error ;
         }

         ++count ;
         lrb = lrb->eduLrbPrev ;

         if ( count >= DPS_TRANSLOCK_DUMP_SLICE_SIZE && NULL != lrb )
         {
            executor->setAccessingLRB( LOCKMGR_TRANS_LOCK, lrb ) ;
            // release lock to let user session work
            executor->releaseLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
            ossYield() ;
            count = 0 ;
            executor->acquireLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
            lrb = executor->getAccessingLRB( LOCKMGR_TRANS_LOCK ) ;
         }
      }

      executor->setAccessingLRB( LOCKMGR_TRANS_LOCK, NULL ) ;
      executor->releaseLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // dump the lock info ( lockId, lockMode, refCounter ) into monTransLockCur
   // via LRB
   void dpsTransLockManager::dumpLockInfo
   (
      dpsTransLRB      *lrb,
      monTransLockCur  &lockCur
   )
   {
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;

      if ( lrb )
      {
         pLRB    = lrb ;
         pLRBHdr  = pLRB->lrbHdr ;

         lockCur._id = pLRBHdr->lockId ;
         lockCur._mode = pLRB->lockMode ;
         lockCur._count = pLRB->refCounter ;
         lockCur._beginTick = pLRB->beginTick ;
      }
   }


   //
   // dump LRB Header and owner / waiter / upgrade list for a specific lock
   // into monTransLockInfo. This function doesn't need to acquire
   // monitoring/dump latch ( acquireMonLatch() ), it will get the bucket latch
   //
   void dpsTransLockManager::dumpLockInfo
   (
      const dpsTransLockId & lockId,
      monTransLockInfo     & monLockInfo
   )
   {
      UINT32             bktIdx  = DPS_LOCK_INVALID_BUCKET_SLOT ;
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;

      monTransLockInfo::lockItem monLockItem ;

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // acquire bucket latch
      _acquireOpLatch( bktIdx ) ;

      pLRBHdr = _LockHdrBkt[bktIdx].lrbHdr ;
      if ( _getLRBHdrByLockId( lockId, pLRBHdr ) )
      {
         monLockInfo._id = pLRBHdr->lockId ;

         // owner list
         if ( pLRBHdr->ownerLRB )
         {
            pLRB = pLRBHdr->ownerLRB ;
            while ( pLRB )
            {
               monLockItem._eduID = pLRB->dpsTxExectr->getEDUID() ;
               monLockItem._mode  = pLRB->lockMode ;
               monLockItem._count = pLRB->refCounter ;
               monLockItem._beginTick = pLRB->beginTick ;

               monLockInfo._vecHolder.push_back( monLockItem ) ;

               pLRB = pLRB->nextLRB ;
            }
         }
         // upgrade list
         if ( pLRBHdr->upgradeLRB )
         {
            pLRB = pLRBHdr->upgradeLRB ;
            while ( pLRB )
            {
               monLockItem._eduID = pLRB->dpsTxExectr->getEDUID() ;
               monLockItem._mode  = pLRB->lockMode ;
               monLockItem._count = pLRB->refCounter ;
               monLockItem._beginTick = pLRB->beginTick ;

               monLockInfo._vecWaiter.push_back( monLockItem ) ;

               pLRB = pLRB->nextLRB ;
            }
         }
         // waiter list
         if ( pLRBHdr->waiterLRB  )
         {
            pLRB = pLRBHdr->waiterLRB ;
            while ( pLRB )
            {
               monLockItem._eduID = pLRB->dpsTxExectr->getEDUID() ;
               monLockItem._mode  = pLRB->lockMode ;
               monLockItem._count = pLRB->refCounter ;
               monLockItem._beginTick = pLRB->beginTick ;

               monLockInfo._vecWaiter.push_back( monLockItem ) ;

               pLRB = pLRB->nextLRB ;
            }
         }
      }

      // release bucket latch
      _releaseOpLatch( bktIdx ) ;
   }


}  // namespace engine
