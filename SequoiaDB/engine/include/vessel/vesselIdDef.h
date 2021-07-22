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

   Source File Name = vesselIdDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VESSEL_ID_DEF_H_
#define VESSEL_VESSEL_ID_DEF_H_

#include "ossUtil.hpp"
#include "vessel/pageIdentifier.h"

namespace engine
{
namespace vessel
{
   typedef UINT16 SPACE_ID;
   const SPACE_ID INVALID_SPACE_ID = 65535;
   const SPACE_ID MAX_SPACE_ID = 16383;
   const SPACE_ID MAX_SU_COUNT = MAX_SPACE_ID + 1;

   typedef UINT32 PAGE_SNAPSHOT_VERION;
   const PAGE_SNAPSHOT_VERION INVALID_PAGE_SNAPSHOT_VERSION = 0xFFFFFFFF;

   typedef UINT16 CL_MB_ID;
   const CL_MB_ID INVALID_CL_MB_ID = 65535;
   const CL_MB_ID MAX_CL_MB_COUNT = 65535;

   typedef UINT16 STRIPING_ID;
   const STRIPING_ID INVALID_STRIPING_ID = 65535;

   static const UINT32 INVALID_CL_PAGE_SEQ = 0xFFFFFFFF;

   static const UINT32 VESSEL_MIN_CS_LID = 0x80000000;
   OSS_INLINE BOOLEAN isVesselCSLogicalID(UINT32 lid)
   {
      return OSS_BIT_TEST(lid, VESSEL_MIN_CS_LID);
   }

} /// end of namespace vessel
} /// end of namespace engine
#endif//VESSEL_VESSEL_ID_DEF_H_