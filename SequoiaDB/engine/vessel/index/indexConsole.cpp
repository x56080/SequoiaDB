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
#include "vessel/clMetaBlockPage.h"
#include "ossLikely.hpp"
#include "vessel/indexSpace.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/indexEntryPageAccessor.h"
#include "vessel/indexEntryPageIniter.h"
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
#include "vessel/clIndexMbpIniter.h"
#include "vessel/clIndexMbpAccessor.h"
#include "vessel/clIndexMetaBlockPage.h"
#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmIndexExecutor.h"
#include "vessel/btreeAccessor.h"
#include "vessel/clIndexMbpIniter.h"
#include "vessel/collectionProperties.h"
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

   INT32 indexConsole::initIndexMetaBlock(requestContext *context)
   {
      INT32 rc = SDB_OK;
      logicalPageBuffer lpb;
      clIndexMbpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      PAGE_ID mbpLpid = INVALID_PAGE_ID;
      INT32 blockPos = -1;
      UINT32 pageSize = 0;

      if (OSS_UNLIKELY(nullptr == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      pageSize = _is->getFileCluster()->getCoreArgs().pageSize;
      mbpLpid = getIndexMetaBlockPageLpid(pageSize, _mbID);
      if (OSS_UNLIKELY(INVALID_PAGE_ID == mbpLpid))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block page lpid, rc:%d");
         goto error;
      }
      blockPos = getIndexMetaBlockPos(pageSize, _mbID);
      if (OSS_UNLIKELY(0 > blockPos))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block position, rc:%d", rc);
         goto error;
      }

      rc = _ensureIndexMetaBlockPage(context, mbpLpid);
      if( SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure cl index meta block page, rc:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, mbpLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", mbpLpid, rc);
         goto error;
      }

      rc = accessor.init(&lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index meta block page accessor, rc:%d", rc);
         goto error;
      }

      rc = accessor.initIndexMetaBlock(context, blockPos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index meta block by accessor,rc :%d", rc);
         goto error;
      }

   done:   
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::resetIndexMetaBlock(requestContext *context)
   {
      INT32 rc = SDB_OK;
      BOOLEAN blocked = FALSE;
      logicalPageBuffer lpb;
      clIndexMbpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      INT32 blockPos = 0;
      PAGE_ID mbpLpid = INVALID_PAGE_ID;
      clIndexMetaBlock block;
      UINT32 pageSize = 0;
      
      if (OSS_UNLIKELY(nullptr == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      pageSize = _is->getFileCluster()->getCoreArgs().pageSize;
      rc = _is->blockCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      mbpLpid = getIndexMetaBlockPageLpid(pageSize, _mbID);
      if (OSS_UNLIKELY(INVALID_PAGE_ID == mbpLpid))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "faild to get cl index meta block page lpid of mb[%d], rc:%d", 
                _mbID, rc);
         goto error;
      }

      blockPos = getIndexMetaBlockPos(pageSize, _mbID);
      if (OSS_UNLIKELY(0 > blockPos))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block position, rc:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, mbpLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", mbpLpid, rc);
         goto error;
      }

      rc = accessor.init(&lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index meta block page accessor, rc:%d", rc);
         goto error; 
      }

      rc = accessor.getIndexMetaBlock(context, blockPos, block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index meta block, rc:%d", rc);
         goto error;
      }

      rc = accessor.resetIndexMetaBlock(context, blockPos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset index meta block, rc:%d", rc);
         goto error;
      }
      accessor.fini();

      for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
      {
         if (INVALID_PAGE_ID != block.entryPageLpids[i])
         {
            rc = _is->releasePage(context, block.entryPageLpids[i]);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to release index entry page[%d], rc:%d",
                      block.entryPageLpids[i], rc);
               goto error;
            }
         }
      }

   done:
      lpb.fini();
      if (blocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::createIndex(requestContext *context,
                                   INT32 indexSlot,
                                   UINT32 indexId,
                                   const slice &defObj,
                                   PAGE_ID &lpid)const
   {
      INT32 rc = SDB_OK;
      BOOLEAN checkpointBlocked = FALSE;

      logicalPageBuffer lpb;
      indexEntryPageIniter entryIniter;
      indexEntryPageAccessor entryAccessor;
      clIndexMbpIniter mbpIniter;
      clIndexMbpAccessor mbpAccessor;

      INT32 blockPos = 0;
      PAGE_ID entryLpid = INVALID_PAGE_ID;
      PAGE_ID mbpLpid = INVALID_PAGE_ID;
      BOOLEAN mbpLocked = FALSE;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      UINT32 pageSize = 0;

      lpid = INVALID_PAGE_ID;

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

      pageSize = _is->getFileCluster()->getCoreArgs().pageSize;
      mbpLpid = getIndexMetaBlockPageLpid(pageSize, _mbID);
      if (OSS_UNLIKELY(INVALID_PAGE_ID == mbpLpid))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block page lpid, rc:%d", rc);
         goto error;
      }

      blockPos = getIndexMetaBlockPos(pageSize, _mbID);
      if (OSS_UNLIKELY(0 > blockPos))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block position, rc:%d", rc);
         goto error;
      }

      rc = _is->blockCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      rc = context->lockLpid(SPACE_TYPE_IDX, mbpLpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock index meta block page lpid[%d], rc:%d", mbpLpid, rc);
         goto error;
      }
      mbpLocked = TRUE;

      rc = _is->ensureReservedPageMapped(context, mbpLpid, &mbpIniter);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure index meta block page mapped, rc:%d", rc);
         goto error;
      }
      
      context->unlockLpid(SPACE_TYPE_IDX, mbpLpid);
      mbpLocked = FALSE;

      rc = _is->allocatePage(context, &entryIniter, entryLpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate page, rc:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, entryLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", entryLpid, rc);
         goto error;
      }

      rc = entryAccessor.createIndex(context, indexId, defObj, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index on entry page, rc:%d", rc);
         goto error;
      }

      lpb.fini();

      rc = _is->getLogicalPageBuffer(context, mbpLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", mbpLpid, rc);
         goto error;
      }

      rc = mbpAccessor.init(&lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index meta block page accessor, rc:%d", rc);
         goto error;
      }

      rc = mbpAccessor.setIndexEntryPageLpid(context, blockPos, indexSlot, 
                                             entryLpid, indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set index entry page lpid[%d] rc:%d", entryLpid, rc);
         goto error;
      }

      lpid = entryLpid;

   done:
      mbpAccessor.fini();
      if (mbpLocked)
      {
         context->unlockLpid(SPACE_TYPE_IDX, mbpLpid);
      }
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::releaseIndexEntryInBlock(requestContext *context,
                                                INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      BOOLEAN blocked = FALSE;
      logicalPageBuffer lpb;
      clIndexMbpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      INT32 blockPos = 0;
      PAGE_ID mbpLpid = INVALID_PAGE_ID;
      clIndexMetaBlock block;
      UINT32 pageSize = 0;
      
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

      pageSize = _is->getFileCluster()->getCoreArgs().pageSize;
      mbpLpid = getIndexMetaBlockPageLpid(pageSize, _mbID);
      if (OSS_UNLIKELY(INVALID_PAGE_ID == mbpLpid))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "faild to get cl index meta block page lpid of mb[%d], rc:%d", 
                _mbID, rc);
         goto error;
      }

      blockPos = getIndexMetaBlockPos(pageSize, _mbID);
      if (OSS_UNLIKELY(0 > blockPos))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block position, rc:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, mbpLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", mbpLpid, rc);
         goto error;
      }

      rc = accessor.init(&lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index meta block page accessor, rc:%d", rc);
         goto error;
      }
      
      rc = accessor.getIndexMetaBlock(context, blockPos, block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index meta block, rc:%d", rc);
         goto error;
      }

      rc = accessor.resetIndexEntryPageLpid(context, blockPos, indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset index entry page lpid[%d], rc:%d",
                block.entryPageLpids[indexSlot], rc);
         goto error;
      }
      accessor.fini();

      rc = _is->releasePage(context, block.entryPageLpids[indexSlot]);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release index entry page[%d], rc:%d",
                block.entryPageLpids[indexSlot], rc);
         goto error;
      }

   done:
      lpb.fini();
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
      SDB_ASSERT(NULL != context && context->isClPropertiesSet(), "can not be null");
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();
      globalCollectionId gcid = context->getClProperties()->getGlobalId();
      SDB_ASSERT(gcid.isValid(), "can not be invalid");

      globalIndexID gid(gcid.getCSLid(),
                        gcid.getCLLid(),
                        obj->getIndexId().getLogicalIndexId());
      lsmIndexMeta lsmMeta(gid, obj->getDescription().getPattern().getOrdering());
      lsmIndexExecutor exec;
      lsmPureKeyEntry lsmEntry;

      exec.init(lsmMeta);

      lsmEntry.set(key, rid, lsn);
      rc = exec.put(lsmEntry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "put key into executor failed, rc:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
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
      SDB_ASSERT(NULL != context && context->isClPropertiesSet(), "can not be invalid");
      SDB_ASSERT(obj->isValid(), "must be valid");
      globalCollectionId gcid = context->getClProperties()->getGlobalId();
      SDB_ASSERT(gcid.isValid(), "can not be invalid");
      globalIndexID gid(gcid.getCSLid(),
                        gcid.getCLLid(),
                        obj->getIndexId().getLogicalIndexId());
      lsmIndexMeta meta(gid, obj->getDescription().getPattern().getOrdering());
      lsmIndexExecutor exec;

      exec.init(meta);

      rc = exec.truncate();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "truncate lsm index failed, index:[%s], rc:%d",
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
      clIndexMbpAccessor mbpAccessor;
      indexEntryPageAccessor entryAccessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      PAGE_ID mbpLpid = INVALID_PAGE_ID;
      INT32 blockPos = 0;
      clIndexMetaBlock block;
      UINT32 pageSize = 0;
      lpageDescriptor desc;

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

      pageSize = _is->getFileCluster()->getCoreArgs().pageSize;
      mbpLpid = getIndexMetaBlockPageLpid(pageSize, _mbID);
      if ((OSS_UNLIKELY(INVALID_PAGE_ID == mbpLpid)))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block page lpid, rc:%d", rc);
         goto error;
      }

      blockPos = getIndexMetaBlockPos(pageSize, _mbID);
      if (OSS_UNLIKELY(0 > blockPos))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get index meta block position, rc:%d", rc);
         goto error;
      }
      
      rc = _is->testLogicalPageMapping(mbpLpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test if lpid[%d] is mapped, rc:%d", mbpLpid, rc);
         goto error;
      }
      
      if (desc.isValid())
      {
         logicalPageBuffer lpb;
         rc = _is->getLogicalPageBuffer(context, mbpLpid, mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", mbpLpid, rc);
            goto error;
         }

         rc = mbpAccessor.init(&lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init index meta block page accessor, rc:%d", rc);
            goto error;
         }

         rc = mbpAccessor.getIndexMetaBlock(context, blockPos, block);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index meta block, rc:%d", rc);
            goto error;
         }
         mbpAccessor.fini();
         if (!block.isValid())
         {
            goto done;
         }
         if (block.clLogicalId != context->getLogicalClId())
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "cl logical ID does not match, expected:%d , actual:%d", context->getLogicalClId(), block.clLogicalId);
            goto error;
         }
         for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
         {
            PAGE_ID entryLpid = INVALID_PAGE_ID;
            logicalPageBuffer entryLpb;
            indexEntryPageHead head;
            indexDescription desc;

            entryLpid = block.entryPageLpids[i];
            if (INVALID_PAGE_ID == entryLpid)
            {
               continue;
            }

            rc = _is->getLogicalPageBuffer(context, entryLpid, mode, entryLpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", entryLpid, rc);
               goto error;
            }

            rc = entryAccessor.getIndexDescription(context, &entryLpb, desc, &head);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get index description "
                      "and entry page head in page[%d], rc:%d", entryLpid, rc);
               goto error;
            }

            if (OSS_UNLIKELY(head.indexLogicalID > block.maxIndexLid))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "current index logical id[%d] exceed the max[%d], rc:%d",
                      head.indexLogicalID, block.maxIndexLid, rc);
               goto error;
            }

            rc = indexes->insert(i, head.indexLogicalID, entryLpid, desc,
                                 (INDEX_STATUS)head.status, head.btreeRoot);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert index[%d] into index object map, rc:%d",i, rc);
               goto error;
            }
         }

         indexes->setMaxIndexLid(block.maxIndexLid);
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

      rc = btreeCommit(context, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into btree index:%d", rc);
         goto error;
      }

      rc = lsmCommit(context, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "insert lsm index failed", rc);
         goto error;
      }

   done:
      return rc;
   error:
      ///TODO: rollback index inserted to btree
      goto done;
   }

   INT32 indexConsole::lsmCommit(dmlContext *context,
                                 const dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && context->isClPropertiesSet(), "can not be null");
      SDB_ASSERT(context->isDmlPositionSet(), "must be set");
      globalCollectionId gcid = context->getClProperties()->getGlobalId();
      SDB_ASSERT(gcid.isValid(), "can not be invalid");

      lsmIndexWriteBatch batch;
      UINT32 size = ra.getSize();
      batch.open();
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
            lsmPureKeyEntry ke;
            lsmIndexValue vl;
            ke.set(key, context->getRid(), context->getDmlLSN());
            vl.reset(LSM_IDX_VALUE_TYPE_INSERT);
            rc = batch.put(meta, ke, vl);
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
            lsmPureKeyEntry ke;
            lsmIndexValue vl;
            ke.set(key, context->getRid(), context->getDmlLSN());
            vl.reset(LSM_IDX_VALUE_TYPE_DELETE);
            rc = batch.put(meta, ke, vl);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push data into batch:%d", rc);
               goto error;
            }
         }

      }

      if (!batch.isEmpty())
      {
         rc = batch.commit();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "write lsm index batch failed, rc:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
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
                       !context->isClPropertiesSet() ||
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

   INT32 indexConsole::_ensureIndexMetaBlockPage(requestContext *context, PAGE_ID mbpLpid)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      clIndexMbpIniter initer;
      BOOLEAN locked = FALSE;
      
      SDB_ASSERT(INVALID_PAGE_ID != mbpLpid, "index meta block page id can not be invalid");
      rc = context->lockLpid(_is->getSpaceType(), mbpLpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock index meta block page lpid, rc:%d", rc);
         goto error;
      }
      locked = TRUE;

      rc = _is->ensureReservedPageMapped(context, mbpLpid, &initer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure index meta block page mapped, rc:%d", rc);
         goto error;
      }
      
   done:
      if (locked)
      {
         context->unlockLpid(_is->getSpaceType(), mbpLpid);
      }
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine