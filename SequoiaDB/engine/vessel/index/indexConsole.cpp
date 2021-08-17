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

#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmIndex.hpp"

namespace engine
{
namespace vessel
{
   INT32 indexConsole::init(const collectionRecord *record,
                            indexSpace *is)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isInitialized(), "do not reinit");
      if (NULL == record || !record->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL == is || !is->isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _record = record;
      _is = is;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::allocateIndexSlot(INT32 &indexSlot)const
   {
      INT32 rc = SDB_OK;
      indexSlot = -1;
      UINT64 indexes = 0;
      UINT64 mask = 1;
      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      indexes = (_record->uniqueIndexes | _record->nonUniqueIndexes);
      if (OSS_UINT64_MAX == indexes)
      {
         rc = SDB_DMS_MAX_INDEX;
         goto error;
      }

      for (INT32 i = 0; i < (INT32)MAX_INDEX_COUNT_PER_CL; ++i)
      {
         if (0 == OSS_BIT_TEST(indexes, mask))
         {
            indexSlot = i;
            break;
         }
         mask <<= 1;
      }
      SDB_ASSERT(0 <= indexSlot, "impossible");
   done:
      return rc;
   error:
      goto done;
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


   INT32 indexConsole::listIndexes(requestContext *context,
                                   ossPoolVector<bson::BSONObj> &indexes)const
   {
      INT32 rc = SDB_OK;
      UINT64 allIndexes = 0;

      if (!isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      allIndexes = (_record->uniqueIndexes | _record->nonUniqueIndexes);
      for (INT32 i = 0; i < (INT32)MAX_INDEX_COUNT_PER_CL; ++i)
      {
         bson::BSONObj obj;
         UINT64 mask = 1;
         mask <<= i;
         if (0 == OSS_BIT_TEST(allIndexes, mask))
         {
            continue;
         }

         rc = dumpIndex(context, i, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed ot dump index[%d], rc:%d", i, rc);
            goto error;
         }

         indexes.push_back(obj);
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

      if (!testIndexSlot(_record, indexSlot))
      {
         PD_LOG(PDERROR, "index with slot[%d] does not exist", indexSlot);
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }

      rc = _is->getIndexDefPage(context, _record->mbID, indexSlot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid of index[%d], rc:%d", indexSlot, rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid,
                                     OSS_SHARED_LATCH_MODE_SHARED, lpb);
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

   INT32 indexConsole::releaseIndexSlot(requestContext *context,
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

      rc = _is->getIndexDefPage(context, _record->mbID, indexSlot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid of index def page:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid,
                                     OSS_SHARED_LATCH_MODE_EXCLUSIVE, lpb);
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
                                        indexObject &obj)
   {
      INT32 rc = SDB_OK;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      indexDefPageAccessor accessor;

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

      rc = _is->getIndexDefPage(context, _record->mbID, indexSlot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid of index def page:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid,
                                     OSS_SHARED_LATCH_MODE_SHARED, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.getIndexObject(context, &lpb, obj, TRUE);
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
      globalIndexID gid(context->getCSLogicalID(),
                        context->getCLLid(),
                        obj.getIndexID());
      lsmIndexMeta lsmMeta(gid, obj.getPattern().getOrdering());
      lsmIndex lsm;
      lsmKeyEntry lsmEntry;
      dmsRBSOffset dummy;

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

      UINT32 pos = 0;
      PAGE_ID mappingPage = _is->getMappingPageLpid(_record->mbID, indexSlot, pos);
      if (INVALID_PAGE_ID == mappingPage)
      {
         PD_LOG(PDERROR, "failed to get mapping page of [%d,%d]", _record->mbID, indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_IDX, mappingPage,
                   OSS_SHARED_LATCH_MODE_EXCLUSIVE);
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

      rc = _is->getLogicalPageBuffer(context, lpid,
                                     OSS_SHARED_LATCH_MODE_EXCLUSIVE,
                                     lpb);
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
      rc = _is->getLogicalPageBuffer(context, mappingPage,
                                     OSS_SHARED_LATCH_MODE_EXCLUSIVE,
                                     lpb);
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

      lpid = _is->getDirectMappedIndexLpid(_record->mbID, indexSlot);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of index def page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_IDX, lpid, OSS_SHARED_LATCH_MODE_EXCLUSIVE);
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

      rc = _is->getLogicalPageBuffer(context, lpid,
                                     OSS_SHARED_LATCH_MODE_EXCLUSIVE,
                                     lpb);
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

      rc = _is->getIndexDefPage(context, _record->mbID, indexSlot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index def page of slot[%d]:%d", indexSlot, rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid, OSS_SHARED_LATCH_MODE_SHARED, lpb);
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

   void indexConsole::fini()
   {
      _record = NULL;
      _is = NULL;
      return;
   }

   BOOLEAN indexConsole::testIndexSlot(const collectionRecord *record,
                                       INT32 indexSlot)const
   {
      SDB_ASSERT(NULL != record, "can not be null");
      SDB_ASSERT(isValidIndexSlot(indexSlot), "must be valid");
      UINT64 indexes = (record->uniqueIndexes | record->nonUniqueIndexes);
      UINT64 mask = ((UINT64)1 << indexSlot);
      return (0 != OSS_BIT_TEST(indexes, mask));
   }
}//namespace vessel
}//namespace engine