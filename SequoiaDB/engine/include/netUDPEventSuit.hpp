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

   Source File Name = netUDPEventSuit.hpp

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
#ifndef NET_UDP_EVENT_SUIT_HPP_
#define NET_UDP_EVENT_SUIT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "utilPooledObject.hpp"
#include "netMsgHandler.hpp"
#include "netUDPEventHandler.hpp"
#include "netInnerTimer.hpp"
#include "ossMemPool.hpp"

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

namespace engine
{

   /*
      _netUDPEventSuit define
    */
   class _netUDPEventSuit : public utilPooledObject
   {
   public:
      _netUDPEventSuit( netFrame *frame,
                        netRoute *route ) ;
      virtual ~_netUDPEventSuit() ;

      static NET_UDP_EV_SUIT createShared( netFrame *frame,
                                           netRoute *route ) ;

   public:
      INT32 listen( const CHAR *hostName,
                    const CHAR *serviceName,
                    UINT32 bufferSize ) ;
      void  setOptions() ;
      // WARNING: close UDP listen is not thread safe
      void  close() ;
      void  shutdown() ;
      INT32 asyncRead() ;
      INT32 syncBroadcast( const void *buf, UINT32 len ) ;

      OSS_INLINE boost::asio::ip::udp::socket *getSocket()
      {
         return ( &_sock ) ;
      }

      OSS_INLINE BOOLEAN isOpened()
      {
         return _sock.is_open() ;
      }

      OSS_INLINE CHAR *getMessage()
      {
         return _buffer ;
      }

      OSS_INLINE netFrame *getFrame()
      {
         return _frame ;
      }

      OSS_INLINE netUDPEndPoint getLocalEndPoint() const
      {
         return _localEndPoint ;
      }

      void  handleMsg( NET_EH eh ) ;
      INT32 getEH( const netUDPEndPoint &endPoint,
                   const MsgRouteID &routeID,
                   NET_EH &eh ) ;
      INT32 getEH( const MsgRouteID &routeID,
                   NET_EH &eh ) ;
      void  removeEH( const netUDPEndPoint &endPoint ) ;
      void  removeEH( const MsgRouteID &routeID ) ;
      void  removeAllEH() ;

      // check if the net suit is stopped
      BOOLEAN isStoppped() const ;

   protected:
      typedef ossPoolMap< netUDPEndPoint, NET_EH > NET_UDP_EP2EH_MAP ;
      typedef ossPoolMap< UINT64, NET_EH >         NET_UDP_ID2EH_MAP ;

   protected:
      OSS_INLINE NET_UDP_EV_SUIT _getShared()
      {
         return NET_UDP_EV_SUIT::makeRaw( this, ALLOC_POOL ) ;
      }

      void    _readCallback( const boost::system::error_code &error ) ;
      INT32   _createEH( const netUDPEndPoint &remoteEndPoint,
                         const MsgRouteID &routeID,
                         NET_EH &eh ) ;
      NET_EH  _getEH( const netUDPEndPoint &endPoint ) ;
      NET_EH  _getEH( const MsgRouteID &routeID ) ;
      INT32   _allocateBuffer( UINT32 bufferSize ) ;

   protected:
      netFrame *                    _frame ;
      netRoute *                    _route ;
      netUDPRestartTimer            _restartTimer ;
      boost::asio::ip::udp::socket  _sock ;
      UINT32                        _bufferSize ;
      CHAR *                        _buffer ;
      netUDPEndPoint                _remoteEndPoint ;
      netUDPEndPoint                _localEndPoint ;
      ossSpinSLatch                 _mtx ;
      NET_UDP_EP2EH_MAP             _ep2ehMap ;
      NET_UDP_ID2EH_MAP             _id2ehMap ;
   } ;

}

#endif // NET_UDP_EVENT_SUIT_HPP_
