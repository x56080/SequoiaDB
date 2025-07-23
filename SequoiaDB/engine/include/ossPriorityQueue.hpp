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

   Source File Name = ossPriorityQueue.hpp

   Descriptive Name = Operating System Services Queue Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains structure for thread-safe
   queue, including push, try_pop, wait_and_pop and timed_wait_and_pop
   operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/20/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_PRIORITY_QUEUE_HPP__
#define OSS_PRIORITY_QUEUE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include <queue>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/cv_status.hpp>

template<typename Data, typename Container=std::vector<Data>,
         typename Compare=std::less<typename Container::value_type> >
class ossPriorityQueue : public SDBObject
{
private :
   std::priority_queue<Data, Container, Compare>   _queue ;
   mutable boost::mutex                            _mutex ;
   boost::condition_variable                       _cond ;
public :
   UINT64 size ()
   {
      boost::mutex::scoped_lock lock ( _mutex ) ;
      return (UINT64)_queue.size() ;
   }
   void push ( Data const& data )
   {
      boost::mutex::scoped_lock lock ( _mutex ) ;
      _queue.push ( data ) ;
      lock.unlock () ;
      _cond.notify_one () ;
   }

   BOOLEAN empty() const
   {
      boost::mutex::scoped_lock lock ( _mutex ) ;
      return _queue.empty () ;
   }

   BOOLEAN try_pop ( Data& value )
   {
      boost::mutex::scoped_lock lock ( _mutex ) ;
      if ( _queue.empty () )
         return FALSE ;
      value = _queue.top () ;
      _queue.pop () ;
      return TRUE ;
   }

   void wait_and_pop ( Data& value )
   {
      boost::mutex::scoped_lock lock ( _mutex ) ;
      while ( _queue.empty () )
         _cond.wait ( lock ) ;
      value = _queue.top () ;
      _queue.pop () ;
   }

   BOOLEAN timed_wait_and_pop ( Data& value, INT64 millsec )
   {
      if ( millsec < 0 )
      {
         millsec = 0x7FFFFFFF ;
      }
      boost::chrono::milliseconds timeout
         = boost::chrono::milliseconds(millsec) ;
      boost::mutex::scoped_lock lock ( _mutex ) ;
      // if timed_wait return false, that means we failed by timeout
      while ( _queue.empty () )
         if ( boost::cv_status::timeout == _cond.wait_for( lock, timeout ) )
            return FALSE ;
      value = _queue.top () ;
      _queue.pop () ;
      return TRUE ;
   }

} ;

#endif //OSS_PRIORITY_QUEUE_HPP__