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

   Source File Name = monClass.cpp

   Descriptive Name = Monitor Class source

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/14/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "monLatch.hpp"

#if defined (SDB_ENGINE)
#include "pmd.hpp"
#endif // SDB_ENGINE

namespace engine
{
#define MON_GET_LATCH_LVL ( g_monMgrPtr->getCollectionLvl(MON_CLASS_LATCH) )

const CHAR* monLatchName[] =
{
   "",
   "SDB_DMSCB stateMtx",
   "dmsStorageBase persistLatch",
   "dmsStorageBase commitLatch",
   "dmsStorageDataCommon latchContext",
   "dpsTransCB MapMutex",
   "dpsTransCB CBMapMutex",
   "dpsTransCB lsnMapMutex",
   "dpsTransCB hisMutex",
   "SDB_RTNCB mutex",
   "dmsStorageDataCommon mblock",
   "catGTSMsgHandler jobLatch",
   "catMainController contextLatch",
   "catSequence latch",
   "clsShardingKeySite mutex",
   "clsDCMgr peerCatLatch",
   "clsDataSrcBaseSession LSNlatch",
   "clsBucket bucketLatch",
   "clsFreezingWindow latch",
   "clsShardMgr catLatch",
   "CoordCB contextLatch",
   "dmsSegmentSpace mutex",
   "dmsPageMappingDispatcher latch",
   "dmsStorageLob delayOpenLatch",
   "dpsArchiveInfoMgr mutex",
   "dpsArchiveMgr mutex",
   "dpsReplicaLogMgr mtx",
   "dpsReplicaLogMgr writeMutex",
   "dpsTransCB maxFileSizeMutex",
   "dpsTransLRBHash maxFileSizeMutex",
   "netEventHandler mtx",
   "omManager contextLatch",
   "omStrategyMgr contextLatch",
   "omAgentMgr scopeLatch",
   "optCachedPlanActivity latch",
   "optAccessPlanManager reinitLatch",
   "pmdSessionMeta Latch",
   "pmdAsycSessionMgr metaLatch",
   "pmdAsycSessionMgr deqDeletingMutex",
   "pmdAsycSessionMgr forceLatch",
   "iPmdDMNChildProc mutex",
   "pmdLightJobMgr unitLatch",
   "pmdBuffPool unitLatch",
   "pmdCfgRecord mutex",
   "restSessionInfo inLatch",
   "pmdSyncMgr unitLatch",
   "rtnExtDataHandler latch",
   "rtnLobAccessInfo lock",
   "schedTaskContanierMgr latch",
   "schedTaskMgr latch",
   "spdFMPMgr mtx",
   "utilCacheUnit pageCleaner",
   "utilMemBlockPool latch",
   "utilSegmentPool latch",
   "catDCLogMgr latch",
   "clsMgr clsLatch",
   "clsShardMgr shardLatch",
   "clsTaskMgr taskLatch",
   "clsTaskMgr regLatch",
   "coordOmStrategyAgent latch",
   "coordResource nodeMutex",
   "coordResource cataMutex",
   "SDB_DMSCB mutex",
   "dmsStorageBase segmentLatch",
   "dmsStorageDataCommon metadataLatch",
   "dmsSysSUMgr mutex",
   "oldVersionCB oldVersionCBLatch",
   "netEHSegment mtx",
   "netFrame suiteMtx",
   "netFrame mtx",
   "netRoute mtx",
   "omManager omLatch",
   "omHostVersion lock",
   "omaAsyncTask planLatch",
   "omAgentOptions latch",
   "omAgentMgr immediatelyTimerLatch",
   "omAgentMgr mgrLatch",
   "omAgentNodeMgr mapLatch",
   "omAgentNodeMgr guardLatch",
   "omaAddHostTask taskLatch",
   "omaRemoveHostTask taskLatch",
   "omaInstDBBusTask taskLatch",
   "omaRemoveDBBusTask taskLatch",
   "omaZNBusTaskBase taskLatch",
   "omaSsqlOlapBusBase taskLatch",
   "omaSsqlExecTask taskLatch",
   "omaTask latch",
   "omaTaskMgr taskLatch",
   "ossASIO mutex",
   "OSS_FILE mutex",
   "SDB_KRCB handlerLatch",
   "pmdController ctrlLatch",
   "pmdEDUCB mutex",
   "pmdEDUMgr latch",
   "pmdRemoteSessionMgr edusLatch",
   "rtnJobMgr latch",
   "rtnJobMgr latchRemove",
   "rtnExtDataProcessorMgr mutex",
   "clsCatalogAgent rwMutex",
   "clsGroupItem rwMutex",
   "clsNodeMgrAgent rwMutex",
   "clsDCBaseInfo rwMutex",
   "clsGroupInfo rwMutex",
   "clsBucket counterLock",
   "clsReplicateSet vecLatch",
   "dmsCompressorEntry lock",
   "dmsSMEMgr mutex",
   "dpsLogPage mtx",
   "dpsTransLockManager rwMutex",
   "netEventSuit rwMutex",
   "netFrame suiteExitMutex",
   "optCachedPlanMonitor clearLock",
   "ossMmapSegment rwMutex",
   "pmdEDUMgr eduExitMutex",
   "rtnContextBase dataLock",
   "rtnContextBase prefetchLock",
   "rtnRemoteMessenger lock",
   "utilCacheBucket rwMutex",
   "utilHashTable bucketNumLock",
} ;

const CHAR* monLatchIDtoName ( MON_LATCH_IDENTIFIER latchID)
{
   if ( (latchID < 1) || (latchID >= MON_LATCH_ID_MAX) )
   {
      return monLatchName[0] ;
   }
   else
   {
      return monLatchName[latchID] ;
   }
}

template <class T>
void _monGetLatch(T* latchObj);

template <class T>
void _monGetLatch(T* latchObj)
{
#if defined (SDB_ENGINE)

   pmdEDUCB *cb = pmdGetThreadEDUCB() ;
   monClassQuery *monQuery = NULL ;

   if ( cb && cb->getMonQueryCB() )
   {
      monQuery = cb->getMonQueryCB() ;
   }

   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      if ( FALSE == latchObj->latch.try_get() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         ossTickDelta delta ;
         monClassLatchData data( latchObj->latchID, (void *) latchObj ) ;
         BOOLEAN delayRegObj = FALSE ;

         if ( monQuery || MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            begin.sample() ;
            if ( monQuery )
            {
               monQuery->startQueryTick( MON_TICK_LATCH, TRUE, &begin ) ;
            }
         }

         if ( MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            data.init( cb, EXCLUSIVE, &begin ) ;
            data.setOwner( latchObj->lastOwnerTID,
                           latchObj->lastOwnerType,
                           latchObj->getNumOwner(),
                           latchObj->lastOwnerMode ) ;

            if ( monGetOptiLevel() > 0 )
            {
               delayRegObj = TRUE ;
            }
            else
            {
               delayRegObj = FALSE ;
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         latchObj->latch.get() ;

         if ( cb )
         {
            latchObj->lastOwnerTID = cb->getTID() ;
            latchObj->lastOwnerType = cb->getType() ;
            latchObj->lastOwnerMode = EXCLUSIVE ;
         }

         if ( ( BOOLEAN )begin )
         {
            end.sample() ;
            delta = end - begin ;

            if ( delayRegObj && !monLatchCB &&
                 delta.toUINT64() / 1000 >= monGetSlowLatchThreshold() )
            {
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         if ( monLatchCB )
         {
            monLatchCB->waitTime += delta ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if ( monQuery )
         {
            monQuery->stopQueryTick( &end ) ;

            if ( MON_LATCH_MBLOCK == latchObj->latchID )
            {
               monQuery->startQueryTick( MON_TICK_FILE, TRUE, &end ) ;
            }
         }
      }
      else
      {
         if ( cb )
         {
            latchObj->lastOwnerTID = cb->getTID() ;
            latchObj->lastOwnerType = cb->getType() ;
            latchObj->lastOwnerMode = EXCLUSIVE ;
         }
         if ( monQuery && MON_LATCH_MBLOCK == latchObj->latchID )
         {
            monQuery->startQueryTick( MON_TICK_FILE ) ;
         }
      }
   }
   else
   {
     latchObj->latch.get() ;

     if ( cb )
     {
        latchObj->lastOwnerTID = cb->getTID() ;
        latchObj->lastOwnerType = cb->getType() ;
        latchObj->lastOwnerMode = EXCLUSIVE ;
     }
     if ( monQuery && MON_LATCH_MBLOCK == latchObj->latchID )
     {
        monQuery->startQueryTick( MON_TICK_FILE ) ;
     }
   }
   latchObj->numXOwner = 1 ;
#else
   latchObj->latch.get() ;
#endif
}

template <class T>
void _monGetSLatch(T* latchObj);

template <class T>
void _monGetSLatch(T* latchObj)
{
#if defined (SDB_ENGINE)

   pmdEDUCB *cb = pmdGetThreadEDUCB() ;
   monClassQuery *monQuery = NULL ;
   
   if ( cb && cb->getMonQueryCB() )
   {
      monQuery = cb->getMonQueryCB() ;
   }

   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      if ( FALSE == latchObj->latch.try_get_shared() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         ossTickDelta delta ;
         monClassLatchData data( latchObj->latchID, (void *) latchObj ) ;
         BOOLEAN delayRegObj = FALSE ;

         if ( monQuery || MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            begin.sample() ;
            if ( monQuery )
            {
               monQuery->startQueryTick( MON_TICK_LATCH, TRUE, &begin ) ;
            }
         }

         if ( MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            data.init( cb, SHARED, &begin ) ;
            data.setOwner( latchObj->lastOwnerTID,
                           latchObj->lastOwnerType,
                           latchObj->getNumOwner(),
                           latchObj->lastOwnerMode ) ;

            if ( monGetOptiLevel() > 0 )
            {
               delayRegObj = TRUE ;
            }
            else
            {
               delayRegObj = FALSE ;
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         latchObj->latch.get_shared() ;

         if ( cb )
         {
            latchObj->lastOwnerTID = cb->getTID() ;
            latchObj->lastOwnerType = cb->getType() ;
            latchObj->lastOwnerMode = SHARED ;
         }

         if ( ( BOOLEAN )begin )
         {
            end.sample() ;
            delta = end - begin ;

            if ( delayRegObj && !monLatchCB &&
                 delta.toUINT64() / 1000 >= monGetSlowLatchThreshold() )
            {
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         if ( monLatchCB )
         {
            monLatchCB->waitTime += delta ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if ( monQuery )
         {
            monQuery->stopQueryTick( &end ) ;

            if ( MON_LATCH_MBLOCK == latchObj->latchID )
            {
               monQuery->startQueryTick( MON_TICK_FILE, TRUE, &end ) ;
            }
         }
      }
      else
      {
         if ( cb )
         {
            latchObj->lastOwnerTID = cb->getTID() ;
            latchObj->lastOwnerType = cb->getType() ;
            latchObj->lastOwnerMode = SHARED ;
         }
         if ( monQuery && MON_LATCH_MBLOCK == latchObj->latchID )
         {
            monQuery->startQueryTick( MON_TICK_FILE ) ;
         }
      }
   }
   else
   {
     latchObj->latch.get_shared() ;

      if ( cb )
      {
         latchObj->lastOwnerTID = cb->getTID() ;
         latchObj->lastOwnerType = cb->getType() ;
         latchObj->lastOwnerMode = SHARED ;
      }
      if ( monQuery && MON_LATCH_MBLOCK == latchObj->latchID )
      {
         monQuery->startQueryTick( MON_TICK_FILE ) ;
      }
   }
   latchObj->numSOwner.inc() ;
#else
   latchObj->latch.get_shared() ;
#endif

}

monSpinXLatch::monSpinXLatch( MON_LATCH_IDENTIFIER latchID )
   : lastOwnerTID( 0 ),
     lastOwnerType( -1 ),
     lastOwnerMode( -1 ),
     numXOwner( 0 )
{
   this->latchID = latchID ;
}

monSpinXLatch::~monSpinXLatch()
{
}

void monSpinXLatch::get()
{
   _monGetLatch<monSpinXLatch>(this) ;
}

void monSpinXLatch::release()
{
#if defined (SDB_ENGINE)
   if ( MON_LATCH_MBLOCK == latchID )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb && cb->getMonQueryCB() )
      {
         cb->getMonQueryCB()->stopQueryTick() ;
      }
   }
#endif

   latch.release() ;
   numXOwner = 0 ;
   lastOwnerTID = 0 ;
   lastOwnerType = -1 ;
   lastOwnerMode = -1 ;
}

BOOLEAN monSpinXLatch::try_get()
{
   BOOLEAN ret = latch.try_get() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numXOwner = 1 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastOwnerTID = cb->getTID() ;
         lastOwnerType = cb->getType() ;
         lastOwnerMode = EXCLUSIVE ;

         if ( cb->getMonQueryCB() && MON_LATCH_MBLOCK == latchID )
         {
            cb->getMonQueryCB()->startQueryTick( MON_TICK_FILE ) ;
         }
      }
   }
#endif
   return ret ;
}

/**
 * monSpinSLatch implements
 */

monSpinSLatch::monSpinSLatch( MON_LATCH_IDENTIFIER latchID )
   : lastOwnerTID( 0 ),
     lastOwnerType( -1 ),
     lastOwnerMode( -1 ),
     numSOwner( 0 ),
     numXOwner( 0 )
{
   this->latchID = latchID ;
}

monSpinSLatch::monSpinSLatch()
   : lastOwnerTID( 0 ),
     lastOwnerType( -1 ),
     lastOwnerMode( -1 ),
     numSOwner( 0 ),
     numXOwner( 0 )
{
   latchID = MON_LATCH_ID_MAX ;
}

monSpinSLatch::~monSpinSLatch()
{
}

void monSpinSLatch::get()
{
   _monGetLatch<monSpinSLatch>( this ) ;
}

void monSpinSLatch::release()
{
#if defined (SDB_ENGINE)
   if ( MON_LATCH_MBLOCK == latchID  )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb && cb->getMonQueryCB() )
      {
         cb->getMonQueryCB()->stopQueryTick() ;
      }
   }
#endif

   latch.release() ;
   numXOwner = 0 ;
   lastOwnerTID = 0 ;
   lastOwnerType = -1 ;
   lastOwnerMode = -1 ;
}

void monSpinSLatch::get_shared ()
{
   _monGetSLatch<monSpinSLatch>( this ) ;
}

void monSpinSLatch::release_shared ()
{
#if defined (SDB_ENGINE)
   if ( MON_LATCH_MBLOCK == latchID )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb && cb->getMonQueryCB() )
      {
         cb->getMonQueryCB()->stopQueryTick() ;
      }
   }
#endif

   latch.release_shared() ;
   if ( 1 == numSOwner.dec() )
   {
      lastOwnerTID = 0 ;
      lastOwnerType = -1 ;
      lastOwnerMode = -1 ;
   }
}

BOOLEAN monSpinSLatch::try_get_shared()
{
   BOOLEAN ret = latch.try_get_shared() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numSOwner.inc() ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastOwnerTID = cb->getTID() ;
         lastOwnerType = cb->getType() ;
         lastOwnerMode = SHARED ;

         if ( cb->getMonQueryCB() && MON_LATCH_MBLOCK == latchID )
         {
            cb->getMonQueryCB()->startQueryTick( MON_TICK_FILE ) ;
         }
      }
   }
