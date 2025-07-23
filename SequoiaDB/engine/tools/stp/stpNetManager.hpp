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
