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

   Source File Name = clsStorageCheckJob.cpp

   Descriptive Name = Storage Checking Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "dms.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "monDMS.hpp"
#include "pmd.hpp"
#include "clsTrace.hpp"
#include "clsShardMgr.hpp"
#include "clsMgr.hpp"
#include "rtn.hpp"
#include "rtnContextDel.hpp"
#include "clsStorageCheckJob.hpp"
#include "pmdDummySession.hpp"

namespace engine
{

   #define DMS_CHECK_ONCE_INTERVAL           ( 100 )
   #define DMS_CHECK_WAIT_INTERVAL           ( OSS_ONE_SEC * 30 )

   /*
      _clsStorageEventHandler implement
   */
   _clsStorageEventHandler::_clsStorageEventHandler()
   {
   }

   _clsStorageEventHandler::~_clsStorageEventHandler()
   {
   }

   INT32 _clsStorageEventHandler::onDropCL( SDB_EVENT_OCCUR_TYPE type,
                                            IDmsEventHolder *pEventHolder,
                                            IDmsSUCacheHolder *pCacheHolder,
                                            const dmsEventCLItem &clItem,
                                            dmsDropCLOptions *options,
                                            pmdEDUCB *cb,
                                            SDB_DPSCB *dpsCB )
   {
      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         dmsEventHolder *holder = dynamic_cast<dmsEventHolder *>( pEventHolder ) ;
         dmsStorageUnit *su = NULL ;

         if ( holder )
         {
            su = holder->getSU() ;
         }

         if ( su && 0 == su->data()->getCollectionNum() )
         {
            SDB_DMSCB *pDmsCB = pmdGetKRCB()->getDMSCB() ;
            /// ignore error
            pDmsCB->pushCheckItem( dmsCheckItem( su->CSName(),
                                                 su->CSUniqueID(),
                                                 DMS_CHECK_EMPTYCS ) ) ;
         }
      }

