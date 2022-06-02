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

   Source File Name = collection.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
#include "vessel/indexEntryPage.h"
#include "vessel/indexConsole.h"
#include "vessel/redoLogUtil.h"
#include "vessel/indexKeyGenerator.h"
#include "vessel/outerResource.h"
#include "vessel/rdpRecordScanner.h"
#include "vessel/indexScanner.h"
#include "vessel/buildingIndexContext.h"
#include "vessel/indexScanContext.h"
#include "vessel/indexScanCursor.h"
#include "interface/IRecordUpdater.h"
#include "vessel/runtimeMbContext.h"
#include "vessel/stackAllocatorRowBatch.h"
#include "vessel/indexScanEntry.h"
#include "ixm_common.hpp"
#include "vessel/clIndexMetaBlockPage.h"
#include "vessel/clIndexMbpAccessor.h"
#include "dmsLobDef.hpp"

namespace engine
{
namespace vessel
{
   collection::collection()
   {
      
   }

   collection::~collection()
   {
      fini();
   }

   INT32 collection::initWhenOpen(requestContext *context,
                                  const clMetaBlock &block,
                                  collectionSpace *cs)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr == _collectionSpace, "do not reinit");
      fsmFile *file = nullptr;
      runtimeMbContext mbContext;
      
      if (OSS_UNLIKELY(nullptr == context ||
                       !block.isValid() ||
                       nullptr == cs ||
                       !cs->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _collectionSpace = cs;
      _clMetaBlock = block;
      mbContext.init(_clMetaBlock,
                     _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);
      
      rc = initPageSequenceWhenOpen(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page sequence:%d", rc);
         goto error;
      }

      file = cs->getSU()->getMainDataSpace().getFsmFile();
      rc = _fsm.open(block.mbID,
                     block.logicalCLID,
                     _rdpCount.load(std::memory_order_relaxed),
                     file,
                     dmsStripingRange(block.minStriping,
                                      block.maxStriping));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open free space map of cl[%d], rc:%d",
                block.mbID, rc);
         goto error;
      }

         rc = initIndexesWhenOpen(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init indexes, rc:%d", rc);
            goto error;
         }

