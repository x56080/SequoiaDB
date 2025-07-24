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
      BOOLEAN reuseEDU = job->reuseEDU() ;
      BOOLEAN isSystem = job->isSystem() ;
      BOOLEAN hasExcp = FALSE ;
      string expStr ;

      PD_LOG( PDINFO, "Start a background job[%s]", job->name() ) ;

#if defined ( SDB_ENGINE )
      if ( !job->useTransLock() )
      {
         cb->getTransExecutor()->setUseTransLock( FALSE ) ;
         cb->getTransExecutor()->setTransIsolation( TRANS_ISOLATION_RU ) ;
      }
#endif // SDB_ENGINE

      cb->setName( job->name() ) ;
      job->attachIn( cb ) ;

      try
      {
         pEDUMgr->activateEDU( cb ) ;
         if ( isSystem )
         {
            pEDUMgr->lockEDU( cb ) ;
         }

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
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         hasExcp = TRUE ;
         expStr = e.what() ;
      }

      job->attachOut () ;

      if ( isSystem )
      {
         if ( PMD_IS_DB_UP() )
         {
            PD_LOG( PDSEVERE, "System job[EDUID:%lld, Type:%s, Name:%s] "
                    "exit with %d. Restart DB", cb->getID(),
                    getEDUName( cb->getType() ), job->name(), rc ) ;
            PMD_RESTART_DB( rc ) ;
         }
         pEDUMgr->unlockEDU( cb ) ;
      }

      // remove from job mgr
      jobMgr->_removeJob ( cb->getID(), rc ) ;
      // force edu
      if ( !reuseEDU )
      {
         pEDUMgr->forceUserEDU( cb->getID() ) ;
      }

      /// throw
      if ( hasExcp )
      {
         throw pdGeneralException( rc, expStr ) ;
      }

      PD_TRACE_EXITRC ( SDB_PMDBGJOBENTPNT, rc );
      return SDB_OK ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_BACKGROUND_JOB, FALSE,
                          pmdBackgroundJobEntryPoint,
                          "Task" ) ;

}

