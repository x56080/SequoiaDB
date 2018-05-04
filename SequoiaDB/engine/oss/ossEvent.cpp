/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ossEvent.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossEvent.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"

namespace engine
{
   _ossEvent::_ossEvent ()
   {
      _signal = 0 ;
      _waitNum = 0 ;
      _useData = 0 ;
   }

   _ossEvent::~_ossEvent ()
   {
      _signal = 0 ;
   }

   UINT32 _ossEvent::waitNum ()
   {
      boost::mutex::scoped_lock lock ( _mutex ) ;
      return _waitNum ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSEVN_WAIT, "_ossEvent::wait" )
   INT32 _ossEvent::wait ( INT64 millisec, INT32 *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OSSEVN_WAIT );

      boost::chrono::milliseconds timeout
         = boost::chrono::milliseconds(millisec) ;
      boost::mutex::scoped_lock lock ( _mutex ) ;

      ++_waitNum ;
      while ( !_signal )
      {
         if ( millisec < 0 )
         {
            _cond.wait ( lock ) ;
         }
         else if ( boost::cv_status::timeout ==
                   _cond.wait_for( lock, timeout ) )
         {
            --_waitNum ;
            rc = SDB_TIMEOUT ;
            goto done ;
         }
      }
      if ( pData )
      {
         *pData = _useData ;
      }
      _onWait () ;

      --_waitNum ;
   done :
      PD_TRACE_EXITRC ( SDB__OSSEVN_WAIT, rc );
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSEN_SIGNAL, "_ossEvent::signal" )
   INT32 _ossEvent::signal ( INT32 data )
   {
      PD_TRACE_ENTRY ( SDB__OSSEN_SIGNAL );
      boost::mutex::scoped_lock lock ( _mutex ) ;
      _signal = 1 ;
      _useData = data ;
      //lock.unlock () ;
      _cond.notify_one () ;

      PD_TRACE_EXIT ( SDB__OSSEN_SIGNAL );
      return SDB_OK ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSEN_SIGALL, "_ossEvent::signalAll" )
   INT32 _ossEvent::signalAll ( INT32 data )
   {
      PD_TRACE_ENTRY ( SDB__OSSEN_SIGALL );
      boost::mutex::scoped_lock lock ( _mutex ) ;
      _signal = 1 ;
      _useData = data ;
      //lock.unlock () ;
      _cond.notify_all () ;

      PD_TRACE_EXIT ( SDB__OSSEN_SIGALL );
      return SDB_OK ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSVN_RESET, "_ossEvent::reset" )
   INT32 _ossEvent::reset ()
   {
      PD_TRACE_ENTRY ( SDB__OSSVN_RESET );
      boost::mutex::scoped_lock lock ( _mutex ) ;
      _signal = 0 ;

      PD_TRACE_EXIT ( SDB__OSSVN_RESET );
      return SDB_OK ;
   }

   void _ossEvent::_onWait ()
   {
   }

   _ossAutoEvent::_ossAutoEvent ()
   {
   }

   _ossAutoEvent::~_ossAutoEvent ()
   {
   }

   void _ossAutoEvent::_onWait ()
   {
      _signal = 0 ;
   }
}
