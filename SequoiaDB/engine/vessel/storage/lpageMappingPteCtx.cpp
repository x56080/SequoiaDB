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

   Source File Name = lpageMappingPteCtx.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
