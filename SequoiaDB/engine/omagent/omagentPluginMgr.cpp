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

   Source File Name = omagentPluginMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/05/2018  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentPluginMgr.hpp"
#include "omagentJob.hpp"

namespace engine
{
   _omAgentPluginMgr::_omAgentPluginMgr() : _options( NULL )
   {
   }

   _omAgentPluginMgr::~_omAgentPluginMgr()
   {
   }

   INT32 _omAgentPluginMgr::init( _pmdCfgRecord *pOptions )
   {
      _options = pOptions ;
      return SDB_OK ;
   }

   INT32 _omAgentPluginMgr::active()
   {
      INT32 rc = SDB_OK ;

      if ( _isGeneral() )
      {
         rc = _startPlugin() ;
      }
      return rc ;
   }

   INT32 _omAgentPluginMgr::deactive()
   {
      return _stopPlugin() ;
   }

   INT32 _omAgentPluginMgr::fini()
   {
      return SDB_OK ;
   }

   void _omAgentPluginMgr::onConfigChange()
   {
      if ( _isGeneral() )
      {
         _startPlugin() ;
      }
      else
      {
         _stopPlugin() ;
      }
   }

   INT32 _omAgentPluginMgr::_startPlugin()
   {
      INT32 rc = SDB_OK ;

      _omaTask *pTask = SDB_OSS_NEW _omaStartPluginsTask( 0 ) ;
      if ( NULL == pTask )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for running task "
                  "with the plugin starter task" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _startTask( pTask ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to start plugin task, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentPluginMgr::_stopPlugin()
   {
      INT32 rc = SDB_OK ;

      _omaTask *pTask = SDB_OSS_NEW _omaStopPluginsTask( 0 ) ;
      if ( NULL == pTask )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for running task "
                  "with the plugin stop task" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _startTask( pTask ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to stop plugin task, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentPluginMgr::_startTask( _omaTask *pTask )
   {
      INT32 rc = SDB_OK ;
      // new job
      EDUID eduID = PMD_INVALID_EDUID ;
      _omagentJob *pJob = NULL ;

      {
         omaTaskPtr myTaskPtr( pTask ) ;
         BSONObj info ;

         pJob = SDB_OSS_NEW _omagentJob( myTaskPtr, info, NULL ) ;
         if ( !pJob )
         {
            PD_LOG ( PDERROR, "Failed to alloc memory for running job "
                     "with the plugin starter job" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         // start job
         rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, &eduID,
                                        FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to start plugin starter task, rc = %d",
                     rc ) ;
            goto done ;
         }

         pTask->setJobInfo( eduID ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _omAgentPluginMgr::_isGeneral()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isGeneral = FALSE ;
      string szTmp ;

      rc = _options->getFieldStr( SDBCM_CONF_ISGENERAL, szTmp, NULL ) ;
      if( SDB_OK == rc )
      {
         if ( SDB_INVALIDARG == ossStrToBoolean( szTmp.c_str(), &isGeneral ) )
         {
            isGeneral = FALSE ;
         }
      }

      return isGeneral ;
   }
}
