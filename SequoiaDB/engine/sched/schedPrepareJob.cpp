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

   Source File Name = schedPrepareJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/28/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "schedPrepareJob.hpp"
#include "schedTaskAdapterBase.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _schedPrepareJob implement
   */
   _schedPrepareJob::_schedPrepareJob( _schedTaskAdapterBase *pTaskAdapter )
   {
      _pTaskAdapter = pTaskAdapter ;
   }

   _schedPrepareJob::~_schedPrepareJob()
   {
   }

   RTN_JOB_TYPE _schedPrepareJob::type() const
   {
      return RTN_JOB_SCHED_PREPARE ;
   }

   const CHAR* _schedPrepareJob::name() const
   {
      return "SCHED-PREPARE" ;
   }

   BOOLEAN _schedPrepareJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _schedPrepareJob::doit()
   {
      UINT32 count = 0 ;

      while( !eduCB()->isForced() )
      {
         count = _pTaskAdapter->prepare( OSS_ONE_SEC ) ;
         eduCB()->incEventCount( count ) ;
      }

      return SDB_OK ;
   }

   /*
      Global function implement
   */
   INT32 schedStartPrepareJob( _schedTaskAdapterBase *pTaskAdapter,
                               EDUID *pEduID )
   {
      INT32 rc = SDB_OK ;
      _schedPrepareJob *pJob = NULL ;

      pJob = SDB_OSS_NEW _schedPrepareJob( pTaskAdapter ) ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Allocate sched-prepare job failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE,
                                     pEduID, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start sched-prepare job failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

