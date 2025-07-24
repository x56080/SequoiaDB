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

   Source File Name = csMetaBlockPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CS_META_BLOCK_PAGE_H_
#define VESSEL_CS_META_BLOCK_PAGE_H_

#include "vessel/pageDef.h"
#include "dms.hpp"
#include "utilUniqueID.hpp"
#include "vessel/strSlice.h"
#include "vessel/objectBaseDef.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 CS_META_BLOCK_VERSION_1 = 1;
   constexpr PAGE_ID CS_META_BLOCK_PAGE_LPID = 0;

   enum CS_META_BLOCK_UPDATE_MASK
   {
      MAX_CL_LOGICAL_ID = 0x01
   };

#pragma pack(4)
   struct csMetaBlock
   {
      UINT32 version = 0;
      UINT16 status = 0;
      UINT16 type = 0;
      UINT32 flags = 0;
      CHAR name[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {};
      UINT32 maxCLLogicalID = DMS_INVALID_LOGICCLID;

      OSS_INLINE BOOLEAN isOnline()const
      {
         return CS_STATUS_ONLINE == status;
      }

      BOOLEAN isValid()const;

      OSS_INLINE void reset()
      {
         version = 0;
         status = 0;
         type = 0;
         flags = 0;
         ossMemset(name, 0, sizeof(name));
         maxCLLogicalID = DMS_INVALID_LOGICCLID;
      }
   };//struct csMetaBlock
   const UINT32 CS_META_BLOCK_LEN = sizeof(csMetaBlock);

#pragma pack()


   BOOLEAN initCSMetaBlockPage(UINT32 pageSize,
                               PAGE_ID pid,
                               PAGE_ID lpid,
                               PAGE_SNAPSHOT_VERION psv,
                               void *buf);
}//namespace vessel
}//namespace engine

#endif//VESSEL_CS_META_BLOCK_PAGE_H_
