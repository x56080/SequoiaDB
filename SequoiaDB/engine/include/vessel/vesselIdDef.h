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

   Source File Name = vesselIdDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_VESSEL_ID_DEF_H_
#define VESSEL_VESSEL_ID_DEF_H_

#include "ossUtil.hpp"
#include "vessel/pageIdentifier.h"

namespace engine
{
namespace vessel
{
   typedef UINT16 SPACE_ID;
   constexpr SPACE_ID INVALID_SPACE_ID = 65535;
   constexpr SPACE_ID MAX_SPACE_ID = 16383;
   constexpr SPACE_ID MAX_SU_COUNT = MAX_SPACE_ID + 1;

   typedef UINT32 PAGE_SNAPSHOT_VERION;
   constexpr PAGE_SNAPSHOT_VERION INVALID_PAGE_SNAPSHOT_VERSION = 0;

   typedef UINT16 CL_MB_ID;
   constexpr CL_MB_ID INVALID_CL_MB_ID = 65535;
   constexpr CL_MB_ID MAX_CL_MB_COUNT = 65535;

} /// end of namespace vessel
} /// end of namespace engine
#endif//VESSEL_VESSEL_ID_DEF_H_