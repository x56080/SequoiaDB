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

   Source File Name = containerUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSE_CONTAINER_UTILS_H_
#define VESSE_CONTAINER_UTILS_H_

#include "vessel/pageIdentifier.h"
#include "vessel/shallowPointer.hpp"

namespace engine
{
namespace vessel
{
   typedef shallowArray<PAGE_ID> PID_ARRAY;
   typedef shallowArray<mappedLogicalPageId> LPID_MAPPING_ARRAY;
} // namespace vessel

} // namespace engine


#endif//VESSE_CONTAINER_UTILS_H_