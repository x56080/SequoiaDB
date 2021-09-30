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

   Source File Name = indexEntryPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_ENTRY_PAGE_H_
#define VESSEL_INDEX_ENTRY_PAGE_H_

#include "vessel/indexDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   const static UINT32 INDEX_DEF_RECORD_VERSION = 1;


#pragma pack(4)
   struct indexEntryPageHead
   {
      indexEntryPageHead()
      {
      }

      ~indexEntryPageHead(){}

      indexEntryPageHead(const indexEntryPageHead &) = delete;

      indexEntryPageHead &operator=(const indexEntryPageHead &o)
      {
         ossMemcpy(this, &o, sizeof(indexEntryPageHead));
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return INDEX_DEF_RECORD_VERSION == version &&
                INDEX_STATUS_INVALID != status &&
                INVALID_LOGICAL_INDEX_ID != indexLogicalID &&
                DMS_INVALID_LOGICCLID != clLogicalID &&
                defObjSize > 0;
      }

      UINT32 version = 0;
      UINT32 indexLogicalID = INVALID_LOGICAL_INDEX_ID;
      UINT32 clLogicalID = DMS_INVALID_LOGICCLID;
      UINT64 createdTime = 0;
      UINT64 alteredTime = 0;
      UINT16 flags = 0;
      UINT16 status = INDEX_STATUS_INVALID;
      UINT32 btreeRoot = INVALID_PAGE_ID;
      UINT32 btreeRootUpdatedTimes = 0;
      UINT32 defObjSize = 0;
      CHAR pad[32] = {};
   };//struct indexEntryPageHead

   static const UINT32 INDEX_ENTRY_PAGE_HEAD_SIZE = sizeof(indexEntryPageHead);

#pragma pack()

   static const UINT32 MAX_INDEX_DEF_OBJ_SIZE = 4096 - PAGE_HEAD_SIZE - INDEX_ENTRY_PAGE_HEAD_SIZE;

   BOOLEAN initIndexEntryPage(UINT32 pageSize,
                              PAGE_ID pid,
                              PAGE_ID lpid,
                              PAGE_SNAPSHOT_VERION psv,
                              CHAR *buf);
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_ENTRY_PAGE_H_