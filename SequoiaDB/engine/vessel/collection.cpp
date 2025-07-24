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

   Source File Name = collection.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/collection.h"
#include "pdTrace.hpp"
#include "ossUtil.hpp"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/storageUnit.h"
#include "vessel/instanceEnv.h"
#include "vessel/clMetaBlockPageAccessor.h"
#include "vessel/collectionSpace.h"
#include "vessel/routePage.h"
#include "vessel/routePageAccessor.h"
#include "vessel/scanCLCursor.h"
#include "vessel/routePageIniter.h"
#include "vessel/atomicOperationList.h"
#include "vessel/rdpIniter.h"
#include "vessel/indexUtils.h"
#include "vessel/redoLogUtil.h"
#include "vessel/indexKeyGenerator.h"
#include "vessel/outerResource.h"
#include "vessel/rdpRecordScanner.h"
#include "vessel/indexScanner.h"
#include "vessel/buildingIndexContext.h"
#include "vessel/indexScanCursor.h"
#include "interface/IRecordUpdater.h"
#include "vessel/elasticBlockRowBatch.hpp"
#include "ixm_common.hpp"
#include "dmsLobDef.hpp"
#include "vessel/hybridIndexTree.h"
#include "vessel/clIndexMetaStorage.h"
#include "vessel/dmlContext.h"
#include "vessel/hitTransferTaskCtx.h"
#include "vessel/btreeWriter.h"
#include "vessel/spacePteAccessCtx.h"
#include "vessel/btreeEntryPageIniter.h"
#include "vessel/lsm/lsmIteratorBound.h"
#include "vessel/keyStringBuilder.h"
#include "vessel/lsmKeyStringEntry.h"
#include "vessel/lsm/lsmIndexEntryValue.h"

namespace engine
{
namespace vessel
{
   collection::~collection()
   {
      fini();
   }

   INT32 collection::initWhenOpen(requestContext *context,
                                  const clMetaBlock &block,
                                  collectionSpace *cs)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr == _cs, "do not reinit");
      fsmFile *file = nullptr;
      
      if (OSS_UNLIKELY(nullptr == context ||
                       !block.isValid() ||
                       nullptr == cs ||
                       !cs->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _cs = cs;
      initProperties(block);
      context->setClProperties(_entryBlock.getProperties());
      
      initRouteMapInBlock(block);
      rc = initPageSequenceWhenOpen(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page sequence:%d", rc);
         goto error;
      }

      file = cs->getSU()->getMainDataSpace().getFsmFile();
      rc = _entryBlock._fsm.open(block.mbID,
                                 block.logicalCLID,
                                 _getRdpCount().load(std::memory_order_relaxed),
                                 file,
                                 dmsStripingRange(block.minStriping,
                                                  block.maxStriping));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open free space map of cl[%d], rc:%d",
                block.mbID, rc);
         goto error;
      }

      rc = _initIndexesWhenOpen(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init indexes, rc:%d", rc);
         goto error;
      }

   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
     
