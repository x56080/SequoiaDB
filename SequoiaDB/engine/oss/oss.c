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

   Source File Name = oss.c

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#include <time.h>
#include "oss.h"

#if defined (_LINUX)
#include <unistd.h>
#include <syscall.h>
#elif defined (_AIX)
#include <pthread.h>
#else
#include <tlhelp32.h>
#endif

#if defined (_WINDOWS)
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
   FILETIME ft;
   UINT64 tmpres = 0;
   static int tzflag;

   if (NULL != tv)
   {
      GetSystemTimeAsFileTime(&ft);

      tmpres |= ft.dwHighDateTime;
      tmpres <<= 32;
      tmpres |= ft.dwLowDateTime;

      /*convert into microseconds*/
      tmpres /= 10;

      /*converting file time to unix epoch*/
      tmpres -= DELTA_EPOCH_IN_MICROSECS;

      tv->tv_sec = (long)(tmpres / 1000000UL);
      tv->tv_usec = (long)(tmpres % 1000000UL);
   }

   if (NULL != tz)
   {
   	  long ttz;
      int tdl;
      if (!tzflag)
      {
         _tzset();
         tzflag++;
      }
      _get_timezone(&ttz);
      _get_daylight(&tdl);
      tz->tz_minuteswest = ttz / 60;
      tz->tz_dsttime = tdl;
   }

   return 0;
}

#endif
OSSPID ossGetCurrentProcessID()
{
#if defined (_WINDOWS)
   return GetCurrentProcessId();
#else
   return getpid();
#endif
}
OSSTID ossGetCurrentThreadID()
{
#if defined (_WINDOWS)
   return GetCurrentThreadId();
#elif defined (_AIX)
   return pthread_self();
#else
   return syscall(SYS_gettid);
#endif
}

UINT32 ossGetLastError()
{
#if defined (_WINDOWS)
   return GetLastError();
#else
   return errno;
#endif
}

void ossSleep(UINT32 milliseconds)
{
#if defined (_WINDOWS)
   Sleep(milliseconds);
#else
   usleep(milliseconds*1000);
#endif
}

void ossPanic()
{
   INT32 *p = NULL ;
   *p = 10 ;
}

OSSPID ossGetParentProcessID()
{
#if defined (_WINDOWS)
   OSSPID         pid         = OSS_INVALID_PID ;
   OSSPID         ppid        = OSS_INVALID_PID ;
   HANDLE         hSnapshot   = INVALID_HANDLE_VALUE ;
   PROCESSENTRY32 pe          = { 0 } ;
   pe.dwSize                  = sizeof (PROCESSENTRY32) ;

   hSnapshot = CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS , 0 ) ;
   if ( hSnapshot == INVALID_HANDLE_VALUE )
      goto error ;

   pid = GetCurrentProcessId() ;

   if ( Process32First ( hSnapshot , &pe ) )
   {
      do
      {
         if ( pe.th32ProcessID == pid )
         {
            ppid = pe.th32ParentProcessID ;
            goto done ;
         }
      } while ( Process32Next ( hSnapshot , &pe ) ) ;
   }
done :
   return ppid ;
error :
   goto done ;

#else
   return getppid() ;
#endif
}

INT32 ossOnceRun(ossOnce* control, void (*func)(void))
{
#ifdef _WINDOWS
   if ( !InterlockedExchange ( control, 1 ) )
   {
      func() ;
   }

   return 0 ;

#else /* Posix */
   return pthread_once ( control, func ) ;
#endif
}

