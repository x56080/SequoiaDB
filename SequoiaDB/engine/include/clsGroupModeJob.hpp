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

   Source File Name = clsGroupModeJob.hpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/21/2023  LCX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLSGROUPMODE_HPP_
#define CLSGROUPMODE_HPP_

#include "utilLightJobBase.hpp"
#include "clsDef.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   #define CLS_GROUPMODE_CHECK_INTERVAL      OSS_ONE_MILLION * 5    /// 5 secs

   class _clsGroupInfo ;
   class _clsReplicateSet ;

   /* 
      _clsGroupModeMonitorJob
    */
   template< class T >
   class _clsGroupModeMonitorJob : public _utilLightJob
   {
   protected:
      _clsGroupModeMonitorJob( _clsGroupInfo *info,
                               const UINT32 &localVersion ) ;

      virtual ~_clsGroupModeMonitorJob() ;

   public:
      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

   public:
      // Use to indicate the latest thread's job version
      static ossAtomic32 version ;

   protected:
      virtual INT32 _checkGroupMode( pmdEDUCB *cb,
                                     UTIL_LJOB_DO_RESULT &result,
                                     UINT64 &sleepTime )
      {
         return SDB_OK ;
      }

      void     pause()
      {
         if ( _hasLock )
         {
            _info->mtx.release_r() ;
            _hasLock = FALSE ;
         }
      }

      INT32    resume()
      {
         if ( !_hasLock )
         {
            _info->mtx.lock_r() ;
            _hasLock = TRUE ;
         }

         return SDB_OK ;
      }

   protected:
      const _clsGroupMode     _groupMode ;
      const UINT32            _localVersion ;
      _clsGroupInfo           *_info ;
      BOOLEAN                 _hasLock ;
   } ;

   template< class T > ossAtomic32 _clsGroupModeMonitorJob< T >::version( 0 ) ;

   /* 
      _clsCriticalModeMonitorJob
    */
   class _clsCriticalModeMonitorJob : public _clsGroupModeMonitorJob< _clsCriticalModeMonitorJob >
   {
   public:
      _clsCriticalModeMonitorJob( _clsGroupInfo *info ) ;

      virtual ~_clsCriticalModeMonitorJob() ;

      virtual const CHAR *name() const
      {
         return "CriticalModeMonitor" ;
      }

   private:
      virtual INT32 _checkGroupMode( pmdEDUCB *cb,
                                     UTIL_LJOB_DO_RESULT &result,
                                     UINT64 &sleepTime ) ;

      INT32 _stopCriticalMode( pmdEDUCB *cb ) ;

   } ;
   typedef _clsCriticalModeMonitorJob clsCriticalModeMonitorJob ;

   INT32 clsStartCriticalModeMonitor( _clsGroupInfo *info ) ;

   class _clsMaintenanceModeMonitorJob :
         public _clsGroupModeMonitorJob< _clsMaintenanceModeMonitorJob >
   {
   public:
      _clsMaintenanceModeMonitorJob( _clsGroupInfo *info ) ;

      virtual ~_clsMaintenanceModeMonitorJob() ;

      virtual const CHAR *name() const
      {
         return "MaintenanceModeMonitor" ;
      }

   private:
      virtual INT32 _checkGroupMode( pmdEDUCB *cb,
                                     UTIL_LJOB_DO_RESULT &result,
                                     UINT64 &sleepTime ) ;

      INT32 _stopMaintenanceMode( pmdEDUCB *cb,
                                  const CHAR *pNodeName ) ;
   } ;
   typedef _clsMaintenanceModeMonitorJob clsMaintenanceModeMonitorJob ;

   INT32 clsStartMaintenanceModeMonitor( _clsGroupInfo *info ) ;

   class _clsGroupModeReqJob : public _utilLightJob
   {
   public:
      _clsGroupModeReqJob( _clsGroupInfo *info, _clsReplicateSet *pRepl, UINT64 delayMS = 0 ) ;

      virtual ~_clsGroupModeReqJob() ;

      virtual const CHAR *name() const
      {
         return "GroupModeRequire" ;
      }

      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

   private:
      // Use to indicate the latest thread's job version
      static ossAtomic32 version ;

   private:
      INT32 _handleGroupModeRes( const BSONObj &grpModeInfo ) ;

   private:
      // This info stores group info, not location info
      _clsGroupInfo                    *_info ;
      _clsReplicateSet                 *_repl ;

      UINT64                           _delayMS ;
      UINT64                           _createTick ;

      const UINT32                     _localVersion ;
   } ;
   typedef _clsGroupModeReqJob clsGroupModeReqJob ;

   INT32 clsStartGroupModeReqJob( _clsGroupInfo *info,
                                  _clsReplicateSet *pRepl,
                                  UINT64 delayMS = 0 ) ;

}

#endif // CLSGROUPMODE_HPP_