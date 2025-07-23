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

   Source File Name = catGTSMsgJob.cpp

   Descriptive Name = GTS message job

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catGTSMsgJob.hpp"
#include "catGTSMsgHandler.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdEnv.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

namespace engine
{
   _catGTSMsgJob::_catGTSMsgJob( _catGTSMsgHandler* msgHandler,
                                 BOOLEAN isController,
                                 INT32 timeout )
   {
      _msgHandler = msgHandler ;
      _isController = isController ;
      _timeout = timeout ;
   }

   _catGTSMsgJob::~_catGTSMsgJob()
   {
   }

   RTN_JOB_TYPE _catGTSMsgJob::type() const
   {
      return RTN_JOB_GTS_DISPATH ;
   }

   const CHAR* _catGTSMsgJob::name() const
   {
      if ( isController() )
      {
         return "GTS-DISPATH-JOB-D" ;
      }
      else
      {
         return "GTS-DISPATH-JOB" ;
      }
   }

   BOOLEAN _catGTSMsgJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_JOB_DOIT, "_catGTSMsgJob::doit" )
   INT32 _catGTSMsgJob::doit()
   {
      INT32 rc = SDB_OK ;
      INT32 timeout = 0 ;
      INT32 POP_WAIT_TIME = OSS_ONE_SEC ;
      UINT64 checkDBTick = pmdGetDBTick() ;
      PD_TRACE_ENTRY( SDB_GTS_MSG_JOB_DOIT ) ;
      pmdEDUMgr* eduMgr = eduCB()->getEDUMgr() ;

      eduMgr->activateEDU( eduCB() ) ;

      while( !eduCB()->isForced() )
      {
         _catGTSMsg* gtsMsg = NULL ;
         BOOLEAN haveMsg = FALSE ;

         if ( isController() )
         {
            // limit check load interval by checkTime
            UINT64 currentDBTick = pmdGetDBTick() ;
            if ( pmdDBTickSpan2Time( currentDBTick - checkDBTick ) >= OSS_ONE_SEC )
            {
               _msgHandler->checkLoad() ;
               checkDBTick = currentDBTick ;
            }
         }

         eduMgr->waitEDU( eduCB() ) ;

         // wait msg
         haveMsg = _msgHandler->popMsg( POP_WAIT_TIME, gtsMsg ) ;
         if ( haveMsg )
         {
            SDB_ASSERT( gtsMsg != NULL, "should be not null" ) ;
            timeout = 0 ;
            eduMgr->activateEDU( eduCB() ) ;
            _msgHandler->processMsg( gtsMsg ) ;
            SDB_THREAD_FREE( gtsMsg ) ;
            eduCB()->incEventCount( 1 ) ;
         }
         else if ( !isController() )
         {
            timeout += POP_WAIT_TIME ;

            if ( timeout >= _timeout )
            {
               // do nothing for a long time, quit the job
               break ;
            }
         }
      }

      _msgHandler->jobExit( isController() ) ;

      PD_TRACE_EXITRC( SDB_GTS_MSG_JOB_DOIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_START_MSG_JOB, "catStartGTSMsgJob" )
   INT32 catStartGTSMsgJob( _catGTSMsgHandler* msgHandler,
                            BOOLEAN isController,
                            INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_START_MSG_JOB ) ;
      _catGTSMsgJob* job = NULL ;

      job = SDB_OSS_NEW _catGTSMsgJob( msgHandler, isController, timeout ) ;
      if ( NULL == job )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc GTS msg job" ) ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_NONE, NULL ) ;
      // neither failed or succeed, the job will release in job controller

   done:
      PD_TRACE_EXITRC( SDB_GTS_START_MSG_JOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