#endif
   return ret ;
}

BOOLEAN monSpinSLatch::try_get()
{
   BOOLEAN ret = latch.try_get() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numXOwner = 1 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastOwnerTID = cb->getTID() ;
         lastOwnerType = cb->getType() ;
         lastOwnerMode = EXCLUSIVE ;

         if ( cb->getMonQueryCB() && MON_LATCH_MBLOCK == latchID )
         {
            cb->getMonQueryCB()->startQueryTick( MON_TICK_FILE ) ;
         }
      }
   }
#endif
   return ret ;
}

INT32 monSpinSLatch::getNumOwner()
{
   return numSOwner.fetch() + numXOwner ;
}

/**
 * monRWMutex implements
 */

monRWMutex::monRWMutex( MON_LATCH_IDENTIFIER latchID, UINT32 type )
: mutex( type ),
  lastOwnerTID( 0 ),
  lastOwnerType( -1 ),
  lastOwnerMode( -1 ),
  numSOwner( 0 ),
  numXOwner( 0 )
{
   this->latchID = latchID ;
}

monRWMutex::~monRWMutex()
{
}

INT32 monRWMutex::lock_r( INT32 millisec )
{
   INT32 rc = SDB_OK ;
#if defined (SDB_ENGINE)
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == this->mutex.try_lock_r() )
      {
         monClassLatch *monLatchCB = NULL ;
         ossTick begin, end ;
         monClassQuery *monQuery = NULL ;

         ossTickDelta delta ;
         monClassLatchData data( this->latchID, (void *)this ) ;
         BOOLEAN delayRegObj = FALSE ;

         if ( cb && cb->getMonQueryCB() )
         {
            monQuery = cb->getMonQueryCB() ;
         }

         if ( monQuery || MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            begin.sample() ;
            if ( monQuery )
            {
               monQuery->startQueryTick( MON_TICK_LATCH, TRUE, &begin ) ;
            }
         }

         if ( MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            data.init( cb, SHARED, &begin ) ;
            data.setOwner( lastOwnerTID, lastOwnerType, getNumOwner(), lastOwnerMode ) ;

            if ( monGetOptiLevel() > 0 )
            {
               delayRegObj = TRUE ;
            }
            else
            {
               delayRegObj = FALSE ;
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         rc = this->mutex.lock_r(millisec) ;

         if ( cb && SDB_OK == rc )
         {
            lastOwnerTID = cb->getTID() ;
            lastOwnerType = cb->getType() ;
            lastOwnerMode = SHARED ;
         }

         if ( ( BOOLEAN )begin )
         {
            end.sample() ;
            delta = end - begin ;

            if ( delayRegObj && !monLatchCB &&
                 delta.toUINT64() / 1000 >= monGetSlowLatchThreshold() )
            {
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         if ( monLatchCB )
         {
            monLatchCB->waitTime += delta ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if ( monQuery )
         {
            monQuery->stopQueryTick( &end ) ;
         }
      }
      else
      {
         if ( cb )
         {
            lastOwnerTID = cb->getTID() ;
            lastOwnerType = cb->getType() ;
            lastOwnerMode = SHARED ;
         }
      }
   }
   else
   {
     rc = this -> mutex.lock_r(millisec) ;
   }
   if ( SDB_OK == rc ) this->numSOwner.inc() ;
#else
   rc = this->mutex.lock_r(millisec) ;
#endif
   return rc;
}

INT32 monRWMutex::release_r()
{
   INT32 rc = mutex.release_r() ;

   if ( SDB_OK == rc )
   {
      if ( 1 == numSOwner.dec() )
      {
         lastOwnerTID = 0 ;
         lastOwnerType = -1 ;
         lastOwnerMode = -1 ;
      }
   }
   return rc ;
}

INT32 monRWMutex::lock_w( INT32 millisec )
{
   INT32 rc = SDB_OK ;
#if defined (SDB_ENGINE)
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == this->mutex.try_lock_w() )
      {
         monClassLatch *monLatchCB = NULL ;
         ossTick begin, end ;
         monClassQuery *monQuery = NULL ;

         ossTickDelta delta ;
         monClassLatchData data( this->latchID, (void *)this ) ;
         BOOLEAN delayRegObj = FALSE ;

         if ( cb && cb->getMonQueryCB() )
         {
            monQuery = cb->getMonQueryCB() ;
         }

         if ( monQuery || MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            begin.sample() ;
            if ( monQuery )
            {
               monQuery->startQueryTick( MON_TICK_LATCH, TRUE, &begin ) ;
            }
         }

         if ( MON_GET_LATCH_LVL >= MON_DATA_LVL_DETAIL )
         {
            data.init( cb, EXCLUSIVE, &begin ) ;
            data.setOwner( lastOwnerTID, lastOwnerType, getNumOwner(), lastOwnerMode ) ;

            if ( monGetOptiLevel() > 0 )
            {
               delayRegObj = TRUE ;
            }
            else
            {
               delayRegObj = FALSE ;
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         rc = this->mutex.lock_w(millisec) ;
         if ( cb  && SDB_OK == rc )
         {
            lastOwnerTID = cb->getTID() ;
            lastOwnerType = cb->getType() ;
            lastOwnerMode = EXCLUSIVE ;
         }

         if ( ( BOOLEAN )begin )
         {
            end.sample() ;
            delta = end - begin ;

            if ( delayRegObj && !monLatchCB &&
                 delta.toUINT64() / 1000 >= monGetSlowLatchThreshold() )
            {
               monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         if ( monLatchCB )
         {
            monLatchCB->waitTime += delta ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if ( monQuery )
         {
            monQuery->stopQueryTick( &end ) ;
         }
      }
      else
      {
         if ( cb )
         {
            lastOwnerTID = cb->getTID() ;
            lastOwnerType = cb->getType() ;
            lastOwnerMode = EXCLUSIVE ;
         }
      }
   }
   else
   {
      rc = this->mutex.lock_w(millisec) ;
   }
   if ( SDB_OK == rc ) this->numXOwner = 1 ;
#else
   rc = this->mutex.lock_w(millisec) ;
#endif
   return rc;
}

INT32 monRWMutex::release_w()
{
   INT32 rc = mutex.release_w() ;

   if ( SDB_OK == rc )
   {
      lastOwnerTID = 0 ;
      lastOwnerType = -1 ;
      lastOwnerMode = -1 ;
      numXOwner = 0 ;
   }

   return rc ;
}

BOOLEAN monRWMutex::try_lock_r()
{
   BOOLEAN ret = mutex.try_lock_r() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numSOwner.inc() ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastOwnerTID = cb->getTID() ;
         lastOwnerType = cb->getType() ;
         lastOwnerMode = SHARED ;
      }
   }
#endif
   return ret ;
}

BOOLEAN monRWMutex::try_lock_w()
{
   BOOLEAN ret = mutex.try_lock_w() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numXOwner = 1 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastOwnerTID = cb->getTID() ;
         lastOwnerType = cb->getType() ;
         lastOwnerMode = EXCLUSIVE ;
      }
   }
#endif
   return ret ;
}

INT32 monRWMutex::getNumOwner()
{
   return numSOwner.fetch() + numXOwner ;
}
}
