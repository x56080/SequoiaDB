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

   Source File Name = idMapFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_ID_MAP_FILE_DEF_H_
#define VESSEL_ID_MAP_FILE_DEF_H_

#include "ossTypes.hpp"
#include "dms.hpp"
#include "vessel/storageFileDef.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 ID_MAP_FILE_PAGE_SIZE = DMS_PAGE_SIZE32K;
   constexpr UINT32 ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG = 32;
   constexpr UINT32 ID_MAP_FILE_SEG_SIZE = ID_MAP_FILE_PAGE_SIZE * ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG;
   constexpr UINT32 ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE = DATA_STORAGE_FILE_SIZE / ID_MAP_FILE_PAGE_SIZE
                                                                          / ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG;
   constexpr UINT32 ID_MAP_FILE_MAX_PAGE_COUNT =  ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG *
                                                  ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE;
}
}

#endif//VESSEL_ID_MAP_FILE_DEF_H_