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
#ifndef TIME64_LIMITS_H
#define TIME64_LIMITS_H

/* Max/min for localtime() */
#define SYSTEM_LOCALTIME_MAX     2147483647
#define SYSTEM_LOCALTIME_MIN    -2147483647-1

/* Max/min for gmtime() */
#define SYSTEM_GMTIME_MAX        2147483647
#define SYSTEM_GMTIME_MIN       -2147483647-1

/* Max/min for mktime() */
static const struct tm SYSTEM_MKTIME_MAX = {
    7,
    14,
    19,
    18,
    0,
    138,
    1,
    17,
    0
#ifdef HAS_TM_TM_GMTOFF
    ,-28800
#endif
#ifdef HAS_TM_TM_ZONE
    ,"PST"
#endif
};

static const struct tm SYSTEM_MKTIME_MIN = {
    52,
    45,
    12,
    13,
    11,
    1,
    5,
    346,
    0
#ifdef HAS_TM_TM_GMTOFF
    ,-28800
#endif
#ifdef HAS_TM_TM_ZONE
    ,"PST"
#endif
};

/* Max/min for timegm() */
#ifdef HAS_TIMEGM
static const struct tm SYSTEM_TIMEGM_MAX = {
    7,
    14,
    3,
    19,
    0,
    138,
    2,
    18,
    0
    #ifdef HAS_TM_TM_GMTOFF
        ,0
    #endif
    #ifdef HAS_TM_TM_ZONE
        ,"UTC"
    #endif
};

static const struct tm SYSTEM_TIMEGM_MIN = {
    52,
    45,
    20,
    13,
    11,
    1,
    5,
    346,
    0
    #ifdef HAS_TM_TM_GMTOFF
        ,0
    #endif
    #ifdef HAS_TM_TM_ZONE
        ,"UTC"
    #endif
};
#endif /* HAS_TIMEGM */

#endif /* TIME64_LIMITS_H */
