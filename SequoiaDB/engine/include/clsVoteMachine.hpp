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

   Source File Name = clsVoteMachine.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLSVOTEMACHINE_HPP_
#define CLSVOTEMACHINE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsVoteStatus.hpp"
#include "ossLatch.hpp"
#include <vector>

using namespace std ;

namespace engine
{

   #define CLS_SHADOWN_TIMEOUT_DFT           ( 3000 )

   class _clsVoteMachine : public SDBObject
   {
   public:
      _clsVoteMachine( _clsGroupInfo *info,
                       _netRouteAgent *agent ) ;
      ~_clsVoteMachine() ;

   public:
      OSS_INLINE BOOLEAN primaryIsMe()
      {
         _groupInfo->mtx.lock_r() ;
         BOOLEAN res = _groupInfo->local.value == _groupInfo->primary.value &&
                       _groupInfo->primary.value != MSG_INVALID_ROUTEID ?
                       TRUE : FALSE ;
         _groupInfo->mtx.release_r() ;
         return res ;
      }

      OSS_INLINE UINT8 getShadowWeight() const
      {
         return _shadowWeight ;
      }

      OSS_INLINE void setShadowWeight( UINT8 weight,
                                       UINT32 timeout = CLS_SHADOWN_TIMEOUT_DFT,
                                       BOOLEAN shadowForReelect = TRUE )
      {
         _shadowWeight = weight ;
         _shadowTimeout = timeout ;
         _shadowForReelect = shadowForReelect ;
      }

      OSS_INLINE UINT8 getElectionWeight() const
      {
         return _electionWeight ;
      }

      OSS_INLINE BOOLEAN hasElectionWeight( UINT8 electionWeight ) const
      {
         return OSS_BIT_TEST( _electionWeight, electionWeight ) ;
      }

      OSS_INLINE void setElectionWeight( UINT8 electionWeight )
      {
         if ( ! hasElectionWeight( electionWeight ) )
         {
            OSS_BIT_SET( _electionWeight, electionWeight ) ;
         }
      }

      OSS_INLINE void resetElectionWeight( UINT8 electionWeight )
      {
         if ( hasElectionWeight( electionWeight ) )
         {
            OSS_BIT_CLEAR( _electionWeight, electionWeight ) ;
         }
      }

      OSS_INLINE BOOLEAN isInStepUp() const
      {
         return 0 < _forceMillis ;
      }

      OSS_INLINE BOOLEAN isShadowTimeout() const
      {
         return 0 == _shadowTimeout ;
      }

      OSS_INLINE BOOLEAN isLocation() const
      {
         return MSG_INVALID_LOCATIONID != _groupInfo->localLocationID ;
      }

      OSS_INLINE BOOLEAN isTmpGrpMode() const
      {
         return 0 < _grpModeShadowTime ;
      }

      OSS_INLINE BOOLEAN isConstantGrpMode() const
      {
         return _grpModeShadowTime < 0 ;
      }

      OSS_INLINE void setGrpModeShadowTime( INT32 grpModeShadowTime )
      {
         _grpModeShadowTime = grpModeShadowTime ;
      }

      void resetGrpModeElectionWeights() ;

      INT32 startCriticalModeMonitor() ;

      INT32 startMaintenanceModeMonitor() ;

   public:
      INT32 init() ;

      void  clear() ;
      void  setImmediatelyTime() ;

      INT32 handleInput( const MsgHeader *header ) ;

      void  handleTimeout( const UINT32 &millisec ) ;

      INT32 active() ;

      void  force( const INT32 &id, UINT32 mills = 0 ) ;
      BOOLEAN  isStatus( const INT32 &id ) const ;
      BOOLEAN  isInit() const { return _current ? TRUE : FALSE ; }

   private:
      vector<_clsVoteStatus *>   _status ;
      _netRouteAgent             *_agent ;
      _clsVoteStatus             *_current ;
      _clsGroupInfo              *_groupInfo ;
      UINT32                     _shadowTimeout ;  /// ms
      BOOLEAN                    _shadowForReelect ;
      UINT32                     _forceMillis ;

      // The total election weight should be calculate as follow:
      //   totalWeight = _electionWeight * 256 + _shadowWeight
      // But for convenience, we can compare the total weight separately:
      // compare _electionWeight first, then compare _shadowWeight.
      UINT8                      _electionWeight ;
      UINT8                      _shadowWeight ;

      // _grpModeShadowTime = -1 means keep grpMode forever
      // _grpModeShadowTime = 0 means grpMode in this group is NORMAL
      // _grpModeShadowTime > 0 means keep grpMode for the next _grpModeShadowTime milliseconds
      INT32                      _grpModeShadowTime ;

      ossSpinXLatch              _latch ;
   } ;

   typedef class _clsVoteMachine clsVoteMachine ;
}

#endif // CLSVOTEMACHINE_HPP_

