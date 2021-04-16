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

   Source File Name = spaceManagementPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SPACE_MANAGEMENT_PAGE_H_
#define VESSEL_SPACE_MANAGEMENT_PAGE_H_

#include "vessel/pageDef.h"

namespace engine
{
namespace vessel
{
   ///capacity is 64bit aligned.
   INT32 getSMPCapacity8BytesAligned(UINT32 pageSize,
                                     UINT32 maxSegmentCount,
                                     UINT32 pageCountOfSeg,
                                     UINT32 &capacity);

#pragma pack(4)
   struct spaceManagementPageHead
   {
      UINT16 version;
      UINT16 flags;
      CHAR pad[12];
   };
#pragma pack()

   const UINT32 INVALID_SMP_VERSION = 0;
   const UINT32 SMP_VERSION_1 = 1;
   const UINT32 SMP_HEAD_LEN = sizeof(spaceManagementPageHead);
   const PAGE_ID SMP_PAGE_ID = 0;
}//namespace vessel
}//namespace engine

#endif//VESSEL_SPACE_MANAGEMENT_PAGE_H_