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

   Source File Name = schedTaskQue.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "schedTaskQue.hpp"

namespace engine
{

   /*
      _schedFIFOTaskQue implement
   */
   _schedFIFOTaskQue::_schedFIFOTaskQue()
   {
   }

   _schedFIFOTaskQue::~_schedFIFOTaskQue()
   {
   }

   UINT32 _schedFIFOTaskQue::size()
   {
      return _queue.size() ;
   }

   BOOLEAN _schedFIFOTaskQue::isEmpty()
   {
      return _queue.empty() ;
   }

   void _schedFIFOTaskQue::push( const pmdEDUEvent &event, INT64 userData, UINT64 recvTimeUs )
   {
      _queue.push( _queData( event, recvTimeUs ) ) ;
   }

   BOOLEAN _schedFIFOTaskQue::pop( pmdEDUEvent &event, UINT64 &recvTimeUs, INT64 millisec )
   {
      BOOLEAN ret = FALSE ;
      _queData data ;

      if ( millisec < 0 )
      {
         _queue.wait_and_pop( data ) ;
         ret = TRUE ;
      }
      else if ( 0 == millisec )
      {
         ret = _queue.try_pop( data ) ;
      }
      else
      {
         ret = _queue.timed_wait_and_pop( data, millisec ) ;
      }

      if ( ret )
      {
         event = data._event ;
         recvTimeUs = data._recvTimeMs ;
      }
      return ret ;
   }

   /*
      _schedPriorityTaskQue implement
   */
   _schedPriorityTaskQue::_schedPriorityTaskQue()
   {
   }

   _schedPriorityTaskQue::~_schedPriorityTaskQue()
   {
   }

   UINT32 _schedPriorityTaskQue::size()
   {
      return _queue.size() ;
   }

   BOOLEAN _schedPriorityTaskQue::isEmpty()
   {
      return _queue.empty() ;
   }

   void _schedPriorityTaskQue::push( const pmdEDUEvent &event, INT64 userData, UINT64 recvTimeUs )
   {
      _queue.push( priorityEvent( event, recvTimeUs, userData ) ) ;
   }

   BOOLEAN _schedPriorityTaskQue::pop( pmdEDUEvent &event, UINT64 &recvTimeUs, INT64 millisec )
   {
      BOOLEAN ret = FALSE ;
      priorityEvent tmpEvent ;

      if ( millisec < 0 )
      {
         _queue.wait_and_pop( tmpEvent ) ;
         ret = TRUE ;
      }
      else if ( 0 == millisec )
      {
         ret = _queue.try_pop( tmpEvent ) ;
      }
      else
      {
         ret = _queue.timed_wait_and_pop( tmpEvent, millisec ) ;
      }

      if ( ret )
      {
         event = tmpEvent._event ;
         recvTimeUs = tmpEvent._recvTimeUs ;
      }
      return ret ;
   }

}

