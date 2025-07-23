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
#include "netFrame.hpp"
#include "ossMem.hpp"
#include "pmdEnv.hpp"
#include "msgDef.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "netTrace.hpp"
#include "msgMessageFormat.hpp"
#include "netEventSuit.hpp"
#include <boost/bind.hpp>
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
     _iops( 0 )
   {
      _id.value = MSG_INVALID_ROUTEID ;
   }

   _netEventHandlerBase::~_netEventHandlerBase()
   {
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

}
