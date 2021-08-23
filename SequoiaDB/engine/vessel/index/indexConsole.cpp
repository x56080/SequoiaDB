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
#include "vessel/indexDefPageAccessor.h"
#include "vessel/indexDefPageIniter.h"
#include "vessel/indexMappingPageIniter.h"
#include "vessel/indexMappingPageAccessor.h"
#include "vessel/requestContext.h"
#include "vessel/indexDef.h"
#include "vessel/globalIndexID.h"
#include "vessel/instanceEnv.h"
#include "dmsRBSSUMgr.hpp"
#include "vessel/collectionIndexContext.h"
#include "vessel/dmlContext.h"
#include "ixmKey.hpp"


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
                                   const slice &defObj)const
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
         rc = createDirectMappedIndex(context, indexSlot, indexId, defObj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create direct mapped index:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = createDoubleMappedIndex(context, indexSlot, indexId, defObj);
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


   INT32 indexConsole::dumpIndex(requestContext *context,
                                 INT32 indexSlot,
                                 bson::BSONObj &obj)const
   {
      INT32 rc = SDB_OK;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexDefPageAccessor accessor;
      bson::BSONObjBuilder builder;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      if (!isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context || !isValidIndexSlot(indexSlot))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _is->getIndexDefPage(context, _mbID, indexSlot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid of index[%d], rc:%d", indexSlot, rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
         goto error;
      }

      builder.append(VESSEL_INDEX_FIELD_NAME_INDEX_SLOT, indexSlot);
      rc = accessor.dump(context, &lpb, builder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to dump index info:%d", rc);
         goto error;
      }

      obj = builder.obj();
   done:
      lpb.fini();
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
                                         INT32 indexSlot,
                                         INDEX_STATUS status)
   {
      INT32 rc = SDB_OK;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexDefPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (!isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               !isValidIndexSlot(indexSlot) ||
               INDEX_STATUS_INVALID == status)
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

      rc = accessor.updateIndexStatus(context, status, &lpb);
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
                                        indexDefHead *head)
   {
      INT32 rc = SDB_OK;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexDefPageAccessor accessor;
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
                              const dmsRecordID &rid)
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
                            !rid.isValid()))
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
                                 const dmsRecordID &rid)
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
                                                const slice &defObj)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      SDB_ASSERT((INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL <= indexSlot, "impossible");

      BOOLEAN checkpointBlocked = FALSE;
      indexMappingPageIniter mappingIniter;
      indexDefPageIniter defIniter;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexDefPageAccessor accessor;
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
      goto done;
   }

   INT32 indexConsole::createDirectMappedIndex(requestContext *context,
                                               INT32 indexSlot,
                                               UINT32 indexId,
                                               const slice &defObj)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      SDB_ASSERT(indexSlot < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL, "impossible");
      
      indexDefPageIniter initer;
      logicalPageBuffer lpb;
      indexDefPageAccessor accessor;
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
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::testIfDuplicated(requestContext *context,
                                        INT32 indexSlot,
                                        const strSlice &indexName,
                                        const indexKeyPattern &pattern,
                                        BOOLEAN &duplicated)const
   {
      INT32 rc = SDB_OK;
    
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexDefPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            !isValidIndexSlot(indexSlot) ||
                            indexName.empty() ||
                            !pattern.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _is->getIndexDefPage(context, _mbID, indexSlot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index def page of slot[%d]:%d", indexSlot, rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.testIndexDef(context, indexName, pattern, &lpb, duplicated);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test if duplicated:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::truncateIndex(requestContext *context,
                                     INT32 indexSlot,
                                     const indexObject &obj)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            !isValidIndexSlot(indexSlot) ||
                            !obj.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INDEX_TYPE_LSM == obj.getIndexType())
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
                                              collectionIndexContext *indexContext)
   {
      INT32 rc = SDB_OK;
      indexDefPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               NULL == indexContext)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      indexContext->fini();

      for (INT32 i = 0; i < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL; ++i)
      {
         indexObject indexObj;
         indexDefHead head;
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

         if (indexContext->getNextIndexId() <= head.indexLogicalID)
         {
            indexContext->setNextIndexId(head.indexLogicalID + 1);
         }

         if (INDEX_STATUS_NORMAL == head.status)
         {
            indexContext->unfreeIndexSlot(i, indexObj.getParams().isUnique);
         }
         else
         {
            SDB_ASSERT(FALSE, "TODO");
         }
      }

      for (INT32 i = (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL;
           i < (INT32)MAX_INDEX_COUNT_PER_CL; ++i)
      {
         indexObject indexObj;
         indexDefHead head;
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

         if (indexContext->getNextIndexId() <= head.indexLogicalID)
         {
            indexContext->setNextIndexId(head.indexLogicalID + 1);
         }

         if (INDEX_STATUS_NORMAL == head.status)
         {
            indexContext->unfreeIndexSlot(i, indexObj.getParams().isUnique);
         }
         else
         {
            SDB_ASSERT(FALSE, "TODO");
         }
      }
   done:
      return rc;
   error:
      if (NULL != indexContext)
      {
         indexContext->fini();
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

      rc = createLsmBatch(context, ra, lsmBatch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lsm batch:%d", rc);
         goto error;
      }

      /// TODO: insert btree first

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
         if (ir->isIgnored() ||
             ir->getIndexType() != INDEX_TYPE_LSM)
         {
            continue;
         }

         globalIndexID gid(context->getLogicalCSID(),
                           context->getLogicalCLID(),
                           ir->getIndexObj().getIndexID());
         lsmIndexMeta meta(gid, ir->getIndexObj().getPattern().getOrdering());


         ossPoolList<bson::BSONObj>::const_iterator itr = ir->getKeys().begin();
         for (; itr != ir->getKeys().end(); ++itr)
         {
            ixmKeyOwned key(*itr);
            lsmKeyEntry ke;
            dmsRecordID rid(context->getLastDmlRid().getPageID(),
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
}//namespace vessel
}//namespace engine