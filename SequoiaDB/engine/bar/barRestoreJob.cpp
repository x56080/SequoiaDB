/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = barRestoreJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "barRestoreJob.hpp"

namespace engine
{
   
   /*
      _barRestoreJob implement
   */
   _barRestoreJob::_barRestoreJob( barRSBaseLogger * pRSLogger )
   {
      _rsLogger = pRSLogger ;
   }

   _barRestoreJob::~_barRestoreJob ()
   {
      _rsLogger = NULL ;
   }

   RTN_JOB_TYPE _barRestoreJob::type() const
   {
      return RTN_JOB_RESTORE ;
   }

   const CHAR *_barRestoreJob::name() const
   {
      return "Restore" ;
   }

   BOOLEAN _barRestoreJob::muteXOn( const _rtnBaseJob * pOther )
   {
      return FALSE ;
   }

   INT32 _barRestoreJob::doit ()
   {
      return _rsLogger->restore( eduCB() ) ;
   }

   INT32 startRestoreJob( EDUID * pEDUID, barRSBaseLogger * pRSLogger )
   {
      INT32 rc                = SDB_OK ;
      barRestoreJob * pJob    = NULL ;

      pJob = SDB_OSS_NEW barRestoreJob ( pRSLogger ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}


