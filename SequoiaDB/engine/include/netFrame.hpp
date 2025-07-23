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

   Source File Name = netFrame.hpp

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
#ifndef NETFRAME_HPP_
#define NETFRAME_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "netEventSuit.hpp"
#include "netEventHandler.hpp"
#include "netTimer.hpp"
#include "ossAtomic.hpp"
#include "sdbInterface.hpp"

#include <map>
#include <vector>

using namespace std ;
namespace engine
{
   class _netMsgHandler ;
   class _netFrame ;
   class _netRoute ;

   typedef INT32 (*NET_START_THREAD_FUNC)( _netEventSuit *pSuit ) ;

   /*
      _netInnerTimeHandle define
   */
   class _netInnerTimeHandle : public _netTimeoutHandler
   {
      public:
         _netInnerTimeHandle( _netFrame *pFrame ) ;
         virtual ~_netInnerTimeHandle() ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      public:
         void         setInfo( const CHAR *pHostName,
                               const CHAR *pSvcName ) ;

         void         startTimer() ;
         INT32        startDummyTimer() ;

      private:
         _netFrame            *_pFrame ;
         UINT32               _timeID ;
         UINT32               _dummyTimerID ;
         string               _hostName ;
         string               _svcName ;
   } ;
   typedef _netInnerTimeHandle netInnerTimeHandle ;

   #define NET_HEARTBEAT_INTERVAL            ( 5000 )
   #define NET_MAKE_STAT_INTERVAL            ( 5000 )

   /*
     _netEHSegment define
     This class is only used by netFrame as the container/manager of
     netEventHandler
   */
   class _netEHSegment : public SDBObject
   {
      typedef vector<NET_EH> VEC_EH ;
      typedef vector<NET_EH>::iterator VEC_EH_IT ;

      friend class _netFrame ;

      public:
         _netEHSegment( _netFrame *pFrame, UINT32 capacity, const _MsgRouteID &id ) ;
         ~_netEHSegment() ;

      public:
         INT32 getEH( NET_EH &eh ) ;

         void close() ;

         void addEH( NET_EH eh ) ;

         void delEH( const NET_HANDLE& handle ) ;

         OSS_INLINE BOOLEAN isEmpty()
         {
            _mtx.get_shared() ;
            BOOLEAN isEmpty = (_vecEH.size() == 0) ? TRUE : FALSE ;
            _mtx.release_shared() ;
            return isEmpty ;
         }

      private:
         BOOLEAN _createEH( NET_EH &eh) ;

      private:
         _netFrame                        *_pFrame ;
         _MsgRouteID                      _id ;
         //total allowed items in the container, is based on config parameter
         UINT32                           _capacity ;
         ossAtomic32                      _index ; // point to a proper item
         ossSpinSLatch                    _mtx ;
         VEC_EH                           _vecEH ; // EV handle list
   } ;

   typedef boost::shared_ptr<_netEHSegment>  netEHSegPtr ;


   /*
      _netFrame define
   */
   class _netFrame : public IIOService
   {
      typedef map<UINT32, NET_TH>         MAP_TIMMER ;
      typedef MAP_TIMMER::iterator        MAP_TIMMER_IT ;

      typedef vector<netEvSuitPtr>        VEC_EVSUIT ;
      typedef VEC_EVSUIT::iterator        VEC_EVSUIT_IT ;

      typedef map<NET_HANDLE, NET_EH>     MAP_EVENT ;
      typedef MAP_EVENT::iterator         MAP_EVENT_IT ;

      typedef map<UINT64, netEHSegPtr>    MAP_ROUTE ;
      typedef MAP_ROUTE::iterator         MAP_ROUTE_IT ;
      typedef pair<MAP_ROUTE_IT, MAP_ROUTE_IT>  MULMAP_ROUTE_IT_PAIR ;

      typedef vector<NET_EH> VEC_EH ;
      typedef vector<NET_EH>::iterator VEC_EH_IT ;

      friend class _netInnerTimeHandle ;
      friend class _netEventHandler ;
      friend class _netEHSegment ;

      public:
         /// handler will not be freed by frame
         _netFrame( _netMsgHandler *handler, _netRoute *pRoute ) ;

         ~_netFrame() ;

      public:
         virtual INT32     run() ;
         virtual void      stop() ;
         virtual void      resetMon() ;

      public:
         OSS_INLINE void setLocal( const MsgRouteID &id )
         {
            _local = id ;
         }

         OSS_INLINE const MsgRouteID& getLocal() const
         {
            return _local ;
         }

         void     setMaxSockPerNode( UINT32 maxSockPerNode ) ;
         void     setMaxSockPerThread( UINT32 maxSockPerThread ) ;
         void     setMaxThreadNum( UINT32 maxThreadNum ) ;

         UINT32   getSockNumByNode( const _MsgRouteID &nodeID ) ;
         UINT32   getSockNum() ;
         UINT32   getNodeNum() ;
         UINT32   getThreadNum() ;

