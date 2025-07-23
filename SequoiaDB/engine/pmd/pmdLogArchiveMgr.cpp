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

   Source File Name = pmdLogArchiveMgr.cpp

   Descriptive Name = Replica Log Archive Mgr Entry

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains entry point for log global
   writer thread.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/7/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdTrace.h"
#include "pdTrace.hpp"
#include "dpsLogWrapper.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOGARCHIVINGENTPNT, "pmdLogArchiveMgrEntryPoint" )
   INT32 pmdLogArchiveMgrEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOGARCHIVINGENTPNT );

      pmdEDUMgr* eduMgr = cb->getEDUMgr() ;
      SDB_DPSCB* dpsCB = ( SDB_DPSCB* )pData ;

      rc = eduMgr->activateEDU ( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to activate EDU" ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Log archive manager start" );

      // just sit here do nothing at the moment
      while ( !cb->isDisconnected() )
      {
         rc = dpsCB->archive() ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to archive, rc = %d", rc ) ;
         }
      }

      PD_LOG( PDEVENT, "Log archive manager is stopped" );

   done :
      PD_TRACE_EXITRC ( SDB_PMDLOGARCHIVINGENTPNT, rc );
      return rc;
   error :
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_LOGARCHIVEMGR, TRUE,
                          pmdLogArchiveMgrEntryPoint,
                          "LogArchiveMgr" ) ;

}
