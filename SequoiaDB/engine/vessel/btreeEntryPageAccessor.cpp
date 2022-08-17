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

   Source File Name = btreeEntryPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeEntryPageAccessor.h"
#include "vessel/requestContext.h"
#include "vessel/indexUtils.h"


namespace engine
{
namespace vessel
{
   btreeEntryPageAccessor::btreeEntryPageAccessor(UINT32 lid):
   _indexLid(lid)
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != _indexLid, "can not be invalid");
   }

   INT32 btreeEntryPageAccessor::resetBtreeRoot(requestContext *context,
                                                PAGE_ID root,
                                                logicalPageBuffer *lpb)const
   {
      INT32 rc = SDB_OK;
      btreeEntryPageHead *head = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == lpb ||
                       !lpb->isWritable()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_BTREE_ENTRY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = lpb->getWritableBodyBuffer().getWritableObjPtr<btreeEntryPageHead>(0);
      if (nullptr == head)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (!head->isValid())
      {
         PD_LOG(PDERROR, "index def page head is not valid");
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
      else if (_indexLid != head->logicalIndexId)
      {
         PD_LOG(PDERROR, "index logical id [%d] does match the one[%d] in head",
                _indexLid, head->logicalIndexId);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      head->btreeRoot = root;
      lpb->commit(context->getExecutor()->getEndLsn());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeEntryPageAccessor::load(logicalPageBuffer *lpb,
                                      PAGE_ID &root,
                                      btreeStatistics &stats)
   {  
      INT32 rc = SDB_OK;
      root = INVALID_PAGE_ID;
      stats.reset();

      if (OSS_UNLIKELY(nullptr == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         const btreeEntryPageHead *readableHead = nullptr;
         rc = lpb->validatePage(PAGE_TYPE_BTREE_ENTRY);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                  lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
            goto error;
         }

         readableHead = lpb->getReadableBodyBuffer().getReadableObjPtr<btreeEntryPageHead>(0);
         if (OSS_UNLIKELY(nullptr == readableHead))
         {
            PD_LOG(PDERROR, "failed to get readable ptr of head");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (!readableHead->isValid())
         {
            PD_LOG(PDERROR, "index def page head is not valid");
            rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
            goto error;
         }
         else if (_indexLid != readableHead->logicalIndexId)
         {
            PD_LOG(PDERROR, "index logical id [%d] does match the one[%d] in head",
                  _indexLid, readableHead->logicalIndexId);
            rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
            goto error;
         }

         root = readableHead->btreeRoot;
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine