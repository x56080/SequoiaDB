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
#include "pmd.hpp"

#if defined (_LINUX)
#include "ossSignal.hpp"
#endif // _LINUX

namespace engine
{

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

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SYNCCLOCK, TRUE,
                          pmdSyncClockEntryPoint,
                          "SyncClockWorker" ) ;

#if defined (_LINUX)
   INT32 pmdSignalTestEntryPoint( pmdEDUCB *cb, void *arg )
   {
      pmdEDUCB *mainCB = ( pmdEDUCB* )arg ;
      UINT32 interval = pmdGetOptionCB()->getSignalInterval() ;
      UINT32 timeCounter = 0 ;

      while( !cb->isDisconnected() )
      {
         ossSleep( OSS_ONE_SEC ) ;
         ++timeCounter ;
         interval = pmdGetOptionCB()->getSignalInterval() ;

         if ( interval > 0 && timeCounter > interval )
         {
            ossPThreadKill( mainCB->getThreadID(), OSS_TEST_SIGNAL ) ;
            timeCounter = 0 ;
         }
      }

      return SDB_OK ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SIGNALTEST, TRUE,
                          pmdSignalTestEntryPoint,
                          "SignalTest" ) ;

#endif //_LINUX

}


