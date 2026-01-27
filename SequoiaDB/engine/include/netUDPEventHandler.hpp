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

   Source File Name = netUDPEventHandler.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef NET_UDP_EVENT_HANDLER_HPP_
#define NET_UDP_EVENT_HANDLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "utilPooledObject.hpp"
#include "netEventHandlerBase.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _netUDPEventHandler define
    */
   class _netUDPEventHandler : public netEventHandlerBase
   {
   public:
      _netUDPEventHandler( NET_UDP_EV_SUIT evSuit,
                           const NET_HANDLE &handle,
                           const MsgRouteID &routeID,
                           const netUDPEndPoint &endPoint ) ;
      virtual ~_netUDPEventHandler() ;

      static NET_EH createShared( NET_UDP_EV_SUIT evSuit,
                                  const NET_HANDLE &handle,
                                  const MsgRouteID &routeID,
                                  const netUDPEndPoint &endPoint ) ;

   public:
      OSS_INLINE virtual NET_EVENT_HANDLER_TYPE getHandlerType() const
      {
         return NET_EVENT_HANDLER_UDP ;
      }

      virtual INT32 syncConnect( const CHAR *hostName,
                                 const CHAR *serviceName ) ;
      virtual INT32 asyncRead() ;
      virtual INT32 syncSendRaw( const void *buf, UINT32 len ) ;
      virtual void close() ;
      virtual void setOpt() ;
      virtual CHAR *msg() ;

      virtual std::string localAddr() const ;
      virtual std::string remoteAddr() const ;
      virtual UINT16 localPort() const ;
      virtual UINT16 remotePort() const ;

      void readCallback( CHAR *buffer, UINT32 bufferSize ) ;
      void setRouteID( const MsgRouteID &routeID ) ;

      OSS_INLINE const netUDPEndPoint &getRemoteEndPoint() const
      {
         return _remoteEndPoint ;
      }

      // check if the net suit is stopped
      virtual BOOLEAN isSuitStopped() const ;

   protected:
      OSS_INLINE NET_UDP_EH _getShared()
      {
         return NET_UDP_EH::makeRaw( this, ALLOC_POOL ) ;
      }

   protected:
      NET_UDP_EV_SUIT   _evSuitPtr ;
      netUDPEndPoint    _remoteEndPoint ;
   } ;

}

#endif // NET_UDP_EVENT_HANDLER_HPP_
