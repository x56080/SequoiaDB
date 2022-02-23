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

   Source File Name = indexConsole.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexConsole.h"
#include "vessel/collectionRecordPage.h"
#include "ossLikely.hpp"
#include "vessel/indexSpace.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/indexEntryPageAccessor.h"
#include "vessel/indexEntryPageIniter.h"
#include "vessel/indexMappingPageIniter.h"
#include "vessel/indexMappingPageAccessor.h"
#include "vessel/requestContext.h"
#include "vessel/indexDef.h"
#include "vessel/globalIndexID.h"
#include "vessel/instanceEnv.h"
#include "dmsRBSSUMgr.hpp"
#include "vessel/indexObjectMap.h"
#include "vessel/dmlContext.h"
#include "ixmKey.hpp"
#include "ossSharedLatch.hpp"
#include "vessel/indexIterator.h"
#include "vessel/runtimeMbContext.h"

#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmIndex.hpp"

#include "vessel/btreeAccessor.h"

namespace engine
{
namespace vessel
{
   void indexConsole::init(CL_MB_ID mbID, indexSpace *is)
   {
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(NULL != is, "can not be null");
      SDB_ASSERT(is->isOpen(), "must be open");
      _mbID = mbID;
      _is = is;
      return;
   }

   void indexConsole::fini()
   {
      _mbID = INVALID_CL_MB_ID;
      _is = NULL;
      return;
   }

