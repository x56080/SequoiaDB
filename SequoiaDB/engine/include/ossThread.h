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

   Source File Name = ossThread.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef OSS_THREAD_H_
#define OSS_THREAD_H_

#include "core.hpp"
#include "oss.hpp"

#include <thread> //c++11

namespace engine
{
   class ossThread : public SDBObject
   {
      public:
         ossThread();
         virtual ~ossThread();
         ossThread(const ossThread &) = delete;
         ossThread &operator=(const ossThread &) = delete;

      public:
         /// implement activeEntry and just call 'active' to start new thread.
         virtual void activeEntry(){return;}

         void active();

         /// An other way to active new thread.
         template<class Function, class ... Args>
         static void active(ossThread &t, Function&& f, Args&& ...args)
         {
            t._thread = std::move(std::thread(f, std::forward<Args>...));
         }

         BOOLEAN isJoinable()const;
         
         void detach();

         void join();

      private:
         static void _activeThread(ossThread *o);

      private:
         std::thread _thread;
   };//class ossThread
}//namespace engine

#endif//OSS_THREAD_H_