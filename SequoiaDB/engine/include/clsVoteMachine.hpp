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
#include <vector>

using namespace std ;

namespace engine
{
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

      OSS_INLINE void setShadowWeight( UINT8 weight )
      {
         _shadowWeight = weight ;
         return ;
      }

      OSS_INLINE BOOLEAN isInStepUp() const
      {
         return 0 < _forceMillis ;
      }

   public:
      INT32 init() ;

      void clear() ;

      INT32 handleInput( const MsgHeader *header ) ;

      void handleTimeout( const UINT32 &millisec ) ;

      INT32 active() ;

      void force( const INT32 &id,
                  UINT32 mills = 0 ) ;


   private:
      vector<_clsVoteStatus *> _status ;
      _netRouteAgent *_agent ;
      _clsVoteStatus *_current ;
      _clsGroupInfo *_groupInfo ;
      UINT8 _shadowWeight ;
      UINT32 _forceMillis ;
   } ;

   typedef class _clsVoteMachine clsVoteMachine ;
}

#endif