      context->detachMbContext();
   done:
      return rc;
   error:
      if (nullptr != context)
      {
         context->detachMbContext();
      }
      fini();
      goto done;
   }

   globalCollectionId collection::getGlobalId()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      globalCollectionId gcid;
      gcid.reset(_collectionSpace->getIdentifier(), getCollectionId());
      return gcid;
   }
   
   collectionId collection::getCollectionId()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      return collectionId(_clMetaBlock.logicalCLID, _clMetaBlock.innerID, _clMetaBlock.mbID);
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
      runtimeMbContext mbContext;

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

      _collectionSpace = cs;

      _clMetaBlock.reset();
      _clMetaBlock.version = CL_META_BLOCK_VERSION;
      _clMetaBlock.mbID = context->getMBID();
      _clMetaBlock.innerID = innerID;
      _clMetaBlock.type = options.type;
      _clMetaBlock.logicalCLID = logicalID;
      _clMetaBlock.minFreePercent = options.minFreePercent;
      _clMetaBlock.minStriping = options.stripingRange.getLow().getValue();
      _clMetaBlock.maxStriping = options.stripingRange.getHigh().getValue();
      ossMemcpy(_clMetaBlock.name, clName.str(), clName.strLen());
      _clMetaBlock.compressionType = options.compressionType;

      fsm = cs->getSU()->getMainDataSpace().getFsmFile();
      rc = _fsm.create(context->getMBID(),
                       logicalID, fsm,
                       dmsStripingRange(_clMetaBlock.minStriping,
                                        _clMetaBlock.maxStriping));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create free space map of mb[%d], rc:%d",
                context->getMBID(), rc);
         goto error;
      }

      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);
      
      rc = initCLMetaBlockOnDisk(context, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cl meta block, rc:%d", rc);
         goto error;
      }

   done:
      if (nullptr != context)
      {
         context->detachMbContext();
      }
      return rc;
   error:
      fini();
      goto done;
   }

   void collection::fini()
   {
      _clMetaBlock.reset();
      _collectionSpace = nullptr;
      _lvl0Count.store(0, std::memory_order_relaxed);
      _rdpCount.store(0, std::memory_order_relaxed);
      _fsm.close();
      _indexes.fini();
      return;
   }

   INT32 collection::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      OSS_LATCH_MODE mode = SHARED;
      runtimeMbContext mbContext;

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

      mbContext.init(_clMetaBlock,
                     _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);
      _fsm.destroy();
      removeAllIndexes(context);
      releaseAllRdps(context);
      removeCLMetaBlockOnDisk(context);
      if (_collectionSpace->getSU()->getLobSpace().isOpen())
      {
         _collectionSpace->getSU()->getLobSpace().removeLobChunksInCL(context);
      }
      
      context->detachMbContext();
      fini();
   done:
      
      return rc;
   error:
      goto done;
   }

   INT32 collection::truncate(requestContext *context)
   {
      INT32 rc = SDB_OK;
      OSS_LATCH_MODE mode;
      runtimeMbContext mbContext;

      ///TODO: hold shared mb lock and exclusive ddl latch.

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

      mbContext.init(_clMetaBlock,
                     _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      if (_collectionSpace->getSU()->getLobSpace().isOpen())
      {
         _collectionSpace->getSU()->getLobSpace().removeLobChunksInCL(context);
      }

      _fsm.truncate();

      rc = truncateAllIndexes(context);
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

      for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
      {
         _clMetaBlock.routePages[i] = INVALID_PAGE_ID;
      }
   done:
      context->detachMbContext();
      return rc;
   error:
      goto done;
   }

   INT32 collection::createIndex(requestContext *context,
                                 const dmsBuildIndexOptions &o,
                                 const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;
      indexIdentifier indexId;
      runtimeMbContext mbContext;
      indexDescription desc;

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

      rc = desc.extractFromBson(adjunct);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extract index description, rc:%d", rc);
         goto error;
      }

      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      rc = _createIndex(context, desc, indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index[%s]:%d", 
                desc.getName().c_str(), rc);
         goto error;
      }
   
      rc = buildIndexInContext(context, indexId, desc.getType(), o);
      if (SDB_OK != rc)
      {
         if (SDB_VESSEL_INDEX_BUILDING_TERMINATED != rc)
         {
            PD_LOG(PDERROR, "failed to build index[%s], rc:%d", rc);
         }
         else
         {
            PD_LOG(PDINFO, "index[%s] creating terminated");
         }
         INT32 tmpRc = rollbackCreatingIndex(context, indexId, rc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDERROR, "failed to rollback index creating[%s], rc:%d", rc);
            ossPanic();
         }
         goto error;
      }
      
   done:
      if (nullptr != context)
      {
         context->detachMbContext();
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
      ossRWMutexGuard guard(&_indexlock, SHARED);

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

      for (indexObjectMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         bson::BSONObjBuilder builder;
         itr->second->dump(builder);
         indexes.push_back(builder.obj());
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
      runtimeMbContext mbContext;
      indexIdentifier indexId;

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

      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      rc = markIndexRemovingByName(context, indexName, indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index[%s] ready to be removed:%d",
                indexName.str(), rc);
         goto error;
      }

      rc = truncateIndex(context, indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index[%d], rc:%d", 
                indexId.getIndexSlot(), rc);
         goto error;
      }

      releaseIndexObjectAndEntryPage(context, indexId);
   done:
      if (nullptr != context)
      {
         context->detachMbContext();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::testNormalIndex(requestContext *context,
                                     const strSlice &indexName,
                                     indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      ossRWMutexGuard guard(&_indexlock, SHARED, FALSE);
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
      for (indexObjectMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         if (itr->second->isNormal() &&
             itr->second->getDescription().getNameSlice() == indexName)
         {
            indexId.reset(itr->first, 
                          itr->second->getIndexId().getLogicalIndexId());
            break;
         }
      }

      if (!indexId.isValid())
      {
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::initCLMetaBlockOnDisk(requestContext *context,
                                           const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(_clMetaBlock.isValid(), "must be valid");
      logicalPageBuffer lpb;
      clMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      UINT32 pageSize = getDataPageSize();
      strSlice csName;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      PAGE_ID lpid = getMbpLpidOfCollection(pageSize, _clMetaBlock.mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", _clMetaBlock.mbID);
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
      
      rc = accessor.createCL(context, _clMetaBlock, options, &lpb);
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
      SDB_ASSERT(_clMetaBlock.isValid(), "must be valid");

      logicalPageBuffer lpb;
      clMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      UINT32 pageSize = getDataPageSize();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      PAGE_ID lpid = getMbpLpidOfCollection(pageSize, _clMetaBlock.mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", _clMetaBlock.mbID);
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
      SDB_ASSERT(_clMetaBlock.isValid(), "must be valid");

      logicalPageBuffer lpb;
      clMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      UINT32 pageSize = getDataPageSize();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      PAGE_ID lpid = getMbpLpidOfCollection(pageSize, _clMetaBlock.mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", _clMetaBlock.mbID);
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

   INT32 collection::dump(requestContext *context,
                          bson::BSONObj &record)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == _collectionSpace))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      record = dumpCollectionWhenList(_collectionSpace->getLogicalID(),
                                      _clMetaBlock);
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
      ossRWMutexGuard guard(&_indexlock, SHARED, FALSE);
      runtimeMbContext mbContext;

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
      else if (_clMetaBlock.isStripingMode() &&
               !request.o.stripingId.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      guard.autoLock();
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);
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
         rc = insertIndexRequests(context, ra);
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
         context->clearHistroyAndDetachMb();
      }

      SDB_ASSERT(mbContext.getRidLatchContext().isEmpty(), "must be empty");
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
      runtimeMbContext mbContext;
      ossRWMutexGuard guard(&_indexlock, SHARED, FALSE);

      recordID rid;
      indexConsole console;
      slice targetRecord;
      slice newRecord;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      guard.autoLock();
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);
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

      console.init(_clMetaBlock.mbID, &(_collectionSpace->getSU()->getIndexSpace()));
      rc = console.handleDmlRequest(context, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to handle index requests:%d", rc);
         goto error;
      }

      if (nullptr != res)
      {
         res->incModifiedNum();
      }

   done:
      if (nullptr != context)
      {
         context->clearHistroyAndDetachMb();
      }

      SDB_ASSERT(mbContext.getRidLatchContext().isEmpty(), "must be empty");
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
      runtimeMbContext mbContext;
      ossRWMutexGuard guard(&_indexlock, SHARED, FALSE);

      indexConsole console;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      guard.autoLock();
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

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

         console.init(_clMetaBlock.mbID, &(_collectionSpace->getSU()->getIndexSpace()));
         rc = console.handleDmlRequest(context, ra);
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
         context->clearHistroyAndDetachMb();
      }

      SDB_ASSERT(mbContext.getRidLatchContext().isEmpty(), "must be empty");
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
      runtimeMbContext mbContext;

      count = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      for (UINT32 i = 0; i < _rdpCount.load(std::memory_order_relaxed); ++i)
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
         context->detachMbContext();
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
      SDB_ASSERT(cursor->getCollectionId().getCLLid() == _clMetaBlock.logicalCLID,
                 "must be same");

      runtimeMbContext mbContext;
      UINT32 pageStep = 2;
      UINT32 totalRead = 0;
      UINT32 maxPageSeq = 0;

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

      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);
      if (0 < cursor->getOptions().pageStep)
      {
         pageStep = cursor->getOptions().pageStep;
      }
      maxPageSeq = cursor->getToScanEntry().getSeq() + (UINT32)pageStep;

      do
      {
         UINT32 readCount = 0;
         PAGE_ID lpid = cursor->getLpid();

         if (_rdpCount.load(std::memory_order_relaxed) <=
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
         context->detachMbContext();
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
      SDB_ASSERT(nullptr != _collectionSpace, "can not be null");
      rdpAccessor accessor;
      logicalPageBuffer lpb;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      SDB_ASSERT(nullptr != _collectionSpace, "can not be null");
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
            if (DMS_SCAN_FOR_NONE != o.so.scanFor)
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

   INT32 collection::getMoreWhenIndexScan(indexScanContext *context)
   {
      INT32 rc = SDB_OK;
      indexObject *obj = nullptr;
      indexIdentifier indexId;
      ossRWMutexGuard guard(&_indexlock, SHARED, FALSE);
      runtimeMbContext mbContext;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isCursorAttached() ||
                            !context->getCursor()->getIndexId().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);
      indexId = context->getCursor()->getIndexId();

      obj = _indexes.find(indexId, INDEX_STATUS_NORMAL);
      if (nullptr == obj)
      {
         PD_LOG(PDERROR, "index[%d] not found", indexId.getIndexSlot());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
      else if (obj->getIndexId().getLogicalIndexId() != 
               indexId.getLogicalIndexId())
      {
         PD_LOG(PDERROR, "index id[%d,%d] not found",
                indexId.getIndexSlot(),
                indexId.getLogicalIndexId());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }

      rc = _getMoreWhenIndexScan(context, obj);
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
         context->detachMbContext();
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
      SDB_ASSERT(nullptr != context && context->isMbContextAttached(),
                 "can not be invalid");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      SDB_ASSERT(candidate.isValid() && INVALID_PAGE_ID != candidate.getLpid(),
                 "can not be invalid");

      logicalPageBuffer lpb;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      const runtimeMbContext *mbContext = context->getMbContext();

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

      if (accessor.getFreeSpacePercent() < mbContext->getFloatMinFreePercent())
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
      SDB_ASSERT(nullptr != context && context->isMbContextAttached(),
                 "can not be invalid");
      SDB_ASSERT(newRowData.isValid(), "can not be invalid");
      SDB_ASSERT(candidate.isValid() && INVALID_PAGE_ID != candidate.getLpid(),
                 "can not be invalid");

      logicalPageBuffer lpb;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      const runtimeMbContext *mbContext = context->getMbContext();

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

      if (accessor.getFreeSpacePercent() < mbContext->getFloatMinFreePercent())
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      const runtimeMbContext *mbContext = context->getMbContext();

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

      if (accessor.getFreeSpacePercent() < mbContext->getFloatMinFreePercent())
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      rdpAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
      INT32 lvl = FSM_INVALID_SPACE_LVL;
      const runtimeMbContext *mbContext = context->getMbContext();

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
      else if (accessor.getFreeSpacePercent() >= mbContext->getFloatMinFreePercent())
      {
         _fsm.upgradePageSpaceLvl(candidate.getSeq(), lvl);
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
         UINT32 totalRdpCount = _rdpCount.load(std::memory_order_relaxed);
         /// do not get latch here.
         ossXLatchGuard guard(&_extendingLatch, FALSE);

         rc = _fsm.find(context, lvl, striping, candidate);
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
         if (totalRdpCount < _rdpCount.load(std::memory_order_relaxed))
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

         rc = _fsm.insertNewPages(firstSeq, lpids, PAGE_COUNT_IN_EXTENT);
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
         UINT32 totalRdpCount = _rdpCount.load(std::memory_order_relaxed);
         /// do not get latch here.
         ossXLatchGuard guard(&_extendingLatch, FALSE);

         rc = _fsm.findAndKick(lvl, candidate);
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
         if (totalRdpCount < _rdpCount.load(std::memory_order_relaxed))
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

         rc = _fsm.insertNewPages(firstSeq, lpids, PAGE_COUNT_IN_EXTENT);
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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

      totalRdpCount = _rdpCount.load(std::memory_order_relaxed);
      totalLvl0Count = _lvl0Count.load(std::memory_order_relaxed);
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
      
      initer.init(_clMetaBlock.logicalCLID, totalRdpCount, count);
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
      _rdpCount.fetch_add(count, std::memory_order_relaxed);

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
      SDB_ASSERT(nullptr != _collectionSpace, "can not be null");
      SDB_ASSERT(_clMetaBlock.isValid(), "must be valid");
   

      if (INVALID_PAGE_ID != _clMetaBlock.routePages[COLLECTION_ROOT_LVL2])
      {
         rc = initPageSequenceByRootLvL2(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by lvl2 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _clMetaBlock.routePages[COLLECTION_SECOND_ROOT_LVL1])
      {
         rc = initPageSequenceByRootLvL1(context, 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by second lvl1 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _clMetaBlock.routePages[COLLECTION_FIRST_ROOT_LVL1])
      {
         rc = initPageSequenceByRootLvL1(context, 0);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by first lvl1 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _clMetaBlock.routePages[COLLECTION_ROOT_LVL0])
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
         _lvl0Count.store(0, std::memory_order_relaxed);
         _rdpCount.store(0, std::memory_order_relaxed);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL2(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_clMetaBlock.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != _clMetaBlock.routePages[COLLECTION_ROOT_LVL2],
                 "can not be invalid");

      UINT32 lvl1Count = 0;
      PAGE_ID lastLvl1 = INVALID_PAGE_ID;
      UINT32 lvl0Count = 0;
      PAGE_ID lastLvl0 = INVALID_PAGE_ID;
      UINT32 rdpCount = 0;
      PAGE_ID lastRdp = INVALID_PAGE_ID;

      UINT32 totalLvl0Count = 0;
      UINT32 totalRdpCount = 0;

      UINT32 pageSize = _collectionSpace->getSU()->getManifest().dataArgs.pageSize;
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
                                         _clMetaBlock.routePages[COLLECTION_ROOT_LVL2],
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

      _lvl0Count.store(totalLvl0Count, std::memory_order_relaxed);
      _rdpCount.store(totalRdpCount, std::memory_order_relaxed);
   done:
      return rc;
   error:
      _lvl0Count.store(0, std::memory_order_relaxed);
      _rdpCount.store(0, std::memory_order_relaxed);
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL1(requestContext *context,
                                                UINT32 rootNo)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_clMetaBlock.isValid(), "can not be invalid");
      SDB_ASSERT(rootNo <= 1, "can not out of bound");
      PAGE_ID lpid = _clMetaBlock.routePages[COLLECTION_FIRST_ROOT_LVL1 + rootNo];
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");

      UINT32 lvl0Count = 0;
      PAGE_ID lastLvl0 = INVALID_PAGE_ID;
      UINT32 rdpCount = 0;
      PAGE_ID lastRdp = INVALID_PAGE_ID;

      UINT32 totalLvl0Count = 0;
      UINT32 totalRdpCount = 0;

      UINT32 pageSize = _collectionSpace->getSU()->getManifest().dataArgs.pageSize;
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

      _lvl0Count.store(totalLvl0Count, std::memory_order_relaxed);
      _rdpCount.store(totalRdpCount, std::memory_order_relaxed);
   done:
      return rc;
   error:
      _lvl0Count.store(0, std::memory_order_relaxed);
      _rdpCount.store(0, std::memory_order_relaxed);
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL0(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_clMetaBlock.isValid(), "can not be invalid");
      PAGE_ID lpid = _clMetaBlock.routePages[COLLECTION_ROOT_LVL0];
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

      _lvl0Count.store(1, std::memory_order_relaxed);
      _rdpCount.store(rdpCount, std::memory_order_relaxed);
   done:
      return rc;
   error:
      _lvl0Count.store(0, std::memory_order_relaxed);
      _rdpCount.store(0, std::memory_order_relaxed);
      goto done;
   }

   INT32 collection::getLpidBySequence(requestContext *context,
                                       UINT32 sequence,
                                       PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _collectionSpace, "can not be null");

      UINT32 capacity = 0;
      UINT32 lvl0Id = 0;
      PAGE_ID lvl0Lpid = INVALID_PAGE_ID;
      UINT32 totalRdpCount = _rdpCount.load(std::memory_order_relaxed);

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
      SDB_ASSERT(nullptr != _collectionSpace, "can not be null");

      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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

      if (_lvl0Count.load(std::memory_order_relaxed) <= lvl0No)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      if (0 == lvl0No)
      {
         lpid = _clMetaBlock.routePages[COLLECTION_ROOT_LVL0];
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
         lvl1Lpid = _clMetaBlock.routePages[COLLECTION_FIRST_ROOT_LVL1];
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
         lvl1Lpid = _clMetaBlock.routePages[COLLECTION_SECOND_ROOT_LVL1];
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
         PAGE_ID lvl2Lpid = _clMetaBlock.routePages[COLLECTION_ROOT_LVL2];
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
      SDB_ASSERT(nullptr != _collectionSpace, "can not be null");
      routePageAccessor accessor;
      logicalPageBuffer lpb;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      UINT32 totalLvl0Count = _lvl0Count.load(std::memory_order_relaxed);
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
         SDB_ASSERT(INVALID_PAGE_ID == _clMetaBlock.routePages[COLLECTION_ROOT_LVL0], "impossible");
         rc = ensureRootRoutePage(context, COLLECTION_ROOT_LVL0);
         if (SDB_OK != rc)
         {
            goto error;
         }

         _lvl0Count.fetch_add(1, std::memory_order_relaxed);
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
         lvl1Lpid = _clMetaBlock.routePages[COLLECTION_FIRST_ROOT_LVL1];
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
         lvl1Lpid = _clMetaBlock.routePages[COLLECTION_SECOND_ROOT_LVL1];
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

      _lvl0Count.fetch_add(1, std::memory_order_relaxed);
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

      if (COLLECTION_FIRST_ROOT_LVL1 == rootSlot ||
          COLLECTION_SECOND_ROOT_LVL1 == rootSlot)
      {
         rootLvl = COLLECTION_ROUTE_PAGE_LVL1;
      }
      else if (COLLECTION_ROOT_LVL2 == rootSlot)
      {
         rootLvl = COLLECTION_ROUTE_PAGE_LVL2;
      }

      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();

      if (INVALID_PAGE_ID != _clMetaBlock.routePages[rootSlot])
      {
         goto done;
      }

      clmbpLpid = getMbpLpidOfCollection(getDataPageSize(), _clMetaBlock.mbID);
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
      _clMetaBlock.routePages[rootSlot] = routeLpid;
      oplist.setWaitingTail();
      rc = accessor.updateRoutePages(context, _clMetaBlock, &lpb);
      if (SDB_OK != rc)
      {
         _clMetaBlock.routePages[rootSlot] = INVALID_PAGE_ID;
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      SDB_ASSERT(INVALID_PAGE_ID != _clMetaBlock.routePages[COLLECTION_ROOT_LVL2], "impossible");
      lpid = INVALID_PAGE_ID;
      routePageAccessor accessor;
      logicalPageBuffer lpb;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      UINT32 targetCount = 0;
      UINT32 minCount = 0;
      UINT32 currentLvl1Count = 0;
      PAGE_ID lvl1 = INVALID_PAGE_ID;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      UINT32 totalLvl0Count = _lvl0Count.load(std::memory_order_relaxed);

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
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
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
                                    _clMetaBlock.routePages[COLLECTION_ROOT_LVL2],
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
                                  _clMetaBlock.routePages[COLLECTION_ROOT_LVL2],
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
      SDB_ASSERT(nullptr != _collectionSpace, "can not be null");
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      routePageIniter initer;
      initer.setLogicalId(_clMetaBlock.logicalCLID);
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
      return _collectionSpace->getSU()->getManifest().dataArgs.pageSize;
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

   INT32 collection::_createIndex(requestContext *context,
                                  const indexDescription &desc,
                                  indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");   
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(desc.isValid(), "can not be invalid");

      INT32 indexSlot = -1;
      slice objSlice;
      bson::BSONObj obj;
      bson::BSONObjBuilder builder;
      ossPoolString fullName;
      strSlice nameSlice;
      UINT32 indexLid = INVALID_LOGICAL_INDEX_ID;
      BOOLEAN duplicated = FALSE;
      indexConsole console;

      BOOLEAN rollbackLog = FALSE;
      BOOLEAN rollbackDefPage = FALSE;
      PAGE_ID lpid = INVALID_PAGE_ID;

      indexId.reset();
      ossScopedRWLock guard(&_indexlock, EXCLUSIVE);

      console.init(_clMetaBlock.mbID, &(_collectionSpace->getSU()->getIndexSpace()));

      if (!_indexes.isMetaBlockEverCreated())
      {
         rc = console.initIndexMetaBlock(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init index meta block, rc:%d", rc);
            goto error;
         }
      }

      if (!_indexes.isAllowedToCreateMore())
      {
         PD_LOG(PDINFO, "no more available logical index id or slot");
         rc = SDB_DMS_MAX_INDEX;
         goto error;
      }

      rc = testIfIndexDuplicated(context, desc.getNameSlice(), desc.getPattern(), duplicated);
      if (SDB_OK != rc)
      { 
         PD_LOG(PDERROR, "failed to test if index duplicated:%d", rc);
         goto error;
      }

      if (duplicated)
      {
         rc = SDB_IXM_EXIST;
         goto error;
      }

      desc.exportToBson(builder);
      obj = builder.obj();
      if ((INT32)MAX_INDEX_DEF_OBJ_SIZE < obj.objsize())
      {
         PD_LOG(PDERROR, "index def obj size over max size:%d", obj.objsize());
         rc = SDB_INVALIDARG;
         goto error;
      }

      objSlice.reset(obj.objsize(), obj.objdata());
      fullName.reserve(128);
      fullName.append(_collectionSpace->getCSName()).append(".").append(getName());
      nameSlice.reset(fullName.c_str(), fullName.size());

      indexSlot = _indexes.findFreeIndexSlot();
      indexLid = _indexes.getNextIndexLid();
      indexId.reset(indexSlot, indexLid, desc.getInnerID());
      SDB_ASSERT(indexId.isValid(), "impossible");

      rc = commitCreateIndexLog(context, nameSlice,
                                indexLid, indexSlot, objSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit create index log:%d", rc);
         goto error;
      }
      rollbackLog = TRUE;

      rc = console.createIndex(context, indexSlot, indexLid, objSlice, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index on index space:%d", rc);
         goto error;
      }
      rollbackDefPage = TRUE;

      rc = _indexes.insert(indexSlot, indexLid, lpid, desc, INDEX_STATUS_BUILDING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to register unstable index[%s], rc:%d",
                desc.getName().c_str(), rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      /// rollback log first, we need a new lsn.
      if (rollbackLog)
      {
         SDB_ASSERT(SDB_OK != rc, "impossible");
         INT32 tmpRc = commitCreateIndexEndLog(context, nameSlice, 
                                               desc.getNameSlice(),
                                               indexLid, indexSlot, rc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to commit creating end log, rc:%d", tmpRc);
            ossPanic();
         }
      }
      if (rollbackDefPage)
      {
         INT32 tmpRc = console.releaseIndexEntryInBlock(context, indexSlot);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to rollback index[%d] def page:%d",
                   indexSlot, tmpRc);
            ossPanic();
         }
      }
      
      indexId.reset();
      goto done;
   }

   INT32 collection::markIndexRemovingByName(requestContext *context,
                                             const strSlice &indexName,
                                             indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      indexId.reset();
      indexObject *obj = nullptr;
      indexConsole console;
      
      ossRWMutexGuard guard(&_indexlock, EXCLUSIVE);
      indexObjectMap::ITERATOR itr = _indexes.begin();
      for (; itr != _indexes.end(); ++itr)
      {
         strSlice nameSlice = itr->second->getDescription().getNameSlice();
         if (nameSlice == indexName)
         {
            obj = itr->second;
            break;
         }
      }

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
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      
      console.init(_clMetaBlock.mbID, &(_collectionSpace->getSU()->getIndexSpace()));
      rc = console.updateIndexStatus(context, obj->getIndexId().getLogicalIndexId(),
                                     obj->getEntryLpid(),
                                     INDEX_STATUS_REMOVING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index statu to removing:%d", rc);
         goto error;
      }

      obj->updateStatus(INDEX_STATUS_REMOVING);
      indexId = obj->getIndexId();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::releaseIndexObjectAndEntryPage(requestContext *context,
                                                    const indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      indexObject *obj = nullptr;
      indexConsole console;
      console.init(_clMetaBlock.mbID, &(_collectionSpace->getSU()->getIndexSpace()));
      ossRWMutexGuard guard(&_indexlock, EXCLUSIVE);

      /// ensure index object firsts
      obj = _indexes.find(indexId);
      if (nullptr == obj)
      {
         PD_LOG(PDERROR, "failed to find index object[%d]", indexId.getIndexSlot());
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_KEY_NOT_FOUND;
         goto error;
      }
      else if (!obj->isRemoving())
      {
         PD_LOG(PDERROR, "can not release index[%d] with status[%d]",
                indexId.getIndexSlot(), obj->getStatus());
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      obj = nullptr;
      _indexes.erase(indexId.getIndexSlot());

      /// release index entry page
      /// TODO: we should ensure releasing always be ok here.
      /// we may reserve physical page first.
      rc = console.releaseIndexEntryInBlock(context, indexId.getIndexSlot());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release index[%d] def page:%d", indexId.getIndexSlot(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::buildIndexInContext(requestContext *context,
                                         const indexIdentifier &indexId,
                                         INDEX_TYPE type,
                                         const dmsBuildIndexOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_INDEX_TYPE != type, "can not be invalid");

      static const UINT64 _MAX_SORT_BUF_SIZE = 1024 * 1024 * 1024;
      static const UINT64 _DEFAULT_SORT_BUF_SIZE = 64 * 1024 * 1024;

      if (!o.blockDML)
      {
         if (INDEX_TYPE_LSM == type ||
             o.isSortingDisabled())
         {
            rc = onlineBuildIndex(context, indexId);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to online build index[%d], rc:%d", 
                      indexId.getIndexSlot(), rc);
               goto error;
            }
         }
         else
         {
            UINT64 sortBufSize = (UINT64)(o.sortBufferSize) * 1024 * 1024;
            if (_MAX_SORT_BUF_SIZE < sortBufSize)
            {
               sortBufSize = _DEFAULT_SORT_BUF_SIZE;
            }

            rc = onlineBuildIndexBySorting(context, indexId, sortBufSize);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to online build index[%d] by sorting, rc:%d",
                      indexId.getIndexSlot(), rc);
               goto error;
            }
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

   INT32 collection::buildIndexBySortingAndUpdateContext(requestContext *context,
                                                         indexObject *obj,
                                                         UINT32 maxRdpCount,
                                                         memoryBlock &sortBuffer)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "must be building");
      SDB_ASSERT(0 < sortBuffer.getCapacity(), "can not be zero");

      btreeRebuildingSortElement::comparer cmp;
      BTREE_SORTOR sortor;
      scanEntry entry;
      orderingWrapper ow = obj->getDescription().getPattern().getOrdering();
      buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext*>(obj->getUnstatbleContext());
      if (OSS_UNLIKELY(nullptr == buildingContext))
      {
         PD_LOG(PDERROR, "failed to get building context ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!buildingContext->getNextBuildingBound(entry))
      {
         PD_LOG(PDERROR, "last batch may not completed yet");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      if (maxRdpCount <= entry.getSeq())
      {
         goto done;
      }

      cmp.ow = ow;
      rc = sortor.init(&cmp, sortBuffer.getCapacity(), &sortBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init sortor:%d", rc);
         goto error;
      }

      rc = fillSorterAndUpdateEntry(context, obj, &sortor, maxRdpCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fill sorter:%d", rc);
         goto error;
      }

      sortor.sort();

      rc = mergeSorterAndContextIntoIndex(context, obj, &sortor);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to merge data into index:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::buildIndexAndUpdateContext(requestContext *context,
                                                indexObject *obj,
                                                UINT32 maxRdpCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "can not be null");
      buildingIndexContext *buildingContext = nullptr;
      scanEntry entry;
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      SDB_ASSERT((!(!keyGen)), "can not be invalid");
      memoryBlock mb;
      indexConsole console;
      bson::BSONObjSet keySet;

      buildingContext = dynamic_cast<buildingIndexContext *>(obj->getUnstatbleContext());
      if (OSS_UNLIKELY(nullptr == buildingContext))
      {
         PD_LOG(PDERROR, "failed to get building context ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!buildingContext->getNextBuildingBound(entry))
      {
         PD_LOG(PDERROR, "failed to get next scan range");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      console.init(_clMetaBlock.mbID, &is);

      while (entry.getSeq() < maxRdpCount)
      {
         rdpRecordScanner scanner;
         PAGE_ID lpid = INVALID_PAGE_ID;
         rc = getLpidBySequence(context, entry.getSeq(), lpid);
         if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS == rc)
         {
            PD_LOG(PDDEBUG, "page seq[%d] does not exist", entry.getSeq());
            entry.incSeqAndZeroSlot();
            buildingContext->updateBuildingHighBound(entry);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid of seq[%d], rc:%d", entry.getSeq(), rc);
            goto error;
         }

         rc = scanner.open(context, lpid, entry.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init scanner on lpid[%d]:%d", lpid, rc);
            goto error;
         }
         
         while (scanner.isReadyToRead())
         {
            keySet.clear();
            slice record;

            record = scanner.getCurrentRecord();

            rc = keyGen(obj->getDescription().getPattern().getPattern(), 
                        obj->getDescription().isNotArray(),
                        record,
                        keySet);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to generate index key:%d", rc);
               goto error;
            }

            for (bson::BSONObjSet::const_iterator itr = keySet.begin();
                 itr != keySet.end(); ++itr)
            {
               rc = console.insert(context, obj,
                                   ixmKeyOwned(*itr),
                                   scanner.getCurrentRid(),
                                   scanner.getCurrentTransID());
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert key into index[%s], rc:%d",
                         obj->getDescription().getName().c_str(), rc);
                  goto error;
               }
            }

            rc = scanner.next();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to move to next visible pos:%d", rc);
               goto error;
            }
         }

         entry.incSeqAndZeroSlot();
         /// update context with page latch
         buildingContext->updateBuildingHighBound(entry);
         scanner.close();
      }

      rc = endToBuildCurrentRange(context, buildingContext);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to end to build current range:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::fillSorterAndUpdateEntry(requestContext *context,
                                              indexObject *obj,
                                              BTREE_SORTOR *sorter,
                                              UINT32 maxRdpCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "must be building");
      SDB_ASSERT(nullptr != sorter && sorter->isValid(), "can not be invalid");
      memoryBlock mb;
      scanEntry entry;
      bson::BSONObjSet keySet;
      BTREE_SORTOR::batch batch;
      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext*>(obj->getUnstatbleContext());
      if (OSS_UNLIKELY(nullptr == buildingContext))
      {
         PD_LOG(PDERROR, "failed to get building context ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!buildingContext->getNextBuildingBound(entry))
      {
         PD_LOG(PDERROR, "failed to get next scan range");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      while (entry.getSeq() < maxRdpCount)
      {
         rdpRecordScanner scanner;
         PAGE_ID lpid = INVALID_PAGE_ID;
         rc = getLpidBySequence(context, entry.getSeq(), lpid);
         if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS == rc)
         {
            PD_LOG(PDDEBUG, "page seq[%d] does not exist", entry.getSeq());
            entry.incSeqAndZeroSlot();
            buildingContext->updateBuildingHighBound(entry);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid of seq[%d], rc:%d", entry.getSeq(), rc);
            goto error;
         }

         rc = scanner.open(context, lpid, entry.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open scanner:%d", rc);
            goto error;
         }

         while (scanner.isReadyToRead())
         {
            keySet.clear();
            batch.reset();
            recordID rid = scanner.getCurrentRid();
            rc = keyGen(obj->getDescription().getPattern().getPattern(), 
                        obj->getDescription().isNotArray(),
                        scanner.getCurrentRecord(),
                        keySet);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to generate index key:%d", rc);
               goto error;
            }

            for (bson::BSONObjSet::const_iterator itr = keySet.begin();
                 itr != keySet.end(); ++itr)
            {
               btreeRebuildingSortElement se;
               se.set(ixmKeyOwned(*itr), rid, scanner.getCurrentTransID());
               batch.pushFragments({se.getKeySlice(), se.getRidSlice(), 
                                    se.getTransIDSlice()});
            }
            
            if (!sorter->push(batch))
            {
               entry.reset(entry.getSeq(), rid.getPos());
               buildingContext->updateBuildingHighBound(entry);
               scanner.close();
               goto done;
            }
            else
            {
               rc = scanner.next();
               if (SDB_OK != rc)
               {
                  scanner.close();
                  PD_LOG(PDERROR, "failed to move to next pos:%d", rc);
                  goto error;
               }
            }
         }

         entry.incSeqAndZeroSlot();
         /// update context with page latch
         buildingContext->updateBuildingHighBound(entry);
         scanner.close();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::onlineBuildIndex(requestContext *context,
                                      const indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      static const UINT32 _SCAN_PAGE_COUNT_PER_LOOP = 4;

      do
      {
         scanEntry buildEntry;
         UINT32 currentRdpCount = 0;
         indexObject *obj = nullptr;
         buildingIndexContext *buildingContext = nullptr;

         ossRWMutexGuard guard(&_indexlock, SHARED);

         obj = _indexes.find(indexId, INDEX_STATUS_BUILDING);
         if (nullptr == obj)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexId.getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(obj->getUnstatbleContext());
         if (OSS_UNLIKELY(nullptr == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (!buildingContext->getNextBuildingBound(buildEntry))
         {
            PD_LOG(PDERROR, "failed to get next building range");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         currentRdpCount = _rdpCount.load(std::memory_order_relaxed);

         if (currentRdpCount <= (buildEntry.getSeq() + _SCAN_PAGE_COUNT_PER_LOOP))
         {
            break;
         }

         rc = buildIndexAndUpdateContext(context, obj, currentRdpCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build index[%s] and update context:%d",
                   obj->getDescription().getName().c_str(), rc);
            goto error;
         }

      } while (TRUE);

      {
         ossRWMutexGuard guard(&_indexlock, EXCLUSIVE);
         scanEntry buildEntry;
         buildingIndexContext *buildingContext = nullptr;
         UINT32 rdpCount = _rdpCount.load(std::memory_order_relaxed);
         indexObject *obj = _indexes.find(indexId, INDEX_STATUS_BUILDING);
         if (nullptr == obj)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexId.getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(obj->getUnstatbleContext());
         if (OSS_UNLIKELY(nullptr == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         
         if (!buildingContext->getNextBuildingBound(buildEntry))
         {
            PD_LOG(PDERROR, "failed to get next building range");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (buildEntry.getSeq() < rdpCount)
         {
            rc = buildIndexAndUpdateContext(context, obj, rdpCount);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build index[%s] and update context:%d",
                     obj->getDescription().getName().c_str(), rc);
               goto error;
            }
         }

         rc = indexBuildDone(context, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to finish building index[%s], rc:%d",
                   obj->getDescription().getName().c_str(), rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::onlineBuildIndexBySorting(requestContext *context,
                                               const indexIdentifier &indexId,
                                               UINT64 sortBufferSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      SDB_ASSERT(0 < sortBufferSize, "can not be zero");

      static const UINT32 _ENDING_LOOP_RDP_COUNT = 2;
      memoryBlock mb;
      scanEntry entry;

      rc = mb.reserve(sortBufferSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve memory size:%d", rc);
         goto error;
      }

      do
      {
         ossRWMutexGuard guard(&_indexlock, SHARED);
         UINT32 currentRdpCount = 0;
         buildingIndexContext *buildingContext = nullptr;
         indexObject *obj = _indexes.find(indexId, INDEX_STATUS_BUILDING);
         if (nullptr == obj)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexId.getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(obj->getUnstatbleContext());
         if (OSS_UNLIKELY(nullptr == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (!buildingContext->getNextBuildingBound(entry))
         {
            PD_LOG(PDERROR, "failed to get next building range");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         currentRdpCount = _rdpCount.load(std::memory_order_relaxed);;

         if (currentRdpCount <= (entry.getSeq() + _ENDING_LOOP_RDP_COUNT))
         {
            break;
         }
         
         rc = buildIndexBySortingAndUpdateContext(context, obj, currentRdpCount, mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build index[%d], rc:%d", 
                   indexId.getIndexSlot(), rc);
            goto error;
         }
      } while (TRUE);

      {
         ossRWMutexGuard guard(&_indexlock, EXCLUSIVE);
         buildingIndexContext *buildingContext = nullptr;
         UINT32 rdpCount = _rdpCount.load(std::memory_order_relaxed);
         indexObject *obj = _indexes.find(indexId, INDEX_STATUS_BUILDING);
         if (nullptr == obj)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexId.getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(obj->getUnstatbleContext());
         if (OSS_UNLIKELY(nullptr == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         do
         {
            if (!buildingContext->getNextBuildingBound(entry))
            {
               PD_LOG(PDERROR, "failed to get next building range");
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
            else if (entry.getSeq() == rdpCount)
            {
               break;
            }

            rc = buildIndexBySortingAndUpdateContext(context, obj, rdpCount, mb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build index[%d], rc:%d", 
                      indexId.getIndexSlot(), rc);
               goto error;
            }
         } while(TRUE);

         rc = indexBuildDone(context, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to finish building index[%s], rc:%d",
                   obj->getDescription().getName().c_str(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::endToBuildCurrentRange(requestContext *context,
                                            buildingIndexContext *buildingContext)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != buildingContext, "can not be null");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexMergingRecordList mrl;
      indexConsole console;
      console.init(_clMetaBlock.mbID, &is);

      while (!buildingContext->endToBuildCurrentRange(mrl))
      {
         SDB_ASSERT(FALSE, "TODO");
      }

      return rc;
   }


   INT32 collection::mergeSorterAndContextIntoIndex(requestContext *context,
                                                    indexObject *obj,
                                                    BTREE_SORTOR *sorter)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != sorter, "can not be null");
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "must be building");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      console.init(_clMetaBlock.mbID, &is);
      buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext *>
                                              (obj->getUnstatbleContext());
      if (OSS_UNLIKELY(nullptr == buildingContext))
      {
         PD_LOG(PDERROR, "failed to get building context ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT64 i = 0; i < sorter->getCount(); ++i)
      {
         btreeRebuildingSortElement se;
         sorter->get(i, se);
         rc = console.insert(context, obj, se.getKey(), se.getRid(), se.getTransID());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert key into index[%s]:%d",
                   obj->getDescription().getName().c_str(), rc);
            goto error;
         }
      }

      rc = endToBuildCurrentRange(context, buildingContext);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to end to build current range:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      
      goto done;
   }
   

   INT32 collection::indexBuildDone(requestContext *context,
                                    indexObject *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "must be building");

      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      ossPoolString fullName;
      strSlice nameSlice;


      fullName.reserve(128);
      fullName.append(_collectionSpace->getCSName()).append(".").append(getName());
      nameSlice.reset(fullName.c_str(), fullName.size());

      console.init(_clMetaBlock.mbID, &is);

      rc = commitCreateIndexEndLog(context, nameSlice,
                                   obj->getDescription().getNameSlice(),
                                   obj->getIndexId().getLogicalIndexId(),
                                   obj->getIndexId().getIndexSlot(),
                                   SDB_OK);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit index creating end log:%d", rc);
         ossPanic();
         goto error;
      }

      rc = console.updateIndexStatus(context, obj->getIndexId().getLogicalIndexId(),
                                     obj->getEntryLpid(),
                                     INDEX_STATUS_NORMAL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to update index[%s] status to normal:%d",
                obj->getDescription().getName().c_str(), rc);
         ossPanic();
         goto error;
      }

      obj->removeUnstableContext();
      obj->updateStatus(INDEX_STATUS_NORMAL);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::testIfIndexDuplicated(requestContext *context,
                                           const strSlice &indexName,
                                           const indexKeyPattern &pattern,
                                           BOOLEAN &duplicated)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(pattern.isValid(), "can not be invalid");

      duplicated = FALSE;
      for (indexObjectMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         if (!itr->second->isNormal() && !itr->second->isBuilding())
         {
            continue;
         }

         if (itr->second->getDescription().getNameSlice() == indexName)
         {
            PD_LOG(PDINFO, "duplidated index name[%s]", indexName.str());
            duplicated = TRUE;
            goto done;
         }
         if (pattern.isCoveredBy(itr->second->getDescription().getPattern()))
         {
            PD_LOG(PDINFO, "duplidated index pattern[%s]",
                   itr->second->getDescription().getName().c_str());
            duplicated = TRUE;
            goto done;
         }
      }

   done:
      return rc;
   }

   INT32 collection::initIndexesWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;

      console.init(_clMetaBlock.mbID, &is);

      rc = console.loadIndexesWhenStartup(context, &_indexes);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index context:%d", rc);
         goto error;
      }

      rc = fixUnstatbleIndexesWhenOpen(context);
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

   INT32 collection::fixUnstatbleIndexesWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      indexObjectMap::CONST_ITERATOR itr = _indexes.begin();
      for (; itr != _indexes.end(); ++itr)
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

      for (indexObjectMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         keySet.clear();
         indexObject *obj = itr->second;
         SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");
         if (!obj->isNormal() && !obj->isBuilding())
         {
            continue;
         }

         rc = keyGen(obj->getDescription().getPattern().getPattern(),
                     obj->getDescription().isNotArray(),
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

      indexObjectMap::CONST_ITERATOR itr = _indexes.begin();
      for (; itr != _indexes.end(); ++itr)
      {
         BOOLEAN associated = FALSE;
         indexObject *obj = itr->second;
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

         rc = keyGen(obj->getDescription().getPattern().getPattern(),
                     obj->getDescription().isNotArray(),
                     oldRecord, keySetToRemove);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to generate index keys:%d", rc);
            goto error;
         }

         rc = keyGen(obj->getDescription().getPattern().getPattern(),
                     obj->getDescription().isNotArray(),
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

      indexObjectMap::CONST_ITERATOR itr = _indexes.begin();
      for (; itr != _indexes.end(); ++itr)
      {
         indexObject *obj = itr->second;
         SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");
         if (!obj->isNormal() && !obj->isBuilding())
         {
            continue;
         }

         rc = keyGen(obj->getDescription().getPattern().getPattern(),
                     obj->getDescription().isNotArray(),
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
      recordID rid;
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
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

      console.init(_clMetaBlock.mbID, &is);

      for (UINT32 i = 0; i < ra.getSize(); ++i)
      {
         
         const dmlIndexRequest *req = ra.get(i);
         SDB_ASSERT(nullptr != req && req->isValid(), "impossible");
         if (!req->withConstraint())
         {
            continue;
         }

         ossPoolList<bson::BSONObj>::const_iterator itr = req->getKeysToInsert().begin();
         for (; itr != req->getKeysToInsert().end(); ++itr)
         {
            rc = console.checkUniqueConstraint(context, req->getObject(), *itr, rid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find key in index:%d", rc);
               goto error;
            }

            if (rid.isValid())
            {
               PD_LOG(PDDEBUG, "duplidated key[%s] found in index[%s], rid[%d,%d]",
                      itr->toString().c_str(),
                      req->getObject()->getDescription().getName().c_str(),
                      rid.getPid(), rid.getPos());
               duplicated = TRUE;
               if (nullptr != res)
               {
                  res->incDuplicatedNum();
                  if (res->isEnaleIndexErrInfo())
                  {
                     indexObject *obj = req->getObject();
                     res->setIndexErrInfo(obj->getDescription().getName().c_str(),
                                          obj->getDescription().getPattern().getPattern(),
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

   INT32 collection::insertIndexRequests(dmlContext *context,
                                         const dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      console.init(_clMetaBlock.mbID, &is);

      rc = console.handleDmlRequest(context, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert indexes:%d", rc);
         goto error;
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
      for (UINT32 i = 0; i < ra.getSize(); ++i)
      {
         dmlIndexRequest *ir = ra.get(i);
         SDB_ASSERT(nullptr != ir && ir->isValid(), "impossible");
         if (!ir->getObject()->isBuilding())
         {
            continue;
         }

         unstableIndexContext *uic = ir->getObject()->getUnstatbleContext();
         buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext*>(uic);
         if (OSS_UNLIKELY(nullptr == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = buildingContext->merge(context, ir);

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

   INT32 collection::rollbackCreatingIndex(requestContext *context,
                                           const indexIdentifier &indexId,
                                           INT32 reason)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(SDB_OK != reason, "can not be ok");

      rc = markIndexRemovingBySlot(context, indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove index[%d], rc:%d", 
                indexId.getIndexSlot(), rc);
         goto error;
      }

      rc = truncateIndex(context, indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index[%d], rc:%d", 
                indexId.getIndexSlot(), rc);
         goto error;
      }

      releaseIndexObjectAndEntryPage(context, indexId);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::markIndexRemovingBySlot(requestContext *context,
                                             const indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      ossRWMutexGuard guard(&_indexlock, EXCLUSIVE);
      console.init(_clMetaBlock.mbID, &is);
      indexObject *obj = _indexes.find(indexId);
      if (nullptr == obj)
      {
         PD_LOG(PDERROR, "index[%d] not found", indexId.getIndexSlot());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
      else if (!obj->isNormal() && !obj->isBuilding())
      {
         PD_LOG(PDERROR, "can not remove index[%d] with status[%d]",
                indexId.getIndexSlot(), obj->getStatus());
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = console.updateIndexStatus(context, 
                                     obj->getIndexId().getLogicalIndexId(),
                                     obj->getEntryLpid(),
                                     INDEX_STATUS_REMOVING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index status:%d", rc);
         goto error;
      }

      obj->updateStatus(INDEX_STATUS_REMOVING);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::truncateIndex(requestContext *context,
                                   const indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      ossRWMutexGuard guard(&_indexlock, SHARED);
      console.init(_clMetaBlock.mbID, &is);
      indexObject *obj = _indexes.find(indexId);
      if (nullptr == obj)
      {
         PD_LOG(PDERROR, "failed to find index context[%d]", indexId.getIndexSlot());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!obj->isRemoving() || obj->isTruncating())
      {
         PD_LOG(PDERROR, "index[%d] with wrong status[%d]", obj->getStatus());
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = console.truncateIndex(context, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index[%s], rc:%d",
                obj->getDescription().getName().c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeAllIndexes(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      OSS_LATCH_MODE mode;
      SDB_ASSERT(context->isMbLocked(&mode) && EXCLUSIVE == mode, "must be locked");
      SDB_ASSERT(isOpen(), "can not be closed");
      if (_indexes.isMetaBlockEverCreated())
      {
         indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
         indexConsole console;
         console.init(_clMetaBlock.mbID, &is);
         
         indexObjectMap::ITERATOR itr = _indexes.begin();
         for (; itr != _indexes.end(); ++itr)
         {
            indexObject *obj = itr->second;
            SDB_ASSERT(nullptr != obj && obj->isNormal(), "can not be other status");
            obj->updateStatus(INDEX_STATUS_REMOVING);
            rc = console.updateIndexStatus(context, obj->getIndexId().getLogicalIndexId(),
                                          obj->getEntryLpid(), INDEX_STATUS_REMOVING);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update index[%s] status, rc:%d",
                     obj->getDescription().getName().c_str(), rc);
               goto error;
            }
            console.truncateIndex(context, obj);
         }

         console.resetIndexMetaBlock(context);
      }
      _indexes.fini();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::truncateAllIndexes(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      OSS_LATCH_MODE mode;
      SDB_ASSERT(context->isMbLocked(&mode) && EXCLUSIVE == mode, "must be locked");
      SDB_ASSERT(isOpen(), "can not be closed");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      console.init(_clMetaBlock.mbID, &is);

      indexObjectMap::ITERATOR itr = _indexes.begin();
      for (; itr != _indexes.end(); ++itr)
      {
         indexObject *obj = itr->second;
         SDB_ASSERT(nullptr != obj && obj->isNormal(), "can not be other status");
         obj->updateStatus(INDEX_STATUS_TRUNCATING);
         rc = console.updateIndexStatus(context, obj->getIndexId().getLogicalIndexId(),
                                        obj->getEntryLpid(), INDEX_STATUS_TRUNCATING);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update index[%s] status, rc:%d",
                   obj->getDescription().getName().c_str(), rc);
            goto error;
         }
         console.truncateIndex(context, obj);
         rc = console.updateIndexStatus(context, obj->getIndexId().getLogicalIndexId(),
                                        obj->getEntryLpid(), INDEX_STATUS_NORMAL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update index[%s] status, rc:%d",
                   obj->getDescription().getName().c_str(), rc);
            goto error;
         }
         obj->updateStatus(INDEX_STATUS_NORMAL);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_getMoreWhenIndexScan(indexScanContext *context,
                                           indexObject *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context && context->isCursorAttached(), "can not be invalid");
      SDB_ASSERT(context->isMbContextAttached(), "must be attached");
      SDB_ASSERT(nullptr != obj && obj->isNormal(), "must be normal");
      SDB_ASSERT(context->getMbContext()->getRidLatchContext().isEmpty(), "must be empty");

      const dmsIndexScanOptions &o = context->getOptions();
      indexScanCursor *cursor = context->getCursor();
      indexScanner scanner;
      bson::BSONObjBuilder keyObjBuilder;
      UINT32 pushed = 0;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      stackAllocatorRowBatch batch;
      BOOLEAN scanForNone = (DMS_SCAN_FOR_NONE == o.scanFor);
      
      batch.setRowLimit(cursor->getBaseOptions().stepSize);

      rc = scanner.open(context, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index scanner:%d", rc);
         goto error;
      }

      rc = scanner.batchNext(batch);
      if (SDB_IXM_EOC == rc)
      {
         rc = SDB_OK;
         cursor->setEOC();
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get next rid from scanner:%d", rc);
         goto error;
      }

      scanner.close();

      SDB_ASSERT(!batch.isEmpty(), "impossible");
      for (UINT32 i = 0; i < batch.getRowCount(); ++i)
      {
         dmsRecordID rid;
         DPS_TRANS_ID transID;
         rdpRecordScanner recordScanner;
         slice recordBody;
         indexScanEntry entry;
         rc = entry.init(obj->getDescription().getType(), batch[i]);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to parse index scan entry:%d", rc);
            goto error;
         }

         rid = entry.getRid().toDMSRid();

         if (o.indexCovered)
         {
            transID = entry.getTransID();
            bson::BSONObj recordObj;
            ixmKey key(entry.getKeySlice().data());
            keyObjBuilder.reset();
            rc = key.toRecord(obj->getDescription().getPattern().getPattern(), 
                              keyObjBuilder);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build key obj:%d", rc);
               goto error;
            }

            recordObj = keyObjBuilder.done();
            recordBody = slice(recordObj.objsize(), recordObj.objdata());
         }
         else
         {
            rc = recordScanner.openToRead(context, entry.getRid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to read record[%s], rc:%d",
                      entry.getRid().toString().c_str(), rc);
               goto error;
            }

            SDB_ASSERT(recordScanner.isReadyToRead(), "must be ready to read");
            transID = recordScanner.getCurrentTransID();
            recordBody = recordScanner.getCurrentRecord();
         }

         rc = cursor->pushDataFragments({slice(sizeof(dmsRecordID), &rid),
                                         slice(sizeof(DPS_TRANS_ID), &transID),
                                         recordBody});
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

         recordScanner.close();
         if (scanForNone)
         {
            context->unlockRid(entry.getRid());
         }
         ++pushed;
      }

      if (OSS_UNLIKELY(0 == pushed))
      {
         PD_LOG(PDERROR, "nothing pushed into cursor");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      context->getCursor()->saveEntry(batch[pushed - 1]);

   done:
      scanner.close();
      if (scanForNone)
      {
         context->unlockRids();
      }
      else
      {
         for (UINT32 i = pushed; i < batch.getRowCount(); ++i)
         {
            indexScanEntry entry;
            rc = entry.init(obj->getDescription().getType(), batch[i]);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to parse index scan entry:%d", rc);
               continue;
            }

            context->releaseTransLock(entry.getRid());
         }
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::lockAndFetchRecordToModify(dmlContext *context, const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context && context->isMbContextAttached(), "can not be invalid");
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();

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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();

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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      recordID currentAddr = context->getMrc().getOverflowAddr();
      const runtimeMbContext *mbContext = context->getMbContext();

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

         // dummy upgrade
         if (tmpAccessor.getFreeSpacePercent() >= mbContext->getFloatMinFreePercent())
         {
            INT32 lvl = getFsmSpaceLvl(getDataPageSize(), tmpAccessor.getFreeSpaceAfterLastSlot());
            _fsm.upgradePageSpaceLvl(head->pageSeq, lvl);
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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      UINT32 totalLvl0Count = _lvl0Count.load(std::memory_order_relaxed);


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

      _rdpCount.store(0, std::memory_order_relaxed);

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
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      SDB_ASSERT(INVALID_PAGE_ID == _clMetaBlock.routePages[COLLECTION_ROOT_LVL2],
                "plan to remove lvl2 node");

      for (INT32 i = COLLECTION_SECOND_ROOT_LVL1; i > (INT32)COLLECTION_ROOT_LVL0; --i)
      {
         routePageAccessor accessor;
         PAGE_ID lpid = _clMetaBlock.routePages[i];
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

      if (INVALID_PAGE_ID != _clMetaBlock.routePages[COLLECTION_ROOT_LVL0])
      {
         batch.push_back(_clMetaBlock.routePages[COLLECTION_ROOT_LVL0]);
      }

      commitReleasingPagesLog(context, batch, bson::BSONObj());
      mds.releasePages(context, batch.size(), batch.data());
      _lvl0Count.store(0, std::memory_order_relaxed);

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
      runtimeMbContext mbContext;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _collectionSpace->getSU()->getLobSpace();
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
         context->detachMbContext();
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
      runtimeMbContext mbContext;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _collectionSpace->getSU()->getLobSpace();
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
         context->detachMbContext();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeLobChunk(requestContext *context,
                                    const lobChunkKey &key)
   {
      INT32 rc = SDB_OK;
      runtimeMbContext mbContext;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _collectionSpace->getSU()->getLobSpace();
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
         context->detachMbContext();
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
      runtimeMbContext mbContext;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _collectionSpace->getSU()->getLobSpace();
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
         context->detachMbContext();
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
      runtimeMbContext mbContext;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _collectionSpace->getSU()->getLobSpace();
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
         context->detachMbContext();
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
      runtimeMbContext mbContext;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _collectionSpace->getSU()->getLobSpace();
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
         context->detachMbContext();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::listLobChunks(requestContext *context,
                                   listLobChunkCursor *cursor)
   {
      INT32 rc = SDB_OK;
      runtimeMbContext mbContext;

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

      SDB_ASSERT(!context->isMbContextAttached(), "can not be attached");
      //mbContext.init(_clMetaBlock, _collectionSpace->getIdentifier());
      //context->attachMbContext(&mbContext);

      {
         largeObjectSpace &los = _collectionSpace->getSU()->getLobSpace();
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
      //    context->detachMbContext();
      // }
      return rc;
   error:
      goto done;
   }


}//namespace vessel
}//namespace engine