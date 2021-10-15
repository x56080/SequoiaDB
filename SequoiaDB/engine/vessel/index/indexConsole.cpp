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
#include "vessel/indexContextMap.h"
#include "vessel/dmlContext.h"
#include "ixmKey.hpp"
#include "ossSharedLatch.hpp"
#include "vessel/indexIterator.h"

#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmIndex.hpp"

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
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::releaseIndexDefPage(requestContext *context,
                                           INT32 indexSlot)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
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

   INT32 indexConsole::getOwnedIndexObj(requestContext *context,
                                        INT32 indexSlot,
                                        indexObject &obj,
                                        indexEntryPageHead *head)
   {
      INT32 rc = SDB_OK;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexEntryPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      if (!isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               !isValidIndexSlot(indexSlot))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _is->getIndexDefPage(context, _mbID, indexSlot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid of index def page:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.getIndexObject(context, &lpb, obj, TRUE, head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get owned in-mem obj:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::insert(requestContext *context,
                              INT32 indexSlot,
                              const indexObject &obj,
                              const ixmKey &key,
                              const DPS_TRANS_ID &transID,
                              DPS_LSN_OFFSET lsn,
                              const recordID &rid)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            !isValidIndexSlot(indexSlot) ||
                            !obj.isValid() ||
                            !key.isValid() ||
                            DPS_INVALID_LSN_OFFSET == lsn ||
                            !rid.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INDEX_TYPE_LSM == obj.getIndexType())
      {
         rc = lsmInsert(context, obj, key, transID, lsn, rid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert into lsm index:%d", rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "TODO");
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::lsmInsert(requestContext *context,
                                 const indexObject &obj,
                                 const ixmKey &key,
                                 const DPS_TRANS_ID &transID,
                                 DPS_LSN_OFFSET lsn,
                                 const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(obj.isValid(), "must be valid");
      globalIndexID gid(context->getLogicalCSID(),
                        context->getLogicalCLID(),
                        obj.getIndexID());
      lsmIndexMeta lsmMeta(gid, obj.getPattern().getOrdering());
      lsmIndex lsm;
      lsmKeyEntry lsmEntry;

      rc = lsm.init(&context->getEnv()->lsm, lsmMeta);
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

      BOOLEAN checkpointBlocked = FALSE;
      indexMappingPageIniter mappingIniter;
      indexEntryPageIniter defIniter;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexEntryPageAccessor accessor;
      indexMappingPageAccessor mappingAccessor;
      lpidLockHelper lh;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      UINT32 pos = 0;
      PAGE_ID mappingPage = _is->getMappingPageLpid(_mbID, indexSlot, pos);
      if (INVALID_PAGE_ID == mappingPage)
      {
         PD_LOG(PDERROR, "failed to get mapping page of [%d,%d]", _mbID, indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_IDX, mappingPage, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", mappingPage, rc);
         goto error;
      }

      rc = _is->ensureReservedPageMapped(context, mappingPage, &mappingIniter);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure mapping page:%d", rc);
         goto error;
      }

      lh.unlock();

      rc = _is->blockCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      rc = _is->allocatePages(context, &defIniter, 1, &lpid);
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
      lh.unlock();
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      if (INVALID_PAGE_ID != lpid)
      {
         _is->releasePages(context, 1, &lpid);
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
      lpidLockHelper lh;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      lpid = _is->getDirectMappedIndexLpid(_mbID, indexSlot);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of index def page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_IDX, lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = _is->ensureReservedPageMapped(context, lpid, &initer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure reserved page:%d", rc);
         goto error;
      }

      lh.unlock();

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
      lpb.fini();
      return rc;
   error:
      out = INVALID_PAGE_ID;
      goto done;
   }

   INT32 indexConsole::truncateIndex(requestContext *context,
                                     indexContext *ic)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            NULL == ic ||
                            !ic->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INDEX_TYPE_LSM == ic->getObj().getIndexType())
      {
         rc = lsmTruncate(context, ic->getObj());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate lsm index:%d", rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "TODO");
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::lsmTruncate(requestContext *context,
                                   const indexObject &obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(obj.isValid(), "must be valid");
      globalIndexID gid(context->getLogicalCSID(),
                        context->getLogicalCLID(),
                        obj.getIndexID());
      lsmIndexMeta meta(gid, obj.getPattern().getOrdering());
      lsmIndex lsm;

      rc = lsm.init(&context->getEnv()->lsm, meta);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm index:%d", rc);
         goto error;
      }

      rc = lsm.truncateIndex(context->getSession()->getLastLSN());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate lsm index[%s]:%d",
                obj.getIndexName().str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::loadIndexesWhenStartup(requestContext *context,
                                              indexContextMap *indexes)
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
         indexObject indexObj;
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

         rc = accessor.getIndexObject(context, &lpb, indexObj, FALSE, &head);
         if (SDB_IXM_NOTEXIST == rc)
         {
            PD_LOG(PDDEBUG, "index[%d] def page is not valid", i);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index def page head in page[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = indexes->insert(i, lpid, indexObj, (INDEX_STATUS)(head.status));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert index[%d] into context map:%d", i, rc);
            goto error;
         }
      }

      for (INT32 i = (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL;
           i < (INT32)MAX_INDEX_COUNT_PER_CL; ++i)
      {
         indexObject indexObj;
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
         if (SDB_IXM_NOTEXIST == rc)
         {
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index def lpid of slot[%d], rc:%d", i, rc);
            goto error;
         }

         rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = accessor.getIndexObject(context, &lpb, indexObj, FALSE, &head);
         if (SDB_IXM_NOTEXIST == rc)
         {
            PD_LOG(PDDEBUG, "index[%d] def page is not valid", i);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index def page head in page[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = indexes->insert(i, lpid, indexObj, (INDEX_STATUS)(head.status));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert index[%d] into context map:%d", i, rc);
            goto error;
         }
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


   INT32 indexConsole::dmlInsert(dmlContext *context,
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

      /// TODO: insert btree first

      rc = createLsmBatch(context, ra, lsmBatch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lsm batch:%d", rc);
         goto error;
      }

      status = context->getEnv()->lsm.Write(lsmBatch.getBatch());
      if (!status.ok())
      {
         PD_LOG(PDERROR, "failed to write lsm batch:%s", status.getState());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::createLsmBatch(dmlContext *context,
                                      const dmlIndexRequestArray &ra,
                                      lsmInsertBatch &lsmBatch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->isDmlPositionSet(), "must be set");

      lsmBatch.clear();
      UINT32 size = ra.getSize();
      for (UINT32 i = 0; i < size; ++i)
      {
         const dmlIndexRequest *ir = ra.get(i);
         SDB_ASSERT(NULL != ir && ir->isValid(), "impossible");
         if (ir->getContext()->getObj().getParams().type != INDEX_TYPE_LSM ||
             ir->isMerged())
         {
            continue;
         }

         globalIndexID gid(context->getLogicalCSID(),
                           context->getLogicalCLID(),
                           ir->getContext()->getIndexID());
         lsmIndexMeta meta(gid, ir->getContext()->getObj().getPattern().getOrdering());


         ossPoolList<bson::BSONObj>::const_iterator itr = ir->getKeys().begin();
         for (; itr != ir->getKeys().end(); ++itr)
         {
            ixmKeyOwned key(*itr);
            lsmKeyEntry ke;
            recordID rid(context->getLastDmlRid().getPageID(),
                            context->getLastDmlRid().getSlotID());

            ke.shallowCopy(key, rid, context->getLastDmlLSN(), context->getTransID());
            rc = lsmBatch.put(meta, ke, NULL);
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
                                             indexContext *ic,
                                             const bson::BSONObj &key,
                                             recordID &rid)
   {
      INT32 rc = SDB_OK;
      indexIterator *iterator = NULL;
      rid = recordID();

      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCSID == context->getLogicalCSID() ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       NULL == ic ||
                       !ic->isValid() ||
                       !ic->getObj().getParams().isUnique))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      iterator = createIndexIterator(ic->getObj().getIndexType());
      if (NULL == iterator)
      {
         PD_LOG(PDERROR, "failed to allocate itr obj");
         rc = SDB_OOM;
         goto error;
      }

      rc = iterator->open(context, ic, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open iterator:%d", rc);
         goto error;
      }
      
      rc = iterator->seek(key, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key:%d", rc);
         goto error;
      }

      while (iterator->isReadyToRead())
      {
         if (iterator->isMarkedRemoved())
         {
            rc = iterator->nextDiffKeyOrRid();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get next tuple:%d", rc);
               goto error;
            }
         }
         else
         {
            ixmKey ik;
            iterator->getKey(ik);
            _ixmKeyOwned ownedKey(key);
            if (ik.woEqual(ownedKey))
            {
               rid = iterator->getRid();
               SDB_ASSERT(rid.valid(), "impossible");
            }
            break;
         }
      }
   done:
      if (NULL != iterator)
      {
         iterator->close();
         SDB_OSS_DEL iterator;
      }
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::btreeInsert(requestContext *context,
                                   indexContext *ic,
                                   const ixmKey &key,
                                   const recordID &rid,
                                   const DPS_TRANS_ID &transID,
                                   DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != ic && ic->isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(rid.valid(), "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine