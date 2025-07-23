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

   Source File Name = schedDispatchJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/28/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "schedDispatchJob.hpp"
#include "schedTaskAdapterBase.hpp"
#include "pmdAsyncSession.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _schedDispatchJob implement
   */
   _schedDispatchJob::_schedDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                                         _pmdAsycSessionMgr *pSessionMgr )
   {
      _pTaskAdapter = pTaskAdapter ;
      _pSessionMgr = pSessionMgr ;
   }

   _schedDispatchJob::~_schedDispatchJob()
   {
   }

   RTN_JOB_TYPE _schedDispatchJob::type() const
   {
      return RTN_JOB_SCHED_DISPATCH ;
   }

   const CHAR* _schedDispatchJob::name() const
   {
      return "SCHED-DISPATCH" ;
   }

   BOOLEAN _schedDispatchJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _schedDispatchJob::doit()
   {
      BOOLEAN bPop = FALSE ;
      MsgHeader *pHeader = NULL ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      pmdEDUMemTypes memType = PMD_EDU_MEM_NONE ;
      UINT64 recvTimeUs = 0 ;
      BOOLEAN hasDispatched = FALSE ;

      while( !eduCB()->isForced() )
      {
         bPop = _pTaskAdapter->pop( OSS_ONE_SEC, &pHeader,
                                    handle, memType, recvTimeUs ) ;

         if ( bPop )
         {
            _pSessionMgr->dispatchMsg( handle, pHeader, memType, recvTimeUs,
                                       TRUE,&hasDispatched ) ;
            if ( !hasDispatched )
            {
               SDB_OSS_FREE( ( CHAR* )pHeader ) ;
            }

            eduCB()->incEventCount() ;
         }
      }

      return SDB_OK ;
   }

   /*
      Gloable Functions implement
   */
   INT32 schedStartDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                                _pmdAsycSessionMgr *pSessionMgr,
                                EDUID *pEduID )
   {
      INT32 rc = SDB_OK ;
      _schedDispatchJob *pJob = NULL ;

      pJob = SDB_OSS_NEW _schedDispatchJob( pTaskAdapter, pSessionMgr ) ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Allocate sched-dispatch job failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE,
                                     pEduID, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start sched-dispatch job failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

