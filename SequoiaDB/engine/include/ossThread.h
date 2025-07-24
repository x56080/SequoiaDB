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

   Source File Name = ossThread.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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