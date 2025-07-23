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

   Source File Name = ossTimeZone.hpp

   Descriptive Name = Operating System Services Utility Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains wrappers for utilities like
   memcpy, strcmp, etc...

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/31/2022  Yang QinCheng  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSTIMEZONE_HPP_
#define OSSTIMEZONE_HPP_

#include "core.h"
#include "ossTypes.h"

#define OSS_TIMEZONE_MAX_LEN 256

/*
 * Get time zone information from system file, like /etc/timezone, /etc/localtime.
 *
 * [in] len  The length of the time zone information, if you are unsure its len, you can use the
 * OSS_TIMEZONE_MAX_LEN macro.
 * [out] pTimeZone  The time zone information of current system. eg: "Asia/Shanghai"
 */
INT32 ossGetSysTimeZone( CHAR *pTimeZone, INT32 len ) ;

/*
 * Get the TZ environment variable.
 *
 * [in] len  The length of the TZ environment variable, if you are unsure its len, you can use the
 * OSS_TIMEZONE_MAX_LEN macro.
 * [out] pTimeZone  The value of the TZ environment variable. eg: "Asia/Shanghai"
 */
INT32 ossGetTZEnv ( CHAR *pTimeZone, INT32 len ) ;

/*
 * Initialize the TZ environment variable.
 */
INT32 ossInitTZEnv () ;

#endif  // OSSTIMEZONE_HPP_
