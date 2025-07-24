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

   Source File Name = btreeEntryPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

   INT32 btreeEntryPageAccessor::load(logicalPageBuffer *lpb,
                                      PAGE_ID &root,
                                      UINT32 &transferTick,
                                      btreeStatistics &stats)
   {  
      INT32 rc = SDB_OK;
      root = INVALID_PAGE_ID;
      transferTick = 0;
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
         transferTick = readableHead->transferTick;
         // btree statistics
         stats.nonleafNodeNum = readableHead->nonleafNodeNum;
         stats.leafNodeNum = readableHead->leafNodeNum;
         stats.compressedNodeNum = readableHead->compressedNodeNum;
         stats.totalEntryNum = readableHead->totalEntryNum;
         stats.compressedEntryNum = readableHead->compressedEntryNum;
         stats.origTotalEntrySize = readableHead->origTotalEntrySize;
         stats.realTotalEntrySize = readableHead->realTotalEntrySize;
         stats.totalPrefixNum = readableHead->totalPrefixNum;
         stats.totalEntryInserted = readableHead->totalEntryInserted;
         stats.totalEntryRemoved = readableHead->totalEntryRemoved;
         stats.nodesAllocated = readableHead->nodesAllocated;
         stats.nodesDestroyed = readableHead->nodesDestroyed;
         stats.newRootCreatedNum = readableHead->newRootCreatedNum;
         stats.childNodesRefilled = readableHead->childNodesRefilled;
         stats.nodesCompressedTimes = readableHead->nodesCompressedTimes;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeEntryPageAccessor::refill(requestContext *context,
                                        PAGE_ID root,
                                        UINT32 transferTick,
                                        const btreeStatistics &stats,
                                        logicalPageBuffer *lpb)
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
      head->transferTick = transferTick;
      /// btree statistics
      head->nonleafNodeNum = stats.nonleafNodeNum;
      head->leafNodeNum = stats.leafNodeNum;
      head->compressedNodeNum = stats.compressedNodeNum;
      head->totalEntryNum = stats.totalEntryNum;
      head->compressedEntryNum = stats.compressedEntryNum;
      head->origTotalEntrySize = stats.origTotalEntrySize;
      head->realTotalEntrySize = stats.realTotalEntrySize;
      head->totalPrefixNum = stats.totalPrefixNum;
      head->totalEntryInserted = stats.totalEntryInserted;
      head->totalEntryRemoved = stats.totalEntryRemoved;
      head->nodesAllocated = stats.nodesAllocated;
      head->nodesDestroyed = stats.nodesDestroyed;
      head->newRootCreatedNum = stats.newRootCreatedNum;
      head->childNodesRefilled = stats.childNodesRefilled;
      head->nodesCompressedTimes = stats.nodesCompressedTimes;
      lpb->commit(context->getExecutor()->getEndLsn());

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine