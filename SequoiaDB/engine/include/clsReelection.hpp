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

   class _clsReelection : public SDBObject
   {
   public:
      _clsReelection( _clsVoteMachine *vote,
                      _clsSyncManager *syncMgr ) ;
      ~_clsReelection() ;

   public:
      INT32 run( CLS_REELECTION_LEVEL lvl,
                 UINT32 seconds,
                 pmdEDUCB *cb ) ;

      INT32 wait( pmdEDUCB *cb ) ;

      void signal( CLS_REELECTION_LEVEL lvl =
                   CLS_REELECTION_LEVEL_MAX ) ;

   private:
      INT32 _wait4AllWriteDone( UINT32 &timePassed,
                                UINT32 timeout,
                                pmdEDUCB *cb ) ;

      INT32 _wait4Replica( UINT32 &timePassed,
                           UINT32 timeout,
                           pmdEDUCB *cb ) ;

      INT32 _stepDown( UINT32 &timePassed,
                       UINT32 timeout,
                       pmdEDUCB *cb ) ;

      INT32 _wait( UINT32 &timePassed,
                   UINT32 timeout,
                   pmdEDUCB *cb ) ;

   private:
      _clsVoteMachine *_vote ;
      _clsSyncManager *_syncMgr ;
      volatile CLS_REELECTION_LEVEL _level ;
      ossEvent _event ;
   } ;
   typedef class _clsReelection clsReelection ;
}

#endif

