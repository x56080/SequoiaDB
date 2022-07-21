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

   Source File Name = btreeEntryPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ENTRY_PAGE_H_
#define VESSEL_BTREE_ENTRY_PAGE_H_

#include "vessel/indexDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 BTREE_ENTRY_PAGE_VERSION = 1;


#pragma pack(4)
   struct btreeEntryPageHead
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return BTREE_ENTRY_PAGE_VERSION == version &&
                INVALID_LOGICAL_INDEX_ID != logicalIndexId;
      }

      UINT32 version = 0;
      UINT32 logicalIndexId = INVALID_LOGICAL_INDEX_ID;
      UINT32 btreeRoot = INVALID_PAGE_ID;
      UINT32 replayTick = 0;
   };//struct btreeEntryPageHead

   constexpr UINT32 BTREE_ENTRY_PAGE_HEAD_SIZE = sizeof(btreeEntryPageHead);

#pragma pack()
   BOOLEAN initBtreeEntryPage(UINT32 pageSize,
                              PAGE_ID pid,
                              PAGE_ID lpid,
                              PAGE_SNAPSHOT_VERION psv,
                              UINT32 logicalIndexId,
                              CHAR *buf);
}//namespace vessel
}//namespace engine

#endif//VESSEL_BTREE_ENTRY_PAGE_H_