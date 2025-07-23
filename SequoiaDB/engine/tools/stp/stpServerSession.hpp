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

   Source File Name = stpServerSession.hpp

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
#ifndef STP_SERVER_SESSION_HPP__
#define STP_SERVER_SESSION_HPP__

#include "stpCBCommon.hpp"
#include "msg.hpp"

namespace engine
{

   /*
      _stpServerSession define
    */
   // _stpServerSession manages sessions to STP server, e.g. chooses server as
   // synchronize source, etc.
   class _stpServerSession : public SDBObject
   {
   public:
      // constructor and destructor
      _stpServerSession( STPCB *stpCB ) ;
      virtual ~_stpServerSession() ;

   public:
      // get route ID for current server
      OSS_INLINE const MsgRouteID &getCurServerRID() const
      {
         return _curServerRID ;
      }

      // set route ID for current server
      OSS_INLINE void setCurServerRID( const MsgRouteID &routeID )
      {
         _curServerRID = routeID ;
      }

      // reset route ID of current server
      OSS_INLINE void resetCurServerRID()
      {
         _curServerRID.value = MSG_INVALID_ROUTEID ;
      }

      // get route ID of current primary
      INT32 getPrimaryRID( MsgRouteID &routeID ) ;

      // get route ID of any server
      INT32 getServerRID( MsgRouteID &routeID ) ;

   protected:
      // route ID of current server
      MsgRouteID        _curServerRID ;
      // pointer to node manager
      stpNodeManager *  _nodeManager ;
   } ;

   typedef class _stpServerSession stpServerSession ;

}

#endif // STP_SERVER_SESSION_HPP__
