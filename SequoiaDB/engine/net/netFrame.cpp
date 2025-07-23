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

   Source File Name = netFrame.cpp

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
#include "core.hpp"
#include "netFrame.hpp"
#include "netMsgHandler.hpp"
#include "msgDef.h"
#include "pmdEnv.hpp"
#include "pd.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "netTrace.hpp"
#include "netRoute.hpp"
#include <boost/bind.hpp>

using namespace boost::asio::ip ;

namespace engine
{
   #define NET_INNER_TIMER_INTERVAL       ( 2000 )
   #define NET_DUMMY_TIMER_INTERVAL       ( 2147483647 )

   #define NET_IOPS_MIN_VALUE             ( 500 )
   #define NET_IOPS_THRESHOLD             ( 5000 )

   /*
      _netInnerTimeHandle implement
   */
   _netInnerTimeHandle::_netInnerTimeHandle( _netFrame *pFrame )
   {
      _pFrame = pFrame ;
      _timeID = 0 ;
      _dummyTimerID = 0 ;
   }

   _netInnerTimeHandle::~_netInnerTimeHandle()
   {
   }

   void _netInnerTimeHandle::handleTimeout( const UINT32 &millisec,
                                            const UINT32 &id )
   {
      INT32 rc = SDB_OK ;

      if ( _timeID == id )
      {
         rc = _pFrame->listen( _hostName.c_str(), _svcName.c_str() ) ;
         if ( SDB_OK == rc || SDB_NET_ALREADY_LISTENED == rc )
         {
            _pFrame->removeTimer( _timeID ) ;
            PD_LOG( PDEVENT, "Restart listening on %s:%s succeed",
                    _hostName.c_str(), _svcName.c_str() ) ;
         }
      }
   }

   void _netInnerTimeHandle::setInfo( const CHAR *pHostName,
                                      const CHAR *pSvcName )
   {
      if ( _hostName.empty() )
      {
         _hostName = pHostName ;
      }
      if ( _svcName.empty() )
      {
         _svcName = pSvcName ;
      }
   }

   void _netInnerTimeHandle::startTimer()
   {
      INT32 rc = _pFrame->addTimer( NET_INNER_TIMER_INTERVAL,
                                    this, _timeID ) ;
      if ( rc )
      {
         PD_LOG( PDSEVERE, "Restore listen error when open files upto "
                 "limit, stop network, rc: %d", rc ) ;
         _pFrame->stop() ;
      }
   }

   INT32 _netInnerTimeHandle::startDummyTimer()
   {
      return _pFrame->addTimer( NET_DUMMY_TIMER_INTERVAL,
                                this, _dummyTimerID ) ;
   }

   /*
     _netEHSegment implement
   */
   _netEHSegment::_netEHSegment( _netFrame *pFrame, UINT32 capacity, const _MsgRouteID &id )
    :_pFrame( pFrame ),
    _id ( id ),
    _index( 0 )
   {
      if ( MSG_ROUTE_SHARD_SERVCIE != id.columns.serviceID ||
           id.columns.groupID < DATA_GROUP_ID_BEGIN ||
           id.columns.groupID > DATA_GROUP_ID_END )
      {
         _capacity = 1 ;
      }
      else
      {
         _capacity = ( capacity > 0) ? capacity : 1 ;
      }
   }

   _netEHSegment::~_netEHSegment()
   {
      _vecEH.clear() ;
   }

   // return an event handler to the caller
   // Algorithm: During start time when # of socket is smaller than capacity,
   // keeping creating it once a time, when this function is called until we hit
   // the capacity. After we hit the capacity, do a round robin.
   INT32 _netEHSegment::getEH( NET_EH &eh )
   {
      INT32 rc = SDB_OK ;
      UINT32 retries = 0 ;
   retry:
      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      if ( _vecEH.size() >= _capacity || retries == 2 )
      {
         if ( _vecEH.size() == 0 )
         {
            PD_LOG( PDSEVERE, "cannot create any net event handler" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         else
         {
            eh = _vecEH[_index.inc() % _vecEH.size()] ;
         }
      }
      }

      if ( NULL == eh.get() )
      {
         BOOLEAN created = _createEH( eh ) ;
         if ( !eh.get() )
         {
            retries++ ;
            goto retry ;
         }
         else if ( created )
         {
            // add to opposite map
            rc = _pFrame->_addOpposite( eh ) ;
            if ( SDB_OK != rc )
            {
               delEH( eh->handle() ) ;
               eh->close() ;
               PD_LOG( PDERROR, "Failed to save handle, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // creating the netEventHandler. We have to get the
   // size of created event handler to make sure after
   // we get x latch, there is still room to create a
   // new one
   BOOLEAN _netEHSegment::_createEH( NET_EH &eh )
   {
      BOOLEAN ret = FALSE ;

      _netEventHandler *pEH = NULL ;
      {
      ossScopedLock lock( &_mtx, EXCLUSIVE ) ;
      if ( _vecEH.size() < _capacity )
      {
         NET_EH tmpEH ;

         /// create a new socket
         pEH = SDB_OSS_NEW _netEventHandler( _pFrame->_getEvSuit( TRUE ),
                                             _pFrame->_handle.inc() ) ;
         if ( !pEH )
         {
            PD_LOG( PDERROR, "Allocate netEventHandler failed" ) ;
            goto done ;
         }

         // boost shared pointer may throw exception
         try
         {
            tmpEH = NET_EH( pEH ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to create shared pointer for net "
                    "event handler, occur exception %s", e.what() ) ;
            goto done ;
         }
         // handle to shared pointer
         pEH = NULL ;

         try
         {
            _vecEH.push_back( tmpEH ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add event handler, occur exception %s",
                    e.what() ) ;
            tmpEH->close() ;
            goto done ;
         }

         eh = tmpEH ;
         eh->id( _id ) ;

         ret = TRUE ;
      }
      else
      {
         eh = _vecEH[_index.inc() % _vecEH.size() ] ;
      }
      }
   done:
      if ( NULL != pEH )
      {
         SDB_OSS_DEL pEH ;
      }
      return ret ;
   }

   void _netEHSegment::close()
   {
      ossScopedLock lock( &_mtx, SHARED ) ;
      for ( VEC_EH_IT itr=_vecEH.begin(); itr!=_vecEH.end(); ++itr )
      {
         (*itr)->close() ;
      }
   }

   // This function is called when a connection is passively
   // created and it needs to be added into netEHSegment,
   // otherwise the connection will be lost. Therefore, it is possible
   // that the new connection has to be added into a container that has
   // already hit the capacity, in that case we will resize the container
   // by increasing the capacity
   INT32 _netEHSegment::addEH( NET_EH eh )
   {
      INT32 rc = SDB_OK ;

      try
      {
         ossScopedLock _lock( &_mtx, EXCLUSIVE ) ;
         _vecEH.push_back(eh) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save event handler to segment, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   void _netEHSegment::delEH( const NET_HANDLE& handle )
   {
      ossScopedLock lock( &_mtx, EXCLUSIVE ) ;
      for ( VEC_EH_IT itr=_vecEH.begin(); itr!=_vecEH.end(); ++itr )
      {
         if ( handle == (*itr)->handle() )
         {
            _vecEH.erase(itr) ;
            break ;
         }
      }
   }

   /// define listen host
   #define NET_LISTEN_HOST          "0.0.0.0"

   /*
      _netFrame implement
   */
   _netFrame::_netFrame( _netMsgHandler *handler,
                         _netRoute *pRoute,
                         const NET_HANDLE &beginID )
   :_pRoute( pRoute ),
    _mainSuitPtr( SDB_OSS_NEW netEventSuit( this ) ),
    _handler( handler ),
    _acceptor( _mainSuitPtr->getIOService() ),
    _handle( beginID ),
    _timerID( NET_INVALID_TIMER_ID ),
    _netOut( 0 ),
    _netIn( 0 ),
    _innerTimeHandle( this ),
    _suiteStopFlag( FALSE )
   {
      _pThreadFunc = NULL ;
      _local.value = MSG_INVALID_ROUTEID ;
      _beatInterval = NET_HEARTBEAT_INTERVAL ;
      _beatPassiveInterval = NET_HEARTBEAT_PASSIVE_INTERVAL ;
      _beatTimeout = 0 ;
      _beatLastTick = pmdGetDBTick() ;
      _beatPassiveLastTick = _beatLastTick ;
      _checkBeat = FALSE ;

      _statInterval = NET_MAKE_STAT_INTERVAL ;
      _statLastTick = 0 ;

      _maxSockPerNode = 1 ;
      _maxSockPerThread = 0 ;
      _maxThreadNum = 0 ;

      SDB_ASSERT( _handle.peek() != NET_INVALID_HANDLE,
                  "Invalid begin net handle" ) ;
      if ( NET_INVALID_HANDLE == _handle.peek() )
      {
         _handle.init( NET_MIN_HANDLE ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_DECONS, "_netFrame::~_netFrame" )
   _netFrame::~_netFrame()
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_DECONS );
      stop() ;
      _route.clear() ;
      _timers.clear() ;
      _opposite.clear() ;
      PD_TRACE_EXIT ( SDB__NETFRAME_DECONS );
   }

   void _netFrame::setMaxSockPerNode( UINT32 maxSockPerNode )
   {
      _maxSockPerNode = maxSockPerNode ;
   }

   void _netFrame::setMaxSockPerThread( UINT32 maxSockPerThread )
   {
      _maxSockPerThread = maxSockPerThread ;
   }

   void _netFrame::setMaxThreadNum( UINT32 maxThreadNum )
   {
      if ( maxThreadNum >= 1 )
      {
         _maxThreadNum = maxThreadNum - 1 ;
      }
      else
      {
         _maxThreadNum = 0 ;
      }
   }

   void _netFrame::onRunSuitStart( netEvSuitPtr evSuitPtr )
   {
      /// do nothing
   }

   void _netFrame::onRunSuitStop( netEvSuitPtr evSuitPtr )
   {
      /// make sure all the netEventHandles have closed
      {
         ossScopedLock lock( &_suiteMtx, EXCLUSIVE ) ;
         _eraseSuit_i( evSuitPtr ) ;
      }

      _netEventSuit::SET_HANDLE setHandles ;

      if ( SDB_OK == evSuitPtr->getHandles( setHandles ) )
      {
         // copy set of handles succeed, just iterate each handle
         _netEventSuit::SET_HANDLE_IT itr = setHandles.begin() ;
         while( itr != setHandles.end() )
         {
            _closeHandle( *itr ) ;
            ++itr ;
         }
      }
      else
      {
         // copy set of handles failed, get handle one by one
         NET_HANDLE curHandle = NET_INVALID_HANDLE ;
         while ( TRUE )
         {
            curHandle = evSuitPtr->getNextHandle( curHandle ) ;
            if ( NET_INVALID_HANDLE == curHandle )
            {
               break ;
            }
            _closeHandle( curHandle ) ;
         }
      }
      // make sure event handlers are released
      // NOTE: handler has shared pointer of event suit, if we
      //       don't release handlers, the event suit will not be
      //       released
      evSuitPtr->removeAllEH() ;
   }

   void _netFrame::_closeHandle( NET_HANDLE handle )
   {
      MsgRouteID nodeID ;
      close( handle, &nodeID ) ;
      if ( MSG_INVALID_ROUTEID != nodeID.value )
      {
         try
         {
            _handler->handleClose( handle, nodeID ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to close handle [%u], "
                    "occur exception %s", handle, e.what() ) ;
         }
      }
   }

   void _netFrame::onSuitTimer( netEvSuitPtr evSuitPtr )
   {
      ossScopedLock lock( &_suiteMtx, EXCLUSIVE ) ;

      if ( 0 == evSuitPtr->getHandleNum() )
      {
         /// stop the suit and remove it
         evSuitPtr->stop() ;
         _eraseSuit_i( evSuitPtr ) ;
      }
   }

   UINT32 _netFrame::getEvSuitSize()
   {
      ossScopedLock lock( &_suiteMtx, SHARED ) ;
      return _vecEvSuit.size() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_RUN, "_netFrame::run" )
   INT32 _netFrame::run()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_RUN ) ;

      INT32 retryCount = 0 ;

      /// start dummy timer
      rc = _innerTimeHandle.startDummyTimer() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start dummy timer failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         /// run main suit ioservice
         _mainSuitPtr->getIOService().run() ;
      }
      catch ( exception &e )
      {
         // main net is down, should restart
         PD_LOG( PDERROR, "Failed to run IO service, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
      }

      /// WARNING: try catch each exceptions of each steps during stop
      /// to make sure each step can tell related sessions and net suits to
      /// stop

      // prepare to stop message handler
      try
      {
         if ( _handler )
         {
            _handler->onPrepareStop() ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to prepare to stop handler, "
                 "occur exception %s", e.what() ) ;
      }

      // stop handles related to this suit
      try
      {
         onRunSuitStop( _mainSuitPtr ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to call on stop suit event, "
                 "occur exception %s", e.what() ) ;
      }

      // stop all sub event suits
      try
      {
         _stopAllEvSuit() ;
         close() ;

         /// wait all evSuit stop
         while( TRUE )
         {
            if ( getEvSuitSize() > 0 )
            {
               ossSleep( 200 ) ;
               // sub-network may be added after quiesced
               // let's retry stop after each second
               ++ retryCount ;
               if ( 0 == retryCount % 5 )
               {
                  _stopAllEvSuit() ;
               }
               continue ;
            }
            break ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to stop all suits, "
                 "occur exception %s", e.what() ) ;
      }

      // stop message handler
      try
      {
         if ( _handler )
         {
            _handler->onStop() ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to call on stop event, "
                 "occur exception %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME_RUN, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_STOP, "_netFrame::stop" )
   void _netFrame::stop()
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_STOP );

      {
         ossScopedLock lock( &_suiteMtx, EXCLUSIVE ) ;
         _suiteStopFlag = TRUE ;
      }

      closeListen() ;
      _mainSuitPtr->getIOService().stop() ;
      PD_TRACE_EXIT ( SDB__NETFRAME_STOP );
   }

   void _netFrame::setNetStartThreadFunc( NET_START_THREAD_FUNC pFunc )
   {
      _pThreadFunc = pFunc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_MAKESTAT, "_netFrame::makeStat" )
   void _netFrame::makeStat( UINT32 timeout )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_MAKESTAT ) ;
      UINT64 span = pmdGetTickSpanTime( _statLastTick ) ;
      NET_EH eh ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_EVENT_IT itr ;

      if ( span >= _statInterval )
      {
         _statLastTick = pmdGetDBTick() ;

         while( TRUE )
         {
            {
            ossScopedLock lock( &_mtx, SHARED ) ;
            itr = _opposite.upper_bound( handle ) ;
            if ( itr == _opposite.end() )
            {
               break ;
            }
            eh = itr->second ;
            handle = itr->first ;
            }

            /// make stat
            eh->makeStat( _statLastTick ) ;
         }
      }

      PD_TRACE_EXIT ( SDB__NETFRAME_MAKESTAT ) ;
   }

   void _netFrame::setBeatInfo( UINT32 beatTimeout, UINT32 beatInteval )
   {
      // beat passive interval will be 1.5x of beat interval
      UINT32 beatPassiveInterval = 0 ;
      if ( beatTimeout > 0 && beatTimeout < 2000 )
      {
         beatTimeout = 2000 ;
         beatInteval = 1000 ;
         beatPassiveInterval = 1500 ;
      }
      if ( 0 == beatInteval )
      {
         beatInteval = beatTimeout / 5 ;
         beatPassiveInterval = beatTimeout / 10 * 3 ;
      }
      if ( beatInteval < 1000 )
      {
         beatInteval = 1000 ;
      }
      if ( beatPassiveInterval < 1500 )
      {
         beatPassiveInterval = 1500 ;
      }
      _beatInterval = beatInteval ;
      _beatPassiveInterval = beatPassiveInterval ;
      _beatTimeout = beatTimeout ;
   }

   void _netFrame::_heartbeat( const netFrameMon &mon )
   {
      MsgHeader beat ;
      NET_EH eh ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_EVENT_IT itr ;

      beat.messageLength = sizeof( MsgHeader ) ;
      beat.opCode = MSG_HEARTBEAT ;
      beat.requestID = 0 ;
      beat.routeID.value = _local.value ;
      beat.TID = 0 ;

      while( TRUE )
      {
         UINT64 lastBeatPassed = 0 ;

         {
         ossScopedLock lock( &_mtx, SHARED ) ;
         itr = _opposite.upper_bound( handle ) ;
         if ( itr == _opposite.end() )
         {
            break ;
         }
         eh = itr->second ;
         handle = itr->first ;
         }

         if ( eh->isNew() )
         {
            continue ;
         }

         /// send msg if had not received message for a while
         lastBeatPassed = pmdGetTickSpanTime( eh->getLastBeatTick() ) ;
         if ( ( ( lastBeatPassed >= _beatInterval ) &&
                ( mon.isInMonitorActive( eh->id().columns.serviceID ) ) ) ||
              ( ( lastBeatPassed >= _beatPassiveInterval ) &&
                ( mon.isInMonitorPassive( eh->id().columns.serviceID ) ) ) )
         {
            ossScopedLock lock( &( eh->mtx() ) ) ;
            beat.requestID = eh->getAndIncMsgID() ;
            eh->syncSend( &beat, beat.messageLength ) ;
         }
      }
   }

   void _netFrame::_checkBreak( UINT32 timeout, const netFrameMon &mon )
   {
      NET_EH eh ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_EVENT_IT itr ;
      UINT64 spanTime = 0 ;
      MsgRouteID routeid ;

      while( timeout > 0 )
      {
         {
         ossScopedLock lock( &_mtx, SHARED ) ;
         itr = _opposite.upper_bound( handle ) ;
         if ( itr == _opposite.end() )
         {
            break ;
         }
         eh = itr->second ;
         handle = itr->first ;
         }

         if ( eh->isNew() )
         {
            continue ;
         }

         spanTime = pmdGetTickSpanTime( eh->getLastRecvTick() ) ;
         /// check break
         if ( mon.isInMonitor( eh->id().columns.serviceID ) &&
              spanTime >= timeout )
         {
            routeid = eh->id() ;
            PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] is "
                    "broken[BrokenTime: %lld(ms)]",
                    handle, routeID2String( routeid ).c_str(),
                    spanTime ) ;
            eh->close() ;
         }
      }
   }

   void _netFrame::heartbeat( UINT32 interval, const netFrameMon &mon )
   {
      UINT32 beatTimeout = _beatTimeout ;
      UINT64 spanTime = pmdGetTickSpanTime( _beatLastTick ) ;
      UINT64 passiveSpanTime = pmdGetTickSpanTime( _beatPassiveLastTick ) ;

      if ( 0 == beatTimeout )
      {
         return ;
      }

      if ( _checkBeat )
      {
         _checkBeat = FALSE ;
         _checkBreak( beatTimeout, mon ) ;
      }
      else
      {
         if ( mon.hasInMonActive() &&
              spanTime >= _beatInterval )
         {
            _beatLastTick = pmdGetDBTick() ;
            _checkBeat = TRUE ;

            if ( spanTime > 3 * _beatInterval )
            {
               PD_LOG( PDWARNING, "Heartbeat span time[%u] is more than "
                       "interval time[%u], the thread maybe blocked by "
                       "some operations", spanTime, _beatInterval ) ;
            }
         }
         if ( mon.hasInMonPassive() &&
              passiveSpanTime >= _beatPassiveInterval )
         {
            _beatPassiveLastTick = pmdGetDBTick() ;
            _checkBeat = TRUE ;

            if ( passiveSpanTime > 3 * _beatPassiveInterval )
            {
               PD_LOG( PDWARNING, "Heartbeat passive span time[%u] is more than "
                       "passive interval time[%u], the thread maybe blocked by "
                       "some operations", passiveSpanTime, _beatPassiveInterval ) ;
            }
         }

         if ( _checkBeat )
         {
            _heartbeat( mon ) ;
         }
      }
   }

   UINT32 _netFrame::getCurrentLocalAddress()
   {
      UINT32 ip = 0 ;

      try
      {
         boost::asio::io_service io_srv ;
         tcp::resolver resolver( io_srv ) ;
         tcp::resolver::query query( boost::asio::ip::host_name(), "") ;
         tcp::resolver::iterator itr = resolver.resolve( query ) ;
         tcp::resolver::iterator end ;
         for ( ; itr != end; itr++ )
         {
            tcp::endpoint ep = *itr ;
            if ( ep.address().is_v4() )
            {
               ip = ep.address().to_v4().to_ulong() ;
               break ;
            }
         }
      }
      catch ( std::exception& )
      {
         // ignore error
      }

      return ip ;
   }

   UINT32 _netFrame::getLocalAddress()
   {
      static UINT32 ip = _netFrame::getCurrentLocalAddress() ;
      return ip ;
   }

   NET_EH _netFrame::getEventHandle( const NET_HANDLE &handle )
   {
      NET_EH eh ;
      MAP_EVENT_IT itr ;

      ossScopedLock lock( &_mtx, SHARED ) ;

      itr = _opposite.find( handle ) ;
      if ( _opposite.end() != itr )
      {
         eh = itr->second ;
      }

      return eh ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_LISTEN, "_netFrame::listen" )
   INT32 _netFrame::listen( const CHAR *hostName,
                            const CHAR *serviceName )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_LISTEN );

      if ( _acceptor.is_open() )
      {
         rc = SDB_NET_ALREADY_LISTENED ;
         goto error ;
      }

      try
      {
         /// here we bind 0.0.0.0.
         tcp::resolver::query query ( tcp::v4(), NET_LISTEN_HOST, serviceName ) ;
         tcp::resolver resolver ( _mainSuitPtr->getIOService() ) ;
         tcp::resolver::iterator itr = resolver.resolve ( query ) ;
         ip::tcp::endpoint endpoint = *itr ;
         _acceptor.open( endpoint.protocol() ) ;
         _acceptor.set_option(tcp::acceptor::reuse_address(TRUE)) ;
         _acceptor.bind( endpoint ) ;
         _acceptor.listen() ;
      }
      catch ( boost::system::system_error &e )
      {
         PD_LOG ( PDERROR, "Failed to listen on %s:%s, error:%s", hostName,
                  serviceName, e.what() ) ;
         rc = SDB_NET_CANNOT_LISTEN ;
         goto error ;
      }
      /// set info
      _innerTimeHandle.setInfo( hostName, serviceName ) ;

      rc = _asyncAccept() ;
      if ( rc )
      {
         goto error ;
      }

      PD_LOG( PDDEBUG, "listening on port %s", serviceName ) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_LISTEN, rc );
      return rc ;
   error:
      closeListen() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNNCCONN, "_netFrame::syncConnect" )
   INT32 _netFrame::syncConnect( const CHAR *hostName,
                                 const CHAR *serviceName,
                                 const _MsgRouteID &id,
                                 NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNNCCONN );
      _netEventHandler *ev = SDB_OSS_NEW _netEventHandler( _getEvSuit( TRUE ),
                                                           _handle.inc() ) ;
      if ( NULL == ev )
      {
         PD_LOG ( PDERROR, "Failed to malloc mem" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      {
         NET_EH eh( ev ) ;
         rc = eh->syncConnect( hostName, serviceName ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         eh->id( id ) ;

         /// add to map
         // addRoute will take latch inside the function
         rc = _addRoute( eh ) ;
         if ( SDB_OK != rc )
         {
            eh->close() ;
            PD_LOG( PDERROR, "Failed to save route, rc: %d", rc ) ;
            goto error ;
         }

         rc = _addOpposite( eh ) ;
         if ( SDB_OK != rc )
         {
            _eraseRoute( eh ) ;
            eh->close() ;
            PD_LOG( PDERROR, "Failed to save handle, rc: %d", rc ) ;
            goto error ;
         }

         if ( pHandle )
         {
            *pHandle = eh->handle() ;
         }

         // Keep eh->asyncRead after handleConnect callback. As for data source
         // connection, the system information check and authentication is done
         // in the callback. They are done in sync way. So async read should be
         // started after that, otherwise, sysinfo/auth reply message will be
         // caught by the async read, and the sync waiting will get nothing.
         // Refer to _coordDataSourceMsgHandler::_authenticate.
         rc = _handler->handleConnect( eh->handle(), id, TRUE ) ;
         if ( rc )
         {
            _erase( eh->handle() ) ;
            eh->close() ;
            *pHandle = NET_INVALID_HANDLE ;

            PD_LOG( PDERROR, "Handle connected failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = eh->asyncRead() ;
         if ( rc )
         {
            *pHandle = NET_INVALID_HANDLE ;

            PD_LOG( PDERROR, "Async read failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNNCCONN, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netFrame::syncConnect( const _MsgRouteID &id,
                                 NET_HANDLE *pHandle )
   {
      INT32 rc = SDB_OK ;
      CHAR host[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
      CHAR service[ OSS_MAX_SERVICENAME + 1] = { 0 } ;

      rc = _pRoute->route( id, host, OSS_MAX_HOSTNAME,
                           service, OSS_MAX_SERVICENAME ) ;
      if ( SDB_OK == rc )
      {
         rc = syncConnect( host, service, id, pHandle ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNNCCONN2, "_netFrame::syncConnect" )
   INT32 _netFrame::syncConnect( NET_EH &eh )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasConnect = FALSE ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNNCCONN2 ) ;

      {
      ossScopedLock lock( &( eh->mtx() ) ) ;
      if ( !eh->isConnected() )
      {
         if ( eh->isNew() )
         {
            MsgRouteID id = eh->id() ;
            CHAR host[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
            CHAR service[ OSS_MAX_SERVICENAME + 1] = { 0 } ;

            rc = _pRoute->route( id, host, OSS_MAX_HOSTNAME,
                                 service, OSS_MAX_SERVICENAME ) ;
            if ( SDB_OK == rc )
            {
               rc = eh->syncConnect( host, service ) ;
               if ( SDB_OK == rc )
               {
                  rc = eh->asyncRead() ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  hasConnect = TRUE ;
               }
            }

            if ( rc )
            {
               eh->close() ;
               _erase( eh->handle() ) ;
            }
         }
         else
         {
            // make sure the handle is erased
            _erase( eh->handle() ) ;

            rc = SDB_NETWORK ;
         }
      }
      }

      if ( rc )
      {
         goto error ;
      }
      if ( hasConnect )
      {
         // callback: handleConnect
         _handler->handleConnect( eh->handle(), eh->id(), TRUE ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNNCCONN2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // This function is called when a sender needs an Event Handler(socket/connection)
   // to do the sending. The logic here is that it will first seach the appropriate
   // netEHSegment, which contains all the socket that assign to the same route
   // id(i.e. ip address and service). Then it will call getEH to get an
   // event handler. If netEHSegment can not be found, will create one.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__GETHANDLE, "_netFrame::_getHandle" )
   INT32 _netFrame::_getHandle( const _MsgRouteID &id, NET_EH &eh )
   {
      INT32 rc = SDB_OK ;
      MAP_ROUTE_IT itr ;
      netEHSegPtr ptr ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__GETHANDLE ) ;

      // protect exit of sub-network
      ossScopedRWLock scopeLock( &_suiteExitMutex, SHARED ) ;

      PD_CHECK( !_suiteStopFlag, SDB_QUIESCED, error, PDWARNING,
                "Suite service of net frame is stopped" ) ;

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      itr = _route.find(id.value) ;
      if ( itr != _route.end() )
      {
         // if we found the netEHSegment in the route table, just use it
         ptr = itr->second ;
      }
      }

      if ( NULL == ptr.get() )
      {
         ossScopedLock lock( &_mtx, EXCLUSIVE ) ;

         // after we get the x latch, re-check if someone has already create
         // the netEHSegment
         itr = _route.find(id.value) ;
         if ( itr != _route.end())
         {
            ptr = itr->second ;
         }
         else
         {
            // create new netEHSegment
            _netEHSegment *pSeg = SDB_OSS_NEW _netEHSegment( this ,
                                                   _maxSockPerNode, id ) ;
            if ( !pSeg)
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Allocate netEHSegment failed" ) ;
               goto error ;
            }
            ptr = netEHSegPtr(pSeg) ;

            try
            {
               // insert the shared ptr into route table
               _route.insert( make_pair(id.value, ptr) ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save route, occur exception %s",
                       e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
      }

      // get event handler
      rc = ptr->getEH(eh) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME__GETHANDLE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const _MsgRouteID &id,
                              void *header,
                              NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL") ;
      SDB_ASSERT( MSG_INVALID_ROUTEID != id.value,
                  "id.value should not be zero" ) ;
      INT32 rc = SDB_OK ;
      MsgHeader *msgHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND );
      NET_EH eh ;

      rc = _getHandle( id, eh ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !eh->isConnected() )
      {
         rc = syncConnect( eh ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      msgHeader = ( MsgHeader* )header ;
      if ( MSG_INVALID_ROUTEID == msgHeader->routeID.value )
      {
         msgHeader->routeID = _local ;
      }

      {
      ossScopedLock lock( &( eh->mtx() ) ) ;
      rc = eh->syncSend( msgHeader, msgHeader->messageLength ) ;
      if ( pHandle )
      {
         *pHandle = eh->handle() ;
      }
      }
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }
      _netOut.add( msgHeader->messageLength ) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND2, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const NET_HANDLE &handle,
                              void *header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL") ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle,
                  "handle should not be invalid" ) ;
      INT32 rc = SDB_OK ;
      MsgHeader *msgHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND2 );
      NET_EH eh ;
      MAP_EVENT_IT itr ;

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         rc = SDB_NET_INVALID_HANDLE ;
         goto error ;
      }
      eh = itr->second ;
      }

      msgHeader = ( MsgHeader * )header ;
      if ( MSG_INVALID_ROUTEID == msgHeader->routeID.value )
      {
         msgHeader->routeID = _local ;
      }
      {
         ossScopedLock lock( &( eh->mtx() ) ) ;
         rc = eh->syncSend( msgHeader, msgHeader->messageLength ) ;
      }
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }
      _netOut.add( msgHeader->messageLength ) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND2, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netFrame::syncSendRaw( const NET_HANDLE &handle,
                                 const CHAR *pBuff,
                                 UINT32 buffSize )
   {
      SDB_ASSERT( NULL != pBuff, "pBuff should not be NULL") ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle,
                  "handle should not be invalid" ) ;
      INT32 rc = SDB_OK ;
      NET_EH eh ;
      MAP_EVENT_IT itr ;

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         rc = SDB_NET_INVALID_HANDLE ;
         goto error ;
      }
      eh = itr->second ;
      }

      {
         ossScopedLock lock( &( eh->mtx() ) ) ;
         rc = eh->syncSend( pBuff, buffSize ) ;
      }
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }
      _netOut.add( buffSize ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND3, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const NET_HANDLE &handle,
                              MsgHeader *header,
                              const void *body,
                              UINT32 bodyLen )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL") ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle,
                  "handle should not be invalid" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND3 );
      UINT32 headLen = header->messageLength - bodyLen ;
      NET_EH eh ;
      MAP_EVENT_IT itr ;
      UINT32 netOut = 0 ;

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         rc = SDB_NET_INVALID_HANDLE ;
         goto error ;
      }
      eh = itr->second ;
      }

      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }

      {
      ossScopedLock lock( &( eh->mtx() ) ) ;
      /// header len should be computed. can not get sizeof(MsgHeader)
      rc = eh->syncSend( header, headLen ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      netOut += headLen ;

      if ( NULL != body )
      {
         rc = eh->syncSend( body, bodyLen ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         netOut += bodyLen ;
      }
      }

   done:
      if ( netOut > 0 )
      {
         _netOut.add( netOut ) ;
      }
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND3, rc );
      return rc ;
   error:
      if ( NULL != eh.get() )
      {
         eh->close() ;
      }
      goto done ;
   }

   INT32 _netFrame::syncSendv( const NET_HANDLE & handle,
                               MsgHeader *header,
                               const netIOVec & iov )
   {
      SDB_ASSERT( NULL != header, "should not be NULL" ) ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle, "invalid handle" ) ;

      INT32 rc = SDB_OK ;
      NET_EH eh ;
      MAP_EVENT_IT itHandle ;
      UINT32 netOut = 0 ;

      INT32 origLen = header->messageLength ;
      header->messageLength = sizeof( MsgHeader ) + netCalcIOVecSize( iov ) ;
      if ( header->messageLength > SDB_MAX_MSG_LENGTH )
      {
         PD_LOG( PDERROR, "Invalid msg size: %d", header->messageLength ) ;
         rc = SDB_INVALIDSIZE ;
         goto error ;
      }
      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      itHandle = _opposite.find( handle ) ;
      if ( _opposite.end() == itHandle )
      {
         rc = SDB_NET_INVALID_HANDLE ;
         goto error ;
      }
      eh = itHandle->second ;
      }

      {
      ossScopedLock lock( &( eh->mtx() ) ) ;
      rc = eh->syncSend( header, sizeof( MsgHeader ) ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      netOut += sizeof(MsgHeader) ;

      for ( netIOVec::const_iterator itr = iov.begin() ; itr != iov.end();
            ++itr )
      {
         SDB_ASSERT( NULL != itr->iovBase, "should not be NULL" ) ;

         if ( itr->iovBase )
         {
            rc = eh->syncSend( itr->iovBase, itr->iovLen ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            netOut += itr->iovLen ;
         }
      }
      }

   done:
      header->messageLength = origLen ;
      if ( netOut > 0 )
      {
         _netOut.add( netOut ) ;
      }
      return rc ;
   error:
      if ( NULL != eh.get() )
      {
         eh->close() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND4, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const  _MsgRouteID &id,
                              MsgHeader *header,
                              const void *body,
                              UINT32 bodyLen,
                              NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header, "should not be NULL") ;
      SDB_ASSERT( MSG_INVALID_ROUTEID != id.value,
                  "id.value should not be zero" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND4 );
      UINT32 headLen = header->messageLength - bodyLen ;
      NET_EH eh ;
      UINT32 netOut = 0 ;

      rc = _getHandle( id, eh ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !eh->isConnected() )
      {
         rc = syncConnect( eh ) ;
         if ( rc )
         {
            eh.reset() ;
            goto error ;
         }
      }

      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }

      {
      ossScopedLock lock( &( eh->mtx() ) ) ;
      if ( pHandle )
      {
         *pHandle = eh->handle() ;
      }
      rc = eh->syncSend( header, headLen ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      netOut += headLen ;

      if ( NULL != body )
      {
         rc = eh->syncSend( body, bodyLen ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         netOut += bodyLen ;
      }
      }

   done:
      if ( netOut > 0 )
      {
         _netOut.add( netOut ) ;
      }
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND4, rc );
      return rc ;
   error:
      if ( NULL != eh.get() )
      {
         eh->close() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSENDV, "_netFrame::syncSendv" )
   INT32 _netFrame::syncSendv( const _MsgRouteID &id,
                               MsgHeader *header,
                               const netIOVec &iov,
                               NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header, "should not be NULL" ) ;
      SDB_ASSERT( MSG_INVALID_ROUTEID != id.value,
                  "id.value should not be zero" ) ;
      PD_TRACE_ENTRY( SDB__NETFRAME_SYNCSENDV ) ;
      INT32 rc = SDB_OK ;
      NET_EH eh ;
      UINT32 netOut = 0 ;

      INT32 origLen = header->messageLength ;
      header->messageLength = sizeof( MsgHeader ) + netCalcIOVecSize( iov ) ;
      if ( header->messageLength > SDB_MAX_MSG_LENGTH )
      {
         PD_LOG( PDERROR, "Invalid msg size: %d", header->messageLength ) ;
         rc = SDB_INVALIDSIZE ;
         goto error ;
      }
      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }

      rc = _getHandle( id, eh ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !eh->isConnected() )
      {
         rc = syncConnect( eh ) ;
         if ( rc )
         {
            eh.reset() ;
            goto error ;
         }
      }

      {
      ossScopedLock lock( &( eh->mtx() ) ) ;
      if ( pHandle )
      {
         *pHandle = eh->handle() ;
      }
      rc = eh->syncSend( header, sizeof(MsgHeader) ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      netOut += sizeof(MsgHeader) ;

      for ( netIOVec::const_iterator itr = iov.begin() ; itr != iov.end() ;
            ++itr )
      {
         SDB_ASSERT( NULL != itr->iovBase, "should not be NULL" ) ;

         if ( itr->iovBase && itr->iovLen > 0 )
         {
            rc = eh->syncSend( itr->iovBase, itr->iovLen ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            netOut += itr->iovLen ;
         }
      }
      }

   done:
      header->messageLength = origLen ;
      if ( netOut > 0 )
      {
         _netOut.add( netOut ) ;
      }
      PD_TRACE_EXITRC( SDB__NETFRAME_SYNCSENDV, rc ) ;
      return rc ;
   error:
      if ( NULL != eh.get() )
      {
         eh->close() ;
      }
      goto done ;
   }

   // close all connections to one id(i.e. node)
   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_CLOSE, "_netFrame::close" )
   void _netFrame::close( const _MsgRouteID &id )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_CLOSE );

