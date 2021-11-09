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

   Source File Name = indexMappingPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexMappingPageAccessor.h"
#include "vessel/requestContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 indexMappingPageAccessor::getIndexDefPage(requestContext *context,
                                                   UINT32 pos,
                                                   logicalPageBuffer &lpb,
                                                   PAGE_ID &lpid)const
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      const PAGE_ID *page = NULL;
      lpid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(NULL == context ||
                       !lpb.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb.validatePage(PAGE_TYPE_INDEX_MAPPING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      capacity = getIndexMappingPageCapacity(lpb.getRuntimeBuffer().getPageSize());
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get index mapping page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (capacity <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      page = lpb.getReadableBodySlice().getReadableObjPtr<PAGE_ID>(sizeof(PAGE_ID) * pos);
      if (NULL == page)
      {
         PD_LOG(PDERROR, "failed to get page ptr of pos[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      if (INVALID_PAGE_ID != *page)
      {
         lpid = *page;
      }
      else
      {
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
   done:
      return rc;
   error:
      lpid = INVALID_PAGE_ID;
      goto done;
   }

   INT32 indexMappingPageAccessor::addNewMapping(requestContext *context,
                                                 UINT32 pos,
                                                 PAGE_ID lpid,
                                                 logicalPageBuffer &lpb)const
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      PAGE_ID *slot = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid ||
                       !lpb.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb.validatePage(PAGE_TYPE_INDEX_MAPPING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      capacity = getIndexMappingPageCapacity(lpb.getPageSize());
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get index mapping page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (capacity <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = lpb.prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }

      slot = lpb.getWritableBodySlice().getWritableObjPtr<PAGE_ID>(pos * sizeof(PAGE_ID));
      if (NULL == slot)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of slot");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      *slot = lpid;
      
      lpb.commit(context->getExecutor()->getEndLsn());

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine