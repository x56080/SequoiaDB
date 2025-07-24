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

   Source File Name = cursorDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CURSOR_DEF_H_
#define VESSEL_CURSOR_DEF_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   enum CURSOR_TYPE
   {
      CURSOR_TYPE_INVALID = 0,
      CURSOR_TYPE_LIST_COLLECTION_SPACE = 1,
      CURSOR_TYPE_LIST_COLLECTION = 2,
      CURSOR_TYPE_SCAN_COLLECTION = 3,
      CURSOR_TYPE_INDEX_SCAN = 4,
      CURSOR_TYPE_LIST_LOBC = 5,
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_DEF_H_