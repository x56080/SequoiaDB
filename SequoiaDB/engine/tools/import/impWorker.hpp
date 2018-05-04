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

   Source File Name = impWorker.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_WORKER_HPP_
#define IMP_WORKER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"

#ifdef _WINDOWS
#include <process.h>
#include <Windows.h>
#else /* POSIX */
#include <pthread.h>
#endif

namespace import
{
   class WorkerArgs: public SDBObject
   {
   private:
      // disallow copy and assign
      WorkerArgs(const WorkerArgs&);
      void operator=(const WorkerArgs&);
   protected:
      WorkerArgs() {}
   public:
      virtual ~WorkerArgs() {}
   };

   typedef void (*WorkerRoutine)(WorkerArgs*);

   struct WorkerThread: public SDBObject
   {
#ifdef _WINDOWS
      HANDLE         thread;
#else /* POSIX */
      pthread_t      thread;
#endif
      WorkerRoutine  routine;
      WorkerArgs*    args;

      WorkerThread(WorkerRoutine routine, WorkerArgs* args)
      {
         ossMemset(&thread, 0, sizeof(thread));
         this->routine = routine;
         this->args = args;
      }
   };

   class Worker: public SDBObject
   {
   public:
      Worker(WorkerRoutine routine, WorkerArgs* args, BOOLEAN managed = FALSE);
      ~Worker();
      INT32 start();
      INT32 waitStop();

   private:
      WorkerThread   _thread;
      BOOLEAN        _started;
      BOOLEAN        _managed; // true if manage the args memory
   };

}

#endif /* IMP_WORKER_HPP_ */
