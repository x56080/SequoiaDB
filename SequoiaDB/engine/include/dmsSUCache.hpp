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

   Source File Name = dmsSUCache.hpp

   Descriptive Name = Data Management Service SU Cache Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS event handler.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef DMS_SUCACHE_HPP_
#define DMS_SUCACHE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "utilSUCache.hpp"
#include "utilBitmap.hpp"

namespace engine
{

   #define DMS_CACHE_TYPE_NUM  ( 0 )

   typedef class _utilSUCache<DMS_MME_SLOTS>          dmsSUCache ;
   typedef class _IUtilSUCacheHolder<DMS_MME_SLOTS>   IDmsSUCacheHolder ;
   typedef class _utilStackBitmap<DMS_MME_SLOTS>      dmsSUCacheBitmap ;
}
#endif //DMS_SUCACHE_HPP_
