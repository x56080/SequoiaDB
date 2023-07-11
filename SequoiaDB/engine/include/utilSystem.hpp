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
}

#endif