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

   Source File Name = btreeNodePageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNodePageIniter.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/btreeNodePage.h"
#include "vessel/btreeNode.h"

namespace engine
{
namespace vessel
{
   void btreeRootPageIniter::set(UINT32 clid, UINT32 indexId)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clid, "can not be invalid");
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexId, "can not be invalid");
      _logicalCLID = clid;
      _indexId = indexId;
      return;
   }

   INT32 btreeRootPageIniter::initPage(requestContext *context,
                                       PAGE_ID lpid,
                                       PAGE_SNAPSHOT_VERION psv,
                                       runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;
      if (NULL == context ||
          INVALID_PAGE_ID == lpid ||
          INVALID_PAGE_SNAPSHOT_VERSION == psv ||
          NULL == rpb ||
          !rpb->isWritingPrepared())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (DMS_INVALID_LOGICCLID == _logicalCLID ||
               INVALID_LOGICAL_INDEX_ID == _indexId)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(!rpb->isCacheBuffer(), "impossible");
      if (!initBtreeNodePage(rpb->getPageSize(),
                             rpb->getGlobalPid().page(),
                             lpid, psv,
                             _logicalCLID, _indexId,
                             (CHAR *)(rpb->getBuffer())))
      {
         PD_LOG(PDERROR, "failed to init btree node page page[%s]",
                rpb->getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rpb->commit(DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      if (NULL != rpb)
      {
         rpb->abort();
      }
      goto done;
   }

///////////////////////btreeRootPageIniter end

   INT32 btreeSplitPageIniter::initPage(requestContext *context,
                                        PAGE_ID lpid,
                                        PAGE_SNAPSHOT_VERION psv,
                                        runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;
      if (NULL == context ||
          INVALID_PAGE_ID == lpid ||
          INVALID_PAGE_SNAPSHOT_VERSION == psv ||
          NULL == rpb ||
          !rpb->isWritingPrepared())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _srcNode ||
                            !_srcNode->isValid() ||
                            INVALID_RECORD_SLOT_ID == _begin))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void btreeSplitPageIniter::set(const btreeNode *srcNode,
                                  RECORD_SLOT_ID begin)
   {
      _srcNode = srcNode;
      _begin = begin;
   }
} // namespace vessel

} // namespace engine