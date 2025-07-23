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

   Source File Name = utilSystem.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/05/2023  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _UTIL_SYSTEM_HPP_
#define _UTIL_SYSTEM_HPP_

#include "ossTypes.h"
#include <string>
#include <vector>
#include <map>

namespace engine
{
   struct _cpuInfo
   {
      std::string processor ;
      std::string modelName ;
      std::string freq ;
      std::string physicalID ;
      std::string coreID ;
      std::string flags ;
      std::string clock ;
      std::string machine ;
#if defined(_WINDOWS)
      UINT32 coreNum ;
      std::string sysInfo ;
#endif
      void reset()
      {
         processor = "" ;
         modelName = "" ;
         freq = "" ;
         physicalID = "" ;
         coreID = "" ;
         flags = "" ;
         clock = "" ;
         machine = "" ;
#if defined(_WINDOWS)
         coreNum = 0 ;
         sysInfo = "" ;
#endif
      }
   } ;
   typedef struct _cpuInfo cpuInfo ;

   INT32 utilSysGetCpuInfo( std::map< std::string, std::vector<cpuInfo> > &cpuInfos ) ;

   INT32 utilSysGetOsReleaseInfoFromCmd( std::string &distributor,
                                         std::string &release,
                                         std::string &description ) ;

   INT32 utilSysGetOsReleaseInfoFromFile( std::string &distributor,
                                          std::string &release,
                                          std::string &description ) ;

   INT32 utilSysCountNumaNodes( UINT32 &numaNodes ) ;

   INT32 utilSysGetOSVersion( std::string &version ) ;

   INT32 utilSysGetOSVersionSignature( std::string &versionSignature ) ;

   INT32 utilSysGetLibcVersion( std::string &version ) ;

   INT64 utilSysGetLimitMem() ;

}

#endif