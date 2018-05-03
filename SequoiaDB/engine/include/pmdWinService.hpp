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

   Source File Name = pmdDaemon.hpp

   Descriptive Name = pmdDaemon

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/09/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDWINSERVICE_HPP__
#define PMDWINSERVICE_HPP__
#include "ossFeat.h"

#if defined (_WINDOWS)

#include "ossTypes.h"

typedef  INT32 (*PMD_WINSERVICE_FUNC)( INT32 argc, CHAR **argv ) ;

#define PMD_WINSVC_SVCNAME_MAX_LEN     255

INT32 WINAPI pmdWinstartService( const CHAR *pServiceName,
                        PMD_WINSERVICE_FUNC svcFun );

#endif //#if defined (_WINDOWS)

#endif //#define PMDWINSERVICE_HPP__