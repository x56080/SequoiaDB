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

   Source File Name = ixmContext.cpp

   Descriptive Name = Index Manager Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for index
   extent implmenetation. This include B tree insert/update/delete.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2019  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"

#include "dmsCB.hpp"
#include "dmsStorageIndex.hpp"
#include "pmdEDU.hpp"
#include "dpsTransLockDef.hpp"      // dpsTransLockId
#include "dmsStorageDataCommon.hpp"
#include "ixmContext.hpp"

using namespace bson ;

namespace engine
{
   _ixmContext::_ixmContext ( _pmdEDUCB * eduCB,
                              UINT32      logicalCSID )
   {
      SDB_ASSERT ( eduCB, "EDUCB can't be NULL" ) ;

      _eduCB         = eduCB ;
      _logicalCSID   = logicalCSID ;

      SDB_DMSCB  *pDMSCB   = pmdGetKRCB()->getDMSCB() ;
      _pIndexLockMgr = pDMSCB->getIndexLockMgrHandle() ;
      SDB_ASSERT( ( NULL != _pIndexLockMgr ), "Lock manager can't be NULL" ) ;
   }


   _ixmContext::~_ixmContext() 
   {
      // force release lock on all index pages include the index tree itself
      _pIndexLockMgr->releaseAll( _eduCB->getTransExecutor() ) ;
   }


   // Get the lock mode held for the index page
   BOOLEAN _ixmContext::getLockHeldInfo( _ixmLockInfo & lockInfo )
   {
      SDB_ASSERT ( lockInfo.isValid(), "Invalid index page number !" ) ;

      dmsRecordID    recordID( lockInfo.page, DMS_INVALID_OFFSET ) ;
      dpsTransLockId   lockId( _logicalCSID,
                               DPS_LOCKID_IDX_COLLECTION,
                               &recordID ) ;
      return _pIndexLockMgr->isHolding( _eduCB->getTransExecutor(),
                                        lockId,
                                        lockInfo.lockMode ) ;
   }


   // validate if an index page is locked
   BOOLEAN _ixmContext::isLocking( const dmsExtentID idxPage )
   {
      SDB_ASSERT ( ( DMS_INVALID_EXTENT != idxPage ),
                   "Invalid index page number !" ) ;

      INT8   lockMode = DPS_TRANSLOCK_MAX ;
      dmsRecordID    recordID( idxPage, DMS_INVALID_OFFSET ) ;
      dpsTransLockId   lockId( _logicalCSID,
                               DPS_LOCKID_IDX_COLLECTION,
                               &recordID ) ;
      return _pIndexLockMgr->isHolding( _eduCB->getTransExecutor(),
                                        lockId,
                                        lockMode ) ;
   }


   // try to lock an index page
   INT32 ixmTryLock( _ixmContext *pContext, dmsExtentID idxPage, INT8 mode )
   {
      SDB_ASSERT ( ( DMS_INVALID_EXTENT != idxPage ),
                   "Invalid index page number !" ) ;

      INT32 rc = SDB_OK ;
      dpsTransRetInfo lockConflict ;
      dmsRecordID    recordID( idxPage, DMS_INVALID_OFFSET ) ;
      dpsTransLockId   lockId( pContext->logicalCSID(),
                               DPS_LOCKID_IDX_COLLECTION,
                               &recordID );

      rc = pContext->getLockMgr()->tryAcquire(
              pContext->eduCB()->getTransExecutor(),
              lockId, mode,
              &lockConflict, // conflict info
              NULL ) ;       // call back

      return rc ;
   }


   // acquire lock on an index page
   INT32 ixmLock( _ixmContext *pContext, dmsExtentID idxPage, INT8 mode )
   {
      SDB_ASSERT ( ( DMS_INVALID_EXTENT != idxPage ),
                   "Invalid index page number !" ) ;

      INT32 rc = SDB_OK ;
      dpsTransRetInfo lockConflict ;
      dmsRecordID    recordID( idxPage, DMS_INVALID_OFFSET ) ;
      dpsTransLockId   lockId( pContext->logicalCSID(),
                               DPS_LOCKID_IDX_COLLECTION,
                               &recordID );
      _pmdEDUCB * eduCB = pContext->eduCB() ;

      rc = pContext->getLockMgr()->acquire(
              eduCB->getTransExecutor(),
              lockId,
              mode,
              NULL,           // context
              &lockConflict,  // conflict info
              NULL ) ;        // call back

      // update lock wait info
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


   // release lock on an index page
   void  ixmUnlock
   (
      _ixmContext * pContext,
      dmsExtentID   idxPage,
      BOOLEAN bForceRelease
   )
   {
      SDB_ASSERT ( ( DMS_INVALID_EXTENT != idxPage ),
                   "Invalid index page number !" ) ;

      INT8   lockMode ;
      dmsRecordID    recordID( idxPage, DMS_INVALID_OFFSET ) ;
      dpsTransLockId   lockId( pContext->logicalCSID(),
                               DPS_LOCKID_IDX_COLLECTION,
                               &recordID );

      if ( pContext->getLockMgr()->isHolding(
              pContext->eduCB()->getTransExecutor(),
              lockId, lockMode ) )
      {
         pContext->getLockMgr()->release( pContext->eduCB()->getTransExecutor(),
                                          lockId,
                                          bForceRelease,
                                          NULL ) ;  // call back
      }
   }


   // release all locks on all index page and on the tree
   void ixmUnlockAll( _ixmContext * pContext )
   {
      pContext->getLockMgr()->releaseAll(pContext->eduCB()->getTransExecutor());
   }


   // count and print( optional ) all index locks
   // this function is mainly for debugging
   UINT32 ixmCountAllIndexLocks
   (
      _ixmContext * pContext,
      BOOLEAN bPrintLog,
      CHAR * memoStr
   )
   {
      return pContext->getLockMgr()->countAllLocks(
                pContext->eduCB()->getTransExecutor(), bPrintLog, memoStr ) ;
   }
}
