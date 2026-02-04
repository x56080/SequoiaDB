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
                                       BOOLEAN shadowAutoRestore = FALSE )
      {
         _shadowWeight = weight ;
         _shadowTimeout = timeout ;
         _shadowAutoRestore = shadowAutoRestore ;
      }

      OSS_INLINE void setShadowWeightForTarget( UINT32 timeout = CLS_SHADOWN_TIMEOUT_DFT )
      {
         _shadowWeight = CLS_ELECTION_WEIGHT_MAX ;
         _shadowTimeout = timeout ;
         _shadowAutoRestore = FALSE ;

         setElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;
      }

      OSS_INLINE void setShadowWeightForNoneTarget( BOOLEAN autoRestore,
                                                    UINT32 timeout = CLS_SHADOWN_TIMEOUT_DFT )
      {
         _shadowWeight = CLS_ELECTION_WEIGHT_MIN ;
         _shadowTimeout = timeout ;
         _shadowAutoRestore = autoRestore ;

         resetElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;
      }

      OSS_INLINE void resetShadowWeight()
      {
         _shadowWeight = CLS_ELECTION_WEIGHT_USR_MIN ;
         _shadowTimeout = 0 ;
         _shadowAutoRestore = FALSE ;

         resetElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;
      }

      OSS_INLINE BOOLEAN isShadowWeightForTarget() const
      {
         if ( CLS_ELECTION_WEIGHT_MAX == _shadowWeight ||
              hasElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) )
         {
            return TRUE ;
         }
         return FALSE ;
      }

      OSS_INLINE BOOLEAN isShadowWeightForNoneTarget() const
      {
         if ( CLS_ELECTION_WEIGHT_MIN == _shadowWeight )
         {
            return TRUE ;
         }
         return FALSE ;
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
      BOOLEAN                    _shadowAutoRestore ;
      UINT32                     _forceMillis ;

      // The total election weight should be calculate as follow:
      //   totalWeight = _electionWeight * 256 + _shadowWeight
      // But for convenience, we can compare the total weight separately:
      // compare _electionWeight first, then compare _shadowWeight.
      UINT8                      _electionWeight ;
      UINT8                      _shadowWeight ;

      ossSpinXLatch              _latch ;
   } ;

   typedef class _clsVoteMachine clsVoteMachine ;
}

#endif // CLSVOTEMACHINE_HPP_