      fini();
      goto done;
   }

   globalCollectionId collection::getGlobalId()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      globalCollectionId gcid;
      gcid.reset(_cs->getIdentifier(), getCollectionId());
      return gcid;
   }

   INT32 collection::create(requestContext *context,
                            const strSlice &clName,
                            utilCLInnerID innerID,
                            UINT32 logicalID,
                            collectionSpace *cs,
                            const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      fsmFile *fsm = nullptr;
      clMetaBlock mb;

      SDB_ASSERT(!isOpen(), "do not reinit");

      if (OSS_UNLIKELY(nullptr == context ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       clName.empty() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       nullptr == cs ||
                       !cs->isOpen() ||
                       !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _cs = cs;

      mb.version = CL_META_BLOCK_VERSION;
      mb.mbID = context->getMBID();
      mb.innerID = innerID;
      mb.type = options.type;
      mb.logicalCLID = logicalID;
      mb.minFreePercent = options.minFreePercent;
      mb.minStriping = options.stripingRange.getLow().getValue();
      mb.maxStriping = options.stripingRange.getHigh().getValue();
      ossMemcpy(mb.name, clName.str(), clName.strLen());
      mb.compressionType = options.compressionType;

      fsm = cs->getSU()->getMainDataSpace().getFsmFile();
      rc = _entryBlock._fsm.create(context->getMBID(),
                                   logicalID, fsm,
                                   dmsStripingRange(mb.minStriping,
                                                    mb.maxStriping));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create free space map of mb[%d], rc:%d",
                context->getMBID(), rc);
         goto error;
      }

      initProperties(mb);
      
      rc = initCLMetaBlockOnDisk(context, mb, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cl meta block, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void collection::fini()
   {
      _cs = nullptr;
      _entryBlock.reset();
      return;
   }

   INT32 collection::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      OSS_LATCH_MODE mode = SHARED;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(nullptr == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!context->isMbLocked(&mode) ||
               EXCLUSIVE != mode)
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      context->setClProperties(_entryBlock.getProperties());

      rc = commitRemoveCLLog(_entryBlock.getProperties()->getFullName(), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit remove cl log:%d", rc);
         goto error;
      }

      _entryBlock._fsm.destroy();
      _removeAllIndexes(context, lsn);
      releaseAllRdps(context);
      removeCLMetaBlockOnDisk(context);
      if (_cs->getSU()->getLobSpace().isOpen())
      {
         _cs->getSU()->getLobSpace().removeLobChunksInCL(context);
      }
      
      context->resetClProperties();
      fini();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::truncate(requestContext *context,
                              LPS_PTE_WRITE_BATCH &batch)
   {
      INT32 rc = SDB_OK;
      OSS_LATCH_MODE mode = SHARED;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(nullptr == context ||
                       !batch))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!context->isMbLocked(&mode) ||
               EXCLUSIVE != mode)
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      context->setClProperties(_entryBlock.getProperties());

      rc = commitTruncateCLLog(_entryBlock.getProperties()->getFullName(), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit truncate log:%d", rc);
         goto error;
      }

      if (_cs->getSU()->getLobSpace().isOpen())
      {
         _cs->getSU()->getLobSpace().removeLobChunksInCL(context);
      }

      _entryBlock._fsm.truncate();

      rc = _truncateAllIndexes(context, lsn, batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate indexes:%d", rc);
         goto error;
      }

      releaseAllRdps(context);
      rc = resetRouteRootOnDisk(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset route map on disk:%d", rc);
         goto error;
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::createIndex(requestContext *context,
                                 const dmsBuildIndexOptions &o,
                                 const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;
      UINT32 indexLid = INVALID_LOGICAL_INDEX_ID;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isMbLocked() ||
                            !adjunct.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context->setClProperties(_entryBlock.getProperties());

      rc = _createNewIndex(context, adjunct, indexLid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index[%s]:%d", 
                adjunct.toPoolString().c_str(), rc);
         goto error;
      }
   
      rc = _buildIndex(context, indexLid, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build index, rc:%d", rc);
         goto error;
      }
      
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::listIndexes(requestContext *context,
                                 ossPoolVector<bson::BSONObj> &indexes)
   {
      INT32 rc = SDB_OK;
      indexes.clear();
      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED);

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (nullptr == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (indexObjectMap::CONST_ITERATOR itr =_entryBlock._indexes.begin();
           itr !=_entryBlock._indexes.end(); ++itr)
      {
         indexes.push_back(itr->second->toBson());
      }
   done:
      return rc;
   error:
      indexes.clear();
      goto done;
   }

   INT32 collection::removeIndex(requestContext *context,
                                 const strSlice &indexName)
   {
      INT32 rc = SDB_OK;
      indexObject *obj = nullptr;
      LPS_PTE_WRITE_BATCH batch;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isMbLocked() ||
                            indexName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context->setClProperties(_entryBlock.getProperties());

      rc = _setIndexRemoving(context, indexName, &obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index[%s] ready to be removed:%d",
                indexName.str(), rc);
         goto error;
      }

      rc = _cs->getSU()->getIndexSpace().initWriteBatch(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init write batch:%d", rc);
         goto error;
      }

      rc = _truncateIndex(context, obj, TRUE, batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index[%s], rc:%d", 
                indexName.str(), rc);
         goto error;
      }

      rc = _cs->getSU()->getIndexSpace().commit(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit to index space:%d", rc);
         SDB_ASSERT(FALSE, "TODO");
         goto error;
      }

      _endToRemoveIndex(context, obj->getLogicalID());
      ///obj is invalid from here
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      SDB_ASSERT(SDB_IXM_NOTEXIST == rc, "TODO: get error when removing index");
      goto done;
   }

   INT32 collection::testNormalIndex(requestContext *context,
                                     const strSlice &indexName,
                                     indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED, FALSE);
      indexObject *obj = nullptr;
      indexId.reset();

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isMbLocked() ||
                            indexName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();
      obj = _entryBlock._indexes.getIndexObj(indexName);
      if (nullptr == obj || !obj->isNormal())
      {
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }

      indexId.reset(obj->getLogicalID());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::initCLMetaBlockOnDisk(requestContext *context,
                                           const clMetaBlock &mb,
                                           const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(mb.isValid(), "must be valid");
      
      logicalPageBuffer lpb;
      clMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      UINT32 pageSize = getDataPageSize();
      strSlice csName;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      PAGE_ID lpid = getMbpLpidOfCollection(pageSize, mb.mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", mb.mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }
      
      rc = accessor.createCL(context, mb, options, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cl on cl meta block page:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeCLMetaBlockOnDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");

      logicalPageBuffer lpb;
      clMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      UINT32 pageSize = getDataPageSize();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      PAGE_ID lpid = getMbpLpidOfCollection(pageSize, getId().getMbId());
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", getId().getMbId());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = accessor.removeCL(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove cl on disk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::resetRouteRootOnDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");

      logicalPageBuffer lpb;
      clMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      UINT32 pageSize = getDataPageSize();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      PAGE_ID lpid = getMbpLpidOfCollection(pageSize, getId().getMbId());
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", getId().getMbId());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = accessor.truncateRouteMap(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate route map on disk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::dump(bson::BSONObj &record)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == _cs))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      record = dumpCollectionWhenList(_cs->getLogicalID(),
                                      _entryBlock.getProperties());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insert(dmlContext *context,
                            const dmlInsertRequest &request,
                            utilInsertResult *res)
   {
      INT32 rc = SDB_OK;
      dmlIndexRequestArray ra;
      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isMbLocked() ||
                            context->getMBID() != getMBID() ||
                            !request.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (getProperties()->stripingRange.isValid() &&
               !request.o.stripingId.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();
      context->setClProperties(_entryBlock.getProperties());
      context->setStripingId(request.o.stripingId);
      
      /// build indexes keys.
      rc = buildDmlIndexRequests(context, request.record, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR,"failed to build unique index requests:%d", rc);
         goto error;
      }

      if (ra.withConstraint())
      {
         BOOLEAN duplicated = FALSE;
         rc = constraintCheck(context, ra, duplicated, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to check constraint:%d", rc);
            goto error;
         }

         if (duplicated)
         {
            rc = SDB_IXM_DUP_KEY;
            goto error;
         }
      }

      context->setIndexReqCount(ra.getSize());

      if (!ra.isEmpty())
      {
         // Block the write thread through sleep function to limit index write rate.
         // Avoid read performance degradation caused by too many sst files.
         context->getEnv()->hitMgr.limitRate();
      }

      rc = insertRecordData(context, request);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert record data, rc:%d", rc);
         goto error;
      }

      if (ra.hasBuildingIndex())
      {
         rc = mergeIntoBuildingContext(context, ra);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert keys into building context:%d", rc);
            SDB_ASSERT(FALSE, "TODO");/// rollback record
            goto error;
         }
      }

      if (!ra.isEmpty())
      {
         hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));
         rc = hit.handleDmlRequests(context, ra.getRequests());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert data into index:%d", rc);
            SDB_ASSERT(FALSE, "TODO");/// rollback record
            goto error;
         }
      }

      if (nullptr != res)
      {
         res->incInsertedNum();
         res->setInsertLoc(context->getRid().getPid(),
                           context->getRid().getPos());
      }
   done:
      if (nullptr != context)
      {
         context->reset();
         context->resetClProperties();
      }

      return rc;
   error:
      goto done;
   }

   INT32 collection::update(dmlContext *context,
                            const dmlUpdateRequest &request,
                            IRecordUpdater *updater,
                            utilUpdateResult *res)
   {
      INT32 rc = SDB_OK;
      dmlIndexRequestArray ra;
      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED, FALSE);

      recordID rid;
      slice targetRecord;
      slice newRecord;
      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isMbLocked() ||
                            context->getMBID() != getMBID() ||
                            !request.isValid() ||
                            nullptr == updater))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();
      context->setClProperties(_entryBlock.getProperties());
      context->setStripingId(request.o.stripingId);

      rc = lockAndFetchRecordToModify(context, request.rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fetch record to update:%d", rc);
         goto error;
      }

      targetRecord = context->getMrc().getTargetRecord();
      SDB_ASSERT(targetRecord.isValid(), "impossible");

      rc = updater->update(targetRecord.getSize(),
                           targetRecord.getData());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update record:%d", rc);
         goto error;
      }

      if (updater->nothingUpdated())
      {
         goto done;
      }

      rc = buildUpdateIndexRequests(context, context->getMrc().getTargetRecord(), 
                                    updater, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build index update request:%d", rc);
         goto error;
      }

      if (ra.withConstraint())
      {
         BOOLEAN duplicated = FALSE;
         rc = constraintCheck(context, ra, duplicated, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to check constraint:%d", rc);
            goto error;
         }

         if (duplicated)
         {
            rc = SDB_IXM_DUP_KEY;
            goto error;
         }
      }

      newRecord.reset(updater->getResultRecordSize(), updater->getResultRecord());

      if (!ra.isEmpty())
      {
         // Block the write thread through sleep function to limit index write rate.
         // Avoid read performance degradation caused by too many sst files.
         context->getEnv()->hitMgr.limitRate();
      }

      rc = updateRecordData(context, newRecord);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update record on disk:%d", rc);
         goto error;
      }

      if (ra.hasBuildingIndex())
      {
         rc = mergeIntoBuildingContext(context, ra);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to merge into index building context:%d", rc);
            goto error;
         }
      }

      if (!ra.isEmpty())
      {
         rc = hit.handleDmlRequests(context, ra.getRequests());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to handle index requests:%d", rc);
            goto error;
         }
      }

      if (nullptr != res)
      {
         res->incModifiedNum();
      }

   done:
      if (nullptr != context)
      {
         context->reset();
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::remove(dmlContext *context,
                            const dmlRemoveRequest &request,
                            utilDeleteResult *res)
   {
      INT32 rc = SDB_OK;
      dmlIndexRequestArray ra;
      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));
      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isMbLocked() ||
                            !request.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();
      context->setClProperties(_entryBlock.getProperties());

      rc = lockAndFetchRecordToModify(context, request.rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fetch record to update:%d", rc);
         goto error;
      }

      rc = buildRemoveIndexRequest(context, 
                                   context->getMrc().getTargetRecord(), 
                                   ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build remove index requests:%d", rc);
         goto error;
      }

      if (ra.withConstraint())
      {
         rc = context->lockUniqueIndexKeys(ra);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock unique index keys:%d", rc);
            goto error;
         }
      }

      if (!ra.isEmpty())
      {
         // Block the write thread through sleep function to limit index write rate.
         // Avoid read performance degradation caused by too many sst files.
         context->getEnv()->hitMgr.limitRate();
      }

      rc = removeRecordData(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove record data:%d", rc);
         goto error;
      }

      if (!ra.isEmpty())
      {
         if (ra.hasBuildingIndex())
         {
            rc = mergeIntoBuildingContext(context, ra);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to merge into index building context:%d", rc);
               goto error;
            }
         }

         rc = hit.handleDmlRequests(context, ra.getRequests());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to handle index requests:%d", rc);
            goto error;
         }
      }

      if (nullptr != res)
      {
         res->incDeletedNum();
      }
   done:
      if (nullptr != context)
      {
         context->reset();
         context->resetClProperties();
      }

      return rc;
   error:
      goto done;
   }

   INT32 collection::getTotalRecordCount(requestContext *context,
                                         UINT64 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      
      IExecutor *executor = context->getExecutor();
      const static UINT32 _QUIT_CHECK = 7;
      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED);

      count = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      context->setClProperties(_entryBlock.getProperties());

      for (UINT32 i = 0; i < _getRdpCount().load(std::memory_order_relaxed); ++i)
      {
         PAGE_ID lpid = INVALID_PAGE_ID;
         UINT32 countInRdp = 0;
         if (0 == (i & _QUIT_CHECK))
         {
            if (executor->isInterrupted())
            {
               rc = SDB_APP_INTERRUPT;
               goto error;
            }
         }

         rc = getLpidBySequence(context, i, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid of seq[%d], rc:%d", i, rc);
            goto error;
         }

         rc = getRecordCountInPage(context, lpid, countInRdp);
         if (SDB_OK != rc)
         {
            goto error;
         }

         count += countInRdp;
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      count = 0;
      goto done;
   }

   INT32 collection::getMoreWhenScan(requestContext *context,
                                     scanCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != cursor, "can not be null");
      SDB_ASSERT(cursor->isOpen(), "must be open");
      SDB_ASSERT(cursor->getCollectionId().getCLLid() == getCollectionId().getLid(),
                 "must be same");

      UINT32 pageStep = 2;
      UINT32 totalRead = 0;
      UINT32 maxPageSeq = 0;

      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == cursor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context->setClProperties(_entryBlock.getProperties());
      if (0 < cursor->getOptions().pageStep)
      {
         pageStep = cursor->getOptions().pageStep;
      }
      maxPageSeq = cursor->getToScanEntry().getSeq() + (UINT32)pageStep;

      do
      {
         UINT32 readCount = 0;
         PAGE_ID lpid = cursor->getLpid();

         if (_getRdpCount().load(std::memory_order_relaxed) <=
              cursor->getToScanEntry().getSeq())
         {
            cursor->setEOC();
            goto done;
         }

         if (INVALID_PAGE_ID == lpid)
         {
            rc = getLpidBySequence(context, cursor->getToScanEntry().getSeq(), lpid);
            if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS == rc)
            {
               rc = SDB_OK;
               cursor->incToScanPage();
               continue;
            }
            else if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get lpid of seq[%d], rc:%d",
                      cursor->getToScanEntry().getSeq(), rc);
               goto error;
            }
            cursor->setLpid(lpid);
         }

         rc = getMoreFromPageInCursor(context, cursor, readCount);
         if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
         {
            rc = SDB_OK;
            goto done;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get more from seq saved:%d", rc);
            goto error;
         }

         totalRead += readCount;
      } while(0 == totalRead ||
              cursor->getToScanEntry().getSeq() < maxPageSeq);
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::getRecordCountInPage(requestContext *context,
                                          PAGE_ID lpid,
                                          UINT32 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(nullptr != _cs, "can not be null");
      rdpAccessor accessor;
      logicalPageBuffer lpb;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      count = 0;

      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor to lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = accessor.getRecordCount(count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get record count of page[%d], rc:%d", lpid, rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::getMoreFromPageInCursor(requestContext *context,
                                             scanCLCursor *cursor,
                                             UINT32 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _cs, "can not be null");
      SDB_ASSERT(nullptr != cursor, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != cursor->getLpid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(cursor->getToScanEntry().getPos()),
                 "can not be invalid");

      rdpRecordScanner scanner;
      rdpRecordScanner::options o;
      o.so = cursor->getOptions();
      count = 0;

      rc = scanner.open(context, cursor->getLpid(),
                        cursor->getToScanEntry().getPos(),
                        &o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open scanner:%d", rc);
         goto error;
      }

      while (scanner.isReadyToRead())
      {
         slice record;
         dmsRecordID rid;
         DPS_TRANS_ID transId;

         rid = scanner.getCurrentRid().toDMSRid();
         transId = scanner.getCurrentTransID();
         record = scanner.getCurrentRecord();
         cursor->setToScanSlot(scanner.getCurrentRid().getPos());

         rc = cursor->pushDataFragments({slice(sizeof(dmsRecordID), &rid),
                                         slice(sizeof(DPS_TRANS_ID), &transId),
                                         record});
         if (SDB_OK != rc)
         {
            /// is scan for none, will release rids at last
            if (DMS_SCAN_FOR::NONE != o.so.scanFor)
            {
               context->releaseTransLock(scanner.getCurrentRid());
            }

            if (SDB_VESSEL_CURSOR_NO_SPACE != rc)
            {
               PD_LOG(PDERROR, "failed to push record to cursor:%d", rc);
            }

            goto error;
         }
         cursor->incToScanSlot();
         ++count;

         if (cursor->noMorePushThisLoop())
         {
            goto done;
         }

         rc = scanner.next();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next:%d", rc);
            goto error;
         }
      }

      cursor->incToScanPage();
      
   done:
      scanner.close();
      return rc;
   error:
      goto done;
   }

   INT32 collection::getMoreWhenIndexScan(requestContext *context,
                                          indexScanCursor *cursor)
   {
      INT32 rc = SDB_OK;
      indexObject *obj = nullptr;
      indexIdentifier indexId;
      ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == cursor ||
                            !cursor->getIndexId().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();
      context->setClProperties(_entryBlock.getProperties());
      indexId = cursor->getIndexId();

      obj =_entryBlock._indexes.getIndexObj(indexId.getLogicalIndexId());
      if (nullptr == obj)
      {
         PD_LOG(PDERROR, "index[%d] not found", indexId.getLogicalIndexId());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
      else if (!obj->isNormal())
      {
         PD_LOG(PDERROR, "index id[%s] is not normal", indexId.getLogicalIndexId());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }

      rc = _getMoreWhenIndexScan(context, obj, cursor);
      if (SDB_OK != rc)
      {
         if (SDB_IXM_EOC != rc)
         {
            PD_LOG(PDERROR, "failed to get next from index:%d", rc);
         }
         goto error;
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertRecordData(dmlContext *context,
                                      const dmlInsertRequest &request)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(request.isValid(), "can not be invalid");

     if (!isBigRecord(getDataPageSize(), request.record.getSize()))
      {
         rc = insertNormalRecord(context, request.record);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert record:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = insertBigRecord(context, request.record);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert big record, rc:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;  
   } 
   INT32 collection::insertNormalRecord(dmlContext *context,
                                        const slice &record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      UINT32 size = estimateNormalRecordSavingSize(record.getSize());
      INT32 targetLvl = getFsmSpaceLvl(getDataPageSize(), size);
      PAGE_ID lpid = INVALID_PAGE_ID;
      fsmCandidate candidate;

      do
      {
         BOOLEAN outOfSpace = FALSE;
         rc = findCandidate(context, targetLvl,
                            context->getStripingId(),
                            candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free space for record:%d", rc);
            goto error;
         }

         lpid = candidate.getLpid();
         if (INVALID_PAGE_ID == lpid)
         {
            rc = getLpidBySequence(context, candidate.getSeq(), lpid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find lpid of seq[%d], rc:%d",
                      candidate.getSeq(), rc);
               goto error;
            }

            candidate.setLpid(lpid);
         }

         rc = insertAndUpdateCandidate(context, record, candidate, outOfSpace);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert record to page:%d", rc);
            goto error;
         }

         candidate.reset();
         if (!outOfSpace)
         {
            break;
         }
      }while(TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertBigRecord(dmlContext *context,
                                     const slice &record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      bigRecordStream recordStream(record);

      // TODO: attach oplist
      rc = insertBigRecordSlices(context, recordStream);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert big record slices, rc:%d", rc);
         goto error;
      }

      rc = overflowBigRecord(context, recordStream.getLastSliceAddr());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to overflow big record, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertBigRecordSlices(dmlContext *context,
                                           bigRecordStream &recordStream)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(recordStream.isValid(), "can not be invalid");

      fsmCandidate candidate;
      PAGE_ID lpid;
      
      do
      {
         rc = findCandidateExclusively(context, FSM_MAX_SPACE_LVL, candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free space for record, rc:%d");
            goto error;
         }

         lpid = candidate.getLpid();
         if (INVALID_PAGE_ID == lpid)
         {
            rc = getLpidBySequence(context, candidate.getSeq(), lpid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find lpid of seq[%d], rc:%d",
                     candidate.getSeq(), rc);
               goto error;
            }
            candidate.setLpid(lpid);
         }

         rc = insertBigRecordSlice(context, recordStream, candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert big record "
                  "entry slice to page, rc:%d", rc);
            goto error;
         }

         candidate.reset();
      }while(!recordStream.isEndOfStream());
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::overflowBigRecord(dmlContext *context,
                                       const recordID &overflowAddr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(overflowAddr.isValid(), "can not be invalid");
      PAGE_ID lpid = INVALID_PAGE_ID;
      fsmCandidate candidate;

      do
      {
         BOOLEAN outOfSpace = FALSE;
         rc = findCandidate(context, FSM_MIN_SPACE_LVL,
                            context->getStripingId(),
                            candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free space for record:%d", rc);
            goto error;
         }

         lpid = candidate.getLpid();
         if (INVALID_PAGE_ID == lpid)
         {
            rc = getLpidBySequence(context, candidate.getSeq(), lpid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find lpid of seq[%d], rc:%d",
                      candidate.getSeq(), rc);
               goto error;
            }

            candidate.setLpid(lpid);
         }

         rc = overflowBigRecordAndUpdateCandidate(context, overflowAddr, candidate, outOfSpace);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to set big record overflowed, rc:%d", rc);
            goto error;
         }

         candidate.reset();
         if (!outOfSpace)
         {
            break;
         }
      }while(TRUE);

   done:
      return rc;
   error:
      goto done;

   }

   INT32 collection::insertInvisibleRecord(dmlContext *context,
                                           const slice &newRowData,
                                           recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(newRowData.isValid(), "can not be invalid");
      UINT32 size = estimateNormalRecordSavingSize(newRowData.getSize());
      INT32 targetLvl = getFsmSpaceLvl(getDataPageSize(), size);
      PAGE_ID lpid = INVALID_PAGE_ID;
      fsmCandidate candidate;
      
      rid.reset();
      do
      {
         BOOLEAN outOfSpace = FALSE;
         rc = findCandidate(context, targetLvl,
                            context->getStripingId(),
                            candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free space for record:%d", rc);
            goto error;
         }

         lpid = candidate.getLpid();
         if (INVALID_PAGE_ID == lpid)
         {
            rc = getLpidBySequence(context, candidate.getSeq(), lpid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find lpid of seq[%d], rc:%d",
                      candidate.getSeq(), rc);
               goto error;
            }

            candidate.setLpid(lpid);
         }

         rc = insertInvisiblyAndUpdateCandidate(context, newRowData, 
                                                candidate, outOfSpace, rid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert record to page:%d", rc);
            goto error;
         }

         candidate.reset();
         if (!outOfSpace)
         {
            break;
         }
      }while(TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertAndUpdateCandidate(dmlContext *context,
                                              const slice &record,
                                              fsmCandidate &candidate,
                                              BOOLEAN &outOfSpace)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context && context->isClPropertiesSet(),
                 "can not be invalid");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      SDB_ASSERT(candidate.isValid() && INVALID_PAGE_ID != candidate.getLpid(),
                 "can not be invalid");

      logicalPageBuffer lpb;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      const collectionProperties *properties = _entryBlock.getProperties();

      outOfSpace = FALSE;

      rc = mds.getLogicalPageBuffer(context, candidate.getLpid(), mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                candidate.getLpid(), rc);
         goto error;
      }

      /// candidate may be reset by prewriter.
      if (candidate.getSpaceLvl() == FSM_INVALID_SPACE_LVL)
      {
         outOfSpace = TRUE;
         goto done;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      SDB_ASSERT(candidate.getSeq() == accessor.getReadablePageHead()->pageSeq,
                 "must be same");

      rc = accessor.insertNormalRecord(context, record);
      if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
      {
         candidate.getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
         outOfSpace = TRUE;
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert normal record into page:%d", rc);
         goto error;
      }

      if (accessor.getFreeSpacePercent() < properties->getMinFreePct())
      {
         candidate.getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
      }
      else
      {
         INT32 newLvl = getFsmSpaceLvl(lpb.getPageSize(), accessor.getFreeSpaceAfterLastSlot());
         if (candidate.getSpaceLvl() != newLvl)
         {
            candidate.getInfoPtr()->_lvl = newLvl;
         }
      }

   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertInvisiblyAndUpdateCandidate(dmlContext *context,
                                                       const slice &newRowData,
                                                       fsmCandidate &candidate,
                                                       BOOLEAN &outOfSpace,
                                                       recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context && context->isClPropertiesSet(),
                 "can not be invalid");
      SDB_ASSERT(newRowData.isValid(), "can not be invalid");
      SDB_ASSERT(candidate.isValid() && INVALID_PAGE_ID != candidate.getLpid(),
                 "can not be invalid");

      logicalPageBuffer lpb;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      const collectionProperties *properties = _entryBlock.getProperties();
      

      outOfSpace = FALSE;
      rid.reset();

      rc = mds.getLogicalPageBuffer(context, candidate.getLpid(), mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                candidate.getLpid(), rc);
         goto error;
      }

      /// candidate may be reset by prewriter.
      if (candidate.getSpaceLvl() == FSM_INVALID_SPACE_LVL)
      {
         outOfSpace = TRUE;
         goto done;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      SDB_ASSERT(candidate.getSeq() == accessor.getReadablePageHead()->pageSeq,
                 "must be same");

      rc = accessor.insertInvisibleNormalRecord(context, newRowData, rid);
      if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
      {
         candidate.getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
         outOfSpace = TRUE;
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert normal record into page:%d", rc);
         goto error;
      }

      if (accessor.getFreeSpacePercent() < properties->getMinFreePct())
      {
         candidate.getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
      }
      else
      {
         INT32 newLvl = getFsmSpaceLvl(lpb.getPageSize(), accessor.getFreeSpaceAfterLastSlot());
         if (candidate.getSpaceLvl() != newLvl)
         {
            candidate.getInfoPtr()->_lvl = newLvl;
         }
      }

   done:
      lpb.fini();
      return rc;
   error:
      goto done;

   }

   INT32 collection::overflowBigRecordAndUpdateCandidate(dmlContext *context,
                                                         const recordID &overflowAddr,
                                                         fsmCandidate &candidate,
                                                         BOOLEAN &outOfSpace)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(overflowAddr.isValid(), "can not be invalid");
      SDB_ASSERT(candidate.isValid() && INVALID_PAGE_ID != candidate.getLpid(),
                 "can not be invalid");

      logicalPageBuffer lpb;
      rdpAccessor accessor;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      const collectionProperties *properties = _entryBlock.getProperties();

      outOfSpace = FALSE;
      rc = mds.getLogicalPageBuffer(context, candidate.getLpid(), mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                candidate.getLpid(), rc);
         goto error;
      }

      /// candidate may be reset by prewriter.
      if (candidate.getSpaceLvl() == FSM_INVALID_SPACE_LVL)
      {
         outOfSpace = TRUE;
         goto done;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor, rc:%d", rc);
         goto error;
      }

      SDB_ASSERT(candidate.getSeq() == accessor.getReadablePageHead()->pageSeq,
                 "must be same");
      
      rc = accessor.insertOverflowedRecord(context, overflowAddr, TRUE);
      if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
      {
         rc = SDB_OK;
         outOfSpace = TRUE;
         candidate.getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set big record overflowed, rc:%d", rc);
         goto error;
      }

      if (accessor.getFreeSpacePercent() < properties->getMinFreePct())
      {
         candidate.getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
      }
      else
      {
         INT32 newLvl = getFsmSpaceLvl(lpb.getPageSize(), accessor.getFreeSpaceAfterLastSlot());
         if (candidate.getSpaceLvl() != newLvl)
         {
            candidate.getInfoPtr()->_lvl = newLvl;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
   INT32 collection::insertBigRecordSlice(dmlContext *context,
                                          bigRecordStream &recordStream,
                                          const fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(recordStream.isValid(), "can not be invalid");
      SDB_ASSERT(candidate.isValid() && INVALID_PAGE_ID != candidate.getLpid(),
                 "can not be invalid");

      logicalPageBuffer lpb;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      INT32 lvl = FSM_INVALID_SPACE_LVL;
      const collectionProperties *properties = _entryBlock.getProperties();

      rc = mds.getLogicalPageBuffer(context, candidate.getLpid(), mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                candidate.getLpid(), rc);
         goto error;
      }
      
      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor, rc:%d", rc);
         goto error;
      }
      SDB_ASSERT(candidate.getSeq() == accessor.getReadablePageHead()->pageSeq,
                 "must be same");

      lvl = getFsmSpaceLvl(getDataPageSize(), accessor.getFreeSpaceAfterLastSlot());
      if (FSM_MAX_SPACE_LVL == lvl)
      {
         rc = accessor.insertBigRecordSlice(context, recordStream);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert big record by accessor, rc:%d", rc);
            goto error;
         }
      }
      else if (accessor.getFreeSpacePercent() >= properties->getMinFreePct())
      {
        _entryBlock._fsm.upgradePageSpaceLvl(candidate.getSeq(), lvl);
      }

   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::findCandidate(requestContext *context,
                                   INT32 lvl,
                                   const dmsStripingId &striping,
                                   fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      PAGE_ID lpids[PAGE_COUNT_IN_EXTENT];
      UINT32 firstSeq = 0;
      candidate.reset();

      do
      {
         UINT32 totalRdpCount = _getRdpCount().load(std::memory_order_relaxed);
         /// do not get latch here.
         std::unique_lock<std::mutex> guard(_entryBlock.getExtLock(),
                                            std::defer_lock);

         rc =_entryBlock._fsm.find(context, lvl, striping, candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find candidate from fsm:%d", rc);
            goto error;
         }

         if (candidate.isValid())
         {
            break;
         }

         guard.lock();
         if (totalRdpCount < _getRdpCount().load(std::memory_order_relaxed))
         {
            continue;
         }

         rc = allocateNewRecordDataPages(context, PAGE_COUNT_IN_EXTENT,
                                         firstSeq, lpids);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,  "failed to allocate new rdps:%d", rc);
            goto error;
         }

         rc =_entryBlock._fsm.insertNewPages(firstSeq, lpids, PAGE_COUNT_IN_EXTENT);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert new pages into fsm:%d", rc);
            goto error;
         }
      } while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::findCandidateExclusively(requestContext *context,
                                              INT32 lvl,
                                              fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      PAGE_ID lpids[PAGE_COUNT_IN_EXTENT];
      UINT32 firstSeq = 0;
      candidate.reset();

      do
      {
         UINT32 totalRdpCount = _getRdpCount().load(std::memory_order_relaxed);
         /// do not get latch here.
         std::unique_lock<std::mutex> guard(_entryBlock.getExtLock(),
                                            std::defer_lock);

         rc =_entryBlock._fsm.findAndKick(lvl, candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find candidate from fsm:%d", rc);
            goto error;
         }

         if (candidate.isValid())
         {
            break;
         }

         guard.lock();
         if (totalRdpCount < _getRdpCount().load(std::memory_order_relaxed))
         {
            continue;
         }

         rc = allocateNewRecordDataPages(context, PAGE_COUNT_IN_EXTENT,
                                         firstSeq, lpids);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,  "failed to allocate new rdps:%d", rc);
            goto error;
         }

         rc =_entryBlock._fsm.insertNewPages(firstSeq, lpids, PAGE_COUNT_IN_EXTENT);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert new pages into fsm:%d", rc);
            goto error;
         }
      } while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::allocateNewRecordDataPages(requestContext *context,
                                                UINT32 count,
                                                UINT32 &firstSeq,
                                                PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(8 == count, "capacity of route page is 32bytes alienged");
      SDB_ASSERT(nullptr != lpids, "can not be null");
      SDB_ASSERT(isOpen(), "can not be closed");

      routePageAccessor accessor;
      logicalPageBuffer lpb;
      rdpIniter initer;
      UINT32 lvl0No = 0;
      UINT32 totalRdpCount = 0;
      UINT32 totalLvl0Count = 0;
      PAGE_ID lvl0Lpid = INVALID_PAGE_ID;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      UINT32 capacity = 0;
      atomicOperationList oplist;
      atomicOperationList *backup = nullptr;
      BOOLEAN switched = FALSE;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      
      capacity = getCapacityOfRoutePage(getDataPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      totalRdpCount = _getRdpCount().load(std::memory_order_relaxed);
      totalLvl0Count = _getLvl0Count().load(std::memory_order_relaxed);
      lvl0No = totalRdpCount / capacity;
      SDB_ASSERT(lvl0No <= totalLvl0Count, "impossible");
      if (lvl0No == totalLvl0Count)
      {
         rc = extendRoutePageMap(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend route page map:%d", rc);
            goto error;
         }
      }
      
      initer.init(getCollectionId().getLid(), totalRdpCount, count);
      context->swtichOplist(&oplist, &backup);
      switched = TRUE;

      rc = mds.allocatePages(context, &initer, count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new rdps:%d", rc);
         goto error;
      }

      rc = getLvl0RoutePage(context, capacity, lvl0No, lvl0Lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid of lvl0[%d], rc:%d", lvl0No, rc);
         goto error;
      }

      rc = mds.getLogicalPageBuffer(context, lvl0Lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                lvl0Lpid, rc);
         goto error;
      }

      oplist.setWaitingTail();
      rc = accessor.append(context, count, lpids, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append lpids to route page:%d", rc);
         goto error;
      }

      firstSeq = totalRdpCount;
      _getRdpCount().fetch_add(count, std::memory_order_relaxed);

   done:
      if (switched)
      {
         context->attachOplist(backup);
      }
      return rc;
   error:
      /// TODO: rollback oplist
      goto done;
   }
   

   INT32 collection::initPageSequenceWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _cs, "can not be null");
   
      if (INVALID_PAGE_ID != _entryBlock.routeMap[COLLECTION_ROOT_LVL2])
      {
         rc = initPageSequenceByRootLvL2(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by lvl2 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _entryBlock.routeMap[COLLECTION_SECOND_ROOT_LVL1])
      {
         rc = initPageSequenceByRootLvL1(context, 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by second lvl1 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _entryBlock.routeMap[COLLECTION_FIRST_ROOT_LVL1])
      {
         rc = initPageSequenceByRootLvL1(context, 0);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by first lvl1 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _entryBlock.routeMap[COLLECTION_ROOT_LVL0])
      {
         rc = initPageSequenceByRootLvL0(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by lvl0 root:%d", rc);
            goto error;
         }
      }
      else
      {
         _getLvl0Count().store(0, std::memory_order_relaxed);
         _getRdpCount().store(0, std::memory_order_relaxed);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL2(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != _entryBlock.routeMap[COLLECTION_ROOT_LVL2],
                 "can not be invalid");

      UINT32 lvl1Count = 0;
      PAGE_ID lastLvl1 = INVALID_PAGE_ID;
      UINT32 lvl0Count = 0;
      PAGE_ID lastLvl0 = INVALID_PAGE_ID;
      UINT32 rdpCount = 0;
      PAGE_ID lastRdp = INVALID_PAGE_ID;

      UINT32 totalLvl0Count = 0;
      UINT32 totalRdpCount = 0;

      UINT32 pageSize = _cs->getSU()->getManifest().dataArgs.pageSize;
      UINT32 capacity = getCapacityOfRoutePage(pageSize);
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      totalLvl0Count = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                        COLLECTION_SECOND_ROOT_LVL1);
      totalRdpCount = totalLvl0Count * capacity;

      rc = getCountAndLastEleInRoutePage(context,
                                         _entryBlock.routeMap[COLLECTION_ROOT_LVL2],
                                         COLLECTION_ROUTE_PAGE_LVL2,
                                         lvl1Count, lastLvl1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last lvl1 page:%d", rc);
         goto error;
      }

      if (0 == lvl1Count)
      {
         goto done;
      }

      totalLvl0Count += ((lvl1Count - 1) * capacity);
      totalRdpCount = totalLvl0Count * capacity;

      rc = getCountAndLastEleInRoutePage(context,
                                         lastLvl1,
                                         COLLECTION_ROUTE_PAGE_LVL1,
                                         lvl0Count, lastLvl0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last lvl0 page:%d", rc);
         goto error;
      }

      if (0 == lvl0Count)
      {
         goto done;
      }

      totalLvl0Count += lvl0Count;
      totalRdpCount += ((lvl0Count - 1) * capacity);

      rc = getCountAndLastEleInRoutePage(context,
                                         lastLvl0,
                                         COLLECTION_ROUTE_PAGE_LVL0,
                                         rdpCount, lastRdp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last rdp:%d", rc);
         goto error;
      }

      totalRdpCount += rdpCount;

      _getLvl0Count().store(totalLvl0Count, std::memory_order_relaxed);
      _getRdpCount().store(totalRdpCount, std::memory_order_relaxed);
   done:
      return rc;
   error:
      _getLvl0Count().store(0, std::memory_order_relaxed);
      _getRdpCount().store(0, std::memory_order_relaxed);
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL1(requestContext *context,
                                                UINT32 rootNo)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(rootNo <= 1, "can not out of bound");
      PAGE_ID lpid = _entryBlock.routeMap[COLLECTION_FIRST_ROOT_LVL1 + rootNo];
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");

      UINT32 lvl0Count = 0;
      PAGE_ID lastLvl0 = INVALID_PAGE_ID;
      UINT32 rdpCount = 0;
      PAGE_ID lastRdp = INVALID_PAGE_ID;

      UINT32 totalLvl0Count = 0;
      UINT32 totalRdpCount = 0;

      UINT32 pageSize = _cs->getSU()->getManifest().dataArgs.pageSize;
      UINT32 capacity = getCapacityOfRoutePage(pageSize);
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (0 == rootNo)
      {
         totalLvl0Count = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                          COLLECTION_ROOT_LVL0);
      }
      else
      {
         totalLvl0Count = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                          COLLECTION_FIRST_ROOT_LVL1);
      }
      totalRdpCount = totalLvl0Count * capacity;

      rc = getCountAndLastEleInRoutePage(context,
                                         lpid,
                                         COLLECTION_ROUTE_PAGE_LVL1,
                                         lvl0Count, lastLvl0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last lvl0 page:%d", rc);
         goto error;
      }

      if (0 == lvl0Count)
      {
         goto done;
      }

      totalLvl0Count += lvl0Count;
      totalRdpCount += ((lvl0Count - 1) * capacity);

      rc = getCountAndLastEleInRoutePage(context,
                                         lastLvl0,
                                         COLLECTION_ROUTE_PAGE_LVL0,
                                         rdpCount, lastRdp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last rdp:%d", rc);
         goto error;
      }

      totalRdpCount += rdpCount;

      _getLvl0Count().store(totalLvl0Count, std::memory_order_relaxed);
      _getRdpCount().store(totalRdpCount, std::memory_order_relaxed);
   done:
      return rc;
   error:
      _getLvl0Count().store(0, std::memory_order_relaxed);
      _getRdpCount().store(0, std::memory_order_relaxed);
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL0(requestContext *context)
   {
      INT32 rc = SDB_OK;
      PAGE_ID lpid = _entryBlock.routeMap[COLLECTION_ROOT_LVL0];
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");

      UINT32 rdpCount = 0;
      PAGE_ID lastRdp = INVALID_PAGE_ID;

      rc = getCountAndLastEleInRoutePage(context, lpid,
                                         COLLECTION_ROUTE_PAGE_LVL0,
                                         rdpCount, lastRdp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last rdp:%d", rc);
         goto error;
      }

      _getLvl0Count().store(1, std::memory_order_relaxed);
      _getRdpCount().store(rdpCount, std::memory_order_relaxed);
   done:
      return rc;
   error:
      _getLvl0Count().store(0, std::memory_order_relaxed);
      _getRdpCount().store(0, std::memory_order_relaxed);
      goto done;
   }

   INT32 collection::getLpidBySequence(requestContext *context,
                                       UINT32 sequence,
                                       PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _cs, "can not be null");

      UINT32 capacity = 0;
      UINT32 lvl0Id = 0;
      PAGE_ID lvl0Lpid = INVALID_PAGE_ID;
      UINT32 totalRdpCount = _getRdpCount().load(std::memory_order_relaxed);

      lpid = INVALID_PAGE_ID;

      if (totalRdpCount <= sequence)
      {
         PD_LOG(PDERROR, "invalid page sequence[%d], current max page count[%d]",
                sequence, totalRdpCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      capacity = getCapacityOfRoutePage(getDataPageSize());
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get route page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      lvl0Id = sequence / capacity;

      rc = getLvl0RoutePage(context, capacity, lvl0Id, lvl0Lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lvl0 page[%d], rc:%d", lvl0Id, rc);
         goto error;
      }

      rc = getLpidFromRoutePage(context, lvl0Lpid,
                                sequence % capacity,
                                lpid);
      if (SDB_OK != rc)
      {
         /// rdp may be released
         if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid from route page[%d], rc:%d",
                   lvl0Lpid, rc);
         }
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }


   INT32 collection::getCountAndLastEleInRoutePage(requestContext *context,
                                                   PAGE_ID lpid,
                                                   INT32 lvl,
                                                   UINT32 &count,
                                                   PAGE_ID &element)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(nullptr != _cs, "can not be null");

      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      logicalPageBuffer lpb;
      routePageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.getSizeAndLast(context, lvl, &lpb, count, element);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get size and last element on page[%d]:%d", lpid, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::getLvl0RoutePage(requestContext *context,
                                      UINT32 capacity,
                                      UINT32 lvl0No,
                                      PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != capacity, "can not be zero");
      PAGE_ID lvl1Lpid = INVALID_PAGE_ID;
      UINT32 pos = 0;
      routePageAccessor accessor;

      lpid = INVALID_PAGE_ID;

      if (_getLvl0Count().load(std::memory_order_relaxed) <= lvl0No)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      if (0 == lvl0No)
      {
         lpid = _entryBlock.routeMap[COLLECTION_ROOT_LVL0];
         if (INVALID_PAGE_ID == lpid)
         {
            PD_LOG(PDERROR, "root lvl0 does not exist");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         goto done;
      }
      else if (lvl0No < getMaxLvL0RoutePageCountLteRoot(capacity,
                                                        COLLECTION_FIRST_ROOT_LVL1))
      {
         pos = lvl0No - 1;
         lvl1Lpid = _entryBlock.routeMap[COLLECTION_FIRST_ROOT_LVL1];
         if (INVALID_PAGE_ID == lvl1Lpid)
         {
            PD_LOG(PDERROR, "the first root lvl1 does not exist");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      else if (lvl0No < getMaxLvL0RoutePageCountLteRoot(capacity,
                                                        COLLECTION_SECOND_ROOT_LVL1))
      {
         pos = lvl0No - 1 - capacity;
         lvl1Lpid = _entryBlock.routeMap[COLLECTION_SECOND_ROOT_LVL1];
         if (INVALID_PAGE_ID == lvl1Lpid)
         {
            PD_LOG(PDERROR, "the second root lvl1 does not exist");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      else if (lvl0No < getMaxLvl0Cnt(capacity))
      {
         pos = (lvl0No - 1 - capacity - capacity) % capacity;
         UINT32 lvl1Pos = (lvl0No - 1 - capacity - capacity) / capacity;
         PAGE_ID lvl2Lpid = _entryBlock.routeMap[COLLECTION_ROOT_LVL2];
         if (INVALID_PAGE_ID == lvl2Lpid)
         {
            PD_LOG(PDERROR, "lvl2 root does not exist");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = getLpidFromRoutePage(context, lvl2Lpid,
                                   lvl1Pos, lvl1Lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lvl1 page in root lvl2:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      /// get lvl0 from lvl1
      rc = getLpidFromRoutePage(context, lvl1Lpid, pos, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lvl0 from lvl1[%d], rc:%d",
                lvl1Lpid, rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS == rc)
      {
         PD_LOG(PDERROR, "lvl0 page should exist all time");
         rc = SDB_VESSEL_INTERNAL_ERR;
      }
      goto done;
   }

   INT32 collection::getLpidFromRoutePage(requestContext *context,
                                          PAGE_ID routePgaeLpid,
                                          UINT32 pos,
                                          PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _cs, "can not be null");
      routePageAccessor accessor;
      logicalPageBuffer lpb;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      rc = mds.getLogicalPageBuffer(context, routePgaeLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                routePgaeLpid, rc);
         goto error;
      }

      rc = accessor.get(context, pos, &lpb, lpid);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::extendRoutePageMap(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isOpen(), "can not be closed");

      PAGE_ID newLvl0 = INVALID_PAGE_ID;
      UINT32 maxLvl0COunt = 0;
      UINT32 totalLvl0Count = _getLvl0Count().load(std::memory_order_relaxed);
      PAGE_ID lvl1Lpid = INVALID_PAGE_ID;
      UINT32 capacity = getCapacityOfRoutePage(getDataPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get route page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      maxLvl0COunt = getMaxLvl0Cnt(capacity);

      if (OSS_UNLIKELY(maxLvl0COunt < totalLvl0Count))
      {
         PD_LOG(PDERROR, "invalid totalLvl0Count[%d]", totalLvl0Count);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (getMaxLvl0Cnt(capacity) == totalLvl0Count)
      {
         PD_LOG(PDERROR, "can not create any more new lvl0 pages");
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }
      
      if (0 == totalLvl0Count)
      {
         /// COLLECTION_ROOT_LVL0 impossible to be valid when totalLvl0Count is zero.
         SDB_ASSERT(INVALID_PAGE_ID == _entryBlock.routeMap[COLLECTION_ROOT_LVL0], "impossible");
         rc = ensureRootRoutePage(context, COLLECTION_ROOT_LVL0);
         if (SDB_OK != rc)
         {
            goto error;
         }

         _getLvl0Count().fetch_add(1, std::memory_order_relaxed);
         goto done;
      }
      else if (totalLvl0Count <
               getMaxLvL0RoutePageCountLteRoot(capacity,
                                               COLLECTION_FIRST_ROOT_LVL1))
      {
         rc = ensureRootRoutePage(context, COLLECTION_FIRST_ROOT_LVL1);
         if (SDB_OK != rc)
         {
            goto error;
         }
         lvl1Lpid = _entryBlock.routeMap[COLLECTION_FIRST_ROOT_LVL1];
      }
      else if (totalLvl0Count <
               getMaxLvL0RoutePageCountLteRoot(capacity,
                                               COLLECTION_SECOND_ROOT_LVL1))
      {
         rc = ensureRootRoutePage(context, COLLECTION_SECOND_ROOT_LVL1);
         if (SDB_OK != rc)
         {
            goto error;
         }
         lvl1Lpid = _entryBlock.routeMap[COLLECTION_SECOND_ROOT_LVL1];
      }
      else
      {
         rc = ensureRootRoutePage(context, COLLECTION_ROOT_LVL2);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = ensureNonRootLvl1RoutePage(context, lvl1Lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure lvl1 page:%d", rc);
            goto error;
         }
      }

      rc = createNonRootRoutePage(context, lvl1Lpid,
                                  COLLECTION_ROUTE_PAGE_LVL0,
                                  newLvl0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new lvl0 page:%d", rc);
         goto error;
      }

      _getLvl0Count().fetch_add(1, std::memory_order_relaxed);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::ensureRootRoutePage(requestContext *context,
                                         UINT32 rootSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(COLLECTION_MIN_ROUTE_ROOT <= rootSlot, "impossible");
      SDB_ASSERT(rootSlot <= COLLECTION_MAX_ROUTE_ROOT, "impossible");

      clMetaBlockPageAccessor accessor;
      logicalPageBuffer lpb;
      PAGE_ID clmbpLpid = INVALID_PAGE_ID;
      PAGE_ID routeLpid = INVALID_PAGE_ID;
      UINT32 rootLvl = COLLECTION_ROUTE_PAGE_LVL0;

      atomicOperationList oplist;
      atomicOperationList *backup = nullptr;
      BOOLEAN swtiched = FALSE;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      clMetaBlock mb;

      if (COLLECTION_FIRST_ROOT_LVL1 == rootSlot ||
          COLLECTION_SECOND_ROOT_LVL1 == rootSlot)
      {
         rootLvl = COLLECTION_ROUTE_PAGE_LVL1;
      }
      else if (COLLECTION_ROOT_LVL2 == rootSlot)
      {
         rootLvl = COLLECTION_ROUTE_PAGE_LVL2;
      }

      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();

      if (INVALID_PAGE_ID != _entryBlock.routeMap[rootSlot])
      {
         goto done;
      }

      clmbpLpid = getMbpLpidOfCollection(getDataPageSize(), getMBID());
      if (OSS_UNLIKELY(INVALID_PAGE_ID == clmbpLpid))
      {
         PD_LOG(PDERROR, "failed to get lpid of cl meta block page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      context->swtichOplist(&oplist, &backup);
      swtiched = TRUE;

      rc = createNewRoutePage(context, rootLvl, routeLpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new route page:%d", rc);
         goto error;
      }

      rc = mds.getLogicalPageBuffer(context, clmbpLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid:%d, rc:%d",
                clmbpLpid, rc);
         goto error;
      }

      /// we are holding ddl s latch and page allocating x latch.
      /// all columns in _record are unchangeable now.
      _entryBlock.routeMap[rootSlot] = routeLpid;
      oplist.setWaitingTail();

      rc = accessor.updateRoutePages(context, _entryBlock.routeMap.data(),
                                     _entryBlock.routeMap.size(), &lpb);
      if (SDB_OK != rc)
      {
         _entryBlock.routeMap[rootSlot] = INVALID_PAGE_ID;
         PD_LOG(PDERROR, "failed to update collection record:%d", rc);
         goto error;
      }

   done:
      lpb.fini();
      if (swtiched)
      {
         context->attachOplist(backup);
      }
      return rc;
   error:
      ///TODO: rollback oplist
      goto done;
   }

   INT32 collection::createNonRootRoutePage(requestContext *context,
                                            PAGE_ID father,
                                            UINT32 pageLvl,
                                            PAGE_ID &out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != father, "can not be invalid");
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(COLLECTION_ROUTE_PAGE_LVL0 <= pageLvl, "impossible");
      SDB_ASSERT(pageLvl <= COLLECTION_ROUTE_PAGE_LVL1, "impossible");

      PAGE_ID lpid = INVALID_PAGE_ID;
      atomicOperationList oplist;
      atomicOperationList *backup = nullptr;
      logicalPageBuffer lpb;
      routePageAccessor accessor;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      context->swtichOplist(&oplist, &backup);

      rc = createNewRoutePage(context, pageLvl, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new route page:%d", rc);
         goto error;
      }

      rc = mds.getLogicalPageBuffer(context, father, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d", father, rc);
         goto error;
      }

      oplist.setWaitingTail();
      rc = accessor.append(context, 1, &lpid, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append new page to father:%d", rc);
         goto error;
      }

      out = lpid;
      
   done:
      context->attachOplist(backup);
      return rc;
   error:
      out = INVALID_PAGE_ID;
      ///TODO: rolblack oplist
      goto done;
   }

   INT32 collection::ensureNonRootLvl1RoutePage(requestContext *context,
                                                PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_PAGE_ID != _entryBlock.routeMap[COLLECTION_ROOT_LVL2], "impossible");
      lpid = INVALID_PAGE_ID;
      routePageAccessor accessor;
      logicalPageBuffer lpb;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      UINT32 targetCount = 0;
      UINT32 minCount = 0;
      UINT32 currentLvl1Count = 0;
      PAGE_ID lvl1 = INVALID_PAGE_ID;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      UINT32 totalLvl0Count = _getLvl0Count().load(std::memory_order_relaxed);

      UINT32 capacity = getCapacityOfRoutePage(getDataPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get route page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      minCount = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                 COLLECTION_SECOND_ROOT_LVL1);
      if (totalLvl0Count < minCount)
      {
         PD_LOG(PDERROR, "invalid totalLvl0Count[%d]", totalLvl0Count);
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (getMaxLvl0Cnt(capacity) < totalLvl0Count)
      {
         PD_LOG(PDERROR, "unexpected totalLvl0Count[%d]", totalLvl0Count);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      targetCount = ((totalLvl0Count - minCount) / capacity) + 1;

      rc = mds.getLogicalPageBuffer(context,
                                    _entryBlock.routeMap[COLLECTION_ROOT_LVL2],
                                    mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get root lvl2:%d", rc);
         goto error;
      }

      rc = accessor.getSizeAndLast(context, COLLECTION_ROUTE_PAGE_LVL2,
                                   &lpb, currentLvl1Count, lvl1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get size:%d", rc);
         goto error;
      }

      lpb.fini();

      if (targetCount == currentLvl1Count)
      {
         if (INVALID_PAGE_ID == lvl1)
         {
            PD_LOG(PDERROR, "current lvl1 count in root lvl2 is %d, but no lpid found",
                   currentLvl1Count);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         lpid = lvl1;
         goto done;
      }
      else if (targetCount != (currentLvl1Count + 1))
      {
         PD_LOG(PDERROR, "target count[%d] does match current count[%d]",
                targetCount, currentLvl1Count);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = createNonRootRoutePage(context,
                                  _entryBlock.routeMap[COLLECTION_ROOT_LVL2],
                                  COLLECTION_ROUTE_PAGE_LVL1,
                                  lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new lvl1 page:%d", rc);
         goto error;
      }

   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }


   INT32 collection::createNewRoutePage(requestContext *context,
                                        UINT32 lvl,
                                        PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _cs, "can not be null");
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      routePageIniter initer;
      initer.setLogicalId(getCollectionId().getLid());
      initer.setLvl(lvl);

      rc = mds.allocatePages(context, &initer, 1, &lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new route page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      lpid = INVALID_PAGE_ID;
      goto done;
   }

   UINT32 collection::getDataPageSize()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      return _cs->getSU()->getManifest().dataArgs.pageSize;
   }

   UINT32 collection::getMaxLvL0RoutePageCountLteRoot(UINT32 capacity,
                                                          UINT32 maxRoot)const
   {
      SDB_ASSERT(0 < capacity, "can not be zero");
      SDB_ASSERT(maxRoot <= COLLECTION_MAX_ROUTE_ROOT, "impossible");
      UINT32 count = 0;
      if (COLLECTION_ROOT_LVL0 <= maxRoot)
      {
         count += 1;
      }
      if (COLLECTION_FIRST_ROOT_LVL1 <= maxRoot)
      {
         count += capacity;
      }
      if (COLLECTION_SECOND_ROOT_LVL1 <= maxRoot)
      {
         count += capacity;
      }
      if (COLLECTION_ROOT_LVL2 <= maxRoot)
      {
         count += (capacity * capacity);
      }
      return count;
   }

   INT32 collection::_createNewIndex(requestContext *context,
                                     const bson::BSONObj &adjunct,
                                     UINT32 &logicalIndexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");   
      SDB_ASSERT(nullptr != context && context->isClPropertiesSet(), "can not be invalid");

      ossPoolString fullName;
      indexProperties properties;
      std::unique_ptr<indexObject> obj;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      indexObjectMap &indexMap = _entryBlock._indexes;
      UINT32 indexLid = INVALID_LOGICAL_INDEX_ID;
      clIndexMetaStorage clStore(_cs->getLogicalID(), getLogicalID());
      SDB_ASSERT(clStore.isValid(), "can not be invalid");

      logicalIndexId = INVALID_LOGICAL_INDEX_ID;

      ossScopedRWLock guard(_entryBlock.getOpLock(), EXCLUSIVE);
      
      if ((INT32)MAX_INDEX_META_ENTRY_SIZE < adjunct.objsize())
      {
         PD_LOG(PDERROR, "index def obj size over max size:%d", adjunct.objsize());
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!indexMap.isAllowedToCreateMore())
      {
         rc = SDB_DMS_MAX_INDEX;
         PD_LOG(PDERROR, "not allowed to create more");
         goto error;
      }

      rc = properties.init(adjunct);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extract index properties:%d", rc);
         goto error;
      }

      if (INDEX_TYPE_HYBRID_TREE != properties.getType())
      {
         PD_LOG(PDERROR, "invalid index type:%d", properties.getType());
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = indexMap.validateCreation(properties);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "invalid index creation, rc:%d", rc);
         goto error;
      }

      obj.reset(SDB_OSS_NEW indexObject());
      if (!obj)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "out of memory");
         goto error;
      }

      rc = _cs->allocateNextIndexLid(indexLid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "allocate next index logical id failed, rc:%d", rc);
         goto error;
      }
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");

      rc = obj->init(indexLid, properties, INDEX_STATUS_BUILDING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index obj:%d", rc);
         goto error;
      }

      fullName = _entryBlock.getProperties()->getFullName();

      rc = commitCreateIndexLog(fullName, indexLid,
                                adjunct, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit create index log:%d", rc);
         goto error;
      }
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");

      obj->resetRebornLSN(lsn);

      rc = indexMap.insertBuildingObject(std::move(obj));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "insert building object failed, rc:%d", rc);
         goto error;
      }

      rc = clStore.upsert(indexLid, indexMap);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "commit index meta data failed, rc:%d", rc);
         goto error;
      }

      logicalIndexId = indexLid;
      
   done:
      return rc;
   error:
      if (DPS_INVALID_LSN_OFFSET != lsn)
      {
         SDB_ASSERT(SDB_OK != rc, "impossible");
         INT32 tmpRc = commitCreateIndexEndLog(fullName, 
                                               properties.getName(),
                                               indexLid, rc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to commit creating end log, rc:%d", tmpRc);
            ossPanic();
         }
      }

      if (!obj && INVALID_LOGICAL_INDEX_ID != indexLid)
      {
         indexMap.destroy(indexLid);
      }
      goto done;
   }

   INT32 collection::_setIndexRemoving(requestContext *context,
                                       const strSlice &indexName,
                                       indexObject **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(nullptr != out, "can not be invalid");

      ossPoolString fullName;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      indexObjectMap &indexMap = _entryBlock._indexes;
      *out = nullptr;
      ossRWMutexGuard guard(_entryBlock.getOpLock(), EXCLUSIVE);

      indexObject *obj = indexMap.getIndexObj(indexName);
      if (nullptr == obj)
      {
         PD_LOG(PDERROR, "index[%s] not found", indexName.str());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
      else if (!obj->isNormal() && !obj->isBuilding())
      {
         PD_LOG(PDERROR, "can not remove index[%s] with status[%d]",
                indexName.str(), obj->getStatus());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }

      fullName = _entryBlock.getProperties()->getFullName();
      rc = commitRemoveIndexLog(fullName, obj->getProperties().getName(),
                                obj->getLogicalID(), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit removing index log:%d", rc);
         goto error;
      }
      
      obj->setStatus(INDEX_STATUS_REMOVING);
      obj->resetRebornLSN(lsn);
      *out = obj;
   done:
      return rc;
   error:
      *out = nullptr;
      goto done;
   }

   INT32 collection::_endToRemoveIndex(requestContext *context,
                                       UINT32 logicalIndexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != logicalIndexId, "can not be invalid");

      clIndexMetaStorage store;
      store.init(_cs->getLogicalID(), getLogicalID());
      SDB_ASSERT(store.isValid(), "can not be invalid");
      indexObjectMap &indexMap = _entryBlock._indexes;

      ossRWMutexGuard guard(_entryBlock.getOpLock(), EXCLUSIVE);

      /// ensure index object firsts
      indexObject *obj = indexMap.getIndexObj(logicalIndexId);
      SDB_ASSERT(nullptr != obj && obj->isRemoving(), "invalid index to remove");
      obj = nullptr;
      _entryBlock._indexes.destroy(logicalIndexId);

      rc = store.removeEntry(logicalIndexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to remove index meta entry:%d", rc);
         ossPanic();
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_buildIndex(requestContext *context,
                                 UINT32 logicalIndexId,
                                 const dmsBuildIndexOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != logicalIndexId, "can not be invalid");

      rc = _buildIndexOnline(context, logicalIndexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build index:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      _abortCreatingIndex(context, logicalIndexId, rc);
      goto done;
   }

   INT32 collection::_buildIndexOnline(requestContext *context,
                                       UINT32 logicalIndexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != logicalIndexId, "can not be invalid");
      constexpr UINT32 _MAX_WINDOW_SIZE = 1;
      OSS_LATCH_MODE mode = SHARED;
      indexObjectMap &indexMap = _entryBlock._indexes;

      do
      {
         UINT32 windowSize = 0;
         UINT32 currentRdpNum = 0;
         ossRWMutexGuard guard(_entryBlock.getOpLock(), mode);
         buildingIndexContext *buildingCtx = indexMap.getBuildingCtx(logicalIndexId);
         if (nullptr == buildingCtx)
         {
            PD_LOG(PDERROR, "failed to get building ctx of index[%d]", logicalIndexId);
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         SDB_ASSERT(!buildingCtx->isScanning(), "can not be scanning");
         currentRdpNum = _getRdpCount().load(std::memory_order_relaxed);
         if (currentRdpNum <= buildingCtx->getLow())
         {
            if (SHARED == mode)
            {
               mode = EXCLUSIVE;
               continue;
            }
            else
            {
               break;
            }
         }

         windowSize = currentRdpNum - buildingCtx->getLow();
         if (_MAX_WINDOW_SIZE < windowSize)
         {
            windowSize = _MAX_WINDOW_SIZE;
         }

         buildingCtx->slideHigh(windowSize);

         rc = _buildIndexInWindow(context, buildingCtx);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build index in window:%d", rc);
            goto error;
         }

         rc = _endToBuildCurrrentWindow(context, buildingCtx);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to finish building current window:%d", rc);
            goto error;
         }

      } while (TRUE);

      rc = _finishIndexBuilding(context, logicalIndexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to finish index building:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_buildIndexInWindow(requestContext *context,
                                         buildingIndexContext *buildingCtx)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(nullptr != buildingCtx && buildingCtx->isScanning(), "can not be invalid");

      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      indexObject *obj = buildingCtx->getIndexObj();
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "can not be invalid");
      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));

      for (UINT32 i = buildingCtx->getLow(); i < buildingCtx->getHigh(); ++i)
      {
         rdpRecordScanner scanner;
         PAGE_ID lpid = INVALID_PAGE_ID;
         rc = getLpidBySequence(context, i, lpid);
         if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS == rc)
         {
            PD_LOG(PDDEBUG, "page seq[%d] does not exist", i);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid of seq[%d], rc:%d", i, rc);
            goto error;
         }

         rc = scanner.open(context, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open scanner:%d", rc);
            goto error;
         }

         while (scanner.isReadyToRead())
         {
            recordID rid;
            bson::BSONObjSet keySet;
            rc = keyGen(obj->getProperties().getPattern().getPattern(), 
                        obj->getProperties().isNotArray(),
                        scanner.getCurrentRecord(),
                        keySet);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to generate index key:%d", rc);
               goto error;
            }

            rid = scanner.getCurrentRid();
            if (obj->getProperties().isUnique())
            {
               recordID duplicatedRid;
               rc = hit.contains(context, obj, keySet, duplicatedRid);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to check if keys already exist:%d", rc);
                  goto error;
               }
               else if (duplicatedRid.isValid())
               {
                  PD_LOG(PDERROR, "duplicated key found");
                  rc = SDB_IXM_DUP_KEY;
                  goto error;
               }
            }

            // Block the write thread through sleep function to limit index write rate.
            // Avoid read performance degradation caused by too many sst files.
            context->getEnv()->hitMgr.limitRate();

            rc = hit.insert(context, obj, keySet, obj->getRebornLSN(),
                            rid, scanner.getCurrentTransID());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert index entries into hybrid tree:%d", rc);
               goto error;
            }

            rc = scanner.next();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to move to next visible pos:%d", rc);
               goto error;
            }
         }//while (scanner.isReadyToRead())
         
      }//for (UINT32 i = buildingCtx->getLow(); i < buildingCtx->getHigh(); ++i)
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_endToBuildCurrrentWindow(requestContext *context,
                                               buildingIndexContext *buildingCtx)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != buildingCtx && buildingCtx->isScanning(), "can not be invalid");
      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));
      INDEX_MERGING_LIST ml;
      indexObject *obj = buildingCtx->getIndexObj();
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "can not be invalid");

      while (!buildingCtx->endToBuildCurrentRange(ml))
      {
         SDB_ASSERT(!ml.empty(), "impossible");
         while (!ml.empty())
         {
            const INDEX_MERGING_RECORD &mr = ml.front();
            if (obj->getProperties().isUnique())
            {
               recordID duplicatedRid;
               for (auto itr = mr->inserting.cbegin();
                    itr != mr->inserting.cend(); ++itr)
               {
                  rc = hit.contains(context, obj, *itr, duplicatedRid);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to check if keys already exist:%d", rc);
                     goto error;
                  }
                  else if (duplicatedRid.isValid())
                  {
                     PD_LOG(PDERROR, "duplicated key found");
                     rc = SDB_IXM_DUP_KEY;
                     goto error;
                  }
               }
            }//if (obj->getProperties().isUnique())

            rc = hit.write(context, obj, &mr->inserting,
                           &mr->discarded, mr->lsn,
                           mr->rid, mr->transID);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to write index entries:%d", rc);
               goto error;
            }

            ml.pop_front();
         }//while (!ml.empty())
      }//while (!buildingCtx->endToBuildCurrentRange(ml))

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_finishIndexBuilding(requestContext *context,
                                          UINT32 logicalIndexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context && context->isClPropertiesSet(), "can not be invalid");
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != logicalIndexId, "can not be invalid");

      clIndexMetaStorage metaStore(_cs->getLogicalID(), getLogicalID());
      indexObject *obj = _entryBlock._indexes.getIndexObj(logicalIndexId);
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "must be building");

      ossPoolString fullName = _entryBlock.getProperties()->getFullName();
      rc = commitCreateIndexEndLog(fullName,
                                   obj->getProperties().getName(),
                                   obj->getLogicalID(),
                                   SDB_OK);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit index creating end log:%d", rc);
         goto error;
      }

      _entryBlock._indexes.finishCreating(logicalIndexId);
      rc = metaStore.upsert(obj->getLogicalID(), obj->toBson());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index meta data:%d", rc);
         SDB_ASSERT(FALSE, "TODO");
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_initIndexesWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      clIndexMetaStorage store(_cs->getLogicalID(), getLogicalID());
      SDB_ASSERT(store.isValid(), "can not be invalid");

      rc = store.reload(_entryBlock._indexes);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load index entries:%d", rc);
         goto error;
      }

      rc = _fixUnstatbleIndexesWhenOpen(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fix unstatble indexes:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_fixUnstatbleIndexesWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      indexObjectMap::CONST_ITERATOR itr =_entryBlock._indexes.begin();
      for (; itr !=_entryBlock._indexes.end(); ++itr)
      {
         if (!itr->second->isNormal())
         {
            SDB_ASSERT(FALSE, "TODO");
         }
      }
      return rc;
   }

   INT32 collection::buildDmlIndexRequests(requestContext *context,
                                           const slice &record,
                                           dmlIndexRequestArray &requests)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      bson::BSONObjSet keySet;

      for (auto itr =_entryBlock._indexes.cbegin();
           itr !=_entryBlock._indexes.cend(); ++itr)
      {
         keySet.clear();
         indexObject *obj = itr->second.get();
         SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");
         if (!obj->isNormal() && !obj->isBuilding())
         {
            continue;
         }

         rc = keyGen(obj->getProperties().getPattern().getPattern(),
                     obj->getProperties().isNotArray(),
                     record, keySet);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to generate index keys:%d", rc);
            goto error;
         }

         rc = requests.append(obj, &keySet, nullptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append index request to array:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      requests.clear();
      goto done;
   }

   INT32 collection::buildUpdateIndexRequests(requestContext *context,
                                              const slice &oldRecord,
                                              IRecordUpdater *updater,
                                              dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(oldRecord.isValid(), "can not be null");
      SDB_ASSERT(nullptr != updater && updater->done(), "can not be invalid");

      slice newRecord(updater->getResultRecordSize(),
                      updater->getResultRecord());
      SDB_ASSERT(newRecord.isValid(), "can not be invalid");
      SDB_ASSERT(!updater->isWholeRecordReset(), "TODO");
      ossPoolVector<const CHAR *> fields;
      updater->dumpUpdatedFields(fields);

      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      bson::BSONObjSet keySetToInsert;
      bson::BSONObjSet keySetToRemove;

      for (auto itr = _entryBlock._indexes.cbegin();
           itr !=_entryBlock._indexes.cend(); ++itr)
      {
         BOOLEAN associated = FALSE;
         indexObject *obj = itr->second.get();
         SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");
         if (!obj->isNormal() && !obj->isBuilding())
         {
            continue;
         }

         for (UINT32 i = 0; i < fields.size(); ++i)
         {
            if (!obj->associates(fields.at(i)))
            {
               continue;
            }

            associated = TRUE;
            break;
         }

         if (!associated)
         {
            continue;
         }

         rc = keyGen(obj->getProperties().getPattern().getPattern(),
                     obj->getProperties().isNotArray(),
                     oldRecord, keySetToRemove);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to generate index keys:%d", rc);
            goto error;
         }

         rc = keyGen(obj->getProperties().getPattern().getPattern(),
                     obj->getProperties().isNotArray(),
                     newRecord, keySetToInsert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to generate index keys:%d", rc);
            goto error;
         }

         rc = ra.append(obj, &keySetToInsert, &keySetToRemove);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make removing index request:%d", rc);
            goto error;
         }
         keySetToRemove.clear();
         keySetToInsert.clear();
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::buildRemoveIndexRequest(requestContext *context,
                                             const slice &oldRecord,
                                             dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(oldRecord.isValid(), "can not be invalid");

      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      bson::BSONObjSet keySetToRemove;

      for (auto itr = _entryBlock._indexes.cbegin();
           itr !=_entryBlock._indexes.end(); ++itr)
      {
         indexObject *obj = itr->second.get();
         SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");
         if (!obj->isNormal() && !obj->isBuilding())
         {
            continue;
         }

         rc = keyGen(obj->getProperties().getPattern().getPattern(),
                     obj->getProperties().isNotArray(),
                     oldRecord, keySetToRemove);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to generate index keys:%d", rc);
            goto error;
         }

         rc = ra.append(obj, nullptr, &keySetToRemove);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make removing index request:%d", rc);
            goto error;
         }
         keySetToRemove.clear();
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::constraintCheck(dmlContext *context,
                                     const dmlIndexRequestArray &ra,
                                     BOOLEAN &duplicated,
                                     utilInsertResult *res)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");

      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));
      recordID rid;
      duplicated = FALSE;

      if (!ra.withConstraint())
      {
         goto done;
      }

      rc = context->lockUniqueIndexKeys(ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock unique index keys:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < ra.getSize(); ++i)
      {
         const dmlIndexRequest *req = ra.get(i);
         SDB_ASSERT(nullptr != req && req->isValid(), "impossible");
         if (!req->withConstraint())
         {
            continue;
         }

         for (auto itr = req->getKeysToInsert().cbegin();
              itr != req->getKeysToInsert().cend(); ++itr)
         {
            rc = hit.contains(context, req->getMutableObject(), *itr, rid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to check if key already exist:%d", rc);
               goto error;
            }

            if (rid.isValid())
            {
               PD_LOG(PDDEBUG, "duplidated key[%s] found in index[%s], rid[%d,%d]",
                      itr->toPoolString().c_str(),
                      req->getObject()->getProperties().getName().c_str(),
                      rid.getPid(), rid.getPos());
               duplicated = TRUE;
               if (nullptr != res)
               {
                  res->incDuplicatedNum();
                  if (res->isEnaleIndexErrInfo())
                  {
                     const indexObject *obj = req->getObject();
                     res->setIndexErrInfo(obj->getProperties().getName().c_str(),
                                          obj->getProperties().getPattern().getPattern(),
                                          *itr);
                  }
               }
               
               goto done;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::mergeIntoBuildingContext(dmlContext *context,
                                              dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");

      indexObjectMap &im = _entryBlock._indexes;
      for (UINT32 i = 0; i < ra.getSize(); ++i)
      {
         dmlIndexRequest *ir = ra.get(i);
         SDB_ASSERT(nullptr != ir && ir->isValid(), "impossible");
         if (!ir->getObject()->isBuilding())
         {
            continue;
         }

         buildingIndexContext *buildingCtx = im.getBuildingCtx(ir->getObject()->getLogicalID());
         SDB_ASSERT(nullptr != buildingCtx, "building context not found");

         rc = buildingCtx->merge(ir, context->getDmlPageSeq(),
                                 context->getRid(), context->getDmlLSN(),
                                 context->getOrigTransId());

         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert merging keys into context:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_abortCreatingIndex(requestContext *context,
                                         UINT32 logicalIndexId,
                                         INT32 reason)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != logicalIndexId, "can not be invalid");
      SDB_ASSERT(SDB_OK != reason, "can not be ok");
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      clIndexMetaStorage store(_cs->getLogicalID(), getLogicalID());
      SDB_ASSERT(store.isValid(), "can not be invalid");
      indexObject *obj = nullptr;
      LPS_PTE_WRITE_BATCH batch;

      {
         ossRWMutexGuard guard(_entryBlock.getOpLock(), EXCLUSIVE);
         obj = _entryBlock._indexes.abortCreating(logicalIndexId);
         SDB_ASSERT(nullptr != obj && obj->isRemoving(), "can not be invalid");

         ossPoolString fullName = _entryBlock.getProperties()->getFullName();
         rc = commitCreateIndexEndLog(fullName,
                                      obj->getProperties().getName(),
                                      logicalIndexId, reason, &lsn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit rollback log:%d", rc);
            ossPanic();
            goto error;
         }

         obj->resetRebornLSN(lsn);
         rc = store.upsert(logicalIndexId, obj->toBson());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert index meta entry:%d", rc);
            ossPanic();
            goto error;
         }
      }

      rc = _cs->getSU()->getIndexSpace().initWriteBatch(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init write batch:%d", rc);
         goto error;
      }

      rc = _truncateIndex(context, obj, TRUE, batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index[%s], rc:%d", 
                obj->getProperties().getName().c_str(), rc);
         goto error;
      }

      rc = _cs->getSU()->getIndexSpace().commit(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit to index space:%d", rc);
         SDB_ASSERT(FALSE, "TODO");
         goto error;
      }

      _endToRemoveIndex(context, logicalIndexId);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_truncateIndex(requestContext *context,
                                    indexObject *obj,
                                    BOOLEAN removeEntryPage,
                                    LPS_PTE_WRITE_BATCH &batch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != obj, "can not be invalid");
      SDB_ASSERT(INDEX_STATUS_TRUNCATING == obj->getStatus() ||
                 INDEX_STATUS_REMOVING == obj->getStatus(), "invalid status");
      SDB_ASSERT(nullptr != batch, "can not be invalid");

      indexSpace &is = _cs->getSU()->getIndexSpace();
      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));
      PTE_ACCESS_CTX_PTR ac(SDB_OSS_NEW spacePteAccessCtx(obj->getLogicalID()));
      if (OSS_UNLIKELY(!ac))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = hit.truncate(context, obj, ac.get(), removeEntryPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index:%d", rc);
         goto error;
      }

      if (!ac->isEmpty())
      {
         rc = is.precommit(context, *batch, std::move(ac));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to precommit:%d", rc);
            is.abort(ac);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_removeAllIndexes(requestContext *context,
                                       DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      OSS_LATCH_MODE mode = SHARED;
      SDB_ASSERT(context->isMbLocked(&mode) && EXCLUSIVE == mode, "must be locked");
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");

      indexSpace &is = _cs->getSU()->getIndexSpace();
      LPS_PTE_WRITE_BATCH batch;
      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));
      clIndexMetaStorage store(_cs->getLogicalID(), getLogicalID());
      SDB_ASSERT(store.isValid(), "can not be invalid");
      indexObjectMap &indexMap = _entryBlock._indexes;

      rc = indexMap.beginToRemoveAll(lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to begin to remove all idnexes:%d", rc);
         goto error;
      }

      /// if hit manager is transferring indexes of this cl:
      /// cl obj will be removed from cs's cl map first.
      /// index transfer will not find this cl any more.
      /// it will block until hit manager release lock.
      rc = is.initWriteBatch(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init write batch:%d", rc);
         goto error;
      }

      for (auto itr = indexMap.begin(); itr != indexMap.end(); ++itr)
      {
         rc = _truncateIndex(context, itr->second.get(), TRUE, batch);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate index[%s], rc:%d", 
                   itr->second->getProperties().getName().c_str(), rc);
            /// continue to remove the next index
         }  
      }

     _entryBlock._indexes.reset();
     rc = is.commit(batch);
     if (SDB_OK != rc)
     {
        PD_LOG(PDERROR, "failed to commit write batch:%d", rc);
        goto error;
     }

      rc = store.destroy();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove index meta entrires:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      SDB_ASSERT(FALSE, "TODO");
      goto done;
   }

   INT32 collection::_truncateAllIndexes(requestContext *context,
                                         DPS_LSN_OFFSET lsn,
                                         LPS_PTE_WRITE_BATCH &batch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      OSS_LATCH_MODE mode;
      SDB_ASSERT(context->isMbLocked(&mode) && EXCLUSIVE == mode, "must be locked");
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");

      clIndexMetaStorage mstore(_cs->getLogicalID(), getLogicalID());
      indexObjectMap &indexMap = _entryBlock._indexes;

      rc = indexMap.beginToTruncateAll(lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set indexes truncating:%d", rc);
         goto error;
      }

      rc = mstore.upsert(indexMap);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index meta data:%d", rc);
         goto error;
      }

      for (auto itr = indexMap.begin(); itr != indexMap.end(); ++itr)
      {
         rc = _truncateIndex(context, itr->second.get(), FALSE, batch);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate index:%d", rc);
            goto error;
         }
      }

      rc = _cs->getSU()->getIndexSpace().commit(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit write batch:%d", rc);
         _cs->getSU()->getIndexSpace().abort(batch);
         goto error;
      }

      indexMap.endToTruncateAll();
      rc = mstore.upsert(indexMap);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index meta data:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_getMoreWhenIndexScan(requestContext *context,
                                           indexObject *obj,
                                           indexScanCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != cursor, "can not be invalid");

      UINT32 maxFetching = cursor->getBaseOptions().stepSize;
      UINT32 fetched = 0;
      hybridIndexTree hit(&(_cs->getSU()->getIndexSpace()));
      indexScanner scanner;
      bson::BufBuilder buf;
      INDEX_ITERATOR_UPTR iterator;
      rc = hit.createIterator(context, obj, iterator);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create iterator:%d", rc);
         goto error;
      }

      rc = scanner.open(cursor, std::move(iterator));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index scanner:%d", rc);
         goto error;
      }
      
      while (fetched < maxFetching)
      {
         rc = scanner.next(context);
         if (SDB_OK == rc)
         {   
            recordID rid = scanner.current()->getRid();
            dmsRecordID dmsRid = rid.toDMSRid(); 

            if (cursor->getOptions().indexCovered)
            { 
               DPS_TRANS_ID transID = scanner.current()->getTransID();
               buf.reset();
               bson::BSONObj recordObj = scanner.current()->getKeyObj(TRUE, &buf);
               slice recordData(recordObj.objsize(), recordObj.objdata());
               rc = cursor->pushDataFragments({slice(sizeof(dmsRecordID), &dmsRid),
                                               slice(sizeof(DPS_TRANS_ID), &transID),
                                               recordData});
               if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
               {
                  rc = SDB_OK;
                  break;
               }
               else if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push data fragments into cursor:%d", rc);
                  goto error;
               }
            } 
            else
            {
               DPS_TRANS_ID transID;
               rdpRecordScanner rdpScanner;
               rc = rdpScanner.openToRead(context, rid);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to read record[%s], rc:%d",
                         rid.toString().c_str(), rc);
                  goto error;
               }

               transID = rdpScanner.getCurrentTransID();
               rc = cursor->pushDataFragments({slice(sizeof(dmsRecordID), &dmsRid),
                                               slice(sizeof(DPS_TRANS_ID), &transID),
                                               rdpScanner.getCurrentRecord()});
               if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
               {
                  rc = SDB_OK;
                  break;
               }
               else if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push data fragments into cursor:%d", rc);
                  goto error;
               }
            }

            cursor->getCtx().markRidScanned(rid);
            rc = scanner.saveLocation();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to save location info:%d", rc);
               goto error;
            }
            ++fetched;

            if (DMS_SCAN_FOR::NONE == cursor->getOptions().scanFor)
            {
               context->unlockRid(rid);
            }
         }
         else if (SDB_IXM_EOC != rc)
         {
            PD_LOG(PDERROR, "failed to fetch next:%d", rc);
            goto error;
         }
         else
         {
            cursor->setEOC();
            rc = SDB_OK;
            break;
         }
      }//while (fetched < maxFetching)

   done:
      scanner.close();
      context->unlockRids();
      return rc;
   error:
      if (DMS_SCAN_FOR::NONE != cursor->getOptions().scanFor)
      {
         context->releaseAllTransLock();
      }
      goto done;
   }

   INT32 collection::lockAndFetchRecordToModify(dmlContext *context, const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context && context->isClPropertiesSet(), "can not be invalid");
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      rdpRecordScanner scanner;
      BOOLEAN locked = FALSE;

      rc = context->lockRid(rid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock rid:%d, rc:%d", rid.toString().c_str(), rc);
         goto error;
      }
      locked = TRUE;

      rc = scanner.openToRead(context, rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open scanner:%d", rc);
         goto error;
      }

      rc = context->saveReocordDataToMrc(scanner.getCurrentRecord());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save record data:%d", rc);
         goto error;
      }
      
      context->getMrc().setTransID(scanner.getCurrentTransID());
      if (scanner.isBigRecord())
      {
         context->getMrc().setAsBigRecord();
      }
      if (scanner.isOverflow())
      {
         context->getMrc().setOverflowAddr(scanner.getOverflowAddr());
      }
      context->setDmlRecordInfo(scanner.getCurrentPageSeq(), rid);

   done:
      scanner.close();
      return rc;
   error:
      if (locked)
      {
         context->unlockRid(rid);
      }
      goto done;
   }

   INT32 collection::updateRecordData(dmlContext *context,
                                      const slice &newRecord)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(newRecord.isValid(), "can not be invalid");


      if (context->getMrc().isBigRecord())
      {
         rc = updateBigRecord(context, newRecord);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update big record, rc:%d", rc);
            goto error;
         }
      }
      else if (context->getMrc().isOverflow())
      {
         rc = updateOverflowedRecord(context, newRecord);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update overflowed record, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = updateNormalRecord(context, newRecord);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update record:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::updateNormalRecord(dmlContext *context,
                                        const slice &newRecord)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(newRecord.isValid(), "can not be invalid");

      logicalPageBuffer lpb;
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      BOOLEAN outOfSpace = FALSE;
      BOOLEAN bigRecord = isBigRecord(getDataPageSize(), newRecord.getSize());
      recordID currentAddr = context->getRid();

      if (!bigRecord)
      {
         rc = mds.getLogicalPageBuffer(context, currentAddr.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", currentAddr.getPid(), rc);
            goto error;
         }

         rc = accessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         rc = accessor.updateNormalRecord(context, currentAddr.getPos(),
                                          newRecord, outOfSpace);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update record by accessor:%d", rc);
            goto error;
         }
         lpb.fini();

         if (outOfSpace)
         {
            rc = overflowRecord(context, newRecord);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update record if out of space, rc:%d", rc);
               goto error;
            }
         }
      }
      else
      {
         bigRecordStream recordStream(newRecord);
         rc = insertBigRecordSlices(context, recordStream);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert big record slices, rc:%d", rc);
            goto error;
         }

         rc = mds.getLogicalPageBuffer(context, currentAddr.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", currentAddr.getPid(), rc);
            goto error;
         }

         rc = accessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         rc = accessor.setRecordOverflowed(context, currentAddr.getPos(),
                                           recordStream.getLastSliceAddr(), 
                                           bigRecord);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to set record overflowed, rc:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::updateOverflowedRecord(dmlContext *context,
                                            const slice &newRecord)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(newRecord.isValid(), "can not be invalid");

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      recordID orid = context->getMrc().getOverflowAddr();
      BOOLEAN bigRecord = isBigRecord(getDataPageSize(), newRecord.getSize());
      BOOLEAN outOfSpace = FALSE;

      // TODO:attach oplist
      if (!bigRecord)
      {
         logicalPageBuffer lpb;
         rdpAccessor accessor;
         rc = mds.getLogicalPageBuffer(context, orid.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", 
                   orid.getPid(), rc);
            goto error;
         }

         rc = accessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor, rc:%d", rc);
            goto error;
         }

         rc = accessor.updateNormalRecord(context, orid.getPos(),
                                          newRecord, outOfSpace);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update record by accessor, rc:%d", rc);
            goto error;
         }
         lpb.fini();

         if (outOfSpace)
         {
            rc = reoverflowRecord(context, newRecord);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update record if out of space, rc:%d", rc);
               goto error;
            }
         }
      }
      else
      {
         bigRecordStream recordStream(newRecord);

         rc = insertBigRecordSlices(context, recordStream);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert overflowed record, rc:%d", rc);
            goto error;
         }

         {
            logicalPageBuffer lpb;
            rdpAccessor accessor;
            recordID currentAddr = context->getRid();

            rc = mds.getLogicalPageBuffer(context, currentAddr.getPid(), 
                                          mode, lpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", 
                     currentAddr.getPid(), rc);
               goto error;
            }

            rc = accessor.init(context, &lpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init accessor:%d", rc);
               goto error;
            }

            rc = accessor.updateOverflowedInfo(context, currentAddr.getPos(),
                                               recordStream.getLastSliceAddr(), 
                                               bigRecord);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update overflowed info, rc:%d", rc);
               goto error;
            }

            lpb.fini();
         }

         {
            logicalPageBuffer lpb;
            rdpAccessor accessor;

            rc = mds.getLogicalPageBuffer(context, orid.getPid(), mode, lpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", orid.getPid(), rc);
               goto error;
            }

            rc = accessor.init(context, &lpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init accessor, rc:%d", rc);
               goto error;
            }

            rc = accessor.destroySlotAndData(context, orid.getPos());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destroy slot and data, rc:%d", rc);
               goto error;
            }

            lpb.fini();
         }
      }
  
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::updateBigRecord(dmlContext *context,
                                     const slice &newRecord)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(newRecord.isValid(), "can not be invalid");

      logicalPageBuffer lpb;
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      BOOLEAN bigRecord = isBigRecord(getDataPageSize(), newRecord.getSize());
      recordID currentAddr = context->getRid();
      recordID overflowAddr;

      // TODO: attach oplist
      if (!bigRecord)
      {
         rc = insertInvisibleRecord(context, newRecord, overflowAddr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert invisible normal record, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         bigRecordStream recordStream(newRecord);
         rc = insertBigRecordSlices(context, recordStream);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert big record slices, rc:%d", rc);
            goto error;
         }
         overflowAddr = recordStream.getLastSliceAddr();
      }

      rc = mds.getLogicalPageBuffer(context, currentAddr.getPid(), 
                                    mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", 
                currentAddr.getPid(), rc);
         goto error;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.updateOverflowedInfo(context, currentAddr.getPos(),
                                         overflowAddr, bigRecord);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update overflowed info, rc:%d", rc);
         goto error;
      }
      lpb.fini();

      rc = removeBigRecordSlices(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove big record slices, rc:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::overflowRecord(dmlContext *context,
                                    const slice &newRecord)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(newRecord.isValid(), "can not be invalid");
      ossSharedLatchMode ridMode;
      recordID rid = context->getRid();
      SDB_ASSERT(context->testRidLocked(rid, &ridMode) && ridMode.isExclusive(), 
                 "rid must be locked");
      logicalPageBuffer lpb;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      recordID orid;
      rdpAccessor accessor;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();

      //TODO:attach oplist
      rc = insertInvisibleRecord(context, newRecord, orid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert overflowed record, rc:%d", rc);
         goto error;
      }

      rc = mds.getLogicalPageBuffer(context, rid.getPid(), mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", rid.getPid(), rc);
         goto error;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }
      rc = accessor.setRecordOverflowed(context, rid.getPos(), orid, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set record to overflowed, rc:%d", rc);
         goto error;
      }

   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::reoverflowRecord(dmlContext *context,
                                      const slice &newRecord)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(newRecord.isValid(), "can not be invalid");

      ossSharedLatchMode ridMode;
      SDB_ASSERT(context->testRidLocked(context->getRid(), &ridMode) && 
                 ridMode.isExclusive(), "rid must be locked");
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      recordID orid;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();

      //TODO:attach oplist
      rc = insertInvisibleRecord(context, newRecord, orid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert overflowed record, rc:%d", rc);
         goto error;
      }

      {
         logicalPageBuffer lpb;
         recordID rid = context->getRid();
         rdpAccessor accessor;
         rc = mds.getLogicalPageBuffer(context, rid.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", rid.getPid(), rc);
            goto error;
         }

         rc = accessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         rc = accessor.updateOverflowedInfo(context, rid.getPos(), orid, FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update overflowed info, rc:%d", rc);
            goto error;
         }
         lpb.fini();
      }

      {
         logicalPageBuffer lpb;
         recordID rid = context->getMrc().getOverflowAddr();
         rdpAccessor accessor;
         rc = mds.getLogicalPageBuffer(context, rid.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", rid.getPid(), rc);
            goto error;
         }

         rc = accessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         rc = accessor.destroySlotAndData(context, rid.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy record[%d], rc:%d",rid.getPos(), rc);
            goto error;
         }
         lpb.fini();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeRecordData(dmlContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be invalid");

      if (context->getMrc().isBigRecord())
      {
         rc = removeBigRecord(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove big record, rc:%d", rc);
            goto error;
         }
      }
      else if (context->getMrc().isOverflow())
      {
         rc = removeOverflowedRecord(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove overflowed record, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = removeNormalRecord(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove record:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeNormalRecord(dmlContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");

      logicalPageBuffer lpb;
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      recordID rid = context->getRid();

      rc = mds.getLogicalPageBuffer(context, rid.getPid(), mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", rid.getPid(), rc);
         goto error;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.deleteRecord(context, rid.getPos());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to delete record by accessor:%d", rc);
         goto error;
      }

      lpb.fini();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeOverflowedRecord(dmlContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      recordID rid = context->getRid();
      recordID overflowAddr = context->getMrc().getOverflowAddr();

      {
         logicalPageBuffer lpb;
         rdpAccessor accessor;
         rc = mds.getLogicalPageBuffer(context, rid.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", rid.getPid(), rc);
            goto error;
         }

         rc = accessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor, rc:%d", rc);
            goto error;
         }

         rc = accessor.deleteRecord(context, rid.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to delete record by accessor, rc:%d", rc);
            goto error;
         }
         lpb.fini();
      }

      {
         logicalPageBuffer lpb;
         rdpAccessor accessor;
         rc = mds.getLogicalPageBuffer(context, overflowAddr.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", overflowAddr.getPid(), rc);
            goto error;
         }

         rc = accessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         rc = accessor.destroySlotAndData(context, overflowAddr.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy record[%d], rc:%d",overflowAddr.getPos(), rc);
            goto error;
         }
         lpb.fini();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeBigRecord(dmlContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      logicalPageBuffer lpb;
      rdpAccessor accessor;
      recordID overflowAddr = context->getMrc().getOverflowAddr();
      recordID currentAddr = overflowAddr;

      rc = mds.getLogicalPageBuffer(context, context->getRid().getPid(),
                                    mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", 
                context->getRid().getPid(), rc);
         goto error;
      }

      rc = accessor.init(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor, rc:%d", rc);
         goto error;
      }

      rc = accessor.deleteRecord(context, context->getRid().getPos());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to delete record by accessor, rc:%d", rc);
         goto error;
      }
      lpb.fini();

      rc = removeBigRecordSlices(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove big record slices, rc:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeBigRecordSlices(dmlContext *context)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      recordID currentAddr = context->getMrc().getOverflowAddr();

      do
      {
         logicalPageBuffer lpb;
         rdpAccessor tmpAccessor;
         const recordDataPageHead *head = nullptr;
         recordSlot rs;
         recordID nextAddr;

         rc = mds.getLogicalPageBuffer(context, currentAddr.getPid(), mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", 
                   currentAddr.getPid(), rc);
            goto error;
         }

         rc = tmpAccessor.init(context, &lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor, rc:%d", rc);
            goto error;
         }

         rc = tmpAccessor.getNextSliceAddrOfBigRecord(currentAddr.getPos(), nextAddr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next slice address, rc:%d", rc);
            goto error;
         }

         rc = tmpAccessor.destroySlotAndData(context, currentAddr.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy "
                   "big record slice's slot and data, rc:%d", rc);
            goto error;
         }

         head = tmpAccessor.getReadablePageHead();
         if (OSS_UNLIKELY(nullptr == head))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get readable page head, rc:%d", rc);
            goto error;
         }

         currentAddr = nextAddr;

      } while (currentAddr.isValid());
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::releaseAllRdps(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      OSS_LATCH_MODE mode = SHARED;
      SDB_ASSERT(context->isMbLocked(&mode) && EXCLUSIVE == mode, "must be exclusive");

      loopReleaseRdpsInLvl0(context);
      loopReleaseRoutePages(context);
   
      return rc;
   }

   INT32 collection::loopReleaseRdpsInLvl0(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      OSS_LATCH_MODE m = SHARED;
      SDB_ASSERT(context->isMbLocked(&m) && EXCLUSIVE == m, "must be exclusive");
      ossPoolVector<PAGE_ID> batch;
      logicalPageBuffer buffer;
      UINT32 capacity = getCapacityOfRoutePage(getDataPageSize());
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      UINT32 totalLvl0Count = _getLvl0Count().load(std::memory_order_relaxed);


      for (INT32 i = (INT32)totalLvl0Count - 1; i >= 0; --i)
      {
         routePageAccessor accessor;
         PAGE_ID lpid = INVALID_PAGE_ID;
         rc = getLvl0RoutePage(context, capacity, (UINT32)i, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid of lvl0[%d], rc:%d", i, rc);
            goto error;
         }

         rc = mds.getLogicalPageBuffer(context, lpid, mode, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page buffer of lpid[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = accessor.dumpValidPages(context, COLLECTION_ROUTE_PAGE_LVL0,
                                      &buffer, batch);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to clear and dump lvl0 page:%d", rc);
            goto error;
         }

         buffer.fini();
         commitReleasingPagesLog(context, batch, bson::BSONObj());
         mds.releasePages(context, batch.size(), batch.data());
         batch.clear();
      }

      _getRdpCount().store(0, std::memory_order_relaxed);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::loopReleaseRoutePages(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      OSS_LATCH_MODE m = SHARED;
      SDB_ASSERT(context->isMbLocked(&m) && EXCLUSIVE == m, "must be exclusive");
      ossPoolVector<PAGE_ID> batch;
      logicalPageBuffer buffer;
      mainDataSpace &mds = _cs->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      SDB_ASSERT(INVALID_PAGE_ID == _entryBlock.routeMap[COLLECTION_ROOT_LVL2],
                "plan to remove lvl2 node");

      for (INT32 i = COLLECTION_SECOND_ROOT_LVL1; i > (INT32)COLLECTION_ROOT_LVL0; --i)
      {
         routePageAccessor accessor;
         PAGE_ID lpid = _entryBlock.routeMap[i];
         if (INVALID_PAGE_ID == lpid)
         {
            continue;
         }

         rc = mds.getLogicalPageBuffer(context, lpid, mode, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page buffer of lpid[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = accessor.dumpValidPages(context, COLLECTION_ROUTE_PAGE_LVL1,
                                      &buffer, batch);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to clear and dump lvl0 page:%d", rc);
            goto error;
         }

         buffer.fini();

         batch.push_back(lpid);
      }

      if (INVALID_PAGE_ID != _entryBlock.routeMap[COLLECTION_ROOT_LVL0])
      {
         batch.push_back(_entryBlock.routeMap[COLLECTION_ROOT_LVL0]);
      }

      commitReleasingPagesLog(context, batch, bson::BSONObj());
      mds.releasePages(context, batch.size(), batch.data());
      _getLvl0Count().store(0, std::memory_order_relaxed);
      _entryBlock.routeMap.fill(INVALID_PAGE_ID);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertLobChunk(requestContext *context,
                                    const lobChunkKey &key,
                                    UINT32 offset,
                                    const slice &data)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                           !context->isMbLocked() ||
                           context->getMBID() != getMBID() ||
                           !key.isValid() ||
                           !data.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isClPropertiesSet(), "can not be attached");
      context->setClProperties(_entryBlock.getProperties());

      {
         largeObjectSpace &los = _cs->getSU()->getLobSpace();
         if (!los.isOpen())
         {
            rc = los.ensureCreated();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to create lob space:%d", rc);
               goto error;
            }
         }

         rc = los.insertLobChunk(context, key, offset, data);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert lob chunk:%d", rc);
            goto error;
         }
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::readLobChunk(requestContext *context,
                                  const lobChunkKey &key,
                                  UINT32 offset,
                                  UINT32 size,
                                  CHAR *data,
                                  UINT32 &readSize)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                           !context->isMbLocked() ||
                           context->getMBID() != getMBID() ||
                           !key.isValid() ||
                           nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isClPropertiesSet(), "can not be attached");
      context->setClProperties(_entryBlock.getProperties());

      {
         largeObjectSpace &los = _cs->getSU()->getLobSpace();
         if (!los.isOpen())
         {
            rc = SDB_LOB_SEQUENCE_NOT_EXIST;
            goto error;
         }

         rc = los.readLobChunk(context, key, offset, size,
                               data, readSize);
         if (SDB_OK != rc)
         {
            if (SDB_LOB_SEQUENCE_NOT_EXIST != rc)
            {
               PD_LOG(PDERROR, "failed to read lob chunk:%d", rc);
            }
            goto error;
         }
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeLobChunk(requestContext *context,
                                    const lobChunkKey &key)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                           !context->isMbLocked() ||
                           context->getMBID() != getMBID() ||
                           !key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isClPropertiesSet(), "can not be attached");
      context->setClProperties(_entryBlock.getProperties());

      {
         largeObjectSpace &los = _cs->getSU()->getLobSpace();
         if (!los.isOpen())
         {
            rc = SDB_LOB_SEQUENCE_NOT_EXIST;
            goto error;
         }

         rc = los.removeLobChunk(context, key);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::updateLobChunk(requestContext *context,
                                    const lobChunkKey &key,
                                    UINT32 offset,
                                    const slice &data,
                                    BOOLEAN createIfNotExists)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                           !context->isMbLocked() ||
                           context->getMBID() != getMBID() ||
                           !key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isClPropertiesSet(), "can not be attached");
      context->setClProperties(_entryBlock.getProperties());

      {
         largeObjectSpace &los = _cs->getSU()->getLobSpace();
         if (!los.isOpen())
         {
            if (!createIfNotExists)
            {
               rc = SDB_LOB_SEQUENCE_NOT_EXIST;
               goto error;
            }
            else
            {
               rc = los.ensureCreated();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to create lob space:%d", rc);
                  goto error;
               }

               rc = los.insertLobChunk(context, key, offset, data);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert lob chunk:%d", rc);
                  goto error;
               }
            }
         }
         else
         {
            rc = los.updateLobChunk(context, key, offset, data, createIfNotExists);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::truncateLobChunk(requestContext *context,
                                      const lobChunkKey &key,
                                      UINT32 size,
                                      UINT32 &tsize)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                           !context->isMbLocked() ||
                           context->getMBID() != getMBID() ||
                           !key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isClPropertiesSet(), "can not be attached");
      context->setClProperties(_entryBlock.getProperties());

      {
         largeObjectSpace &los = _cs->getSU()->getLobSpace();
         if (!los.isOpen())
         {
            rc = SDB_LOB_SEQUENCE_NOT_EXIST;
            goto error;
         }
         else
         {
            rc = los.truncateLobChunk(context, key, size, tsize);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::testLobChunk(requestContext *context,
                                  const lobChunkKey &key,
                                  dmsLobChunkProfile *profile)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                           !context->isMbLocked() ||
                           context->getMBID() != getMBID() ||
                           !key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isClPropertiesSet(), "can not be attached");
      context->setClProperties(_entryBlock.getProperties());

      {
         largeObjectSpace &los = _cs->getSU()->getLobSpace();
         if (!los.isOpen())
         {
            rc = SDB_LOB_SEQUENCE_NOT_EXIST;
            goto error;
         }
         else
         {
            rc = los.testLobChunk(context, key, profile);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
      }
   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::listLobChunks(requestContext *context,
                                   listLobChunkCursor *cursor)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                           !context->isMbLocked() ||
                           context->getMBID() != getMBID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isClPropertiesSet(), "can not be attached");
      //mbContext.init(_clMetaBlock, _cs->getIdentifier());
      //context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _cs->getSU()->getLobSpace();
         if (!los.isOpen())
         {
            cursor->setEOC();
         }
         else
         {
            rc = los.list(cursor);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to list lob chunks:%d", rc);
               goto error;
            }
         }
      }
   done:
      // if (nullptr != context)
      // {
      //    context->resetClProperties();
      // }
      return rc;
   error:
      goto done;
   }

   void collection::initProperties(const clMetaBlock &block)
   {
      SDB_ASSERT(block.isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != _cs, "can not be invalid");
      collectionProperties &properties = _entryBlock._properties;

      properties.csproperties = _cs->getProperties();
      properties.clid = collectionId(block.logicalCLID,
                                     block.innerID,
                                     block.mbID);
      properties.name.assign(block.name);
      properties.type = (CL_TYPE)block.type;
      properties.compressor = (UTIL_COMPRESSOR_TYPE)block.compressionType;
      properties._minFreePct = block.minFreePercent;
      properties.stripingRange = dmsStripingRange(block.minStriping,
                                                  block.maxStriping);
      return;
   }

   void collection::initRouteMapInBlock(const clMetaBlock &block)
   {
      SDB_ASSERT(block.isValid(), "can not be invalid");
      for (UINT32 i = 0; i < _entryBlock.routeMap.size(); ++i)
      {
         _entryBlock.routeMap.at(i) = block.routePages[i];
      }
   }

   void collection::_exportMetaBlock(clMetaBlock &mb)const
   {
      mb.reset();
      mb.version = CL_META_BLOCK_VERSION;
      mb.type = (UINT16)(getProperties()->type);
      mb.mbID = getMBID();
      mb.innerID = getCollectionId().getInnerId();
      mb.logicalCLID = getCollectionId().getLid();
      for (UINT32 i = 0; i < _entryBlock.routeMap.size(); ++i)
      {
         mb.routePages[i] = _entryBlock.routeMap[i];
      }
      mb.minStriping = getProperties()->stripingRange.getLow().getValue();
      mb.maxStriping = getProperties()->stripingRange.getHigh().getValue();
      ossMemset(mb.name, 0, sizeof(mb.name));
      ossMemcpy(mb.name, getProperties()->name.c_str(), getProperties()->name.size());
      mb.compressionType = (UINT8)(getProperties()->compressor);
      mb.minFreePercent = (UINT8)(getProperties()->_minFreePct);
   }

   INT32 collection::transferIndexEntries(requestContext *context,
                                          hitTransferTaskCtx *tc)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == tc ||
                            !tc->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         const globalIndexID &indexId = tc->getTask().getGlobalIndexID();
         SDB_ASSERT(indexId.getLogicalCLID() == getLogicalID(), "must be same");
         ossRWMutexGuard guard(_entryBlock.getOpLock(), SHARED);
         
         context->setClProperties(_entryBlock.getProperties());
         indexObject *obj = _entryBlock._indexes.getIndexObj(indexId.getLogicalIndexID());
         if (nullptr == obj || !obj->isWritable())
         {
            PD_LOG(PDERROR, "index[%s] is not ready to transfer", indexId.toString().c_str());
            rc = SDB_IXM_NOTEXIST;
            goto error;
         }

         rc = _transferIndexEntries(context, tc, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to transfer index entries:%d", rc);
            goto error;
         }
      }

   done:
      if (nullptr != context)
      {
         context->resetClProperties();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::_transferIndexEntries(requestContext *context,
                                           hitTransferTaskCtx *taskCtx,
                                           indexObject *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(!taskCtx->isBtreeEntryPageUnstable(), "invalid status");
    
      STACK_KEY_STRING_BUILDER builder;
      lsmIteratorBound bound;
      rocksdb::ReadOptions o;
      o.total_order_seek = TRUE;
      std::unique_ptr<rocksdb::Iterator> itr;
      btreeWriter bw;
      indexSpace &is = _cs->getSU()->getIndexSpace();
      hitIndexTransferTask &task = taskCtx->getTask();
      PTE_ACCESS_CTX_PTR ac(SDB_OSS_NEW spacePteAccessCtx(task.getTaskId()));
      if (OSS_UNLIKELY(!ac))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = bound.init(task.getGlobalIndexID());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init itr bound:%d", rc);
         goto error;
      }

      o.iterate_lower_bound = bound.getLowBound();
      o.iterate_upper_bound = bound.getUpBound();
      itr.reset(task.getReader()->NewIterator(o));
      if (OSS_UNLIKELY(!itr))
      {
         PD_LOG(PDERROR, "failed to new lsm iterator");
         rc = SDB_OOM;
         goto error;
      }

      if (!obj->getBtreeEntryAddr().isValid())
      {
         rc = _createBtreeEntryPage(context, obj, taskCtx, ac.get());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create btree entry page:%d", rc);
            goto error;
         }
      }

      rc = bw.init(context, &is, obj, ac.get());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree writer:%d", rc);
         goto error;
      }

      ///TODO: validate itr' status
      itr->SeekToFirst();
      while (itr->Valid())
      {
         lsmIndexEntryValueRef valueRef(itr->value());
         lsmKeyStringEntry entry(itr->key().size(), itr->key().data());
         if (OSS_UNLIKELY(!entry.isValid()))
         {
            PD_LOG(PDERROR, "failed to load index entry");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else if (OSS_UNLIKELY(!valueRef.isValid()))
         {
            PD_LOG(PDERROR, "failed to load index entry value");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = builder.rebuildEntryKey(entry, entry.getRid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build btree index entry: %d", rc);
            goto error;
         }
         else
         {
            keyString ks = builder.getShallowKeyString();
            btreeKeyStringEntry be;
            be.shallowCopy(ks);
            SDB_ASSERT(be.isValid(), "impossible");
            const lsmIndexEntryValue *value = valueRef.getValuePtr();

            if (!value->isDeleted())
            {
               rc = bw.insert(be, value->lsn, value->transID);
               if (SDB_IXM_IDENTICAL_KEY == rc)
               {
                  /// idempotent transferring
                  rc = SDB_OK;
               }
               else if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert btree record:%d", rc);
                  goto error;
               }
               else
               {
                  taskCtx->incInsertedEntryNum();
               }
            }
            else
            {
               rc = bw.remove(be, value->lsn, value->transID);
               if (SDB_VESSEL_IXM_ITEM_NOT_FOUND == rc)
               {
                  /// idempotent transferring
                  rc = SDB_OK;
               }
               else if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to remove btree record:%d", rc);
                  goto error;
               }
               else
               {
                  taskCtx->incRemovedEntryNum();
               }
            }
         }

         itr->Next();
      }

      if (0 != taskCtx->getInsertedEntryNum() ||
          0 != taskCtx->getRemovedEntryNum())
      {
         rc = bw.refreshEntryPage();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to refresh entry page:%d", rc);
            goto error;
         }
      }

      PD_LOG(PDDEBUG, "insert num[%d], remove num[%d]",
             taskCtx->getInsertedEntryNum(),
             taskCtx->getRemovedEntryNum());

      rc = is.precommit(context, *taskCtx->getBatch(), std::move(ac));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to precommit to inde space:%d", rc);
         goto error;
      }

      
   done:
      return rc;
   error:
      if (taskCtx->isBtreeEntryPageUnstable())
      {
         _rollbackUnstableBtreeEntryPage(context, obj, taskCtx);
      }
      if (nullptr != ac && !ac->isEmpty())
      {
         is.abort(ac);
      }
      goto done;
   }

   INT32 collection::_createBtreeEntryPage(requestContext *context,
                                           indexObject *obj,
                                           hitTransferTaskCtx *taskCtx,
                                           spacePteAccessCtx *ac)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!obj->getBtreeEntryAddr().isValid(), "do not recreate");
      SDB_ASSERT(!taskCtx->isBtreeEntryPageUnstable(), "do not recreate");
      indexSpace &is = _cs->getSU()->getIndexSpace();
      btreeEntryPageIniter initer(obj->getLogicalID());
      PAGE_ID lpid = INVALID_PAGE_ID;
      clIndexMetaStorage mstore;

      rc = is.allocatePtePage(context, ac, &initer, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate entry page:%d", rc);
         goto error;
      }

      mstore.init(_cs->getLogicalID(), getLogicalID());
      rc = mstore.upsert(obj->getLogicalID(), obj->toBson());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index meta entry:%d", rc);
         goto error;
      }

      obj->setBtreeEntryAddr(lpid, taskCtx->getBatch()->getWritingPSN());
      taskCtx->setBtreeEntryPageUnstable();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_rollbackUnstableBtreeEntryPage(requestContext *context,
                                                     indexObject *obj,
                                                     hitTransferTaskCtx *taskCtx)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(taskCtx->isBtreeEntryPageUnstable(), "nothing to rollback");
      SDB_ASSERT(obj->getBtreeEntryAddr().isValid(), "invalid status");
      clIndexMetaStorage mstore;
      mstore.init(_cs->getLogicalID(), getLogicalID());
      obj->resetBtreeEntryAddr();
      rc = mstore.upsert(obj->getLogicalID(), obj->toBson());
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to update index meta entry:%d", rc);
         ossPanic();
         goto error;
      }

      taskCtx->resetBtreeEntryPageUnstabl();
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine