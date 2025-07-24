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

   Source File Name = netInnerTimer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft
          09/01/2019  HGM Moved from netFrame.cpp

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "netInnerTimer.hpp"
#include "netFrame.hpp"

namespace engine
{

   #define NET_RESTART_TIMER_INTERVAL        ( 2000 )
   #define NET_DUMMY_TIMER_INTERVAL          ( 2147483647 )
   #define NET_UDP_CLEAR_EH_TIMER_INTERVAL   ( 60000 )

   /*
      _netRestartTimer implement
    */
   _netRestartTimer::_netRestartTimer( netFrame *frame )
   : _frame( frame ),
     _timerID( NET_INVALID_TIMER_ID ),
     _dummyTimerID( NET_INVALID_TIMER_ID )
   {
   }

   _netRestartTimer::~_netRestartTimer ()
   {
   }

   void _netRestartTimer::handleTimeout( const UINT32 &millisec,
                                         const UINT32 &id )
   {
      INT32 rc = SDB_OK ;

      if ( _timerID == id )
      {
         rc = _frame->_listenTCP( _hostName.c_str(), _svcName.c_str() ) ;
         if ( SDB_OK == rc || SDB_NET_ALREADY_LISTENED == rc )
         {
            _frame->removeTimer( _timerID ) ;
            PD_LOG( PDEVENT, "Restart listening TCP on %s:%s succeed",
                    _hostName.c_str(), _svcName.c_str() ) ;
         }
      }
   }

   void _netRestartTimer::setInfo( const CHAR *hostName,
                                   const CHAR *serviceName )
   {
      if ( _hostName.empty() )
      {
         _hostName = hostName ;
      }
      if ( _svcName.empty() )
      {
         _svcName = serviceName ;
      }
   }

   void _netRestartTimer::startTimer()
   {
      INT32 rc = _frame->addTimer( NET_RESTART_TIMER_INTERVAL, this,
                                   _timerID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDSEVERE, "Restore listen error when open files up to "
                 "limit, stop network, rc: %d", rc ) ;
         _frame->stop() ;
      }
   }

   INT32 _netRestartTimer::startDummyTimer()
   {
      return _frame->addTimer( NET_DUMMY_TIMER_INTERVAL, this, _dummyTimerID ) ;
   }

   void _netRestartTimer::stopDummyTimer()
   {
      if ( NET_INVALID_TIMER_ID != _dummyTimerID && _frame )
      {
         _frame->removeTimer( _dummyTimerID ) ;
      }
      _dummyTimerID = NET_INVALID_TIMER_ID ;
   }

   /*
      _netUDPRestartTimer implement
    */
   _netUDPRestartTimer::_netUDPRestartTimer( netFrame *frame )
   : _netRestartTimer( frame ),
     _bufferSize( NET_UDP_DEFAULT_BUFFER_SIZE )
   {
   }

   _netUDPRestartTimer::~_netUDPRestartTimer()
   {
   }

   void _netUDPRestartTimer::handleTimeout( const UINT32 &millisec,
                                            const UINT32 &id )
   {
      INT32 rc = SDB_OK ;

      if ( _timerID == id )
      {
         rc = _frame->_listenUDP( _hostName.c_str(),
                                  _svcName.c_str(),
                                  _bufferSize ) ;
         if ( SDB_OK == rc || SDB_NET_ALREADY_LISTENED == rc )
         {
            _frame->removeTimer( _timerID ) ;
            PD_LOG( PDEVENT, "Restart listening UDP on %s:%s succeed",
                    _hostName.c_str(), _svcName.c_str() ) ;
         }
      }
   }

   void _netUDPRestartTimer::setInfo( const CHAR *hostName,
                                      const CHAR *serviceName,
                                      UINT32 bufferSize )
   {
      _netRestartTimer::setInfo( hostName, serviceName ) ;
      _bufferSize = bufferSize ;
   }

}
