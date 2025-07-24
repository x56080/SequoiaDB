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

   Source File Name = netMultiRouteAgent.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef NETMULTIROUTEAGENT_HPP__
#define NETMULTIROUTEAGENT_HPP__

#include "core.hpp"
#include "pd.hpp"
#include "netRouteAgent.hpp"
#include "ossAtomic.hpp"
#include "netMsgHandler.hpp"
#include "msg.h"
#include "pmdEDU.hpp"
#include "coordDef.hpp"
#include "utilMap.hpp"

namespace engine
{
   typedef  _utilMap< UINT64, MsgRouteID, 20 >     REQUESTID_MAP ;
   typedef  std::set<UINT64>                       ROUTE_SET;
   class    CoordSession ;

   class netMultiRouteAgent : public _netMsgHandler
   {
   typedef std::map<SINT64, CoordSession*>         COORD_SESSION_MAP;

   public:
      netMultiRouteAgent() ;
      ~netMultiRouteAgent() ;

      void  setNetWork( _netRouteAgent *pNetWork)   ;

   public:
      virtual INT32 handleMsg( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg ) ;

      virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;

      INT32 updateRoute( const _MsgRouteID &id,
                         const CHAR *pHost,
                         const CHAR *pService ) ;

      INT32 addSession( pmdEDUCB *pEduCB );

      void  delSession( UINT32 tID );

      BOOLEAN isSubSessionConnected( UINT32 tID, const MsgRouteID &ID );

      CoordSession *getSession( UINT32 tID );

      UINT64 reqIDNew() ;

   public:
      INT32 syncSend( const MsgRouteID &id, void *header, UINT64 &reqID,
                      pmdEDUCB *pEduCB, void *body = NULL,
                      UINT32 bodyLen = 0 ) ;

      INT32 syncSend( const MsgRouteID &id, MsgHeader *header,
                      const netIOVec &iov, UINT64 &reqID,
                      pmdEDUCB *pEduCB ) ;

      INT32 syncSendWithoutCheck( const MsgRouteID &id, void *header,
                                 UINT64 &reqID, pmdEDUCB *pEduCB,
                                 void *body = NULL, UINT32 bodyLen = 0 );

      void  multiSyncSend( const ROUTE_SET &routeSet, void *header ) ;

   private:
      _netRouteAgent       *_pNetWork;
      ossSpinSLatch        _mutex ;
      ossAtomic64          *_pReqID;
      COORD_SESSION_MAP    _sessionMap;
   };
}

#endif
