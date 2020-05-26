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

   Source File Name = stpNetManager.hpp

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

#ifndef STP_NET_MANAGER_HPP__
#define STP_NET_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "netRouteAgent.hpp"
#include "stpMsg.hpp"

namespace engine
{

   /*
      _stpNetManager define
    */
   class _stpNetManager : public utilPooledObject
   {
   public:
      // constructor and destructor
      _stpNetManager( stpNetMsgHandlerBase *handler ) ;
      virtual ~_stpNetManager() ;

   public:
      OSS_INLINE netRouteAgent *getNetAgent()
      {
         return &_agent ;
      }

      INT32 initNetAgent( const CHAR *hostName,
                          const CHAR *serviceName,
                          UINT32 protocolMask ) ;

      INT32 activeNetAgent() ;
      INT32 deactiveNetAgent() ;

      // get route ID for given host name and service name
      INT32 getRouteID( const CHAR *hostName,
                        const CHAR *serviceName,
                        MsgRouteID &routeID ) ;

      // get route ID for remote handle
      INT32 getRouteID( netRouteAgent *netAgent,
                        const NET_HANDLE &handle,
                        MsgRouteID &routeID ) ;

      // update route ID in net agent for given host name and service name
      INT32 updateRouteID( const MsgRouteID &routeID,
                           const CHAR *hostName,
                           const CHAR *serviceName ) ;

      // delete route ID from net agent
      INT32 deleteRouteID( const MsgRouteID &routeID ) ;

   protected:
      stpNetMsgHandlerBase *  _handler ;
      netRouteAgent           _agent ;
   } ;

   typedef class _stpNetManager stpNetManager ;

}

#endif // STP_NET_MANAGER_HPP__
