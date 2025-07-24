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

   Source File Name = pmdEDUEntryPoint.cpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/06/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdEDUEntryPoint.hpp"
#include "pmd.hpp"

#if defined (_LINUX)
#include "ossSignal.hpp"
#endif // _LINUX

namespace engine
{

   pmdEntryPoint getEntryFuncByType ( EDU_TYPES type )
   {
      pmdEntryPoint rt = NULL ;
      static const _eduEntryInfo entry[] = {
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_SHARDAGENT, FALSE,
                                pmdAsyncSessionAgentEntryPoint,
                                "ShardAgent" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_AGENT, FALSE,
                                pmdLocalAgentEntryPoint,
                                "Agent" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_REPLAGENT, FALSE,
                                pmdAsyncSessionAgentEntryPoint,
                                "ReplAgent" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_RESTAGENT, FALSE,
                                pmdRestAgentEntryPoint,
                                "RestAgent" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_FAPAGENT, FALSE,
                                pmdFapAgentEntryPoint,
                                "FAPAgent" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_TCPLISTENER, TRUE,
                                pmdTcpListenerEntryPoint,
                                "TCPListener" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_RESTLISTENER, TRUE,
                                pmdRestSvcEntryPoint,
                                "RestListener" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_CLUSTER, TRUE,
                                pmdCBMgrEntryPoint,
                                "Cluster" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_CLUSTERSHARD, TRUE,
                                pmdCBMgrEntryPoint,
                                "ClusterShard" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_CLSLOGNTY, TRUE,
                                pmdClsNtyEntryPoint,
                                "ClusterLogNotify" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_REPR, TRUE,
                                pmdAsyncNetEntryPoint,
                                "ReplReader" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_LOGGW, TRUE,
                                pmdLoggWEntryPoint,
                                "LogWriter" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_LOGARCHIVEMGR, TRUE,
                                pmdLogArchiveMgrEntryPoint,
                                "LogArchiveMgr" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_SHARDR, TRUE,
                                pmdAsyncNetEntryPoint,
                                "ShardReader" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_PIPESLISTENER, TRUE,
                                pmdPipeListenerEntryPoint,
                                "PipeListener" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_BACKGROUND_JOB, FALSE,
                                pmdBackgroundJobEntryPoint,
                                "Task" ),

         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_CATMGR, TRUE,
                                pmdCBMgrEntryPoint,
                                "CatalogMgr" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_CATNETWORK, TRUE,
                                pmdAsyncNetEntryPoint,
                                "CatalogNetwork" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_COORDNETWORK, TRUE,
                                pmdCoordNetWorkEntryPoint,
                                "CoordNetwork" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_DPSROLLBACK, TRUE,
                                pmdDpsTransRollbackEntryPoint,
                                "DpsRollback"),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_LOADWORKER, FALSE,
                                pmdLoadWorkerEntryPoint,
                                "MigLoadWork" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_PREFETCHER, FALSE,
                                pmdPreLoaderEntryPoint,
                                "PreLoader" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_OMMGR, TRUE,
                                pmdCBMgrEntryPoint,
                                "OMManager" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_OMNET, TRUE,
                                pmdAsyncNetEntryPoint,
                                "OMNet" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_SYNCCLOCK, TRUE,
                                pmdSyncClockEntryPoint,
                                "SyncClockWorker" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_FAPLISTENER, TRUE,
                                pmdFapListenerEntryPoint,
                                "FAPListener" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_DBMONITOR, TRUE,
                                pmdDBMonitorEntryPoint,
                                "DBMonitor" ),
#if defined (_LINUX)
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_SIGNALTEST, TRUE,
                                pmdSignalTestEntryPoint,
                                "SignalTest" ),
#endif // _LINUX

         // For the end
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_MAXIMUM, FALSE,
                                NULL,
                                "Unknow" )
      };

      static const UINT32 number = sizeof ( entry ) / sizeof ( _eduEntryInfo ) ;

      UINT32 index = 0 ;
      for ( ; index < number ; index ++ )
      {
         if ( entry[index].type == type )
         {
            rt = entry[index].entryFunc ;
            goto done ;
         }
      }

   done :
      return rt ;
   }

   /*
      ENTRY POINTER FUNCTIONS
   */
   INT32 pmdSyncClockEntryPoint( pmdEDUCB * cb, void * arg )
   {
      ossTick tmp ;
      pmdKRCB *pKrcb = pmdGetKRCB() ;

      pKrcb->getEDUMgr()->activateEDU( cb ) ;

      while ( !cb->isDisconnected() )
      {
         pKrcb->syncCurTime() ;
         pmdUpdateDBTick() ;
         ossSleep( PMD_SYNC_CLOCK_INTERVAL ) ;
      }
      return SDB_OK ;
   }

#if defined (_LINUX)
   INT32 pmdSignalTestEntryPoint( pmdEDUCB *cb, void *arg )
   {
      pmdEDUCB *mainCB = ( pmdEDUCB* )arg ;
      INT32 interval = pmdGetOptionCB()->getSignalInterval() ;
      UINT32 timeCounter = 0 ;

      while( !cb->isDisconnected() )
      {
         ossSleep( OSS_ONE_SEC ) ;
         ++timeCounter ;

         if ( interval > 0 && timeCounter > (UINT32)interval )
         {
            ossPThreadKill( mainCB->getThreadID(), OSS_TEST_SIGNAL ) ;
            timeCounter = 0 ;
         }
      }

      return SDB_OK ;
   }
#endif

}


