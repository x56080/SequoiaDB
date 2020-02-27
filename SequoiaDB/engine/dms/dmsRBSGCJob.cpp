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

   Source File Name = dmsRBSGCJob.cpp

   Descriptive Name = DMS Rollback Segment Garbage Collection Job

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   rollback segment creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/05/2020  CYX Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmd.hpp"
#include "dmsCB.hpp"
#include "dmsRBSGCJob.hpp"
#include "dmsRBSSUMgr.hpp"
#include "dmsTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{

   _dmsRBSGCJob::_dmsRBSGCJob( _dmsRBSSUMgr  *rbsSUMgr )
   {
      _rbsSUMgr = rbsSUMgr ;
      _rbsSUMgr->incActiveGC() ;
   }

   _dmsRBSGCJob::~_dmsRBSGCJob()
   {
#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Finishing dmsRBSGCJob " ) ;
#endif
      _rbsSUMgr->decActiveGC() ;
   }

   const CHAR* _dmsRBSGCJob::name() const
   {
      return "RBS GC" ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DMSRBSGCJOB_DOIT, "_dmsRBSGCJob::doit" )
   INT32 _dmsRBSGCJob::doit( IExecutor *pExe,
                             UTIL_LJOB_DO_RESULT &result,
                             UINT64 &sleepTime )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSGCJOB_DOIT );
      // check for interrupt
      if ( PMD_IS_DB_DOWN() || pExe->isInterrupted() || pExe->isForced() )
      {
         result = UTIL_LJOB_DO_FINISH ;
      }
      else
      {
         _rbsSUMgr->gcRBS() ;

         // finish the job if there are already max number running
         // We do simple lockless comparison as the race condition is unlikely
         // and this is not critical.
         if ( _rbsSUMgr->getNumActiveGC() >= MAX_RBS_GC_TASK )
         {
            result = UTIL_LJOB_DO_FINISH ;
         }
         else
         {
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = 60000000 ;  // 60 second
         }
      }
      PD_TRACE_EXIT ( SDB__DMSRBSGCJOB_DOIT );
      return SDB_OK ;
   }


   // submit async job to do garbage collection on RBS including data and idx
   INT32  dmsStartAsyncRBSGC( )
   {
      INT32         rc   = SDB_OK ;
      dmsRBSGCJob * pJob = NULL ; 

      if ( pmdGetOptionCB()->mvccOn() && sdbGetTransCB()->isTransOn() )
      {
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Creating dmsRBSGCJob " ) ;
#endif
         pJob = SDB_OSS_NEW dmsRBSGCJob( pmdGetKRCB()->
                                         getDMSCB()->getRBSSUMgr() ) ;

         PD_CHECK( (pJob != NULL), SDB_OOM, error, PDERROR, "Alloc dmsRBSGCJob failed" ) ;

         rc = pJob->submit( TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Star dmsRBSGCJob failed,rc:%d", rc ) ;
               
      }
   done:
      return rc ;
   error:
      goto done ;

   }

}
