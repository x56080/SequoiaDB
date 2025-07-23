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

   Source File Name = netEventHandler.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef NETEVENTHANDLER_HPP_
#define NETEVENTHANDLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "utilPooledObject.hpp"

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
using namespace boost::asio ;
using namespace std ;

namespace engine
{
   /*
      NET_EVENT_HANDLER_STATE define
   */
   enum NET_EVENT_HANDLER_STATE
   {
      NET_EVENT_HANDLER_STATE_HEADER         = 0,
      NET_EVENT_HANDLER_STATE_HEADER_LAST,
      NET_EVENT_HANDLER_STATE_BODY
   } ;

   class _netEventSuit ;
   typedef boost::shared_ptr<_netEventSuit>     netEvSuitPtr ;

   /*
      _netEventHandler define
   */
   class _netEventHandler :
         public boost::enable_shared_from_this<_netEventHandler>,
         public utilPooledObject
   {
      public:
         _netEventHandler( netEvSuitPtr evSuitPtr,
                           const NET_HANDLE &handle  ) ;
         ~_netEventHandler() ;

         netEvSuitPtr getEVSuitPtr() const { return _evSuitPtr ; }

         BOOLEAN  isConnected() const { return _isConnected ; }
         BOOLEAN  isNew() const { return _isNew ; }

      public:
         OSS_INLINE void id( const _MsgRouteID &id )
         {
            _id = id ;
         }
         OSS_INLINE const _MsgRouteID &id()
         {
            return _id ;
         }
         OSS_INLINE UINT64 getAndIncMsgID( BOOLEAN inc = TRUE )
         {
            if ( inc )
            {
               return _msgid++ ;
            }
            return _msgid ;
         }
         OSS_INLINE boost::asio::ip::tcp::socket &socket()
         {
            return _sock ;
         }
         OSS_INLINE _ossSpinXLatch &mtx()
         {
            return _mtx ;
         }
         OSS_INLINE NET_HANDLE handle() const
         {
            return _handle ;
         }
         CHAR *msg()
         {
            return _buf ;
         }
         OSS_INLINE NET_EVENT_HANDLER_STATE state() const
         {
            return _state ;
         }

         void  close() ;

         UINT64 getLastSendTick() const { return _lastSendTick ; }
         UINT64 getLastRecvTick() const { return _lastRecvTick ; }
         UINT64 getLastBeatTick() const { return _lastBeatTick ; }

         UINT32 getIOPS() const { return _iops ; }

         void   syncLastBeatTick() ;
         void   makeStat( UINT64 curTick ) ;

      public:
         void  asyncRead() ;

         INT32 syncConnect( const CHAR *hostName,
                            const CHAR *serviceName ) ;

         INT32 syncSend( const void *buf,
                         UINT32 len ) ;

         void  setOpt() ;

         string localAddr() const ;
         string remoteAddr() const ;
         UINT16 localPort() const ;
         UINT16 remotePort() const ;

         BOOLEAN isLocalConnection() const ;

      private:
         void  _readCallback( const boost::system::error_code &error ) ;
         INT32 _allocateBuf( UINT32 len ) ;

      private:
         boost::asio::ip::tcp::socket     _sock ;
         _ossSpinXLatch                   _mtx ;
         _MsgHeader                       _header ;
         CHAR                             *_buf ;
         UINT32                           _bufLen ;
         NET_EVENT_HANDLER_STATE          _state ;
         _MsgRouteID                      _id ;
         netEvSuitPtr                     _evSuitPtr ;
         NET_HANDLE                       _handle ;
         volatile BOOLEAN                 _isConnected ;
         volatile BOOLEAN                 _isNew ;
         BOOLEAN                          _hasRecvMsg ;
         UINT64                           _lastSendTick ;
         UINT64                           _lastRecvTick ;
         UINT64                           _lastBeatTick ;
         UINT64                           _msgid ;

         UINT64                           _lastStatTick ;
         UINT64                           _totalIOTimes ;
         UINT32                           _iops ;     /// times/s

   } ;
   typedef _netEventHandler netEventHandler ;

   typedef boost::shared_ptr<_netEventHandler> NET_EH ;

}

#endif // NETEVENTHANDLER_HPP_

