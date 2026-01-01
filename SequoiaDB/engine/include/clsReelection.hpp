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

   Source File Name = clsReelection.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_REELECTION_HPP_
#define CLS_REELECTION_HPP_

#include "clsDef.hpp"
#include "ossEvent.hpp"

namespace engine
{
   class _clsVoteMachine ;
   class _clsSyncManager ;
   class _netRouteAgent ;

   enum CLS_REELECTION_STEP
   {
      CLS_REELECTION_STEP_NONE = 0,
      CLS_REELECTION_STEP_WAIT_WRITE,
      CLS_REELECTION_STEP_WAIT_REPLICA,
      CLS_REELECTION_STEP_DEST_NOTIFY,
      CLS_REELECTION_STEP_STEPDOWN,
      CLS_REELECTION_STEP_WAIT_PRIMARY,
      CLS_REELECTION_STEP_DONE
   } ;

   /*
      Tool functions
   */
   const CHAR* clsGetReelectionStepStr( CLS_REELECTION_STEP step ) ;

   /*
      _clsReelection define
   */
   class _clsReelection : public SDBObject
   {
   public:
      _clsReelection( _clsVoteMachine *vote,
                      _clsSyncManager *syncMgr,
                      _clsGroupInfo *info,
                      _netRouteAgent *pAgent ) ;
      ~_clsReelection() ;

   public:
      INT32 run( CLS_REELECTION_LEVEL lvl,
                 INT32 seconds,
                 pmdEDUCB *cb,
                 const SET_UINT16 &setDestID ) ;

      INT32 runAsync( CLS_REELECTION_LEVEL lvl,
                      INT32 waitMS,
                      UINT16 destID = 0 ) ;

      INT32 onTimer( UINT32 interval ) ;

      INT32 wait( pmdEDUCB *cb, UINT32 timeout = CLS_WAIT_REELECT_TIMEOUT ) ;

      void  signal() ;

   private:
      INT32 _wait4AllWriteDone( UINT32 &timePassed,
                                UINT32 timeout,
                                CLS_REELECTION_LEVEL lvl,
                                pmdEDUCB *cb ) ;

      INT32 _wait4SyncDone( UINT32 &timePassed,
                            UINT32 timeout,
                            CLS_REELECTION_LEVEL lvl,
                            pmdEDUCB *cb ) ;

      INT32 _wait4Replica( UINT32 &timePassed,
                           UINT32 timeout,
                           pmdEDUCB *cb,
                           const SET_UINT16 &setDestID ) ;

      INT32 _wait4Replica( UINT32 &timePassed,
                           UINT32 timeout,
                           pmdEDUCB *cb,
                           UINT16 destID ) ;

      INT32 _wait4ReplicaByBeat( UINT32 &timePassed,
                                 UINT32 timeout,
                                 pmdEDUCB *cb,
                                 const SET_UINT16 &setDestID ) ;

      INT32 _stepDown( UINT32 &timePassed,
                       UINT32 timeout,
                       BOOLEAN isLocation,
                       pmdEDUCB *cb ) ;

      INT32 _wait( UINT32 &timePassed,
                   UINT32 timeout,
                   pmdEDUCB *cb,
                   BOOLEAN canSetBlock ) ;

      INT32 _asyncWait( pmdEDUCB *cb ) ;

      OSS_INLINE BOOLEAN _isLocation() const ;

      string _nodesToString( const SET_UINT16 &setDestID ) ;

   private:
      _clsVoteMachine *_vote ;
      _clsSyncManager *_syncMgr ;
      _clsGroupInfo   *_info ;
      _netRouteAgent  *_pAgent ;
      volatile UINT32 _level ;

      /// for async
      CLS_REELECTION_STEP  _step ;
      UINT32               _waitMS ;
      UINT16               _destID ;

      ossEvent _event ;

      BOOLEAN _blockSync ;
   } ;
   typedef class _clsReelection clsReelection ;
}

#endif

