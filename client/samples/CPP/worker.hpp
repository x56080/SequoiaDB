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

   
*******************************************************************************/
#ifndef WORKER_HPP__
#define WORKER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "string.h"
#ifdef _WINDOWS
#include <process.h>
#include <Windows.h>
#else /* POSIX */
#include <pthread.h>
#endif


namespace sample
{
    typedef void (*workerFunc)(void *);

    struct workThread
    {
#ifdef _WIN32
        HANDLE     thread;
#else /*POSIX*/
        pthread_t  thread;
#endif
        workerFunc func;
        void*      args;

        /* 构造函数 */
        workThread(workerFunc func, void *args)
            : func(func), args(args)
        {
            memset(&thread, 0, sizeof(thread));
        }
    };

    class worker
    {
    public:
        /* 构造函数 */
        worker(workerFunc func, void *args);
        /* 析构函数 */
        ~worker();
        INT32 start();
        INT32 waitStop();

    private:
        workThread   _thread;
        BOOLEAN      _started;
    };
}

#endif // WORKER_HPP__

