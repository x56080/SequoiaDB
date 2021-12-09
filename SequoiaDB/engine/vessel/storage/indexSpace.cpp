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

   Source File Name = indexSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexSpace.h"
#include "vessel/idMapPage.h"
#include "vessel/indexDef.h"
#include "vessel/indexMappingPage.h"
#include "vessel/indexMappingPageAccessor.h"

namespace engine
{
namespace vessel
{
   static const UINT32 TOTAL_DIRECT_MAPPED_IMP = 65536 * DIRECT_MAPPING_INDEX_COUNT_PER_CL / ID_MAP_PAGE_CAPACITY;

   UINT32 indexSpace::getReservedImpCount()const
   {
      return TOTAL_DIRECT_MAPPED_IMP + 1;
   }

   PAGE_ID indexSpace::getDirectMappedIndexLpid(CL_MB_ID mbID, INT32 slot)const
   {
      PAGE_ID lpid = INVALID_PAGE_ID;
      if (INVALID_CL_MB_ID != mbID &&
          isValidIndexSlot(slot))
      {
         lpid = mbID * DIRECT_MAPPING_INDEX_COUNT_PER_CL + slot;
      }
      return lpid;
   }

   PAGE_ID indexSpace::getMappingPageLpid(CL_MB_ID mbID,
                                          INT32 slot,
                                          UINT32 &pos)const
   {
      PAGE_ID lpid = INVALID_PAGE_ID;
      static constexpr UINT32 _BEGIN_LPID = TOTAL_DIRECT_MAPPED_IMP *
                                            ID_MAP_PAGE_CAPACITY;

      if (INVALID_CL_MB_ID != mbID &&
          isValidIndexSlot(slot) &&
          (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL <= slot)
      {
         UINT32 globalPos = ((MAX_INDEX_COUNT_PER_CL - DIRECT_MAPPING_INDEX_COUNT_PER_CL)
                             * mbID + slot - DIRECT_MAPPING_INDEX_COUNT_PER_CL);
         UINT32 pageSize = _storage.getCoreArgs().pageSize;
         UINT32 capacity = getIndexMappingPageCapacity(pageSize);
         if (0 == capacity)
         {
            goto done;
         }
         lpid = (globalPos / capacity) + _BEGIN_LPID;
         pos = globalPos % capacity;
         SDB_ASSERT(lpid < (getReservedImpCount() * ID_MAP_PAGE_CAPACITY), "impossible");
      }

   done:
      return lpid;
   }

   INT32 indexSpace::getIndexDefPage(requestContext *context,
                                     CL_MB_ID mbID,
                                     INT32 slot,
                                     PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      logicalPageBuffer lpb;
      
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_CL_MB_ID == mbID ||
                            !isValidIndexSlot(slot)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (slot < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL)
      {
         lpid = getDirectMappedIndexLpid(mbID, slot);
      }
      else
      {
         indexMappingPageAccessor accessor;
         PAGE_ID mappingPageLpid = INVALID_PAGE_ID;
         UINT32 pos = -1;
         mappingPageLpid = getMappingPageLpid(mbID, slot, pos);
         if (INVALID_PAGE_ID == mappingPageLpid)
         {
            PD_LOG(PDERROR, "failed to get mapping page of [%d,%d]",
                   mbID, slot);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = getLogicalPageBuffer(context, mappingPageLpid,
                                   ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED),
                                   lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] buffer:%d",
                   mappingPageLpid, rc);
            goto error;
         }

         rc = accessor.getIndexDefPage(context, pos, lpb, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index def page:%d", rc);
            goto error;
         }
         
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine