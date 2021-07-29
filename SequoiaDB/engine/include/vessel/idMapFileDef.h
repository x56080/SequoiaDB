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

   Source File Name = idMapFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
   constexpr UINT32 ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG = 128;
   constexpr UINT32 ID_MAP_FILE_SEG_SIZE = ID_MAP_FILE_PAGE_SIZE * ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG;
   constexpr UINT32 ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE = STORAGE_FILE_SIZE / ID_MAP_FILE_PAGE_SIZE
                                                                          / ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG;
   constexpr UINT32 ID_MAP_FILE_MAX_PAGE_COUNT =  ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG *
                                                  ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE;
}
}

#endif//VESSEL_ID_MAP_FILE_DEF_H_