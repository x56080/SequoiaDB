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
#include "pmd.hpp"

namespace engine
{
#define MON_GET_LATCH_LVL ( pmdGetKRCB()->getMonMgr() \
                       ->getCollectionLvl(MON_CLASS_LATCH) )

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
   "monClassContainer activeListLatch",
   "monClassContainer activeListHeadLatch",
   "monClassContainer archiveListLatch",
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
   "clsCatalogAgent rwMutex",
   "clsGroupItem rwMutex",
   "clsNodeMgrAgent rwMutex",
   "clsCatalogAgent rwMutex",
   "clsGroupItem rwMutex",
   "clsNodeMgrAgent rwMutex",
   "clsDCBaseInfo rwMutex",
   "clsGroupInfo rwMutex",
   "clsBucket counterLock",
   "clsReplicateSet vecLatch",
   "dmsCompressorEntry lock",
   "dmsSMEMgr mutex",
   "dpsLogPage mtx;",
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
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == latchObj->latch.try_get() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         begin.sample() ;

         if ( MON_GET_LATCH_LVL == MON_DATA_LVL_DETAIL )
         {
            monClassLatchData data;
            data.latchID = latchObj->latchID ;

            if ( cb )
            {
               data.waiterTID = cb->getTID() ;
            }
            data.ownerTID = latchObj->xOwnerTID ;
            data.latchAddr = (void *) latchObj ;
            data.latchMode = EXCLUSIVE ;
            data.numOwner = latchObj->numOwner.fetch() ;
            data.lastSOwner = latchObj->lastSOwnerTID ;

            if ( pmdGetKRCB() && pmdGetKRCB()->getMonMgr() )
            {
               monLatchCB = pmdGetKRCB()->getMonMgr()->
                            registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         latchObj->latch.get() ;

         if ( cb )
         {
            latchObj->xOwnerTID = cb->getTID() ;
         }

         end.sample() ;

         if ( monLatchCB )
         {
            monLatchCB->waitTime += end - begin ;
            pmdGetKRCB()->getMonMgr()->removeMonitorObject( monLatchCB ) ;
         }

         if (cb && cb->getMonQueryCB() )
         {
           cb->getMonQueryCB()->latchWaitTime += end - begin ;
         }
      }
      else
      {
         if ( cb )
         {
            latchObj->xOwnerTID = cb->getTID() ;
         }
      }
   }
   else
   {
     latchObj->latch.get() ;
   }
   latchObj->numOwner.inc() ;
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
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == latchObj->latch.try_get_shared() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         begin.sample() ;

         if ( MON_GET_LATCH_LVL == MON_DATA_LVL_DETAIL )
         {
            monClassLatchData data;
            data.latchID = latchObj->latchID ;
            if ( cb )
            {
               data.waiterTID = cb->getTID() ;
            }
            data.ownerTID = latchObj->xOwnerTID ;
            data.latchAddr = (void *)latchObj ;
            data.latchMode = SHARED ;
            data.numOwner = latchObj->numOwner.fetch() ;
            data.lastSOwner = latchObj->lastSOwnerTID ;

            if ( pmdGetKRCB() && pmdGetKRCB()->getMonMgr() )
            {
               monLatchCB = pmdGetKRCB()->getMonMgr()->
                            registerMonitorObject<monClassLatch>(&data) ;
            }
         }

         latchObj->latch.get_shared() ;

         if ( cb )
         {
            latchObj->lastSOwnerTID = cb->getTID() ;
         }

         end.sample() ;

         if ( monLatchCB )
         {
            monLatchCB->waitTime += end - begin ;
            pmdGetKRCB()->getMonMgr()->removeMonitorObject( monLatchCB ) ;
         }

         if (cb && cb->getMonQueryCB() )
         {
           cb->getMonQueryCB()->latchWaitTime += end - begin ;
         }
      }
      else
      {
         if ( cb )
         {
            latchObj->lastSOwnerTID = cb->getTID() ;
         }
      }
   }
   else
   {
     latchObj->latch.get_shared() ;
   }
   latchObj->numOwner.inc() ;
#else
   latchObj->latch.get() ;
#endif

}
monSpinXLatch::monSpinXLatch( MON_LATCH_IDENTIFIER latchID )
   : xOwnerTID( 0 ),
     numOwner( 0 ),
     lastSOwnerTID( 0 )
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
   latch.release() ;
   numOwner.dec() ;
   xOwnerTID = 0 ;
}

BOOLEAN monSpinXLatch::try_get()
{
   BOOLEAN ret = latch.try_get() ;
   if ( ret )
   {
      numOwner.inc() ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         xOwnerTID = cb->getTID() ;
      }
   }

   return ret ;
}

/**
 * monSpinSLatch implements
 */

monSpinSLatch::monSpinSLatch( MON_LATCH_IDENTIFIER latchID )
   : xOwnerTID( 0 ),
     lastSOwnerTID( 0 ),
     numOwner( 0 )
{
   this->latchID = latchID ;
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
   latch.release() ;
   xOwnerTID = 0 ;
   numOwner.dec() ;
}

void monSpinSLatch::get_shared ()
{
   _monGetSLatch<monSpinSLatch>( this ) ;
}

void monSpinSLatch::release_shared ()
{
   latch.release_shared() ;
   numOwner.dec() ;
}

BOOLEAN monSpinSLatch::try_get_shared()
{
   BOOLEAN ret = latch.try_get_shared() ;
   if ( ret )
   {
      numOwner.inc() ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastSOwnerTID = cb->getTID() ;
      }
   }

   return ret ;
}

BOOLEAN monSpinSLatch::try_get()
{
   BOOLEAN ret = latch.try_get() ;
   if ( ret )
   {
      numOwner.inc() ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         xOwnerTID = cb->getTID() ;
      }
   }

   return ret ;
}

}