   INT32 indexConsole::createIndex(requestContext *context,
                                   INT32 indexSlot,
                                   UINT32 indexId,
                                   const slice &defObj,
                                   PAGE_ID &lpid)const
   {
      INT32 rc = SDB_OK;
      BOOLEAN checkpointBlocked = FALSE;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                           !isValidIndexSlot(indexSlot) ||
                           INVALID_LOGICAL_INDEX_ID == indexId ||
                           !defObj.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _is->blockCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      if (indexSlot < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL)
      {
         rc = createDirectMappedIndex(context, indexSlot, indexId, defObj, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create direct mapped index:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = createDoubleMappedIndex(context, indexSlot, indexId, defObj, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create double mapped index:%d", rc);
            goto error;
         }
      }
   done:
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::releaseIndexEntryPage(requestContext *context,
                                             INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      BOOLEAN blocked = FALSE;
      
      if (OSS_UNLIKELY(NULL == context ||
                       !isValidIndexSlot(indexSlot)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _is->blockCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      if (indexSlot < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL)
      {
         SDB_ASSERT(FALSE, "impossible");
      }
      else
      {
         rc = releaseDoubleMappedIndexEntry(context, indexSlot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to release index entry:%d", rc);
            goto error;
         }
      }
      
   done:
      if (blocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::updateIndexStatus(requestContext *context,
                                         UINT32 indexId,
                                         PAGE_ID lpid,
                                         INDEX_STATUS status)
   {
      INT32 rc = SDB_OK;
      logicalPageBuffer lpb;
      indexEntryPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (!isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               INVALID_LOGICAL_INDEX_ID == indexId ||
               INVALID_PAGE_ID == lpid ||
               INDEX_STATUS_INVALID == status)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.updateIndexStatus(context, indexId, status, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index status:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::insert(requestContext *context,
                              indexObject *obj,
                              const ixmKey &key,
                              const recordID &rid,
                              const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            NULL == obj ||
                            !obj->isValid() ||
                            !key.isValid() ||
                            !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INDEX_TYPE_LSM == obj->getDescription().getType())
      {
         rc = lsmInsert(context, obj, key, rid, transID);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert into lsm index:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = btreeInsert(context, obj, key, rid, transID);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert into btree index:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::lsmInsert(requestContext *context,
                                 indexObject *obj,
                                 const ixmKey &key,
                                 const recordID &rid,
                                 const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && context->isMbContextAttached(), "can not be null");
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();
      const globalCollectionId &gcid = context->getMbContext()->getGlobalId();
      SDB_ASSERT(gcid.isValid(), "can not be invalid");

      globalIndexID gid(gcid.getCSLid(),
                        gcid.getCLLid(),
                        obj->getIndexId().getLogicalIndexId());
      lsmIndexMeta lsmMeta(gid, obj->getDescription().getPattern().getOrdering());
      lsmIndex lsm;
      lsmKeyEntry lsmEntry;

      rc = lsm.init(context->getEnv()->lsm, lsmMeta);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm index:%d", rc);
         goto error;
      }

      lsmEntry.shallowCopy(key, rid, lsn, transID);

      rc = lsm.keyInsert(lsmEntry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into lsm index:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::createDoubleMappedIndex(requestContext *context,
                                               INT32 indexSlot,
                                               UINT32 indexId,
                                               const slice &defObj,
                                               PAGE_ID &out)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      SDB_ASSERT((INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL <= indexSlot, "impossible");

      
      indexMappingPageIniter mappingIniter;
      indexEntryPageIniter defIniter;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexEntryPageAccessor accessor;
      indexMappingPageAccessor mappingAccessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      BOOLEAN mappingPageLocked = FALSE;

      UINT32 pos = 0;
      PAGE_ID mappingPage = _is->getMappingPageLpid(_mbID, indexSlot, pos);
      if (INVALID_PAGE_ID == mappingPage)
      {
         PD_LOG(PDERROR, "failed to get mapping page of [%d,%d]", _mbID, indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = context->lockLpid(SPACE_TYPE_IDX, mappingPage, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", mappingPage, rc);
         goto error;
      }
      mappingPageLocked = TRUE;

      rc = _is->ensureReservedPageMapped(context, mappingPage, &mappingIniter);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure mapping page:%d", rc);
         goto error;
      }

      context->unlockLpid(SPACE_TYPE_IDX, mappingPage);
      mappingPageLocked = FALSE;

      rc = _is->allocatePage(context, &defIniter, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate index def page:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.createIndex(context, indexId, defObj, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create save index info on page:%d", rc);
         goto error;
      }

      lpb.fini();
      rc = _is->getLogicalPageBuffer(context, mappingPage, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] buffer:%d", mappingPage, rc);
         goto error;
      }

      rc = mappingAccessor.addNewMapping(context, pos, lpid, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to add new index mapping:%d", rc);
         goto error;
      }

      out = lpid;
      
   done:
      if (mappingPageLocked)
      {
         context->unlockLpid(SPACE_TYPE_IDX, mappingPage);
      }
      lpb.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != lpid)
      {
         _is->releasePage(context, lpid);
      }
      out = INVALID_PAGE_ID;
      goto done;
   }

   INT32 indexConsole::createDirectMappedIndex(requestContext *context,
                                               INT32 indexSlot,
                                               UINT32 indexId,
                                               const slice &defObj,
                                               PAGE_ID &out)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      SDB_ASSERT(indexSlot < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL, "impossible");
      
      indexEntryPageIniter initer;
      logicalPageBuffer lpb;
      indexEntryPageAccessor accessor;
      PAGE_ID lpid = INVALID_PAGE_ID;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      BOOLEAN locked = FALSE;

      lpid = _is->getDirectMappedIndexLpid(_mbID, indexSlot);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of index def page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = context->lockLpid(SPACE_TYPE_IDX, lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      locked = TRUE;

      rc = _is->ensureReservedPageMapped(context, lpid, &initer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure reserved page:%d", rc);
         goto error;
      }

      context->unlockLpid(SPACE_TYPE_IDX, lpid);
      locked = FALSE;

      rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = accessor.createIndex(context, indexId, defObj, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save index info when creating:%d", rc);
         goto error;
      }

      out = lpid;
   done:
      if (locked)
      {
         context->unlockLpid(SPACE_TYPE_IDX, lpid);
      }
      lpb.fini();
      return rc;
   error:
      out = INVALID_PAGE_ID;
      goto done;
   }

   INT32 indexConsole::truncateIndex(requestContext *context,
                                     indexObject *obj)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            NULL == obj ||
                            !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INDEX_TYPE_LSM == obj->getDescription().getType())
      {
         rc = lsmTruncate(context, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate lsm index:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = btreeTruncate(context, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate btree index:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::lsmTruncate(requestContext *context,
                                   const indexObject *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && context->isMbContextAttached(), "can not be invalid");
      SDB_ASSERT(obj->isValid(), "must be valid");
      const globalCollectionId &gcid = context->getMbContext()->getGlobalId();
      SDB_ASSERT(gcid.isValid(), "can not be invalid");
      globalIndexID gid(gcid.getCSLid(),
                        gcid.getCLLid(),
                        obj->getIndexId().getLogicalIndexId());
      lsmIndexMeta meta(gid, obj->getDescription().getPattern().getOrdering());
      lsmIndex lsm;

      rc = lsm.init(context->getEnv()->lsm, meta);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm index:%d", rc);
         goto error;
      }

      rc = lsm.truncateIndex(context->getExecutor()->getEndLsn());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate lsm index[%s]:%d",
                obj->getDescription().getName().c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::loadIndexesWhenStartup(requestContext *context,
                                              indexObjectMap *indexes)
   {
      INT32 rc = SDB_OK;
      indexEntryPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               NULL == indexes)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      indexes->fini();

      for (INT32 i = 0; i < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL; ++i)
      {
         indexDescription desc;
         indexEntryPageHead head;
         logicalPageBuffer lpb;
         PAGE_ID lpid = _is->getDirectMappedIndexLpid(_mbID, i);
         SDB_ASSERT(INVALID_PAGE_ID != lpid, "impossible");
         BOOLEAN mapped = FALSE;

         rc = _is->isLogicalPageMapped(context, lpid, mapped);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get test if lpid[%d] mapped:%d", lpid, rc);
            goto error;
         }
         
         if (!mapped)
         {
            continue;
         }

         rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = accessor.getIndexDescription(context, &lpb, desc, &head);
         if (SDB_IXM_NOTEXIST == rc)
         {
            PD_LOG(PDDEBUG, "index[%d] def page is not valid", i);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index description "
                   "and def page head in page[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = indexes->insert(i, head.indexLogicalID, lpid, desc, 
                              (INDEX_STATUS)(head.status),
                              head.btreeRoot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert index[%d] into context map:%d", i, rc);
            goto error;
         }
      }

      for (INT32 i = (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL;
           i < (INT32)MAX_INDEX_COUNT_PER_CL; ++i)
      {
         indexDescription desc;
         indexEntryPageHead head;
         logicalPageBuffer lpb;
         UINT32 pos = 0;
         PAGE_ID lpid = INVALID_PAGE_ID;
         PAGE_ID mappingLpid = _is->getMappingPageLpid(_mbID, i, pos);
         SDB_ASSERT(INVALID_PAGE_ID != mappingLpid, "impossible");
         BOOLEAN mapped = FALSE;

         rc = _is->isLogicalPageMapped(context, mappingLpid, mapped);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get test if lpid[%d] mapped:%d", lpid, rc);
            goto error;
         }
         
         if (!mapped)
         {
            continue;
         }

         rc = _is->getIndexDefPage(context, _mbID, i, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index def lpid of slot[%d], rc:%d", i, rc);
            goto error;
         }

         if (INVALID_PAGE_ID == lpid)
         {
            continue;
         }

         rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = accessor.getIndexDescription(context, &lpb, desc, &head);
         if (SDB_IXM_NOTEXIST == rc)
         {
            PD_LOG(PDDEBUG, "index[%d] def page is not valid", i);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index description "
                   "and def page head in page[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = indexes->insert(i, head.indexLogicalID, lpid, desc, 
                              (INDEX_STATUS)(head.status),
                              head.btreeRoot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert index[%d] into context map:%d", i, rc);
            goto error;
         }
      }

      rc = cacheBtreeRootSplitTimes(context, indexes);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache btree root split times:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != indexes)
      {
         indexes->fini();
      }
      goto done;
   }


   INT32 indexConsole::handleDmlRequest(dmlContext *context,
                                        const dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      lsmInsertBatch lsmBatch;
      rocksdb::Status status;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               !context->isDmlPositionSet())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (ra.isEmpty())
      {
         goto done;
      }

      rc = createLsmBatch(context, ra, lsmBatch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lsm batch:%d", rc);
         goto error;
      }

      if (!lsmBatch.isEmpty() && NULL == context->getEnv()->lsm)
      {
         PD_LOG(PDERROR, "lsm index instance not inited yet");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = btreeCommit(context, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into btree index:%d", rc);
         goto error;
      }

      if (!lsmBatch.isEmpty())
      {
         status = context->getEnv()->lsm->Write(lsmBatch.getBatch());
         if (!status.ok())
         {
            PD_LOG(PDERROR, "failed to write lsm batch:%s", status.getState());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      return rc;
   error:
      ///TODO: rollback index inserted to btree
      goto done;
   }

   INT32 indexConsole::createLsmBatch(dmlContext *context,
                                      const dmlIndexRequestArray &ra,
                                      lsmInsertBatch &lsmBatch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && context->isMbContextAttached(), "can not be null");
      SDB_ASSERT(context->isDmlPositionSet(), "must be set");
      const globalCollectionId &gcid = context->getMbContext()->getGlobalId();
      SDB_ASSERT(gcid.isValid(), "can not be invalid");

      lsmBatch.clear();
      UINT32 size = ra.getSize();
      for (UINT32 i = 0; i < size; ++i)
      {
         const dmlIndexRequest *ir = ra.get(i);
         SDB_ASSERT(NULL != ir && ir->isValid(), "impossible");
         if (ir->getObject()->getDescription().getType() != INDEX_TYPE_LSM ||
             ir->isExecuted())
         {
            continue;
         }

         globalIndexID gid(gcid.getCSLid(),
                           gcid.getCLLid(),
                           ir->getObject()->getIndexId().getLogicalIndexId());
         lsmIndexMeta meta(gid, ir->getObject()->getDescription().getPattern().getOrdering());


         ossPoolList<bson::BSONObj>::const_iterator itr = ir->getKeysToInsert().begin();
         for (; itr != ir->getKeysToInsert().end(); ++itr)
         {
            ixmKeyOwned key(*itr);
            lsmKeyEntry ke;
            lsmIndexValue vl;
            ke.shallowCopy(key, context->getRid(), context->getDmlLSN(),
                           context->getExecutor()->getTransID());
            vl.reset(LSM_VALUE_TYPE_INSERT);
            rc = lsmBatch.put(meta, ke, &vl);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push data into batch:%d", rc);
               goto error;
            }
         }
         
         itr = ir->getKeysToRemove().begin();
         for (; itr != ir->getKeysToRemove().end(); ++itr)
         {
            ixmKeyOwned key(*itr);
            lsmKeyEntry ke;
            lsmIndexValue vl;
            ke.shallowCopy(key, context->getRid(), context->getDmlLSN(),
                           context->getExecutor()->getTransID());
            vl.reset(LSM_VALUE_TYPE_DELETE);
            rc = lsmBatch.put(meta, ke, &vl);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push data into batch:%d", rc);
               goto error;
            }
         }

      }
   done:
      return rc;
   error:
      lsmBatch.clear();
      goto done;
   }

   INT32 indexConsole::checkUniqueConstraint(requestContext *context,
                                             indexObject *obj,
                                             const bson::BSONObj &key,
                                             recordID &rid)
   {
      INT32 rc = SDB_OK;
      rid = recordID();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       NULL == obj ||
                       !obj->isValid() ||
                       !obj->getDescription().isUnique()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INDEX_TYPE_LSM == obj->getDescription().getType())
      {
         lsmIndexIterator lsmItr;
         
         rc = checkUniqueConstraintByIterator(context, obj, &lsmItr, key, rid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to check unique constraint by lsm iterator:%d", rc);
            goto error;
         }
      }
      else if (INDEX_TYPE_BTREE == obj->getDescription().getType())
      {
         btreeIndexIterator btreeItr;
         rc = checkUniqueConstraintByIterator(context, obj, &btreeItr, key, rid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to check unique constraint by btree iterator:%d", rc);
            goto error;
         }
      }

      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::checkUniqueConstraintByIterator(requestContext *context,
                                                       indexObject *obj,
                                                       indexIterator *iterator,
                                                       const bson::BSONObj &key,
                                                       recordID &rid)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != iterator, "can not be null");
      SDB_ASSERT(key.isValid(), "can not be invalid");

      ixmKeyOwned ownedKey(key);
      indexIterator::options o(FALSE, TRUE);
      rid = recordID();

      rc = iterator->open(context, obj, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index iterator:%d", rc);
         goto error;
      }
         
      rc = iterator->contains(ownedKey, rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to check if contains key:%d", rc);
         goto error;
      }
   done:
      iterator->close();
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::btreeInsert(requestContext *context,
                                   indexObject *obj,
                                   const ixmKey &key,
                                   const recordID &rid,
                                   const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      btreeAccessor accessor;
      rc = accessor.init(context, obj, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree accessor:%d", rc);
         goto error;
      }

      rc = accessor.insert(key, rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into btree:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::btreeCommit(dmlContext *context,
                                   const dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");

      for (UINT32 i = 0; i < ra.getSize(); ++i)
      {
         btreeAccessor accessor;
         const dmlIndexRequest *req = ra.get(i);
         SDB_ASSERT(NULL != req && req->isValid(), "can not be invalid");
         if (!req->getObject()->getDescription().isBtreeIndex() ||
              req->isExecuted())
         {
            continue;
         }
             
         rc = accessor.init(context, req->getObject(),
                            context->getOrigTransId());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init btree accessor[%s]:%d",
                   req->getObject()->getDescription().getName().c_str(), rc);
            goto error;  
         }

         for (ossPoolList<bson::BSONObj>::const_iterator itr = req->getKeysToInsert().begin();
              itr != req->getKeysToInsert().end(); ++itr)
         {
            rc = accessor.insert(ixmKeyOwned(*itr),
                                 context->getRid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert index key:%d", rc);
               goto error;
            }
         }

         for (ossPoolList<bson::BSONObj>::const_iterator itr = req->getKeysToRemove().begin();
              itr != req->getKeysToRemove().end(); ++itr)
         {
            rc = accessor.remove(ixmKeyOwned(*itr),
                                 context->getRid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove index key:%d", rc);
               goto error;
            }
         } 
      }
   done:
      return rc;
   error:
      SDB_ASSERT(FALSE, "TODO");
      goto done;
   }

   INT32 indexConsole::cacheBtreeRootSplitTimes(requestContext *context,
                                                indexObjectMap *indexes)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be invalid");
      SDB_ASSERT(NULL != indexes, "can not be invalid");

      ossSharedLatchMode mode;
      mode.setShared();
      logicalPageSpace *lps = NULL;
      rc = context->getEnv()->dms.getLogicalPageSpace(context->getSpaceID(),
                                                      SPACE_TYPE_IDX, &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps of[%d], rc:%d", context->getSpaceID(), rc);
         goto error;
      }
      
      for (indexObjectMap::ITERATOR itr = indexes->begin();
           itr != indexes->end(); ++itr)
      {
         btreeNode node;
         indexObject *obj = itr->second;
         SDB_ASSERT(NULL != obj && obj->isValid(), "can not be invalid");
         if (INDEX_TYPE_BTREE == obj->getDescription().getType() &&
             obj->hasBtreeRoot())
         {
            logicalPageBuffer lpb;
            rc = lps->getLogicalPageBuffer(context, obj->getBtreeRoot(),
                                           mode, lpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d",
                      obj->getBtreeRoot(), rc);
               goto error;
            }

            node = btreeNode(&lpb, 0, obj);
            obj->updateBtreeRootSplitTimes(node.getSplitedTimes());

            lpb.fini();
         }

      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::releaseDoubleMappedIndexEntry(requestContext *context,
                                                     INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      UINT32 pos = 0;
      logicalPageBuffer mappingBuffer;
      indexMappingPageAccessor accessor;
      PAGE_ID entryPage = INVALID_PAGE_ID;

      PAGE_ID mappingPage = _is->getMappingPageLpid(_mbID, indexSlot, pos);
      if (INVALID_PAGE_ID == mappingPage)
      {
         PD_LOG(PDERROR, "failed to get mapping page of [%d,%d]", _mbID, indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, mappingPage, mode, mappingBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", mappingPage, rc);
         goto error;
      }

      rc = accessor.unmapIndex(context, mappingBuffer, pos, entryPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to unmap index at[%d, %d], rc:%d",
                mappingPage, pos, rc);
         goto error;
      }

      mappingBuffer.fini();

      _is->releasePage(context, entryPage);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::btreeTruncate(requestContext *context,
                                     indexObject *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != obj && obj->isValid(), "can not be invalid");
      SDB_ASSERT(obj->getDescription().getType() == INDEX_TYPE_BTREE, "must be btree");
      SDB_ASSERT(obj->isTruncating() || obj->isRemoving(), "update status first");

      btreeAccessor accessor;
      rc = accessor.init(context, obj, DPS_TRANS_ID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree accessor:%d", rc);
         goto error;
      }

      rc = accessor.truncate();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate btree index:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine