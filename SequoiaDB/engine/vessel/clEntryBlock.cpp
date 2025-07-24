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

   Source File Name = clEntryBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/clEntryBlock.h"

namespace engine
{
namespace vessel
{
   void clEntryBlock::reset()
   {
      _properties.reset();
      _lvl0Count.store(0, std::memory_order_relaxed);
      _rdpCount.store(0, std::memory_order_relaxed);
      routeMap.fill(INVALID_PAGE_ID);
      _fsm.close();
      _indexes.reset();
      return;
   }
} // namespace vessel

} // namespace engine
