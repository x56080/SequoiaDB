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
#include "netEventHandlerBase.hpp"

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

   /*
      _netEventHandler define
    */
   class _netEventHandler : public netEventHandlerBase
   {
      public:
         _netEventHandler( netEvSuitPtr evSuitPtr,
                           const NET_HANDLE &handle ) ;
         virtual ~_netEventHandler() ;

         static NET_EH createShared( netEvSuitPtr evSuitPtr,
                                     const NET_HANDLE &handle ) ;

      public:
         OSS_INLINE boost::asio::ip::tcp::socket &socket()
         {
            return _sock ;
         }
         virtual CHAR *msg()
         {
            return _buf ;
         }
         OSS_INLINE NET_EVENT_HANDLER_STATE state() const
         {
            return _state ;
         }

         virtual void  close() ;

      public:
         virtual void  asyncRead() ;

         virtual INT32 syncConnect( const CHAR *hostName,
                                    const CHAR *serviceName ) ;

         virtual INT32 syncSend( const void *buf,
                                 UINT32 len ) ;

         virtual void  setOpt() ;

         virtual std::string localAddr() const ;
         virtual std::string remoteAddr() const ;
         virtual UINT16 localPort() const ;
         virtual UINT16 remotePort() const ;

         OSS_INLINE virtual NET_EVENT_HANDLER_TYPE getHandlerType() const
         {
            return NET_EVENT_HANDLER_TCP ;
         }

      protected:
         void  _readCallback( const boost::system::error_code &error ) ;
         INT32 _allocateBuf( UINT32 len ) ;

         OSS_INLINE NET_TCP_EH _getShared()
         {
            return NET_TCP_EH::makeRaw( this, ALLOC_POOL ) ;
         }

      protected:
         netEvSuitPtr                     _evSuitPtr ;
         boost::asio::ip::tcp::socket     _sock ;
         _MsgHeader                       _header ;
         CHAR                             *_buf ;
         UINT32                           _bufLen ;
         NET_EVENT_HANDLER_STATE          _state ;
         BOOLEAN                          _hasRecvMsg ;
   } ;

}

#endif // NETEVENTHANDLER_HPP_

