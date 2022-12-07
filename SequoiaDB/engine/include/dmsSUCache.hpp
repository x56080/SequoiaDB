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
