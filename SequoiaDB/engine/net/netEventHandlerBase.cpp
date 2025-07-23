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

   Source File Name = netEventHandlerBase.cpp

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
#include "core.hpp"
#include "netEventHandlerBase.hpp"
#include "ossMem.hpp"
#include "pmdEnv.hpp"
#include "pdTrace.hpp"
#include "msgConvertorImpl.hpp"
#if defined (_WINDOWS)
#include <mstcpip.h>
#endif

using namespace boost::asio::ip ;
using namespace std ;

namespace engine
{

   #define NET_STAT_CLEAR_INTERVAL ( 120 * OSS_ONE_SEC )

   /*
      _netEventHandlerBase implement
    */
   _netEventHandlerBase::_netEventHandlerBase( const NET_HANDLE &handle )
   : _handle( handle ),
     _isConnected( FALSE ),
     _isNew( TRUE ),
     _msgid( 0 ),
     _lastSendTick( pmdGetDBTick() ),
     _lastRecvTick( _lastSendTick ),
     _lastBeatTick( _lastSendTick ),
     _lastStatTick( _lastSendTick ),
     _totalIOTimes( 0 ),
     _iops( 0 ),
     _peerVersion( SDB_PROTOCOL_VER_INVALID ),
     _inMsgConvertor( NULL ),
     _outMsgConvertor( NULL )
   {
      _id.value = MSG_INVALID_ROUTEID ;
   }

   _netEventHandlerBase::~_netEventHandlerBase()
   {
      if ( _inMsgConvertor )
      {
         SDB_OSS_DEL _inMsgConvertor ;
         _inMsgConvertor = NULL ;
      }
      if ( _outMsgConvertor )
      {
         SDB_OSS_DEL _outMsgConvertor ;
         _outMsgConvertor = NULL ;
      }
   }

   void _netEventHandlerBase::syncLastBeatTick()
   {
      _lastBeatTick = pmdGetDBTick() ;
   }

   void _netEventHandlerBase::makeStat( UINT64 curTick )
   {
      UINT64 spanTime = pmdDBTickSpan2Time( curTick - _lastStatTick ) ;
      if ( spanTime > 0 )
      {
         _iops = _totalIOTimes / spanTime ;

         if ( spanTime >= NET_STAT_CLEAR_INTERVAL )
         {
            _lastStatTick = curTick ;
            _totalIOTimes = 0 ;
         }
      }
   }

   INT32 _netEventHandlerBase::_enableMsgConvertor()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_PROTOCOL_VER_1 != _peerVersion )
      {
         SDB_ASSERT( FALSE, "Version is not 1" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !_inMsgConvertor )
      {
         _inMsgConvertor = SDB_OSS_NEW msgConvertorImpl ;
         if ( !_inMsgConvertor )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to build a input msg convertor, rc: %d", rc ) ;
            goto error ;
         }
      }

      if ( !_outMsgConvertor )
      {
         _outMsgConvertor = SDB_OSS_NEW msgConvertorImpl ;
         if ( !_outMsgConvertor )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to build a output msg convertor, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( _inMsgConvertor ) ;
      SAFE_OSS_DELETE( _outMsgConvertor ) ;
      goto done ;
   }

   IMsgConvertor *_netEventHandlerBase::getInMsgConvertor()
   {
      return _inMsgConvertor ;
   }

   IMsgConvertor *_netEventHandlerBase::getOutMsgConvertor()
   {
      return _outMsgConvertor ;
   }
}
