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

   Source File Name = clIndexMetaBlockPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CL_INDEX_META_BLOCK_PAGE_H_
#define VESSEL_CL_INDEX_META_BLOCK_PAGE_H_

#include "dms.hpp"
#include "vessel/pageIdentifier.h"
#include "vessel/indexDef.h"
#include "vessel/vesselIdDef.h"


namespace engine
{
namespace vessel
{
   constexpr UINT32 CL_DISK_INDEX_META_BLOCK_LEN = 512;
   constexpr UINT32 CL_INDEX_META_BLOCK_VERSION = 1;
   constexpr UINT32 CL_INVALID_INDEX_META_BLOCK_VERSION = 0;

#pragma pack(4)
   struct clIndexMetaBlock
   {
      clIndexMetaBlock()
      {
         for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
         {
            entryPageLpids[i] = INVALID_PAGE_ID;
         }
      }

      clIndexMetaBlock &operator=(const clIndexMetaBlock &o)
      {
         ossMemcpy(this, &o, sizeof(clIndexMetaBlock)); 
         return *this;
      }

      BOOLEAN isValid()const
      {
         return CL_INDEX_META_BLOCK_VERSION == version &&
                DMS_INVALID_LOGICCLID != clLogicalId;
      }

      void reset()
      {
         version = CL_INVALID_INDEX_META_BLOCK_VERSION;
         clLogicalId = DMS_INVALID_LOGICCLID;
         maxIndexLid = INVALID_LOGICAL_INDEX_ID;
         for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
         {
            entryPageLpids[i] = INVALID_PAGE_ID;
         }
      }

      UINT32 version = CL_INVALID_INDEX_META_BLOCK_VERSION;
      UINT32 clLogicalId = DMS_INVALID_LOGICCLID;
      UINT32 maxIndexLid = INVALID_LOGICAL_INDEX_ID;
      PAGE_ID entryPageLpids[MAX_INDEX_COUNT_PER_CL];
   };
   constexpr UINT32 CL_INDEX_META_BLOCK_LEN = sizeof(clIndexMetaBlock);

   struct clIndexMetaBlockOnDisk
   {
      clIndexMetaBlock block;
      CHAR pad[CL_DISK_INDEX_META_BLOCK_LEN - CL_INDEX_META_BLOCK_LEN];
   };
   static_assert(CL_DISK_INDEX_META_BLOCK_LEN == sizeof(clIndexMetaBlockOnDisk),
                 "invalid size");

#pragma pack()


   UINT32 getCapacityOfIndexMetaBlockPage(UINT32 pageSize);

   BOOLEAN initCLIndexMetaBlockPage(UINT32 pageSize,
                                    PAGE_ID pid,
                                    PAGE_ID lpid,
                                    PAGE_SNAPSHOT_VERION psv,
                                    void *buf);
   
   PAGE_ID getIndexMetaBlockPageLpid(UINT32 pageSize,
                                     CL_MB_ID mbID);

   INT32 getIndexMetaBlockPos(UINT32 pageSize,
                              CL_MB_ID mbID);

}// namespace vessel
}// namespace engine

#endif // VESSEL_CL_INDEX_META_BLOCK_H