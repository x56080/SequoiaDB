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

   Source File Name = pmdEDUEntryPoint.hpp

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
#ifndef PMD_EDU_ENTRYPOINT_HPP__
#define PMD_EDU_ENTRYPOINT_HPP__

#include "pmdEDU.hpp"

namespace engine
{

   /*
      PMD ENTRY POINTERS
   */

   /*
      @Common Entry Point For CB Manager
      @pData: must be the _pmdObjBase pointer
   */
   INT32 pmdCBMgrEntryPoint ( pmdEDUCB *cb, void *pData ) ;

   /*
      @Common Entry Point For Async Session
      @pData: must be the pmdAsyncSession pointer
   */
   INT32 pmdAsyncSessionAgentEntryPoint ( pmdEDUCB *cb, void *pData ) ;

   /*
      @Common Entry Point For Async Net
      @pData: must be the _netRouteAgent pointer
   */
   INT32 pmdAsyncNetEntryPoint ( pmdEDUCB *cb, void *pData ) ;
   INT32 pmdLocalAgentEntryPoint( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdRestAgentEntryPoint ( pmdEDUCB *cb, void *pData ) ;
   INT32 pmdTcpListenerEntryPoint ( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdHTTPListenerEntryPoint ( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdRestSvcEntryPoint ( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdLoggWEntryPoint ( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdLogArchiveMgrEntryPoint ( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdClsNtyEntryPoint( pmdEDUCB * cb, void * arg ) ;
   INT32 pmdCoordNetWorkEntryPoint ( pmdEDUCB *cb, void *pData );
   INT32 pmdPreLoaderEntryPoint ( pmdEDUCB *cb, void *pData ) ;
   INT32 pmdBackgroundJobEntryPoint ( pmdEDUCB *cb, void *pData ) ;
   INT32 pmdDpsTransRollbackEntryPoint( pmdEDUCB *cb, void *pData ) ;
   INT32 pmdPipeListenerEntryPoint ( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdLoadWorkerEntryPoint ( pmdEDUCB *cb, void *pData ) ;
   INT32 pmdSyncClockEntryPoint( pmdEDUCB *cb, void *arg ) ;
   INT32 pmdDBMonitorEntryPoint( pmdEDUCB *cb, void *arg ) ;

   INT32 pmdFapListenerEntryPoint ( pmdEDUCB *cb, void *pData ) ;
   INT32 pmdFapAgentEntryPoint( pmdEDUCB *cb, void *arg ) ;

#if defined (_LINUX)
   INT32 pmdSignalTestEntryPoint( pmdEDUCB *cb, void *arg ) ;
#endif

}

#endif // PMD_EDU_ENTRYPOINT_HPP__

