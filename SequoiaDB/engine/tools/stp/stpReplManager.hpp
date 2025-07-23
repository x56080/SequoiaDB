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

   Source File Name = stpReplManager.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef STP_REPL_MANAGER_HPP__
#define STP_REPL_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "dpsLogDef.hpp"
#include "ossRWMutex.hpp"
#include "clsReplAgent.hpp"

namespace engine
{

   /*
      _stpReplManager define
    */
   // _stpReplManager manages replica votes between servers
   class _stpReplManager : public stpManagerBase,
                           public ICLSReplAgent
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      // constructor and destructor
      _stpReplManager( STPCB *stpCB ) ;
      ~_stpReplManager() ;

   public:
      // override functions of STP module

      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_REPL_MANAGER_NAME ;
      }

      // get role mask of module
      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         // only used in server role
         return STP_ROLE_MASK_SERVER ;
      }

      // on timer callback
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      // process message callback
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

      // process event
      INT32 handleEvent( pmdEDUEvent *event ) ;

   protected:
      // override protected functions of STP module

      // initialize module
      virtual INT32 _initialize() ;

      // event post activate
      virtual INT32 _postActivate() ;

      // event previous to deactivate
      virtual INT32 _preDeactivate() ;

      // event on change servers
      virtual INT32 _onChangeServers( UINT32 version,
                                      const STP_SERVER_LIST &servers ) ;

   public:
      // implements of ICLSReplAgent

      // check if it is valid to launch vote
      virtual BOOLEAN checkVoteLaunch() ;
      // get expected LSN of local
      virtual DPS_LSN getLocalExpectLSN() ;
      // get current LSN of local
      virtual DPS_LSN getLocalCurrentLSN() ;
      // get LSN window of local
      // NOTE: no file or memory concept in STP, returned with 0
      virtual void getLSNWindow( DPS_LSN &fileBeginLSN,
                                 DPS_LSN &memBeginLSN,
                                 DPS_LSN &endLSN,
                                 DPS_LSN &expectLSN ) ;
      // check if local is OK
      virtual BOOLEAN isLocalOK() ;
      // check if local is spare
      virtual BOOLEAN isLocalSpare() ;
      // get vote weight
      virtual UINT8 getVoteWeight() ;
      // get sharing break time
      virtual UINT32 getSharingBreakTime() ;
      // get synchronize strategy
      virtual INT32 getSyncStrategy() ;
      // on event if local is not found in replca group
      virtual INT32 onLocalNotFoundInGroup() ;
      // event before primary active
      virtual void beforePrimaryActive() ;
      // event on primary active
      virtual void onPrimaryActive( const MsgRouteID &newPrimaryRID,
                                    const MsgRouteID &oldPrimaryRID ) ;
      // event after primary active
      virtual void afterPrimaryActive( const MsgRouteID &newPrimaryRID,
                                       const MsgRouteID &oldPrimaryRID ) ;
      // event before primary deactive
      virtual void beforePrimaryDeactive() ;
      // event on primary deactive
      virtual void onPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                      const MsgRouteID &oldPrimaryRID ) ;
      // event after primary deactive
      virtual void afterPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                         const MsgRouteID &oldPrimaryRID ) ;
      // event on local group expired
      virtual void onLocalGroupExpired() ;
      // event before new primary is found
      virtual void beforeFoundNewPrimary() ;
      // event after new primary is found
      virtual void afterFoundNewPrimary( const MsgRouteID &newPrimaryRID ) ;
      // process LSN reported by beat
      virtual void processBeatLSN( const MsgRouteID &remote,
                                   const DPS_LSN &lsn ) ;

   protected:
      // activate replica group to vote
      INT32 _activateReplGroup() ;
      // update replica group
      INT32 _updateReplGroup( UINT32 version, const STP_SERVER_LIST &servers ) ;

      // build vote nodes from server list
      static INT32 _buildReplGroup( const STP_SERVER_LIST &servers,
                                    NET_ROUTE_MAP &nodes ) ;
   } ;

}

#endif // STP_REPL_MANAGER_HPP__
