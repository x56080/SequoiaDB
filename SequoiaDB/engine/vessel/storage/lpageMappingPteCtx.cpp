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

   Source File Name = lpageMappingPteCtx.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpageMappingPteCtx.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   void lpageMappingPteCtx::reset()
   {
      _root.reset();
      _obsoleteSet.clear();
      _brandNewSet.clear();
   }

   BOOLEAN lpageMappingPteCtx::isBrandNewPid(PAGE_ID pid, BOOLEAN lock)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      std::unique_lock<std::mutex> guard(_pathLock, std::defer_lock);
      if (lock)
      {
         guard.lock();
      }
      return 0 < _brandNewSet.count(pid);
   }
} // namespace vesel

} // namespace engine