      public:
         void     onRunSuitStart( netEvSuitPtr evSuitPtr ) ;
         void     onRunSuitStop( netEvSuitPtr evSuitPtr ) ;
         void     onSuitTimer( netEvSuitPtr evSuitPtr ) ;
         UINT32   getEvSuitSize() ;

      public:
         // return 0 if error happened
         static UINT32 getCurrentLocalAddress() ;
         static UINT32 getLocalAddress() ;

      public:
         void     heartbeat( UINT32 interval, INT32 serviceType = -1 ) ;

         void     setBeatInfo( UINT32 beatTimeout,
                               UINT32 beatInteval = 0 ) ;

         NET_EH   getEventHandle( const NET_HANDLE &handle ) ;

         INT32    listen( const CHAR *hostName,
                          const CHAR *serviceName ) ;

         /// if call this func with same params for twice,
         /// will create two connections.
         /// the connection will be maintained until the
         /// disconnect happens.
         INT32 syncConnect( const CHAR *hostName,
                            const CHAR *serviceName,
                            const _MsgRouteID &id,
                            NET_HANDLE *pHandle = NULL ) ;

         INT32 syncConnect( const _MsgRouteID &id,
                            NET_HANDLE *pHandle = NULL ) ;

         INT32 syncConnect( NET_EH &eh ) ;

         INT32 syncSend( const NET_HANDLE &handle,
                         void *header ) ;

         INT32 syncSendRaw( const NET_HANDLE &handle,
                            const CHAR *pBuff,
                            UINT32 buffSize ) ;

         INT32 syncSend( const _MsgRouteID &id,
                         void *header,
                         NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSend( const NET_HANDLE &handle,
                         MsgHeader *header,
                         const void *body,
                         UINT32 bodyLen ) ;

         INT32 syncSend( const _MsgRouteID &id,
                         MsgHeader *header,
                         const void *body,
                         UINT32 bodyLen,
                         NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSendv( const _MsgRouteID &id,
                          MsgHeader *header,
                          const netIOVec &iov,
                          NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSendv( const NET_HANDLE &handle,
                          MsgHeader *header,
                          const netIOVec &iov ) ;

         /// frame will not release handler for ever
         INT32 addTimer( UINT32 millsec, _netTimeoutHandler *handler,
                         UINT32 &timerid );

         INT32 removeTimer( UINT32 timerid ) ;

         void  close( const _MsgRouteID &id ) ;

         void  close( const NET_HANDLE &handle, MsgRouteID *pID = NULL ) ;

         void  close() ;

         INT32 closeListen() ;

         void  handleMsg( NET_EH eh ) ;

         void  handleClose( NET_EH eh, _MsgRouteID id ) ;

         INT64 netIn() ;

         INT64 netOut() ;

         void  makeStat( UINT32 timeout ) ;
         void  setNetStartThreadFunc( NET_START_THREAD_FUNC pFunc ) ;

      protected:
         netEvSuitPtr      _getEvSuit( BOOLEAN needLock ) ;
         void              _stopAllEvSuit() ;
         void              _eraseSuit_i( netEvSuitPtr &ptr ) ;

         INT32             _getHandle( const _MsgRouteID &id,
                                       NET_EH &eh ) ;

         NET_EH            _createEvHandler() ;

         void              _addOpposite( NET_EH eh ) ;

      private:
         INT32    _asyncAccept() ;
         void     _acceptCallback( NET_EH eh,
                                   const boost::system::error_code &error ) ;

         void     _erase( const NET_HANDLE &handle ) ;

         void     _addRoute( NET_EH eh ) ;

         void     _heartbeat( INT32 serviceType ) ;

         void     _checkBreak( UINT32 timeout, INT32 serviceType ) ;

      private:
         _netRoute                        *_pRoute ;
         netEvSuitPtr                     _mainSuitPtr ;
         NET_START_THREAD_FUNC            _pThreadFunc ;

         _ossSpinSLatch                   _suiteMtx ;
         VEC_EVSUIT                       _vecEvSuit ;

         MAP_ROUTE                        _route ;
         MAP_EVENT                        _opposite ;

         MAP_TIMMER                       _timers ;

         _netMsgHandler                   *_handler ;
         MsgRouteID                       _local ;
         _ossSpinSLatch                   _mtx ;
         boost::asio::ip::tcp::acceptor   _acceptor ;
         _ossAtomic32                     _handle ;
         UINT32                           _timerID;
         ossAtomicSigned64                _netOut;
         ossAtomicSigned64                _netIn;

         UINT32                           _beatInterval ;
         UINT32                           _beatTimeout ;
         UINT64                           _beatLastTick ;
         BOOLEAN                          _checkBeat ;

         netInnerTimeHandle               _innerTimeHandle ;

         /// communicate shedule config info
         UINT32                           _statInterval ;
         UINT64                           _statLastTick ;
         UINT32                           _maxSockPerNode ;    /// 0 for unlimited
         UINT32                           _maxSockPerThread ;  /// 0 for unlimited
         UINT32                           _maxThreadNum ;

         ossRWMutex                       _suiteExitMutex ;
         BOOLEAN                          _suiteStopFlag ;
   } ;

}

#endif