      return SDB_OK ;
   }

   /*
    *  _clsStorageCheckJob implement
    */
   _clsStorageCheckJob::_clsStorageCheckJob ()
   {
      _lastCheckCSTick = 0 ;
      _lastCheckEmptyCSTick = 0 ;
   }

   _clsStorageCheckJob::~_clsStorageCheckJob ()
   {
   }

   INT32 _clsStorageCheckJob::doit ()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdEDUMgr *pEduMgr = krcb->getEDUMgr() ;
      pmdEDUCB *cb = eduCB() ;
      pmdEDUEvent event ;

      BOOLEAN attachedDummySession = FALSE ;
      pmdDummySession session( TRUE ) ;

      if ( NULL == cb->getSession() )
      {
         session.attachCB( cb ) ;
         attachedDummySession = TRUE ;
      }

      while ( !PMD_IS_DB_DOWN() && !cb->isForced() )
      {
         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
         */
         pEduMgr->waitEDU( cb ) ;
         cb->resetDisconnect() ;
         cb->waitEvent( event, DMS_CHECK_WAIT_INTERVAL ) ;

         // Check stop signal first
         if ( cb->isInterrupted() )
         {
            continue ;
         }

         /// set edu active
         pEduMgr->activateEDU( cb ) ;

         /// check notify item
         _checkNotifyItem() ;

         /// check empty collectionspaces
         _checkEmptyCS() ;

         /// check collectionspaces
         _checkCS() ;

         /// release mem
         cb->shrink() ;
      }

      if ( attachedDummySession )
      {
         session.detachCB() ;
         attachedDummySession = FALSE ;
      }

      return SDB_OK ;
   }

   void _clsStorageCheckJob::_checkCS()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
      pmdEDUCB *cb = eduCB() ;
      UINT64 msInterval = krcb->getOptionCB()->getDmsChkInterval() * STORAGE_CHECK_UNIT_INTERVAL ;
      MON_CSNAME_VEC csVec ;
      MON_CSNAME_VEC::iterator itCS ;
      dmsNoAccessCSFilter filter( msInterval ) ;

      if ( 0 == msInterval )
      {
         _lastCheckCSTick = pmdGetDBTick() ;
         goto done ;
      }
      else if ( pmdGetTickSpanTime( _lastCheckCSTick ) < msInterval )
      {
         goto done ;
      }

      /// reset check tick
      _lastCheckCSTick = pmdGetDBTick() ;
      cb->incEventCount( 1 ) ;

      if ( !krcb->isPrimary() || cb->isInterrupted() )
      {
         goto done ;
      }

      /// dump collectionspace, and check
      pDmsCB->dumpInfo( csVec, FALSE, &filter ) ;

      for ( itCS = csVec.begin() ; itCS != csVec.end(); ++itCS )
      {
         const monCSName &csItem = *itCS ;
         INT32 rcTmp = _checkCSItem( csItem, TRUE, msInterval ) ;

         /// check edu stop and primary down
         if ( SDB_CLS_NOT_PRIMARY == rcTmp || cb->isInterrupted() )
         {
            break ;
         }

         /// sleep a time
         ossSleep( DMS_CHECK_ONCE_INTERVAL ) ;
      }

   done:
      return ;
   }

   void _clsStorageCheckJob::_checkEmptyCS()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
      pmdEDUCB *cb = eduCB() ;
      UINT64 msInterval = krcb->getOptionCB()->getDmsChkInterval() * STORAGE_CHECK_UNIT_INTERVAL / 6 ;
      MON_CSNAME_VEC csVec ;
      MON_CSNAME_VEC::iterator itCS ;
      _dmsEmptyCSFilter filter ;

      if ( 0 == msInterval )
      {
         _lastCheckEmptyCSTick = pmdGetDBTick() ;
         goto done ;
      }
      else if ( pmdGetTickSpanTime( _lastCheckEmptyCSTick ) < msInterval )
      {
         goto done ;
      }

      /// reset check tick
      _lastCheckEmptyCSTick = pmdGetDBTick() ;
      cb->incEventCount( 1 ) ;

      if ( !krcb->isPrimary() || cb->isInterrupted() )
      {
         goto done ;
      }

      /// dump collectionspace, and check
      pDmsCB->dumpInfo( csVec, FALSE, &filter ) ;

      for ( itCS = csVec.begin() ; itCS != csVec.end(); ++itCS )
      {
         const monCSName &csItem = *itCS ;
         INT32 rcTmp = _checkEmptyCSItem( csItem, FALSE, msInterval ) ;

         /// check edu stop and primary down
         if ( SDB_CLS_NOT_PRIMARY == rcTmp || cb->isInterrupted() )
         {
            break ;
         }
      }

   done:
      return ;
   }

   void _clsStorageCheckJob::_checkNotifyItem()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
      const UINT64 msInterval = 30 * OSS_ONE_SEC ;       /// 30 secs
      pmdEDUCB *cb = eduCB() ;
      dmsCheckItem item ;

      if ( cb->isInterrupted() )
      {
         goto done ;
      }

      while( pDmsCB->dispatchCheckItem( item ) )
      {
         INT32 rcTmp = SDB_OK ;
         INT32 checkCataStatus = CLS_CHK_CATA_NONE ;

         /// push back and break
         if ( pmdGetTickSpanTime( item._dbTick ) < msInterval )
         {
            pDmsCB->pushCheckItem( item ) ;
            break ;
         }
         else if ( !krcb->isPrimary() )
         {
            continue ;
         }

         cb->incEventCount( 1 ) ;

         if ( DMS_CHECK_CS == item._type )
         {
            monCSName csName( item._name, item._csUniqueID ) ;
            rcTmp = _checkCSItem( csName, TRUE, 0, &checkCataStatus ) ;
         }
         else if ( DMS_CHECK_CL == item._type )
         {
            rcTmp = _checkCLItem( item._name, item._clUniqueID, &checkCataStatus ) ;
         }
         else if ( DMS_CHECK_EMPTYCS == item._type )
         {
            monCSName csName( item._name, item._csUniqueID ) ;
            rcTmp = _checkEmptyCSItem( csName, TRUE, 0 ) ;

            if ( SDB_DMS_CS_NOT_EMPTY == rcTmp ||
                 SDB_LOCK_FAILED == rcTmp ||
                 SDB_DPS_TRANS_LOCK_INCOMPATIBLE == rcTmp )
            {
               /// need check the collectionspace is empty again
               dmsStorageUnitID suID = DMS_INVALID_SUID ;
               dmsStorageUnit *su = NULL ;
               if ( SDB_OK == pDmsCB->idToSUAndLock( item._csUniqueID, suID, &su ) )
               {
                  if ( su && 0 == su->data()->getCollectionNum() )
                  {
                     /// ignore push failed
                     pDmsCB->pushCheckItem( dmsCheckItem( item._name,
                                                          item._csUniqueID,
                                                          DMS_CHECK_EMPTYCS ) ) ;
                  }
                  pDmsCB->suUnlock( suID ) ;
               }
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Unknow notify item(Name: %s, Type: %u)",
                    item._name, item._type ) ;
         }

         /// when check with catalog failed, need push and retry next time
         if ( CLS_CHK_CATA_FAILED == checkCataStatus )
         {
            item._dbTick = pmdGetDBTick() ;
            pDmsCB->pushCheckItem( item ) ;
         }

         /// check edu stop
         if ( cb->isInterrupted() )
         {
            break ;
         }
      }

   done:
      return ;
   }

   INT32 _clsStorageCheckJob::_checkCSItem( monCSName csName,
                                            BOOLEAN checkUniqueID,
                                            UINT64 noAccessTime,
                                            INT32 *pCheckCataStatus )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_RTNCB *pRtnCB = krcb->getRTNCB() ;
      SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
      shardCB  *pShdMgr = krcb->getClsCB()->getShardCB() ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;
      SINT64 contextID = -1 ;
      UINT64 lastAccessDBTick = 0 ;
      rtnContextBuf buffObj ;
      rtnContextDelCS::sharePtr pDelContext ;

      if ( pCheckCataStatus )
      {
         *pCheckCataStatus = CLS_CHK_CATA_NONE ;
      }

      // only check cs which has valid unique id
      if ( checkUniqueID && ! UTIL_IS_VALID_CSUNIQUEID( csName._csUniqueID ) )
      {
         goto done ;
      }

      // Lock space first
      if ( UTIL_IS_VALID_CSUNIQUEID( csName._csUniqueID ) )
      {
         rc = pDmsCB->idToSUAndLock( csName._csUniqueID, suID, &su, SHARED ) ;
      }
      else
      {
         rc = pDmsCB->nameToSUAndLock( csName._csName, suID, &su, SHARED ) ;
      }

      if ( rc )
      {
         PD_LOG( PDDEBUG, "Collection space(%s) might be dropped by another command, rc: %d",
                 csName._csName, rc ) ;
         goto error ;
      }

      /// check unique id
      if ( UTIL_UNIQUEID_NULL != csName._csUniqueID &&
           csName._csUniqueID != su->CSUniqueID() )
      {
         PD_LOG( PDDEBUG, "Collection space(%s, %u) unique ID is not the same(%u)",
                 csName._csName, su->CSUniqueID(), csName._csUniqueID ) ;
         goto done ;
      }

      lastAccessDBTick = su->getLastAccessDBTick() ;
      /// check access time
      if ( noAccessTime > 0 && pmdGetTickSpanTime( lastAccessDBTick ) < noAccessTime )
      {
         PD_LOG( PDDEBUG, "Collection space(%s) is accessed in last %llu ms, "
                 "last access tick: %llu, time span: %llu ms",
                 csName._csName, noAccessTime, lastAccessDBTick,
                 pmdGetTickSpanTime( lastAccessDBTick ) ) ;
         goto done ;
      }

      /// unlock
      pDmsCB->suUnlock( suID, SHARED ) ;
      suID = DMS_INVALID_SUID ;
      su = NULL ;

      /// check with catalog
      rc = pShdMgr->rGetCSInfo( csName._csName, csName._csUniqueID ) ;
      if ( SDB_DMS_CS_NOTEXIST != rc )
      {
         if ( pCheckCataStatus )
         {
            *pCheckCataStatus = ( SDB_OK == rc ) ? CLS_CHK_CATA_SUC : CLS_CHK_CATA_FAILED ;
         }
         rc = SDB_OK ;
         goto done ;
      }

      // Create a DelCS context to drop the collection space
      rc = pRtnCB->contextNew( RTN_CONTEXT_DELCS,
                               pDelContext,
                               contextID, eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Create DelCS context(Name: %s, UniqueID: %u) failed, rc: %d",
                 csName._csName, csName._csUniqueID, rc ) ;
         goto error ;
      }

      // Open the context, execute phase 1
      rc = pDelContext->open( csName._csName, NULL, eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Open DelCS context(Name: %s, UniqueID: %u) failed, rc: %d",
                 csName._csName, csName._csUniqueID, rc ) ;
         goto error ;
      }

      // Now, check the catalog again, if someone re-create the
      // collection space, kill the context
      rc = pShdMgr->rGetCSInfo( csName._csName, csName._csUniqueID ) ;
      if ( SDB_DMS_CS_NOTEXIST != rc )
      {
         if ( pCheckCataStatus )
         {
            *pCheckCataStatus = ( SDB_OK == rc ) ? CLS_CHK_CATA_SUC : CLS_CHK_CATA_FAILED ;
         }
         rc = SDB_OK ;
         goto done ;
      }

      if ( pCheckCataStatus )
      {
         *pCheckCataStatus = CLS_CHK_CATA_SUC ;
      }

      /// re-check primary
      if ( !krcb->isPrimary() )
      {
         /// not primary, can't do drop operator
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }

      // Continue to process the phase 2 of context
      rc = pDelContext->getMore( -1, buffObj, eduCB() ) ;
      if ( SDB_OK == rc || SDB_DMS_EOC == rc )
      {
         PD_LOG( PDEVENT, "Drop collection space(Name: %s, ID: %u) succeed, because it's no "
                 "longer exists in catalog", csName._csName, csName._csUniqueID ) ;
      }
      else
      {
         PD_LOG( PDWARNING, "Drop collection space(Name: %s, ID: %u) failed, rc: %d",
                 csName._csName, csName._csUniqueID, rc ) ;
         goto error ;
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         pDmsCB->suUnlock( suID, SHARED ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;
      }

      if ( -1 != contextID )
      {
         pRtnCB->contextDelete( contextID, eduCB() ) ;
         contextID = -1 ;
      }

      return rc ;
   error:
      goto done ;
   }

   INT32 _clsStorageCheckJob::_checkEmptyCSItem( const monCSName &csName,
                                                 BOOLEAN checkUniqueID,
                                                 UINT64 noAccessTime )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *pDpsCB = krcb->getDPSCB() ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;
      UINT64 lastAccessDBTick = 0 ;

      // only check cs which has valid unique id
      if ( checkUniqueID && ! UTIL_IS_VALID_CSUNIQUEID( csName._csUniqueID ) )
      {
         goto done ;
      }

      // Lock space first
      if ( UTIL_IS_VALID_CSUNIQUEID( csName._csUniqueID ) )
      {
         rc = pDmsCB->idToSUAndLock( csName._csUniqueID, suID, &su, SHARED ) ;
      }
      else
      {
         rc = pDmsCB->nameToSUAndLock( csName._csName, suID, &su, SHARED ) ;
      }

      if ( rc )
      {
         PD_LOG( PDDEBUG, "Collection space(%s) might be dropped by another command, rc: %d",
                 csName._csName, rc ) ;
         goto error ;
      }

      /// check unique id
      if ( csName._csUniqueID != su->CSUniqueID() )
      {
         PD_LOG( PDDEBUG, "Collection space(%s, %u) unique ID is not the same(%u)",
                 csName._csName, su->CSUniqueID(), csName._csUniqueID ) ;
         goto done ;
      }

      lastAccessDBTick = su->getLastAccessDBTick() ;
      /// check access time
      if ( noAccessTime > 0 && pmdGetTickSpanTime( lastAccessDBTick ) < noAccessTime )
      {
         PD_LOG( PDDEBUG, "Collection space(%s) is accessed in last %llu ms, "
                 "last access tick: %llu, time span: %llu ms",
                 csName._csName, noAccessTime, lastAccessDBTick,
                 pmdGetTickSpanTime( lastAccessDBTick ) ) ;
         goto done ;
      }

      /// unlock
      pDmsCB->suUnlock( suID, SHARED ) ;
      suID = DMS_INVALID_SUID ;
      su = NULL ;

      /// re-check primary
      if ( !krcb->isPrimary() )
      {
         /// not primary, can't do drop operator
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }

      rc = pDmsCB->dropEmptyCollectionSpace( csName._csName, eduCB(), pDpsCB ) ;
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Drop empty collection space(Name: %s, ID: %u) succeed",
                 csName._csName, csName._csUniqueID ) ;
      }
      else
      {
         PD_LOG( PDWARNING, "Drop empty collection space(Name: %s, ID: %u) failed, rc: %d",
                 csName._csName, csName._csUniqueID, rc ) ;
         goto error ;
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         pDmsCB->suUnlock( suID, SHARED ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsStorageCheckJob::_checkCLItem( const CHAR *clName,
                                            utilCLUniqueID clUniqueID,
                                            INT32 *pCheckCataStatus )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdOptionsCB *optionCB = krcb->getOptionCB() ;
      SDB_RTNCB *pRtnCB = krcb->getRTNCB() ;

      SINT64 contextID = -1 ;
      rtnContextBuf buffObj ;
      rtnContextDelCL::sharePtr pDelContext ;

      BOOLEAN exist = FALSE ;

      if ( pCheckCataStatus )
      {
         *pCheckCataStatus = CLS_CHK_CATA_NONE ;
      }

      // only check cl which has valid unique id
      if ( ! UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         goto done ;
      }

      /// check with catalog
      rc = _checkCLExist( clName, clUniqueID, exist ) ;
      if ( rc )
      {
         if ( pCheckCataStatus )
         {
            *pCheckCataStatus = CLS_CHK_CATA_FAILED ;
         }
         rc = SDB_OK ; /// cat not primary should reset
         goto done ;
      }
      else if ( exist )
      {
         goto done ;
      }

      // Create a DelCL context to drop the collection
      rc = pRtnCB->contextNew( RTN_CONTEXT_DELCL,
                               pDelContext,
                               contextID, eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Create DelCL context(Name: %s, UniqueID: %llu) failed, rc: %d",
                 clName, clUniqueID, rc ) ;
         goto error ;
      }

      // Open the context, execute phase 1
      rc = pDelContext->open( clName, NULL, eduCB(), optionCB->transReplSize() ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Open DelCL context(Name: %s, UniqueID: %llu) failed, rc: %d",
                 clName, clUniqueID, rc ) ;
         goto error ;
      }

      // Now, check the catalog again, if someone re-create the collection, kill the context
      rc = _checkCLExist( clName, clUniqueID, exist ) ;
      if ( rc )
      {
         if ( pCheckCataStatus )
         {
            *pCheckCataStatus = CLS_CHK_CATA_FAILED ;
         }
         rc = SDB_OK ; /// cat not primary should reset
         goto done ;
      }
      else if ( exist )
      {
         goto done ;
      }

      if ( pCheckCataStatus )
      {
         *pCheckCataStatus = CLS_CHK_CATA_SUC ;
      }

      /// re-check primary
      if ( !krcb->isPrimary() )
      {
         /// not primary, can't do drop operator
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }

      // Continue to process the phase 2 of context
      rc = pDelContext->getMore( -1, buffObj, eduCB() ) ;
      if ( SDB_OK == rc || SDB_DMS_EOC == rc )
      {
         PD_LOG( PDEVENT, "Drop collection(Name: %s, ID: %llu) succeed, because it's no "
                 "longer exists in catalog", clName, clUniqueID ) ;
      }
      else
      {
         PD_LOG( PDWARNING, "Drop collection(Name: %s, ID: %llu) failed, rc: %d",
                 clName, clUniqueID, rc ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete( contextID, eduCB() ) ;
         contextID = -1 ;
      }
      if ( exist && pCheckCataStatus )
      {
         *pCheckCataStatus = CLS_CHK_CATA_SUC ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsStorageCheckJob::_checkCLExist( const CHAR *clName,
                                             utilCLUniqueID clUniqueID,
                                             BOOLEAN &exist )
   {
      INT32 rc = SDB_OK ;
      shardCB *pShdMgr = sdbGetShardCB() ;
      clsCatalogSet *pSet = NULL ;
      UINT32 groupCount = 0 ;
      utilCLUniqueID remoteCLUniqueID = UTIL_UNIQUEID_NULL ;

      rc = pShdMgr->syncUpdateCatalog( clUniqueID, clName ) ;
      if ( SDB_DMS_NOTEXIST == rc || SDB_DMS_CS_NOTEXIST == rc )
      {
         exist = FALSE ;
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDWARNING, "Update catalog info(Name: %s, ID: %llu) failed, rc: %d",
                 clName, clUniqueID, rc ) ;
         goto error ;
      }

      rc = pShdMgr->getAndLockCataSet( clName, &pSet, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDINFO, "Get catalog info(Name: %s, ID: %llu) failed, rc: %d",
                 clName, clUniqueID, rc ) ;
         goto error ;
      }

      /// check unique id
      groupCount = pSet->groupCount() ;
      remoteCLUniqueID = pSet->clUniqueID() ;
      pShdMgr->unlockCataSet( pSet ) ;

      if ( 0 == groupCount )
      {
         /// clear local catalog info
         pShdMgr->getCataAgent()->lock_w() ;
         pShdMgr->getCataAgent()->clear( clName ) ;
         pShdMgr->getCataAgent()->release_w() ;
      }

      /// the unique id not the same
      if ( remoteCLUniqueID != clUniqueID )
      {
         rc = SDB_DMS_UNIQUEID_CONFLICT ;
         goto error ;
      }
      else if ( 0 == groupCount )
      {
         exist = FALSE ;
      }
      else
      {
         exist = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 startStorageCheckJob ( EDUID *pEDUID )
   {
      INT32 rc = SDB_OK ;
      clsStorageCheckJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsStorageCheckJob() ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   #define CLS_SYNCNOTIFY_QUE_WIATTIME             ( 300 )        // ms
   #define CLS_SYNCNOTIFY_SHRINK_TIMEINTERVAL      ( 30 * OSS_ONE_SEC )

   /*
      _clsSyncNotifyJob implement
   */
   _clsSyncNotifyJob::_clsSyncNotifyJob()
   {
   }

   _clsSyncNotifyJob::~_clsSyncNotifyJob()
   {
   }

   INT32 _clsSyncNotifyJob::doit()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      clsCB *pClsCB = krcb->getClsCB() ;
      replCB *pReplCB = pClsCB->getReplCB() ;
      clsSyncNotifyQueue *pSyncQue = pReplCB->getSyncNotifyQue() ;
      _clsSyncManager *pSyncMgr = pReplCB->syncMgr() ;
      pmdEDUMgr *pEduMgr = krcb->getEDUMgr() ;
      pmdEDUCB *cb = eduCB() ;
      CLS_NODE_ARRAY nodes ;
      UINT64 lastShrinkTick = pmdGetDBTick() ;

      while ( !PMD_IS_DB_DOWN() && !cb->isForced() )
      {
         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
         */
         pEduMgr->waitEDU( cb ) ;
         cb->resetDisconnect() ;

         if ( pSyncQue->timed_wait_and_pop( nodes, CLS_SYNCNOTIFY_QUE_WIATTIME ) )
         {
            /// set edu active
            pEduMgr->activateEDU( cb ) ;
            pSyncMgr->notifyNodes( nodes ) ;
            cb->incEventCount() ;
         }

         if ( pmdGetTickSpanTime( lastShrinkTick ) >= CLS_SYNCNOTIFY_SHRINK_TIMEINTERVAL )
         {
            /// release mem
            cb->shrink() ;
            lastShrinkTick = pmdGetDBTick() ;
         }
      }

      return SDB_OK ;
   }

   INT32 clsStartSyncNotifyJob( EDUID *pEDUID )
   {
      INT32 rc = SDB_OK ;
      clsSyncNotifyJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsSyncNotifyJob() ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

