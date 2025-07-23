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

   Source File Name = time64.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef TIME64_H
#    define TIME64_H

#include <time.h>
#include "time64_config.h"

/* Set our custom types */
typedef INT_64_T        Int64;
typedef Int64           Time64_T;
typedef Int64           Year;


/* A copy of the tm struct but with a 64 bit year */
struct TM64 {
        int     tm_sec;
        int     tm_min;
        int     tm_hour;
        int     tm_mday;
        int     tm_mon;
        Year    tm_year;
        int     tm_wday;
        int     tm_yday;
        int     tm_isdst;

#ifdef HAS_TM_TM_GMTOFF
        long    tm_gmtoff;
#endif

#ifdef HAS_TM_TM_ZONE
        char    *tm_zone;
#endif
};


/* Decide which tm struct to use */
#ifdef USE_TM64
#define TM      TM64
#else
#define TM      tm
#endif   


/* Declare public functions */
struct TM *gmtime64_r    (const Time64_T *, struct TM *);
struct TM *localtime64_r (const Time64_T *, struct TM *);
struct TM *gmtime64      (const Time64_T *);
struct TM *localtime64   (const Time64_T *);

Time64_T   timegm64      (const struct TM *);
Time64_T   mktime64      (const struct TM *);
Time64_T   timelocal64   (const struct TM *);


/* Not everyone has gm/localtime_r(), provide a replacement */
#ifdef HAS_LOCALTIME_R
#    define LOCALTIME_R(clock, result) localtime_r(clock, result)
#else
#    define LOCALTIME_R(clock, result) fake_localtime_r(clock, result)
#endif
#ifdef HAS_GMTIME_R
#    define GMTIME_R(clock, result)    gmtime_r(clock, result)
#else
#    define GMTIME_R(clock, result)    fake_gmtime_r(clock, result)
#endif


#endif
