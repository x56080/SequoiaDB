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

   Source File Name = dmsWTDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_DEF_HPP_
#define DMS_WT_DEF_HPP_

#include "ossUtil.hpp"

namespace engine
{
namespace wiredtiger
{

   #define DMS_DFT_WT_CACHE_SIZE       ( 2048 )
   #define DMS_MAX_WT_CACHE_SIZE       ( 10 * 1024 * 1024 )
   #define DMS_MIN_WT_CACHE_SIZE       ( 256 )

   #define DMS_DFT_WT_EVICT_TARGET     ( 80 )
   #define DMS_MIN_WT_EVICT_TARGET     ( 10 )
   #define DMS_MAX_WT_EVICT_TARGET     ( 100 )

   #define DMS_DFT_WT_EVICT_TRIGGER    ( 95 )
   #define DMS_MIN_WT_EVICT_TRIGGER    ( 10 )
   #define DMS_MAX_WT_EVICT_TRIGGER    ( 100 )

   #define DMS_DFT_WT_EVICT_DIRTY_TARGET ( 5 )
   #define DMS_MIN_WT_EVICT_DIRTY_TARGET ( 1 )
   #define DMS_MAX_WT_EVICT_DIRTY_TARGET ( 100 )

   #define DMS_DFT_WT_EVICT_DIRTY_TRIGGER ( 20 )
   #define DMS_MIN_WT_EVICT_DIRTY_TRIGGER ( 1 )
   #define DMS_MAX_WT_EVICT_DIRTY_TRIGGER ( 100 )

   #define DMS_DFT_WT_EVICT_UPDATES_TARGET ( 2 )
   #define DMS_MIN_WT_EVICT_UPDATES_TARGET ( 0 )
   #define DMS_MAX_WT_EVICT_UPDATES_TARGET ( 100 )

   #define DMS_DFT_WT_EVICT_UPDATES_TRIGGER ( 10 )
   #define DMS_MIN_WT_EVICT_UPDATES_TRIGGER ( 0 )
   #define DMS_MAX_WT_EVICT_UPDATES_TRIGGER ( 100 )

   #define DMS_DFT_WT_EVICT_THREADS_MIN ( 4 )
   #define DMS_MIN_WT_EVICT_THREADS_MIN ( 1 )
   #define DMS_MAX_WT_EVICT_THREADS_MIN ( 20 )

   #define DMS_DFT_WT_EVICT_THREADS_MAX ( 4 )
   #define DMS_MIN_WT_EVICT_THREADS_MAX ( 1 )
   #define DMS_MAX_WT_EVICT_THREADS_MAX ( 20 )

   #define DMS_DFT_WT_CHECK_POINT_INTERVAL ( 60 )
   #define DMS_MIN_WT_CHECK_POINT_INTERVAL ( 1 )
   #define DMS_MAX_WT_CHECK_POINT_INTERVAL ( OSS_UINT32_MAX )

   #define DMS_DFT_WT_CHECK_POINT_LOG_SIZE ( 2048 )
   #define DMS_MIN_WT_CHECK_POINT_LOG_SIZE ( 1 )
   #define DMS_MAX_WT_CHECK_POINT_LOG_SIZE ( OSS_UINT32_MAX )

   #define DMS_WT_FORMART_V1 ( 1 )
   #define DMS_WT_FORMART_VER_CUR ( DMS_WT_FORMART_V1 )

}
}

#endif // DMS_WT_DEF_HPP_
