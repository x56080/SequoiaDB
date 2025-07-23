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

   Source File Name = netEventHandlerBase.hpp

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
#ifndef NET_EVENT_HANDLER_BASE_HPP_
#define NET_EVENT_HANDLER_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "utilPooledObject.hpp"
#include "msgConvertor.hpp"

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

namespace engine
{

   /*
      NET_EVENT_HANDLER_TYPE
    */
   enum NET_EVENT_HANDLER_TYPE
   {
      NET_EVENT_HANDLER_UNKNOWN  = 0,
      NET_EVENT_HANDLER_TCP      = 1,
      NET_EVENT_HANDLER_UDP      = 2
   } ;

   /*
      _INetUserData define
    */
   // use to pass information between net handle and running session
   enum NET_USER_DATA_TYPE
   {
      // sharding message user data
      NET_USER_DATA_SHARD = 0
   } ;

   class _INetUserData : public utilPooledObject
   {
   public:
      // constructor and destructor
      _INetUserData()
      : _handle( NET_INVALID_HANDLE ),
        _opCode( 0 )
      {
      }

      virtual ~_INetUserData()
      {
      }

   public:
      // set net handle
      OSS_INLINE void setHandle( NET_HANDLE handle )
      {
         _handle = handle ;
      }

      // get net handle
      OSS_INLINE NET_HANDLE getHandle()
      {
         return _handle ;
      }

      // set opcode for current message
      OSS_INLINE void setOpCode( INT32 opCode )
      {
         _opCode = opCode ;
      }

      // get opcode for current message
      OSS_INLINE INT32 getOpCode() const
      {
         return _opCode ;
      }

      // get user data
      OSS_INLINE virtual UINT64 getUserData() const
      {
         return 0LL ;
      }

      // get type of user data
      virtual NET_USER_DATA_TYPE getType() const = 0 ;

   protected:
      NET_HANDLE  _handle ;
      INT32       _opCode ;
   } ;

   /*
      _netUserDataHolder define
    */
   // holder to net user data
   class _netUserDataHolder
   {
   public:
      // constructor and destructor
      _netUserDataHolder()
      : _userData( NULL )
      {
      }

      virtual ~_netUserDataHolder()
      {
         SAFE_OSS_DELETE( _userData ) ;
      }

      // get user data
      OSS_INLINE INetUserData *getUserDataPtr()
      {
         return _userData ;
      }

      // set user data
      OSS_INLINE void setUserDataPtr( INetUserData *userData )
      {
         SAFE_OSS_DELETE( _userData ) ;
         _userData = userData ;
      }

      // check if has user data
      OSS_INLINE BOOLEAN hasUserData() const
      {
         return NULL != _userData ;
      }

      // check if has user data of specified type
      OSS_INLINE BOOLEAN hasUserData( NET_USER_DATA_TYPE type ) const
      {
         return ( NULL != _userData &&
                  type == _userData->getType() ) ;
      }

      // get user data
      OSS_INLINE UINT64 getUserData() const
      {
         return ( NULL != _userData ) ?
                      ( _userData->getUserData() ) : 0LL ;
      }

   protected:
      // net user data
      INetUserData *_userData ;
   } ;

   /*
      _netEventHandlerBase define
      Handler to deal with network events. The main task is to establish the
      connection(for TCP), and to send messages. It should be used in exclusive
      mode, that is, the user need to take X lock on it before using.
    */
   class _netEventHandlerBase : public utilPooledObject,
                                public netUserDataHolder
   {
   public:
      _netEventHandlerBase( const NET_HANDLE &handle ) ;
      virtual ~_netEventHandlerBase() ;

   public:
      OSS_INLINE UINT64 getAndIncMsgID( BOOLEAN inc = TRUE )
      {
         if ( inc )
         {
            return _msgid++ ;
         }
         return _msgid ;
      }

      OSS_INLINE ossSpinXLatch &mtx()
      {
         return _mtx ;
      }

      OSS_INLINE NET_HANDLE handle() const
      {
         return _handle ;
      }

      OSS_INLINE BOOLEAN isConnected() const
      {
         return _isConnected ;
      }

      OSS_INLINE BOOLEAN isNew() const
      {
         return _isNew ;
      }

      OSS_INLINE void id( const MsgRouteID &id )
      {
         _id = id ;
      }

      OSS_INLINE const MsgRouteID &id()
      {
         return _id ;
      }

      OSS_INLINE UINT64 getLastSendTick() const
      {
         return _lastSendTick ;
      }

      OSS_INLINE UINT64 getLastRecvTick() const
      {
         return _lastRecvTick ;
      }

      OSS_INLINE UINT64 getLastBeatTick() const
      {
         return _lastBeatTick ;
      }

      OSS_INLINE UINT32 getIOPS() const
      {
         return _iops ;
      }

      void  syncLastBeatTick() ;
      void  makeStat( UINT64 curTick ) ;

      IMsgConvertor *getInMsgConvertor() ;
      IMsgConvertor *getOutMsgConvertor() ;

      virtual NET_EVENT_HANDLER_TYPE getHandlerType() const = 0 ;
      virtual INT32 syncConnect( const CHAR *hostName,
                                 const CHAR *serviceName ) = 0 ;
      virtual INT32 asyncRead() = 0 ;

      /**
       * @brief Send data in its raw format. It will NOT be treated as in any
       *        particular format.
       * @param buff Data to be sent.
       * @param len  Length of the data.
       */
      virtual INT32 syncSendRaw( const void *buff, UINT32 len ) = 0 ;

      virtual void close() = 0 ;
      virtual CHAR *msg() = 0 ;
      virtual void setOpt() = 0 ;

      // get available size in the receive buffer
      virtual UINT32 getAvailableSize() = 0 ;

      virtual boost::asio::ip::address_v4 localIP() const = 0 ;
      virtual boost::asio::ip::address_v4 remoteIP() const = 0 ;

      virtual std::string localAddr() const = 0 ;
      virtual std::string remoteAddr() const = 0 ;
      virtual UINT16 localPort() const = 0 ;
      virtual UINT16 remotePort() const = 0 ;

      OSS_INLINE BOOLEAN isLocalConnection() const
      {
         return localAddr() == remoteAddr() ? TRUE : FALSE ;
      }

   protected:
      OSS_INLINE NET_EH _getSharedBase()
      {
         return NET_EH::makeRaw( this, ALLOC_POOL ) ;
      }

      INT32 _enableMsgConvertor() ;

   protected:
      ossSpinXLatch           _mtx ;
      NET_HANDLE              _handle ;
      MsgRouteID              _id ;
      volatile BOOLEAN        _isConnected ;
      volatile BOOLEAN        _isNew ;
      UINT64                  _msgid ;
      UINT64                  _lastSendTick ;
      UINT64                  _lastRecvTick ;
      UINT64                  _lastBeatTick ;
      UINT64                  _lastStatTick ;
      UINT64                  _totalIOTimes ;
      UINT32                  _iops ;     /// times/s
      SDB_PROTOCOL_VERSION    _peerVersion ;
      IMsgConvertor           *_inMsgConvertor ;
      IMsgConvertor           *_outMsgConvertor ;
   } ;

}

#endif // NET_EVENT_HANDLER_BASE_HPP_
