/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = tpReplManager.hpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef TP_REPL_MANAGER_HPP__
#define TP_REPL_MANAGER_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "dpsLogDef.hpp"
#include "ossRWMutex.hpp"
#include "clsReplAgent.hpp"

namespace engine
{

   /*
      _tpReplManager define
    */
   // _tpReplManager manages votes between servers
   class _tpReplManager : public tpManagerBase,
                          public ICLSReplAgent
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _tpReplManager( SDB_TPCB *tpCB ) ;
      ~_tpReplManager() ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_REPL_MANAGER_NAME ;
      }

      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         return TP_ROLE_MASK_SERVER ;
      }

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      virtual INT32 _initialize() ;
      virtual INT32 _postActivate() ;
      virtual INT32 _preDeactivate() ;

      virtual INT32 _onChangeServers( UINT32 version,
                                      const TP_SERVER_LIST &servers ) ;

   public:
      // implements of ICLSReplAgent
      virtual BOOLEAN checkVoteLaunch() ;
      virtual DPS_LSN getLocalExpectLSN() ;
      virtual DPS_LSN getLocalCurrentLSN() ;
      virtual void getLSNWindow( DPS_LSN &fileBeginLSN,
                                 DPS_LSN &memBeginLSN,
                                 DPS_LSN &endLSN,
                                 DPS_LSN &expectLSN ) ;
      virtual BOOLEAN isLocalOK() ;
      virtual BOOLEAN isLocalSpare() ;
      virtual UINT8 getVoteWeight() ;
      virtual UINT32 getSharingBreakTime() ;
      virtual INT32 getSyncStrategy() ;
      virtual INT32 onLocalNotFoundInGroup() ;
      virtual void beforePrimaryActive() ;
      virtual void onPrimaryActive( const MsgRouteID &newPrimaryRID,
                                    const MsgRouteID &oldPrimaryRID ) ;
      virtual void afterPrimaryActive( const MsgRouteID &newPrimaryRID,
                                       const MsgRouteID &oldPrimaryRID ) ;
      virtual void beforePrimaryDeactive() ;
      virtual void onPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                      const MsgRouteID &oldPrimaryRID ) ;
      virtual void afterPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                         const MsgRouteID &oldPrimaryRID ) ;
      virtual void onLocalGroupExpired() ;
      virtual void onNotifiedPrimaryChange() ;

   protected:
      // activate replica group to vote
      INT32 _activateReplGroup() ;
      // update replica group
      INT32 _updateReplGroup( UINT32 version, const TP_SERVER_LIST &servers ) ;

      // build vote nodes from server list
      static INT32 _buildReplGroup( const TP_SERVER_LIST &servers,
                                    NET_ROUTE_MAP &nodes ) ;
   } ;

}

#endif // TP_REPL_MANAGER_HPP__
