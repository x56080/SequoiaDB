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

#include "clsReplDef.hpp"
#include "ossEvent.hpp"
#include "clsVoteMachine.hpp"
#include "clsSyncManager.hpp"

namespace engine
{

   class _clsReelection : public SDBObject
   {
   public:
<<<<<<< HEAD
      _clsReelection( _clsVoteMachine *vote,
                      _clsSyncManager *syncMgr,
                      _clsGroupInfo *info ) ;
=======
      _clsReelection( ICLSReplAgent *replAgent ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      ~_clsReelection() ;

   public:
      INT32 run( CLS_REELECTION_LEVEL lvl,
                 INT32 seconds,
                 pmdEDUCB *cb,
                 const MsgRouteID &destRID ) ;

      INT32 wait( pmdEDUCB *cb ) ;

      void  signal() ;

   private:
      INT32 _wait4AllWriteDone( UINT32 &timePassed,
                                UINT32 timeout,
                                pmdEDUCB *cb ) ;

      INT32 wait4SyncDone( UINT32 &timePassed,
                           UINT32 timeout ) ;

      INT32 _wait4Replica( UINT32 &timePassed,
                           UINT32 timeout,
                           pmdEDUCB *cb,
                           const MsgRouteID &destRID ) ;

      INT32 _wait4ReplicaByBeat( UINT32 &timePassed,
                                 UINT32 timeout,
                                 UINT16 destID ) ;

      INT32 _stepDown( UINT32 &timePassed,
                       UINT32 timeout,
                       BOOLEAN isLocation,
                       pmdEDUCB *cb ) ;

      INT32 _wait( UINT32 &timePassed,
                   UINT32 timeout,
                   pmdEDUCB *cb,
                   BOOLEAN canSetBlock ) ;

<<<<<<< HEAD
      OSS_INLINE BOOLEAN _isLocation() const ;

   private:
      _clsVoteMachine *_vote ;
      _clsSyncManager *_syncMgr ;
      _clsGroupInfo   *_info ;
=======
   protected:
      ICLSReplAgent *   _replAgent ;
      clsVoteMachine *  _vote ;
      clsSyncManager *  _syncMgr ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      volatile UINT32 _level ;
      ossEvent _event ;

      BOOLEAN _blockSync ;
   } ;
   typedef class _clsReelection clsReelection ;
}

#endif

