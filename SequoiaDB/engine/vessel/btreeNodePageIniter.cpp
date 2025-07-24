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

   Source File Name = btreeNodePageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/btreeNodePageIniter.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/btreeNodePage.h"
#include "vessel/btreeNode.h"

namespace engine
{
namespace vessel
{
   INT32 btreeNodePageIniter::initPage(requestContext *context,
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

      SDB_ASSERT(!rpb->isCacheBuffer(), "impossible");
      if (!initBtreeNodePage(rpb->getPageSize(),
                             rpb->getGlobalPid().page(),
                             lpid, psv,
                             _indexId, _isLeaf, _isRoot,
                             rpb->getWritableBuffer().getWPtr()))
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
      goto done;
   }

///////////////////////btreeRootPageIniter end

   INT32 btreeNodePageSplitIniter::initPage(requestContext *context,
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
      else if (OSS_UNLIKELY(!_data.isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!initCommonPage(PAGE_TYPE_BTREE_NODE,
                          rpb->getPageSize(),
                          rpb->getGlobalPid().page(),
                          lpid,
                          psv,
                          rpb->getWritableBuffer().getWPtr()))
      {
         PD_LOG(PDERROR, "failed to init btree node page page[%s]",
                rpb->getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = rpb->getWritableBodyBuffer().write(0, _data.getSize(), _data.data());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy page data");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rpb->commit(DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine