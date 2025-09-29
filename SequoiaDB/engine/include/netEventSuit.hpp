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

   Source File Name = netEventSuit.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/07/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef NET_EVENT_SUIT_HPP__
#define NET_EVENT_SUIT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "netTimer.hpp"
#include "ossAtomic.hpp"
#include "ossRWMutex.hpp"
#include "ossEvent.hpp"
#include "sdbInterface.hpp"
#include "utilPooledObject.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include <set>

using namespace boost::asio ;
using namespace std ;
namespace engine
{

   class _netFrame ;

   /*
      _netEventSuit define
   */
   class _netEventSuit : public _IIOService, public utilPooledObject
   {
      public:
         typedef set<NET_HANDLE>                SET_HANDLE ;
         typedef SET_HANDLE::iterator           SET_HANDLE_IT ;

      public:
         _netEventSuit( _netFrame *pFrame ) ;
         virtual ~_netEventSuit() ;

         static netEvSuitPtr createShared( _netFrame *frame ) ;

         io_service&    getIOService() ;
         _netFrame*     getFrame() { return _pFrame ; }

         INT32          addHandle( const NET_HANDLE &handle ) ;
         void           delHandle( const NET_HANDLE &handle ) ;
         BOOLEAN        exist( const NET_HANDLE &handle ) ;
         INT32          getHandles( SET_HANDLE &setHandle ) ;
         NET_HANDLE     getNextHandle( NET_HANDLE curHandle ) ;
         void           removeAllEH() ;
         UINT32         getHandleNum() ;

         // check if the net suit is stopped
         BOOLEAN        isStoppped() const
         {
            return _stopped ;
         }

         // mark the net suit is stopped
         void           setStopped()
         {
            _stopped = TRUE ;
         }

      public:
         virtual INT32     run() ;
         virtual void      stop() ;
         virtual void      resetMon() {}

      protected:
         void           _asyncWait() ;
         void           _timeoutCallback( const boost::system::error_code &error ) ;

         OSS_INLINE netEvSuitPtr _getShared()
         {
            return netEvSuitPtr::makeRaw( this, ALLOC_POOL ) ;
         }

      private:

         io_service                       _ioservice ;
         _netFrame                        *_pFrame ;

         SET_HANDLE                       _setHandle ;
         ossRWMutex                       _rwMutex ;

         BOOLEAN                          _active ;
         BOOLEAN                          _stopped ;
         UINT32                           _noAttachTimeout ;

         /// timer
         boost::asio::steady_timer        _timer;

   } ;
   typedef _netEventSuit netEventSuit ;

}

#endif // NET_EVENT_SUIT_HPP__

