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
