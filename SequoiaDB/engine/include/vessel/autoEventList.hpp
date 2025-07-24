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

   Source File Name = autoEventList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_AUTO_EVENT_LIST_HPP_
#define VESSEL_AUTO_EVENT_LIST_HPP_

#include "ossMemPool.hpp"
#include <mutex>
#include <condition_variable> //c++11
#include <chrono>//c++11

namespace engine
{
namespace vessel
{
   /// EVENT must implement "release"
   template<class EVENT>
   class autoEventList : public SDBObject
   {
      public:
         autoEventList(){}
         ~autoEventList(){}
         autoEventList(const autoEventList &) = delete;
         autoEventList &operator=(const autoEventList &) = delete;

      public:
         UINT32 getSize()
         {
            std::unique_lock<std::mutex> lk(_mutex);
            return _list.size();
         }

         BOOLEAN isEmpty()
         {
            std::unique_lock<std::mutex> lk(_mutex);
            return _list.empty();
         }

         void pushPriority(const EVENT &e)
         {
            std::unique_lock<std::mutex> lk(_mutex);
            _list.push_back(e);
            _cv.notify_one();
         }

         void push(const EVENT &e)
         {
            std::unique_lock<std::mutex> lk(_mutex);
            _list.push_front(e);
            _cv.notify_one();
         }

         /// will return false when closed
         void popOrWait(EVENT &e)
         {
            e.reset();
            std::unique_lock<std::mutex> lk(_mutex);
            ++_waiting;
            _cv.wait(lk, [this]{return !_list.empty();});
            --_waiting;
            e = _list.back();
            _list.pop_back();
            if (!_list.empty() && 0 < _waiting)
            {
               _cv.notify_one();
            }

            /// auto unlock
            
            return;
         }

         BOOLEAN tryToPop(EVENT &e)
         {
            BOOLEAN r = FALSE;
            e.reset();
            std::unique_lock<std::mutex> lk(_mutex);
            if (!_list.empty())
            {
               e = _list.back();
               _list.pop_back();
               r = TRUE;
               if (!_list.empty() && 0 < _waiting)
               {
                  _cv.notify_one();
               }
            }
            return r;
         }

         BOOLEAN popOrWaitFor(UINT32 millis, EVENT &e)
         {
            BOOLEAN r = FALSE;
            e.reset();
            std::unique_lock<std::mutex> lk(_mutex);
            ++_waiting;
            if (_cv.wait_for(lk, std::chrono::milliseconds(millis),
                             [this]{return !_list.empty();}))
            {
               --_waiting;
               e = _list.back();
               _list.pop_back();
               r = TRUE;
               if (!_list.empty() && 0 < _waiting)
               {
                  _cv.notify_one();
               }
            }
            else
            {
               --_waiting;
            }
            return r;
         }

         void clear()
         {
            std::unique_lock<std::mutex> lk(_mutex);
            _list.clear();
            SDB_ASSERT(0 == _waiting, "some one be waiting");
            _waiting = 0;
         }
      private:
         std::condition_variable _cv;
         std::mutex _mutex;
         ossPoolList<EVENT> _list;
         UINT32 _waiting = 0;
   };//class autoEventList
}//namespace vessel
}//namespace engine

#endif//VESSEL_AUTO_EVENT_LIST_HPP_