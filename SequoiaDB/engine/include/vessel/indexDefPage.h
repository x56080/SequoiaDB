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

   Source File Name = indexDefPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_DEF_PAGE_H_
#define VESSEL_INDEX_DEF_PAGE_H_

#include "vessel/indexDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   const static UINT16 INDEX_DEF_RECORD_VERSION = 1;


#pragma pack(4)
   struct indexDefRecord
   {
      indexDefRecord()
      {
      }

      ~indexDefRecord(){}

      OSS_INLINE indexDefRecord &operator=(const indexDefRecord &o)
      {
         ossMemcpy(this, &o, sizeof(indexDefRecord));
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return INDEX_DEF_RECORD_VERSION == version;
      }

      UINT16 version = 0;
      UINT16 type = INVALID_INDEX_TYPE;
      UINT32 indexLogicalID = INVALID_LOGICAL_INDEX_ID;
      UINT32 clLogicalID = DMS_INVALID_LOGICCLID;
      UINT64 flags = 0;
      UINT32 ordering = 0;
      UINT8 keyCount = 0;
      UINT8 status = INDEX_STATUS_INVALID;
      UINT8 btreePrefixCompressionColumns = 0;
      UINT8 pad = 0;
      UINT32 btreeRoot = INVALID_PAGE_ID;
      UINT32 rebuiding = INVALID_CL_PAGE_SEQ;
      UINT32 lsmCF = 0;
      UINT32 indexNameLen = 0; /// \0 included in indexNameLen
      UINT32 indexNameOffset = 0;
      UINT32 keyPatternLen = 0;/// bsonobj len.
      UINT32 keyPatternOffset = 0;
   };//struct indexDefRecord

   static const UINT32 INDEX_DEF_RECORD_LEN = sizeof(indexDefRecord);

#pragma pack()

   BOOLEAN initIndexDefPage(UINT32 pageSize, UINT32 lpid,
                            const indexDefRecord &record,
                            CHAR *buf);
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_DEF_PAGE_H_