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

   Source File Name = pmdReplay.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/11/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "rtnBackgroundJobBase.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDBGJOBENTPNT, "pmdBackgroundJobEntryPoint" )
   INT32 pmdBackgroundJobEntryPoint( pmdEDUCB *cb, void *pData )
   {
      SDB_ASSERT( NULL != pData, "impossible" ) ;
      PD_TRACE_ENTRY ( SDB_PMDBGJOBENTPNT );
      rtnJobMgr *jobMgr = rtnGetJobMgr () ;
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      rtnBaseJob *job = (rtnBaseJob*)pData ;
      INT32 rc = SDB_OK ;

      PD_LOG( PDINFO, "Start a background job[%s]", job->name() ) ;

      cb->setName( job->name() ) ;
      job->attachIn( cb ) ;

      pEDUMgr->activateEDU( cb->getID() ) ;
      rc = job->doit () ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDWARNING, "Background job[%s] do failed, rc = %d",
                  job->name(), rc ) ;
      }
      else
      {
         PD_LOG ( PDINFO, "Background job[%s] finished", job->name() ) ;
      }

      job->attachOut () ;

      // remove from job mgr
      jobMgr->_removeJob ( cb->getID(), rc ) ;

      PD_TRACE_EXITRC ( SDB_PMDBGJOBENTPNT, rc );
      return SDB_OK ;
   }
}