      MAP_ROUTE_IT routeItr ;
      netEHSegPtr ptr ;

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      routeItr = _route.find( id.value ) ;
      // check if the entry with corresponding id exists
      if ( routeItr != _route.end() )
      {
         ptr = routeItr->second ;
      }
      }

      if ( NULL != ptr.get() )
      {
         // retrieve the netEHSegment shared ptr and release
         // s latch for the route table
         // call the netEHSEgment::close interface to close all
         // sockets in the netEHSEgment
         ptr->close() ;
      }

      PD_TRACE_EXIT ( SDB__NETFRAME_CLOSE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_CLOSE2, "_netFrame::close" )
   // close all connections
   void _netFrame::close()
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_CLOSE2 );
      MAP_EVENT_IT itr ;

      // protect exit of sub-network
      ossScopedRWLock scopeLock( &_suiteExitMutex, EXCLUSIVE ) ;

      ossScopedLock lock( &_mtx, SHARED ) ;
      itr = _opposite.begin() ;
      for ( ; itr != _opposite.end(); itr++ )
      {
         itr->second->close() ;
      }

      PD_TRACE_EXIT ( SDB__NETFRAME_CLOSE2 );
      return ;
   }

   INT32 _netFrame::closeListen ()
   {
      try
      {
         if ( _acceptor.is_open() )
         {
            _acceptor.close() ;
         }
      }
      catch( boost::system::system_error &e )
      {
         PD_LOG ( PDERROR, "Close listen occur error: %s,%d",
                  e.what(), e.code().value() ) ;
         return SDB_NETWORK ;
      }

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__NETFRAME_CLOSE3, "_netFrame::close" )
   void _netFrame::close( const NET_HANDLE &handle,
                          MsgRouteID *pID )
   {
      PD_TRACE_ENTRY( SDB__NETFRAME_CLOSE3 ) ;
      MAP_EVENT_IT itr ;
      UINT64 routeID = MSG_INVALID_ROUTEID ;

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() != itr )
      {
         itr->second->close() ;
         routeID = itr->second->id().value ;
      }
      }

      if ( pID )
      {
         if ( MSG_INVALID_ROUTEID == routeID )
         {
            PD_LOG( PDINFO, "invalid net handle:%d", handle ) ;
         }
         pID->value = routeID ;
      }

      PD_TRACE_EXIT( SDB__NETFRAME_CLOSE3 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_ADDTIMER, "_netFrame::addTimer" )
   INT32 _netFrame::addTimer( UINT32 millsec,
                              _netTimeoutHandler *handler,
                              UINT32 &timerid )
   {
      INT32 rc = SDB_OK ;
      _netTimer *t = NULL ;
      timerid = NET_INVALID_TIMER_ID ;
      NET_TH timer ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_ADDTIMER );

      t = SDB_OSS_NEW _netTimer( millsec, ++_timerID,
                                 _mainSuitPtr->getIOService(),
                                 handler ) ;
      if ( !t )
      {
         PD_LOG( PDERROR, "Allocate netTimer failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      timer = NET_TH( t ) ;
      t = NULL ;

      try
      {
         /// lock
         ossScopedLock _lock( &_mtx, EXCLUSIVE ) ;
         _timers.insert( std::make_pair( timer->id(), timer ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save timer, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      timerid = timer->id() ;
      timer->asyncWait() ;

   done:
      if ( t )
      {
         SDB_OSS_DEL t ;
      }
      PD_TRACE_EXITRC ( SDB__NETFRAME_ADDTIMER, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_REMTIMER, "_netFrame::removeTimer" )
   INT32 _netFrame::removeTimer( UINT32 id )
   {
      INT32 rc = SDB_OK ;
      MAP_TIMMER_IT it ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_REMTIMER ) ;

      ossScopedLock lock( &_mtx, EXCLUSIVE ) ;
      it = _timers.find( id ) ;
      if ( _timers.end() == it )
      {
         rc = SDB_NET_TIMER_ID_NOT_FOUND ;
      }
      else
      {
         it->second->cancel() ;
         _timers.erase( it ) ;
      }

      PD_TRACE_EXITRC ( SDB__NETFRAME_REMTIMER, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_HNDMSG, "_netFrame::handleMsg" )
   void _netFrame::handleMsg( NET_EH eh )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_HNDMSG );
      INT32 rc = SDB_OK ;
      MsgHeader *pMsg = (_MsgHeader *)eh->msg() ;

      if ( MSG_HEARTBEAT == pMsg->opCode )
      {
         MsgOpReply reply ;
         reply.header.messageLength = sizeof( MsgOpReply ) ;
         reply.header.opCode = MSG_HEARTBEAT_RES ;
         reply.header.requestID = pMsg->requestID ;
         reply.header.routeID.value = 0 ;
         reply.header.TID = pMsg->TID ;
         reply.contextID = -1 ;
         reply.numReturned = 0 ;
         reply.startFrom = 0 ;
         reply.flags = pmdDBIsAbnormal() ? SDB_SYS : SDB_OK ;

         // try to get lock of event handle
         // if failed, means someone is using the handle to send data
         // which can be just instead of heart beat
         ossScopedTryLock lock( &( eh->mtx() ) ) ;
         if ( lock.isLocked() )
         {
            reply.header.routeID = _local ;
            eh->syncSend( (const void*)&reply, reply.header.messageLength ) ;
         }
      }
      else if ( MSG_HEARTBEAT_RES == pMsg->opCode )
      {
         MsgOpReply *pReply = ( MsgOpReply* )pMsg ;
         if ( SDB_OK != pReply->flags )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] is broken "
                    "because of node is abnormal[%d]",
                    eh->handle(), routeID2String( eh->id() ).c_str(),
                    pReply->flags ) ;
            eh->close() ;
         }
      }
      else
      {
         rc = _handler->handleMsg( eh->handle(), pMsg, eh->msg() ) ;
         _netIn.add( pMsg->messageLength ) ;
         if ( SDB_NET_BROKEN_MSG == rc )
         {
            eh->close() ;
         }
      }
      PD_TRACE1 ( SDB__NETFRAME_HNDMSG, PD_PACK_INT(rc) );
      PD_TRACE_EXIT ( SDB__NETFRAME_HNDMSG );
      return ;
   }

   void _netFrame::handleClose( NET_EH eh, _MsgRouteID id )
   {
      _handler->handleClose( eh->handle(), id ) ;
   }

   //TODO rewrite it later
   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ADDRT, "_netFrame::_addRoute" )
   INT32 _netFrame::_addRoute( NET_EH eh )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__NETFRAME__ADDRT ) ;

      MAP_ROUTE_IT itr ;
      netEHSegPtr ptr ;

      {
      ossScopedLock lock( &_mtx, SHARED ) ;
      itr = _route.find(eh->id().value) ;
      if ( itr != _route.end() )
      {
         // if we found the netEHSegment in the route table, just use it
         ptr = itr->second ;
      }
      }

      if ( NULL == ptr.get() )
      {
         ossScopedLock _lock( &_mtx, EXCLUSIVE ) ;

         // after we get the x latch, re-check if someone has already create
         // the netEHSegment
         itr = _route.find(eh->id().value) ;
         if ( itr != _route.end())
         {
            ptr = itr->second ;
         }
         else
         {
            // create new netEHSegment
            _netEHSegment *pSeg = SDB_OSS_NEW _netEHSegment( this ,
                                                   _maxSockPerNode, eh->id() ) ;
            if ( !pSeg)
            {
               PD_LOG( PDERROR, "Allocate netEHSegment failed" ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            ptr = netEHSegPtr(pSeg) ;

            try
            {
               // insert the shared ptr into route table
               _route.insert( make_pair(eh->id().value, ptr) ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save route, occur exception %s",
                       e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
      }

      // get event handler
      rc = ptr->addEH(eh) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add event handler to event "
                   "handler segment, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME__ADDRT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ERASERT, "_netFrame::_eraseRoute" )
   void _netFrame::_eraseRoute( NET_EH eh )
   {
      PD_TRACE_ENTRY( SDB__NETFRAME__ERASERT ) ;

      ossScopedLock _lock( &_mtx, EXCLUSIVE ) ;

      MAP_ROUTE_IT routeItr = _route.find( eh->id().value ) ;
      if ( routeItr != _route.end() )
      {
         routeItr->second->delEH( eh->handle() ) ;
         /// when nobody used and is empty
         if ( routeItr->second->isEmpty() &&
              1 == routeItr->second.use_count() )
         {
            _route.erase( routeItr ) ;
         }
      }

      PD_TRACE_EXIT( SDB__NETFRAME__ERASERT ) ;
   }

   void _netFrame::_eraseSuit_i( netEvSuitPtr &ptr )
   {
      VEC_EVSUIT_IT itr = _vecEvSuit.begin() ;
      while( itr != _vecEvSuit.end() )
      {
         if ( (*itr).get() == ptr.get() )
         {
            _vecEvSuit.erase( itr ) ;
            break ;
         }
         ++itr ;
      }
   }

   INT32 _netFrame::_addOpposite( NET_EH eh )
   {
      INT32 rc = SDB_OK ;

      try
      {
         ossScopedLock _lock( &_mtx, EXCLUSIVE ) ;
         _opposite.insert( make_pair( eh->handle(), eh ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save handle, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__GETEVSUIT, "_netFrame::_getEvSuit" )
   netEvSuitPtr _netFrame::_getEvSuit( BOOLEAN needLock )
   {
      netEvSuitPtr ptr = _mainSuitPtr ;
      UINT32 minSockNum = ptr->getHandleNum() ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__GETEVSUIT ) ;

      if ( _pThreadFunc && _maxSockPerThread > 0 &&
           minSockNum >= _maxSockPerThread )
      {
         VEC_EVSUIT_IT itr ;
         UINT32 curSockNum = 0 ;

         ossScopedLock _lock( needLock ? &_suiteMtx : NULL, EXCLUSIVE ) ;

         if ( _suiteStopFlag )
         {
            PD_LOG( PDWARNING, "Suite service of net frame is stopped" ) ;
            goto done ;
         }

         itr = _vecEvSuit.begin() ;
         while( itr != _vecEvSuit.end() )
         {
            curSockNum = (*itr)->getHandleNum() ;
            if ( curSockNum < minSockNum )
            {
               minSockNum = curSockNum ;
               ptr = (*itr) ;

               if ( minSockNum < _maxSockPerThread )
               {
                  /// find
                  goto done ;
               }
            }
            ++itr ;
         }

         /// when all suit's socket is >= _maxSockPerThread
         if ( _maxThreadNum > 0 && _vecEvSuit.size() < _maxThreadNum )
         {
            /// create new
            netEventSuit *pSuit = SDB_OSS_NEW netEventSuit( this ) ;
            if ( pSuit )
            {
               netEvSuitPtr tmpPtr ;

               // boost shared pointer may throw exception
               try
               {
                  tmpPtr = netEvSuitPtr( pSuit ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to create shared pointer for "
                          "new suit, occur exception %s", e.what() ) ;
                  SDB_OSS_DEL pSuit ;
                  goto done ;
               }

               try
               {
                  _vecEvSuit.reserve( _vecEvSuit.size() + 1 ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to reserved memory for new suit, "
                          "occur exception %s", e.what() ) ;
                  goto done ;
               }

               /// start thread
               INT32 rc = _pThreadFunc( tmpPtr.get() ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Call _pThreadFunc failed, rc: %d", rc ) ;
                  goto done ;
               }

               // already reserved, no need to try-catch
               _vecEvSuit.push_back( tmpPtr ) ;
               ptr = tmpPtr ;
            }
         }
      }

   done:
      PD_TRACE_EXIT( SDB__NETFRAME__GETEVSUIT ) ;
      return ptr ;
   }

   void _netFrame::_stopAllEvSuit()
   {
      ossScopedLock lock( &_suiteMtx, SHARED ) ;
      for ( UINT32 i = 0 ; i < _vecEvSuit.size() ; ++i )
      {
         _vecEvSuit[ i ]->stop() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ASYNCAPT, "_netFrame::_asyncAccept" )
   INT32 _netFrame::_asyncAccept()
   {
      INT32 rc = SDB_OK ;
      _netEventHandler *pEH = NULL ;
      NET_EH eh ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__ASYNCAPT ) ;

      pEH = SDB_OSS_NEW _netEventHandler( _getEvSuit( TRUE ), _handle.inc() ) ;
      if ( !pEH )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate netEventHandler failed" ) ;
         goto error ;
      }

      // boost shared pointer may throw exception
      try
      {
         eh = NET_EH( pEH ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to create shared pointer for net "
                 "event handler, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
      // handle to shared pointer
      pEH = NULL ;

      _acceptor.async_accept( eh->socket(),
                              boost::bind( &_netFrame::_acceptCallback,
                                           this,
                                           eh,
                                           boost::asio::placeholders::error ) ) ;

   done:
      if ( pEH )
      {
         SDB_OSS_DEL pEH ;
      }
      PD_TRACE_EXITRC( SDB__NETFRAME__ASYNCAPT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__APTCALLBCK, "_netFrame::_acceptCallback" )
   void _netFrame::_acceptCallback( NET_EH eh,
                                    const boost::system::error_code &error )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME__APTCALLBCK );
      if ( error )
      {
         PD_LOG ( PDERROR, "Accept connection occur exception: %s, %d",
                  error.message().c_str(), error.value() ) ;

         if ( boost::system::errc::too_many_files_open == error.value() ||
              boost::system::errc::too_many_files_open_in_system ==
              error.value() )
         {
            closeListen() ;
            PD_LOG( PDERROR, "Can not accept more connections because of "
                    "open files upto limits, restart listening" ) ;
            _innerTimeHandle.startTimer() ;
            pmdIncErrNum( SDB_TOO_MANY_OPEN_FD ) ;
         }

         goto done ;
      }

      eh->setOpt() ;

      /// add to map
      if ( SDB_OK == _addOpposite( eh ) )
      {
         // callback: handleConnect
         _handler->handleConnect( eh->handle(), eh->id(), FALSE ) ;
         eh->asyncRead() ;
      }
      else
      {
         // failed to add to map, close connection
         eh->close() ;
      }

      _asyncAccept() ;

   done:
      PD_TRACE_EXIT ( SDB__NETFRAME__APTCALLBCK ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ERASE, "_netFrame::_erase" )
   void _netFrame::_erase( const NET_HANDLE &handle )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME__ERASE );
      MAP_EVENT_IT itr ;
      MAP_ROUTE_IT routeItr ;

      {
      ossScopedLock lock( &_mtx, EXCLUSIVE ) ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         goto done ;
      }

      routeItr = _route.find( itr->second->id().value ) ;
      if ( routeItr != _route.end() )
      {
         routeItr->second->delEH( handle ) ;
         /// when nobody used and is empty
         if ( routeItr->second->isEmpty() &&
              1 == routeItr->second.use_count() )
         {
            _route.erase(routeItr) ;
         }
      }
      _opposite.erase( itr ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB__NETFRAME__ERASE );
   }

   INT64 _netFrame::netIn()
   {
      return _netIn.peek() ;
   }

   INT64 _netFrame::netOut()
   {
      return _netOut.peek() ;
   }

   void _netFrame::resetMon()
   {
      _netIn.poke( 0 ) ;
      _netOut.poke( 0 ) ;
   }

   /*
      Common function
   */
   UINT32 netCalcIOVecSize( const netIOVec &ioVec )
   {
      UINT32 size = 0 ;
      for ( UINT32 i = 0 ; i < ioVec.size() ; ++i )
      {
         if ( ioVec[ i ].iovBase )
         {
            size += ioVec[ i ].iovLen ;
         }
      }
      return size ;
   }

   NET_NODE_STATUS netResult2Status( INT32 result )
   {
      NET_NODE_STATUS status = NET_NODE_STAT_NORMAL ;

      switch ( result )
      {
         case SDB_CLS_FULL_SYNC:
            status = NET_NODE_STAT_FULLSYNC ;
            break ;
         case SDB_RTN_IN_REBUILD:
            status = NET_NODE_STAT_REBUILD ;
            break ;
         case SDB_RTN_IN_BACKUP:
            status = NET_NODE_STAT_BACKUP ;
            break ;
         case SDB_NETWORK:
         case SDB_NETWORK_CLOSE:
         case SDB_NET_CANNOT_CONNECT:
         case SDB_COORD_REMOTE_DISC:
         case SDB_INVALID_ROUTEID:
         case SDB_TIMEOUT:
         case SDB_DATABASE_DOWN:
            status = NET_NODE_STAT_OFFLINE ;
            break ;
         case SDB_CLS_DATA_NOT_SYNC:
            status = NET_NODE_STAT_DATA_NOT_SYNC ;
            break ;
         default:
            break ;
      }

      return status ;
   }

}

