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

   Source File Name = vesselDef.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VESSEL_DEF_H_
#define VESSEL_VESSEL_DEF_H_

#include "dms.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   typedef UINT64 COLLECTION_HANDLE;
   const COLLECTION_HANDLE INVALID_COLLECTION_HANDLE = OSS_UINT64_MAX;

   typedef UINT64 CURSOR_ID;
   const CURSOR_ID INVALID_CURSOR_ID = OSS_UINT64_MAX;

   typedef UINT16 SPACE_ID;
   const SPACE_ID INVALID_SPACE_ID = 65535;
   const SPACE_ID MAX_SPACE_ID = 16384;
   const SPACE_ID MAX_SPACE_COUNT = MAX_SPACE_ID + 1;

   typedef UINT8 SPACE_TYPE;
   const SPACE_TYPE INVALID_SPACE_TYPE = 255;
   const SPACE_TYPE SPACE_TYPE_RECORD_M = 0;
   const SPACE_TYPE SPACE_TYPE_RECORD_D = 1;
   const SPACE_TYPE SPACE_TYPE_IDX_M = 2;
   const SPACE_TYPE SPACE_TYPE_IDX_D = 3;
   const SPACE_TYPE SPACE_TYPE_LOB_M = 4;
   const SPACE_TYPE SPACE_TYPE_LOB_DM = 5;
   const SPACE_TYPE SPACE_TYPE_LOB_DD = 7;
   const SPACE_TYPE SPACE_TYPE_NAME = 8;
   const SPACE_TYPE SPACE_TYPE_SPACE_MAP = 9;
   const SPACE_TYPE SPACE_TYPE_MAX = SPACE_TYPE_SPACE_MAP;

   typedef UINT32 SEGMENT_ID;
   const SEGMENT_ID INVALID_SEG_ID = UINT32(-1);

   typedef UINT32 SNAPSHOT_ID;
   const SNAPSHOT_ID INVALID_SNAPSHOT_ID = UINT32(-1);

   typedef UINT16 STRIPING_GROUP;
   const STRIPING_GROUP INVALID_STRIPING_GROUP = 65535;
   
   const UINT64 INVALID_CONTEXT_ID = OSS_UINT64_MAX;

   typedef UINT16 CL_MB_ID;
   const CL_MB_ID INVALID_CL_MB_ID = 65535;
   const CL_MB_ID MAX_CL_MB_COUNT = 65535;

   enum CURSOR_TYPE
   {
      CURSOR_TYPE_INVALID = 0,
      CURSOR_TYPE_LIST_COLLECTION_SPACE = 1,
      CURSOR_TYPE_LIST_COLLECTION = 2,
   };

} /// end of namespace vessel
} /// end of namespace engine
#endif