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

   Source File Name = sharedObjLatchEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SHARED_OBJ_LATCH_ENV_H_
#define VESSEL_SHARED_OBJ_LATCH_ENV_H_

#include "vessel/objectLatchMap.hpp"
#include "vessel/fixedLatchArray.hpp"

namespace engine
{
namespace vessel
{
   class sharedObjLatchEnv : public SDBObject
   {
      public:
         LOGICAL_PID_LATCH_MAP lpidLatchMap;
         RECORD_ID_LATCH_MAP ridLatchMap;
         UNIQUE_INDEX_LATCH_MAP uniqueIndexLathMap;
         LOBC_LATCH_MAP lobcLatchMap;
         FIXED_POSIX_S_LATCH_ARRAY lobRegionLatchVec;
   };//class sharedObjLatchEnv
} // namespace vessel

} // namespace engine


#endif//VESSEL_SHARED_OBJ_LATCH_ENV_H_
