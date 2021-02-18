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

   Source File Name = liteCacheDef.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LITE_CACHE_DEF_H_
#define VESSEL_LITE_CACHE_DEF_H_

#include "utilArray.hpp"
#include "vessel/lcChunkPage.h"
namespace engine
{
namespace vessel
{
   class lcExtentTag;

   typedef _utilArray<lcChunkPage, 8> LC_LRU_EVICT_ARRAY;
   typedef _utilArray<lcExtentTag *, 8> LC_LRU_EVICT_TO_BEREMOVED;


   enum SyncLevel
   {
      STOP_WHEN_HIT_PENDING,
      SKIP_WHEN_HIT_PENDING,
      WAIT_WHEN_HIT_PENDING,
   };

   enum LC_ALLOCATE_MODE
   {
      NORMAL = 1,
      ONLY_IF_IN_POOL
   };

   struct liteCacheAllocateOptions
   {
      OSS_INLINE liteCacheAllocateOptions()
      :readonly(TRUE),
      holdExclusiveLock(FALSE),
      mode(NORMAL),
      lockTimeout(-1)
      {}

      BOOLEAN readonly;
      BOOLEAN holdExclusiveLock; /// always get page exclusive lock if set as true.
      enum LC_ALLOCATE_MODE mode;
      INT32 lockTimeout;
   };

} /// end of namespace vessel
} /// end of namespace engine
#endif