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

   Source File Name = dmsWTStats.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_STATS_HPP_
#define DMS_WT_STATS_HPP_

#include "dmsDef.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   enum class dmsWTStatsCatalog
   {
      STATS_ALL,
      STATS_SIZE,
      STATS_FAST
   } ;

   OSS_INLINE const CHAR *dmsWTGetStatsConfig( dmsWTStatsCatalog catalog )
   {
      switch ( catalog )
      {
         case dmsWTStatsCatalog::STATS_ALL:
            return "statistics=(all)" ;
         case dmsWTStatsCatalog::STATS_SIZE:
            return "statistics=(size)" ;
         case dmsWTStatsCatalog::STATS_FAST:
            return "statistics=(fast)" ;
         default:
            return "statistics=(all)" ;
      }
   }

}
}

#endif // DMS_WT_ITEM_HPP_
