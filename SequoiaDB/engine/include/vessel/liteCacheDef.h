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

#include "ossTypes.hpp"
#include "ossUtil.hpp"
#include "ossSharedLatch.hpp"

namespace engine
{
namespace vessel
{
   enum LC_ALLOCATE_MODE
   {
      NORMAL = 1,
      ONLY_IF_IN_POOL
   };

   class liteCacheAllocateOptions : public SDBObject
   {
      public:
      OSS_INLINE liteCacheAllocateOptions(){}
      OSS_INLINE ~liteCacheAllocateOptions(){}
      liteCacheAllocateOptions(const liteCacheAllocateOptions &) = delete;
      liteCacheAllocateOptions &operator=(const liteCacheAllocateOptions &o) = delete;

      ossSharedLatchMode lockMode;
      enum LC_ALLOCATE_MODE mode = NORMAL;
      INT32 lockTimeout = -1;
   };

} /// end of namespace vessel
} /// end of namespace engine
#endif