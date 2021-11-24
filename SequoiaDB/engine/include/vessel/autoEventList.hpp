/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = autoEventList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
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
            lk.unlock();
            _cv.notify_one();
         }

         void push(const EVENT &e)
         {
            std::unique_lock<std::mutex> lk(_mutex);
            _list.push_front(e);
            lk.unlock();
            _cv.notify_one();
         }

         /// will return false when closed
         void popOrWait(EVENT &e)
         {
            e.release();
            std::unique_lock<std::mutex> lk(_mutex);
            ++_waiting;
            _cv.wait(lk, [this]{return !_list.empty();});
            --_waiting;
            e = _list.back();
            _list.pop_back();
            if (!_list.empty() && 0 < _waiting)
            {
               lk.unlock();
               _cv.notify_one();
            }

            /// auto unlock
            
            return;
         }

         BOOLEAN tryToPop(EVENT &e)
         {
            BOOLEAN r = FALSE;
            e.release();
            std::unique_lock<std::mutex> lk(_mutex);
            if (!_list.empty())
            {
               e = _list.back();
               _list.pop_back();
               r = TRUE;
               if (!_list.empty() && 0 < _waiting)
               {
                  lk.unlock();
                  _cv.notify_one();
               }
            }
            return r;
         }

         BOOLEAN popOrWaitFor(UINT32 millis, EVENT &e)
         {
            BOOLEAN r = FALSE;
            e.release();
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
                  lk.unlock();
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