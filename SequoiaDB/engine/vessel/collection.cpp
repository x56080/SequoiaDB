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
#include "vessel/crpAccessor.h"
#include "vessel/collectionSpace.h"
#include "vessel/lpidLockHelper.h"
#include "vessel/insertContext.h"
#include "vessel/routePage.h"
#include "vessel/routePageAccessor.h"
#include "vessel/scanCLCursor.h"
#include "vessel/rdpScanner.h"
#include "vessel/routePageIniter.h"
#include "vessel/atomicOperationList.h"
#include "vessel/rdpIniter.h"
#include "vessel/rdpInsertExecutor.h"
#include "vessel/indexUtils.h"
#include "vessel/indexEntryPage.h"
#include "vessel/indexConsole.h"
#include "vessel/redoLogUtil.h"
#include "rtnIxmKeySorter.hpp"
#include "vessel/indexKeyGenerator.h"
#include "vessel/outerResource.h"
#include "vessel/recordReader.h"
#include "ixmIndexKey.hpp"
#include "vessel/indexScanner.h"
#include "vessel/buildingIndexContext.h"
#include "vessel/indexScanContext.h"
#include "vessel/indexScanCursor.h"

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
                                  const collectionRecord &record,
                                  collectionSpace *cs)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _collectionSpace, "do not reinit");
      fsmFile *file = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !record.isValid() ||
                       NULL == cs ||
                       !cs->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _collectionSpace = cs;
      _record = record;
      
      rc = initPageSequenceWhenOpen(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page sequence:%d", rc);
         goto error;
      }

      file = cs->getSU()->getMainDataSpace().getFsmFile();
      rc = _fsm.open(record.mbID,
                     record.logicalCLID,
                     _totalRdpCount,
                     file,
                     record.minStriping,
                     record.maxStriping);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open free space map of cl[%d], rc:%d",
                record.mbID, rc);
         goto error;
      }

      rc = initIndexesWhenOpen(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index context:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 collection::create(requestContext *context,
                            const strSlice &clName,
                            utilCLInnerID innerID,
                            UINT32 logicalID,
                            collectionSpace *cs,
                            const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      fsmFile *fsm = NULL;
      SDB_ASSERT(!isOpen(), "do not reinit");

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       clName.empty() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       NULL == cs ||
                       !cs->isOpen() ||
                       !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _collectionSpace = cs;

      _record.reset();
      _record.version = COLLECTION_RECORD_VERSION;
      _record.mbID = context->getMBID();
      _record.innerID = innerID;
      _record.type = options.type;
      _record.logicalCLID = logicalID;
      _record.freeSizeReserved = options.freeSizeReserved;
      _record.minStriping = options.minStriping;
      _record.maxStriping = options.maxStriping;
      ossMemcpy(_record.name, clName.str(), clName.strLen());
      _record.compressionType = options.compressionType;

      fsm = cs->getSU()->getMainDataSpace().getFsmFile();
      rc = _fsm.create(context->getMBID(),
                       logicalID, fsm,
                       options.minStriping,
                       options.maxStriping);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create free space map of mb[%d], rc:%d",
                context->getMBID(), rc);
         goto error;
      }

      rc = saveOnDiskWhenCreating(context, options);
      if (SDB_OK != rc)
      {
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
      _record.reset();
      _collectionSpace = NULL;
      _totalLvl0Count = 0;
      _totalRdpCount = 0;
      _fsm.close();
      _indexes.fini();
      return;
   }

   INT32 collection::createIndex(requestContext *context,
                                 const strSlice &indexName,
                                 const indexKeyPattern &keyPattern,
                                 const indexParameters &params,
                                 const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      INT32 indexSlot = -1;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = dmsCheckIndexName(indexName.str(), FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }
      else if (!keyPattern.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!params.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INDEX_TYPE_BTREE == params.type &&
          keyPattern.getKeyCount() < params.btreeMaxPrefixFields)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _createIndex(context, indexName, keyPattern, params, indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index[%s]:%d", indexName.str(), rc);
         goto error;
      }

      rc = buildIndexInContext(context, indexSlot, params.type, options.build);
      if (SDB_OK != rc)
      {
         if (SDB_VESSEL_INDEX_BUILDING_TERMINATED != rc)
         {
            PD_LOG(PDERROR, "failed to build index[%s], rc:%d",
                   indexName.str(), rc);
         }
         else
         {
            PD_LOG(PDINFO, "index[%s] creating terminated", indexName.str());
         }
         INT32 tmpRc = rollbackCreatingIndex(context, indexSlot, rc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDERROR, "failed to rollback index creating[%s], rc:%d",
                   indexName.str(), rc);
            ossPanic();
         }
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::listIndexes(requestContext *context,
                                 ossPoolVector<bson::BSONObj> &indexes)
   {
      INT32 rc = SDB_OK;
      indexes.clear();
      ossRWMutexGuard guard(&_dmlLatch, SHARED, FALSE);

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();

      for (indexContextMap::CONST_ITERATOR itr = _indexes.begin();
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

   INT32 collection::saveOnDiskWhenCreating(requestContext *context,
                                             const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_record.isValid(), "must be valid");
      logicalPageBuffer lpb;
      crpAccessor accessor;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      UINT32 pageSize = getDataPageSize();
      strSlice csName;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      PAGE_ID lpid = getCrpLpidOfCollection(pageSize, _record.mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", _record.mbID);
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
      
      rc = accessor.createCL(context, _record, options, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cl on crp:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::dump(requestContext *context,
                          bson::BSONObj &record)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == _collectionSpace))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      record = dumpCollectionWhenList(_collectionSpace->getLogicalID(),
                                      _record);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insert(insertContext *context,
                            utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 == context->getUniqueKeyHashSize(), "must be zero");
      dmlIndexRequestArray ra;
      ossRWMutexGuard guard(&_dmlLatch, SHARED, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               !context->getOriginalRecord().isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_record.isStripingMode() &&
               INVALID_STRIPING_ID == context->getStriping())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();

      /// build indexes keys.
      rc = buildDmlIndexRequests(context, context->getOriginalRecord(), ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR,"failed to build unique index requests:%d", rc);
         goto error;
      }

      rc = constraintCheck(context, ra, res);
      if (SDB_OK != rc)
      {
         if (SDB_IXM_DUP_KEY != rc)
         {
            PD_LOG(PDERROR, "failed to check constraint:%d", rc);
         }
         goto error;
      }
      
      if (!ra.isEmpty())
      {
         context->setLockRid(TRUE);
      }

      if (!isBigRecord(getDataPageSize(), context->getOriginalRecord().getSize()))
      {
         rc = insertNonBigRecord(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert record:%d", rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "todo");
      }

      if (ra.hasBuildingIndex())
      {
         rc = insertNewKeysToBuildingContext(context, ra);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert keys into building context:%d", rc);
            SDB_ASSERT(FALSE, "TODO");/// rollback record
            goto error;
         }
      }

      rc = insertIndexRequests(context, ra);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert data into index:%d", rc);
         SDB_ASSERT(FALSE, "TODO");/// rollback record
         goto error;
      }

      res.incInsertedNum();
      if (res.isEnableReturnIDInfo())
      {
         res.setInsertLoc(context->getLastDmlRid().getPageID(),
                          context->getLastDmlRid().getSlotID());
      }
   done:
      context->unlockRidsAndUniqueKeys();
      return rc;
   error:
      goto done;
   }

   INT32 collection::getTotalCountInRdpHead(requestContext *context,
                                            UINT64 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      count = 0;
      ISession *session = context->getSession();
      const static UINT32 _QUIT_CHECK = 7;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      for (UINT32 i = 0; i < _totalRdpCount; ++i)
      {
         PAGE_ID lpid = INVALID_PAGE_ID;
         UINT32 countInRdp = 0;
         if (0 == (i & _QUIT_CHECK))
         {
            if (session->quit())
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

         rc = getRecordCountInPageHead(context, lpid, countInRdp);
         if (SDB_OK != rc)
         {
            goto error;
         }

         count += countInRdp;
      }
   done:
      return rc;
   error:
      count = 0;
      goto done;
   }

   INT32 collection::testIndex(requestContext *context,
                               const strSlice &indexName,
                               indexHandle &ih)
   {
      INT32 rc = SDB_OK;
      ih = indexHandle();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            indexName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (indexContextMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         indexContext *ic = itr->second;
         if (ic->isNormal() && (ic->getObj().getIndexName() == indexName))
         {
            ih = indexHandle(itr->first, ic->getIndexID());
            goto done;
         }
      }

      rc = SDB_IXM_NOTEXIST;
      goto error;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::testIndex(requestContext *context,
                               UINT32 indexId,
                               INT32 &indexSlot)
   {
      INT32 rc = SDB_OK;
      indexSlot = -1;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_LOGICAL_INDEX_ID == indexId))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (indexContextMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         indexContext *ic = itr->second;
         if (ic->isNormal() && (ic->getObj().getIndexID() == indexId))
         {
            indexSlot = itr->first;
            goto done;
         }
      }

      rc = SDB_IXM_NOTEXIST;
      goto error;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::getMoreWhenScan(requestContext *context,
                                     scanCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != cursor, "can not be null");
      SDB_ASSERT(cursor->isOpen(), "must be open");
      SDB_ASSERT(cursor->getHandle().getCLLId() == _record.logicalCLID, "must be same");

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            NULL == cursor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         PAGE_ID lpid = cursor->getLpid();

         if (_totalRdpCount <= cursor->getToScanEntry().getSeq())
         {
            cursor->pushEnd();
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

         rc = getMoreFromPageInCursor(context, cursor);
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
      } while(TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::getRecordCountInPageHead(requestContext *context,
                                              PAGE_ID lpid,
                                              UINT32 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      logicalPageBuffer lpb;
      rdpScanner scanner;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = scanner.open(context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open scanner of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      count = scanner.getPageHead().recordCount;
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::getMoreFromPageInCursor(requestContext *context,
                                             scanCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(NULL != cursor, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != cursor->getLpid(), "can not be invalid");

      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      recordReader rr;
      memoryBlock mb;
      
      rc = rr.init(context, cursor->getLpid(), &mds,
                   cursor->getToScanEntry().getSlot(), &mb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init record reader:%d", rc);
         goto error;
      }

      do
      {
         slice record;
         recordID rid;
         DPS_TRANS_ID transId;
         BOOLEAN hitTheEnd = FALSE;
         rc = rr.fetchNextToReader(hitTheEnd);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch next:%d", rc);
            goto error;
         }
         else if (hitTheEnd)
         {
            cursor->incToScanPage();
            break;
         }

         if (rr.currentRecordIsTombstone())
         {
            SDB_ASSERT(FALSE, "TODO");
         }

         rid = rr.getCurrentRid();
         cursor->setToScanSlot(rid.getSlotID());
         transId.setNodeID(rr.getCurrentRecordHead().getTransNode());
         transId.setSN(rr.getCurrentRecordHead().getTransSN());
         record = rr.getCurrentRecordBody();

         rc = cursor->pushDataFragments({slice(sizeof(recordID), &rid),
                                         slice(sizeof(DPS_TRANS_ID), &transId),
                                         record});
         if (SDB_OK != rc)
         {
            if (SDB_VESSEL_CURSOR_NO_SPACE != rc)
            {
               PD_LOG(PDERROR, "failed to push record to cursor:%d", rc);
            }
            goto error;
         }

         cursor->setToScanSlot(rid.getSlotID() + 1);
         if (!cursor->isWaitingMorePushing())
         {
            break;
         }
      } while (TRUE);
      
   done:
      rr.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::getMoreWhenIndexScan(indexScanContext *context)
   {
      INT32 rc = SDB_OK;
      indexContext *ic = NULL;
      ossRWMutexGuard guard(&_dmlLatch, SHARED, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            !context->isCursorAttached()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.autoLock();

      if (!context->getHandle().isValid())
      {
         indexHandle h;
         if (INVALID_LOGICAL_INDEX_ID != context->getCursor()->getIndexId())
         {
            INT32 indexSlot = -1;
            rc = testIndex(context, context->getCursor()->getIndexId(), indexSlot);
            if (SDB_OK != rc)
            {
               goto error;
            }
            h = indexHandle(indexSlot, context->getCursor()->getIndexId());
         }
         else if (!context->getCursor()->getIndexName().empty())
         {
            rc = testIndex(context, context->getCursor()->getIndexName(), h);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
         else
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         context->getCursor()->setIndexHandle(h);
      }

      ic = _indexes.find(context->getHandle().getIndexSlot());
      if (NULL == ic)
      {
         PD_LOG(PDERROR, "index[%d] not found", context->getHandle().getIndexSlot());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
      else if (ic->getIndexID() != context->getHandle().getIndexId())
      {
         PD_LOG(PDERROR, "index handle[%d,%d] not found",
                context->getHandle().getIndexId(),
                context->getHandle().getIndexSlot());
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }

      rc = _getMoreWhenIndexScan(context, ic);
      if (SDB_OK != rc)
      {
         if (SDB_IXM_EOC != rc)
         {
            PD_LOG(PDERROR, "failed to get next from index:%d", rc);
         }
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertNonBigRecord(insertContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      UINT32 size = getMaxSizeOfRecordInRdp(context->getOriginalRecord().getSize());
      INT32 targetLvl = getFsmSpaceLvl(getDataPageSize(), size);
      PAGE_ID lpid = INVALID_PAGE_ID;

      do
      {
         fsmCandidate &candidate = context->getCandidate();
         rc = findCandidate(context,
                            targetLvl,
                            context->getStriping(),
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
         }

         rc = insertNonBigRecordToPage(context, lpid);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            PD_LOG(PDDEBUG, "page seq[%] free size may be not correct",
                   candidate.getSeq());
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert record to page:%d", rc);
            goto error;
         }

         break;
      }while(TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertNonBigRecordToPage(insertContext *context,
                                              PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      logicalPageBuffer lpb;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      rdpInsertExecutor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = accessor.insertNormalRecord(context, &lpb);
      if (SDB_OK != rc)
      {
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE != rc)
         {
            PD_LOG(PDERROR, "failed to insert record:%d", rc);
         }
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::findCandidate(requestContext *context,
                                   INT32 lvl,
                                   STRIPING_ID striping,
                                   fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      UINT32 totalRdpCount = _totalRdpCount;
      PAGE_ID lpids[PAGE_COUNT_IN_EXTENT];
      UINT32 firstSeq = INVALID_CL_PAGE_SEQ;
      candidate.reset();

      do
      {
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
         if (totalRdpCount < _totalRdpCount)
         {
            totalRdpCount = _totalRdpCount;
            continue;
         }

         rc = allocateNewRecordDataPages(context, PAGE_COUNT_IN_EXTENT,
                                         firstSeq, lpids);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,  "failed to allocate new rdps:%d", rc);
            goto error;
         }

         totalRdpCount = _totalRdpCount;

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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(8 == count, "capacity of route page is 32bytes alienged");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(isOpen(), "can not be closed");

      routePageAccessor accessor;
      logicalPageBuffer lpb;
      rdpIniter initer;
      UINT32 lvl0No = 0;
      PAGE_ID lvl0Lpid = INVALID_PAGE_ID;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      UINT32 capacity = 0;
      atomicOperationList oplist;
      atomicOperationList *backup = NULL;
      BOOLEAN switched = FALSE;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      
      capacity = getCapacityOfRoutePage(getDataPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      lvl0No = _totalRdpCount / capacity;
      SDB_ASSERT(lvl0No <= _totalLvl0Count, "impossible");
      if (lvl0No == _totalLvl0Count)
      {
         rc = extendRoutePageMap(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend route page map:%d", rc);
            goto error;
         }
      }
      
      initer.init(_record.logicalCLID, _totalRdpCount, count);
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

      firstSeq = _totalRdpCount;
      _totalRdpCount += count;
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(_record.isValid(), "must be valid");
   

      if (INVALID_PAGE_ID != _record.routePages[COLLECTION_ROOT_LVL2])
      {
         rc = initPageSequenceByRootLvL2(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by lvl2 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _record.routePages[COLLECTION_SECOND_ROOT_LVL1])
      {
         rc = initPageSequenceByRootLvL1(context, 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by second lvl1 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _record.routePages[COLLECTION_FIRST_ROOT_LVL1])
      {
         rc = initPageSequenceByRootLvL1(context, 0);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init by first lvl1 root:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID != _record.routePages[COLLECTION_ROOT_LVL0])
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
         _totalLvl0Count = 0;
         _totalRdpCount = 0;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL2(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_record.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != _record.routePages[COLLECTION_ROOT_LVL2],
                 "can not be invalid");

      UINT32 lvl1Count = 0;
      PAGE_ID lastLvl1 = INVALID_PAGE_ID;
      UINT32 lvl0Count = 0;
      PAGE_ID lastLvl0 = INVALID_PAGE_ID;
      UINT32 rdpCount = 0;
      PAGE_ID lastRdp = INVALID_PAGE_ID;

      UINT32 pageSize = _collectionSpace->getSU()->getMainDataSpace().
                        getStorageCoreArgs().pageSize;
      UINT32 capacity = getCapacityOfRoutePage(pageSize);
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _totalLvl0Count = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                            COLLECTION_SECOND_ROOT_LVL1);
      _totalRdpCount = _totalLvl0Count * capacity;

      rc = getCountAndLastEleInRoutePage(context,
                                         _record.routePages[COLLECTION_ROOT_LVL2],
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

      _totalLvl0Count += ((lvl1Count - 1) * capacity);
      _totalRdpCount = _totalLvl0Count * capacity;

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

      _totalLvl0Count += lvl0Count;
      _totalRdpCount += ((lvl0Count - 1) * capacity);

      rc = getCountAndLastEleInRoutePage(context,
                                         lastLvl0,
                                         COLLECTION_ROUTE_PAGE_LVL0,
                                         rdpCount, lastRdp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last rdp:%d", rc);
         goto error;
      }

      _totalRdpCount += rdpCount;
   done:
      return rc;
   error:
      _totalRdpCount = 0;
      _totalLvl0Count = 0;
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL1(requestContext *context,
                                                UINT32 rootNo)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_record.isValid(), "can not be invalid");
      SDB_ASSERT(rootNo <= 1, "can not out of bound");
      PAGE_ID lpid = _record.routePages[COLLECTION_FIRST_ROOT_LVL1 + rootNo];
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");

      UINT32 lvl0Count = 0;
      PAGE_ID lastLvl0 = INVALID_PAGE_ID;
      UINT32 rdpCount = 0;
      PAGE_ID lastRdp = INVALID_PAGE_ID;

      UINT32 pageSize = _collectionSpace->getSU()->getMainDataSpace().
                        getStorageCoreArgs().pageSize;
      UINT32 capacity = getCapacityOfRoutePage(pageSize);
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (0 == rootNo)
      {
         _totalLvl0Count = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                               COLLECTION_ROOT_LVL0);
      }
      else
      {
         _totalLvl0Count = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                               COLLECTION_FIRST_ROOT_LVL1);
      }
      _totalRdpCount = _totalLvl0Count * capacity;

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

      _totalLvl0Count += lvl0Count;
      _totalRdpCount += ((lvl0Count - 1) * capacity);

      rc = getCountAndLastEleInRoutePage(context,
                                         lastLvl0,
                                         COLLECTION_ROUTE_PAGE_LVL0,
                                         rdpCount, lastRdp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get the last rdp:%d", rc);
         goto error;
      }

      _totalRdpCount += rdpCount;
   done:
      return rc;
   error:
      _totalLvl0Count = 0;
      _totalRdpCount = 0;
      goto done;
   }

   INT32 collection::initPageSequenceByRootLvL0(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_record.isValid(), "can not be invalid");
      PAGE_ID lpid = _record.routePages[COLLECTION_ROOT_LVL0];
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

      _totalLvl0Count = 1;
      _totalRdpCount = rdpCount;
   done:
      return rc;
   error:
      _totalLvl0Count = 0;
      _totalRdpCount = 0;
      goto done;
   }

   INT32 collection::getLpidBySequence(requestContext *context,
                                       UINT32 sequence,
                                       PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != sequence, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");

      UINT32 capacity = 0;
      UINT32 lvl0Id = 0;
      PAGE_ID lvl0Lpid = INVALID_PAGE_ID;

      lpid = INVALID_PAGE_ID;

      if (_totalRdpCount <= sequence)
      {
         PD_LOG(PDERROR, "invalid page sequence[%d], current max page count[%d]",
                sequence, _totalRdpCount);
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
         PD_LOG(PDERROR, "failed to get lvl0 page[%d], rc:%d", lvl0Lpid, rc);
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");

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

      if (_totalLvl0Count <= lvl0No)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      if (0 == lvl0No)
      {
         lpid = _record.routePages[COLLECTION_ROOT_LVL0];
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
         lvl1Lpid = _record.routePages[COLLECTION_FIRST_ROOT_LVL1];
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
         lvl1Lpid = _record.routePages[COLLECTION_SECOND_ROOT_LVL1];
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
         PAGE_ID lvl2Lpid = _record.routePages[COLLECTION_ROOT_LVL2];
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
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isOpen(), "can not be closed");

      PAGE_ID newLvl0 = INVALID_PAGE_ID;
      UINT32 maxLvl0COunt = 0;
      PAGE_ID lvl1Lpid = INVALID_PAGE_ID;
      UINT32 capacity = getCapacityOfRoutePage(getDataPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get route page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      maxLvl0COunt = getMaxLvl0Cnt(capacity);

      if (OSS_UNLIKELY(maxLvl0COunt < _totalLvl0Count))
      {
         PD_LOG(PDERROR, "invalid _totalLvl0Count[%d]", _totalLvl0Count);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (getMaxLvl0Cnt(capacity) == _totalLvl0Count)
      {
         PD_LOG(PDERROR, "can not create any more new lvl0 pages");
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }
      
      if (0 == _totalLvl0Count)
      {
         /// COLLECTION_ROOT_LVL0 impossible to be valid when _totalLvl0Count is zero.
         SDB_ASSERT(INVALID_PAGE_ID == _record.routePages[COLLECTION_ROOT_LVL0], "impossible");
         rc = ensureRootRoutePage(context, COLLECTION_ROOT_LVL0);
         if (SDB_OK != rc)
         {
            goto error;
         }
         ++_totalLvl0Count;
         goto done;
      }
      else if (_totalLvl0Count <
               getMaxLvL0RoutePageCountLteRoot(capacity,
                                                COLLECTION_FIRST_ROOT_LVL1))
      {
         rc = ensureRootRoutePage(context, COLLECTION_FIRST_ROOT_LVL1);
         if (SDB_OK != rc)
         {
            goto error;
         }
         lvl1Lpid = _record.routePages[COLLECTION_FIRST_ROOT_LVL1];
      }
      else if (_totalLvl0Count <
               getMaxLvL0RoutePageCountLteRoot(capacity,
                                                   COLLECTION_SECOND_ROOT_LVL1))
      {
         rc = ensureRootRoutePage(context, COLLECTION_SECOND_ROOT_LVL1);
         if (SDB_OK != rc)
         {
            goto error;
         }
         lvl1Lpid = _record.routePages[COLLECTION_SECOND_ROOT_LVL1];
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

      ++_totalLvl0Count;
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(COLLECTION_MIN_ROUTE_ROOT <= rootSlot, "impossible");
      SDB_ASSERT(rootSlot <= COLLECTION_MAX_ROUTE_ROOT, "impossible");

      crpAccessor accessor;
      logicalPageBuffer lpb;
      PAGE_ID crpLpid = INVALID_PAGE_ID;
      PAGE_ID routeLpid = INVALID_PAGE_ID;
      UINT32 rootLvl = COLLECTION_ROUTE_PAGE_LVL0;

      atomicOperationList oplist;
      atomicOperationList *backup = NULL;
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

      if (INVALID_PAGE_ID != _record.routePages[rootSlot])
      {
         goto done;
      }

      crpLpid = getCrpLpidOfCollection(getDataPageSize(), _record.mbID);
      if (OSS_UNLIKELY(INVALID_PAGE_ID == crpLpid))
      {
         PD_LOG(PDERROR, "failed to get lpid of crp");
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

      rc = mds.getLogicalPageBuffer(context, crpLpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid:%d, rc:%d",
                crpLpid, rc);
         goto error;
      }

      /// we are holding ddl s latch and page allocating x latch.
      /// all columns in _record are unchangeable now.
      _record.routePages[rootSlot] = routeLpid;
      oplist.setWaitingTail();
      rc = accessor.updateRoutePages(context, _record, &lpb);
      if (SDB_OK != rc)
      {
         _record.routePages[rootSlot] = INVALID_PAGE_ID;
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != father, "can not be invalid");
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(COLLECTION_ROUTE_PAGE_LVL0 <= pageLvl, "impossible");
      SDB_ASSERT(pageLvl <= COLLECTION_ROUTE_PAGE_LVL1, "impossible");

      PAGE_ID lpid = INVALID_PAGE_ID;
      atomicOperationList oplist;
      atomicOperationList *backup = NULL;
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
      SDB_ASSERT(INVALID_PAGE_ID != _record.routePages[COLLECTION_ROOT_LVL2], "impossible");
      lpid = INVALID_PAGE_ID;
      routePageAccessor accessor;
      logicalPageBuffer lpb;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      UINT32 targetCount = 0;
      UINT32 minCount = 0;
      UINT32 currentLvl1Count = 0;
      PAGE_ID lvl1 = INVALID_PAGE_ID;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      UINT32 capacity = getCapacityOfRoutePage(getDataPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get route page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      minCount = getMaxLvL0RoutePageCountLteRoot(capacity,
                                                     COLLECTION_SECOND_ROOT_LVL1);
      if (_totalLvl0Count < minCount)
      {
         PD_LOG(PDERROR, "invalid _totalLvl0Count[%d]", _totalLvl0Count);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (getMaxLvl0Cnt(capacity) < _totalLvl0Count)
      {
         PD_LOG(PDERROR, "unexpected _totalLvl0Count[%d]", _totalLvl0Count);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      targetCount = ((_totalLvl0Count - minCount) / capacity) + 1;

      rc = mds.getLogicalPageBuffer(context,
                                    _record.routePages[COLLECTION_ROOT_LVL2],
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
                                  _record.routePages[COLLECTION_ROOT_LVL2],
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      routePageIniter initer;
      initer.setLogicalId(_record.logicalCLID);
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
      return _collectionSpace->getSU()->getMainDataSpace().getStorageCoreArgs().pageSize;
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
                                  const strSlice &indexName,
                                  const indexKeyPattern &pattern,
                                  const indexParameters &params,
                                  INT32 &indexSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");   
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(pattern.isValid(), "must be valid");
      SDB_ASSERT(params.isValid(), "must be valid");

      indexSlot = -1;
      bson::BSONObj obj;
      slice objSlice;
      CHAR *fullNameBuffer = NULL;
      UINT32 fullNameBufferSize = 0;
      strSlice nameSlice;
      UINT32 indexId = INVALID_LOGICAL_INDEX_ID;
      BOOLEAN duplicated = FALSE;
      indexConsole console;

      BOOLEAN rollbackLog = FALSE;
      BOOLEAN rollbackDefPage = FALSE;
      indexObject indexObj;
      PAGE_ID lpid = INVALID_PAGE_ID;

      ossScopedRWLock guard(&_dmlLatch, EXCLUSIVE);

      console.init(_record.mbID, &(_collectionSpace->getSU()->getIndexSpace()));

      if (!_indexes.isAllowedToCreateMore())
      {
         PD_LOG(PDINFO, "no more available logical index id or slot");
         rc = SDB_DMS_MAX_INDEX;
         goto error;
      }

      rc = testIfIndexDuplicated(context, indexName, pattern, duplicated);
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

      obj = indexUtils::buildIndexDefObj(indexName, pattern, params);
      if ((INT32)MAX_INDEX_DEF_OBJ_SIZE < obj.objsize())
      {
         PD_LOG(PDERROR, "index def obj size over max size:%d", obj.objsize());
         rc = SDB_INVALIDARG;
         goto error;
      }

      objSlice.reset(obj.objsize(), obj.objdata());

      SDB_ASSERT(!context->getCSName().empty(), "can not be empty");
      SDB_ASSERT(!context->getCLName().empty(), "can not be empty");
      fullNameBufferSize = context->getCSName().strLen() +
                           context->getCLName().strLen() + 2;

      fullNameBuffer = context->allocateBuffer(fullNameBufferSize);
      if (NULL == fullNameBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!buildFullName(fullNameBufferSize,
                         fullNameBuffer,
                         context->getCSName(),
                         context->getCLName()))
      {
         PD_LOG(PDERROR, "failed to build full name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      nameSlice.reset(fullNameBuffer, fullNameBufferSize - 1);

      indexSlot = _indexes.findFreeIndexSlot();
      indexId = _indexes.getNextIndexId();
      SDB_ASSERT(isValidIndexSlot(indexSlot) && (INVALID_LOGICAL_INDEX_ID != indexId),
                 "impossible");

      rc = indexObj.shallowInit(indexId, indexName, pattern, params);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index obj:%d", rc);
         goto error;
      }

      rc = commitCreateIndexLog(context, nameSlice,
                               indexId, indexSlot, objSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit create index log:%d", rc);
         goto error;
      }
      rollbackLog = TRUE;

      rc = console.createIndex(context, indexSlot, indexId, objSlice, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index on index space:%d", rc);
         goto error;
      }
      rollbackDefPage = TRUE;

      rc = _indexes.insert(indexSlot, lpid, indexObj, INDEX_STATUS_BUILDING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to register unstable index[%s], rc:%d",
                indexName.str(), rc);
         goto error;
      }
      
   done:
      if (NULL != fullNameBuffer)
      {
         context->releaseBuffer(fullNameBuffer, fullNameBufferSize);
      }
      return rc;
   error:
      /// rollback log first, we need a new lsn.
      if (rollbackLog)
      {
         SDB_ASSERT(SDB_OK != rc, "impossible");
         INT32 tmpRc = commitCreateIndexEndLog(context, nameSlice, indexName,
                                               indexId, indexSlot, rc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to commit creating end log, rc:%d", tmpRc);
            ossPanic();
         }
      }
      if (rollbackDefPage)
      {
         INT32 tmpRc = console.releaseIndexDefPage(context, indexSlot);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to rollback index[%d] def page:%d",
                  indexSlot, tmpRc);
            ossPanic();
         }
      }
      
      indexSlot = -1;
      goto done;
   }

   INT32 collection::buildIndexInContext(requestContext *context,
                                         INT32 indexSlot,
                                         INDEX_TYPE type,
                                         const buildIndexOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      SDB_ASSERT(INVALID_INDEX_TYPE != type, "can not be invalid");

      static const UINT64 _MAX_SORT_BUF_SIZE = 1024 * 1024 * 1024;
      static const UINT64 _DEFAULT_SORT_BUF_SIZE = 64 * 1024 * 1024;

      if (!o.blockDML)
      {
         if (INDEX_TYPE_LSM == type ||
             o.isSortingDisabled())
         {
            rc = onlineBuildIndex(context, indexSlot);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to online build index[%d], rc:%d", indexSlot, rc);
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

            rc = onlineBuildIndexBySorting(context, indexSlot, sortBufSize);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to online build index[%d] by sorting, rc:%d",
                      indexSlot, rc);
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
                                                         indexContext *ic,
                                                         UINT32 maxRdpCount,
                                                         memoryBlock &sortBuffer)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != ic && ic->isBuilding(), "must be building");
      SDB_ASSERT(0 < sortBuffer.getCapacity(), "can not be zero");

      rtnIxmKeySorterCreator creator;
      dmsIxmKeySorter *sorter = NULL;
      scanEntry entry;
      orderingWrapper ow = ic->getObj().getPattern().getOrdering();
      buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext*>(ic->getUnstatbleContext());
      if (OSS_UNLIKELY(NULL == buildingContext))
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

      rc = creator.createSorter(sortBuffer.getCapacity(),
                                sortBuffer.getBuffer(),
                                dmsIxmKeyComparer(ow.toBsonOrdering()),
                                &sorter);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create sorter:%d", rc);
         goto error;
      }


      rc = fillSorterAndUpdateEntry(context, ic, sorter, maxRdpCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fill sorter:%d", rc);
         goto error;
      }

      rc = sorter->sort();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to sort index keys:%d", rc);
         goto error;
      }

      rc = mergeSorterAndContextIntoIndex(context, ic, sorter);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to merge data into index:%d", rc);
         goto error;
      }

   done:
      if (NULL != sorter)
      {
         creator.releaseSorter(sorter);
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::buildIndexAndUpdateContext(requestContext *context,
                                                indexContext *ic,
                                                UINT32 maxRdpCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != ic && ic->isBuilding(), "can not be null");
      buildingIndexContext *buildingContext = NULL;
      scanEntry entry;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      SDB_ASSERT((!(!keyGen)), "can not be invalid");
      memoryBlock mb;
      indexConsole console;
      bson::BSONObjSet keySet;

      buildingContext = dynamic_cast<buildingIndexContext *>(ic->getUnstatbleContext());
      if (OSS_UNLIKELY(NULL == buildingContext))
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

      console.init(_record.mbID, &is);

      while (entry.getSeq() < maxRdpCount)
      {
         recordReader rr;
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

         rc = rr.init(context, lpid, &mds, entry.getSlot(), &mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init record reader on lpid[%d]:%d", lpid, rc);
            goto error;
         }

         do
         {
            keySet.clear();
            slice record;
            BOOLEAN hitThePageEnd = FALSE;
            rc = rr.fetchNextToReader(hitThePageEnd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fetch the next record:%d", rc);
               goto error;
            }
            else if (hitThePageEnd)
            {
               entry.incSeqAndZeroSlot();
               /// update context with page latch
               buildingContext->updateBuildingHighBound(entry);
               rr.fini();

               /// break to scan the next page.
               break;
            }

            SDB_ASSERT(!rr.currentRecordIsTombstone(), "TODO");
            record = rr.getCurrentRecordBody();
            SDB_ASSERT(record.isValid(), "impossible");

            rc = keyGen(ic->getObj().getPattern().getPattern(), 
                        ic->getObj().getParams().notArray,
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
               rc = console.insert(context,
                                   ic->getIndexSlot(),
                                   ic->getObj(),
                                   ixmKeyOwned(*itr),
                                   rr.getCurrentRecordHead().getTransID(),
                                   context->getSession()->getLastLSN(),
                                   rr.getCurrentRid());
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert key into index[%s], rc:%d",
                         ic->getObj().getIndexName().str(), rc);
                  goto error;
               }
            }
         } while (TRUE);
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
                                              indexContext *ic,
                                              _dmsIxmKeySorter *sorter,
                                              UINT32 maxRdpCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != ic && ic->isBuilding(), "must be building");
      memoryBlock mb;
      mainDataSpace *mds = &(_collectionSpace->getSU()->getMainDataSpace());
      scanEntry entry;
      bson::BSONObjSet keySet;
      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext*>(ic->getUnstatbleContext());
      if (OSS_UNLIKELY(NULL == buildingContext))
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
         recordReader rr;
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

         rc = rr.init(context, lpid, mds, entry.getSlot(), &mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init record reader on lpid[%d]:%d", lpid, rc);
            goto error;
         }

         do
         {
            keySet.clear();

            slice record;
            BOOLEAN hitThePageEnd = FALSE;
            rc = rr.fetchNextToReader(hitThePageEnd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fetch the next record:%d", rc);
               goto error;
            }
            else if (hitThePageEnd)
            {
               entry.incSeqAndZeroSlot();
               /// update context with page latch
               buildingContext->updateBuildingHighBound(entry);
               rr.fini();

               /// break to scan the next page.
               break;
            }

            SDB_ASSERT(!rr.currentRecordIsTombstone(), "TODO");
            record = rr.getCurrentRecordBody();
            SDB_ASSERT(record.isValid(), "impossible");

            rc = keyGen(ic->getObj().getPattern().getPattern(), 
                        ic->getObj().getParams().notArray,
                        record,
                        keySet);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to generate index key:%d", rc);
               goto error;
            }

            rc = sorter->push(keySet, dmsRecordID(lpid, rr.getCurrentRid().getSlotID()));
            if (SDB_DMS_EOC == rc)
            {
               entry.reset(entry.getSeq(), rr.getCurrentRid().getSlotID());
               buildingContext->updateBuildingHighBound(entry);
               rr.fini();
               goto done;
            }
            else if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push key into sorter:%d", rc);
               goto error;
            }
            else
            {
               continue;
            }
         } while (TRUE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::onlineBuildIndex(requestContext *context,
                                      INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      static const UINT32 _SCAN_PAGE_COUNT_PER_LOOP = 4;

      do
      {
         scanEntry buildEntry;
         UINT32 currentRdpCount = 0;
         indexContext *ic = NULL;
         buildingIndexContext *buildingContext = NULL;

         ossRWMutexGuard guard(&_dmlLatch, SHARED);

         ic = _indexes.find(indexSlot, INDEX_STATUS_BUILDING);
         if (NULL == ic)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexSlot);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(ic->getUnstatbleContext());
         if (OSS_UNLIKELY(NULL == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (buildingContext->isTerminated())
         {
            PD_LOG(PDINFO, "index[%d] building terminated", ic->getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         if (!buildingContext->getNextBuildingBound(buildEntry))
         {
            PD_LOG(PDERROR, "failed to get next building range");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         /// _totalRdpCount is not protected by latch here.
         currentRdpCount = _totalRdpCount;

         if (currentRdpCount <= (buildEntry.getSeq() + _SCAN_PAGE_COUNT_PER_LOOP))
         {
            break;
         }

         rc = buildIndexAndUpdateContext(context, ic, currentRdpCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build index[%s] and update context:%d",
                   ic->getObj().getIndexName().str(), rc);
            goto error;
         }

      } while (TRUE);

      {
         ossRWMutexGuard guard(&_dmlLatch, EXCLUSIVE);
         scanEntry buildEntry;
         buildingIndexContext *buildingContext = NULL;
         indexContext *ic = _indexes.find(indexSlot, INDEX_STATUS_BUILDING);
         if (NULL == ic)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexSlot);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(ic->getUnstatbleContext());
         if (OSS_UNLIKELY(NULL == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (buildingContext->isTerminated())
         {
            PD_LOG(PDINFO, "index[%d] building terminated", ic->getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         if (!buildingContext->getNextBuildingBound(buildEntry))
         {
            PD_LOG(PDERROR, "failed to get next building range");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (buildEntry.getSeq() < _totalRdpCount)
         {
            rc = buildIndexAndUpdateContext(context, ic, _totalRdpCount);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build index[%s] and update context:%d",
                     ic->getObj().getIndexName().str(), rc);
               goto error;
            }
         }

         rc = indexBuildDone(context, ic);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to finish building index[%s], rc:%d",
                   ic->getObj().getIndexName().str(), rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::onlineBuildIndexBySorting(requestContext *context,
                                               INT32 indexSlot,
                                               UINT64 sortBufferSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
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
         ossRWMutexGuard guard(&_dmlLatch, SHARED);
         UINT32 currentRdpCount = 0;
         buildingIndexContext *buildingContext = NULL;
         indexContext *ic = _indexes.find(indexSlot, INDEX_STATUS_BUILDING);
         if (NULL == ic)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexSlot);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(ic->getUnstatbleContext());
         if (OSS_UNLIKELY(NULL == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (buildingContext->isTerminated())
         {
            PD_LOG(PDINFO, "index[%d] building terminated", ic->getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         if (!buildingContext->getNextBuildingBound(entry))
         {
            PD_LOG(PDERROR, "failed to get next building range");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         /// _totalRdpCount is not protected by latch here.
         currentRdpCount = _totalRdpCount;

         if (currentRdpCount <= (entry.getSeq() + _ENDING_LOOP_RDP_COUNT))
         {
            break;
         }
         
         rc = buildIndexBySortingAndUpdateContext(context, ic, currentRdpCount, mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build index[%d], rc:%d", indexSlot, rc);
            goto error;
         }
      } while (TRUE);

      {
         ossRWMutexGuard guard(&_dmlLatch, EXCLUSIVE);
         buildingIndexContext *buildingContext = NULL;
         indexContext *ic = _indexes.find(indexSlot, INDEX_STATUS_BUILDING);
         if (NULL == ic)
         {
            PD_LOG(PDERROR, "index[%d] not found", indexSlot);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         buildingContext = dynamic_cast<buildingIndexContext *>(ic->getUnstatbleContext());
         if (OSS_UNLIKELY(NULL == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (buildingContext->isTerminated())
         {
            PD_LOG(PDINFO, "index[%d] building terminated", ic->getIndexSlot());
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
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
            else if (entry.getSeq() == _totalRdpCount)
            {
               break;
            }

            rc = buildIndexBySortingAndUpdateContext(context, ic, _totalRdpCount, mb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build index[%d], rc:%d", indexSlot, rc);
               goto error;
            }
         } while(TRUE);

         rc = indexBuildDone(context, ic);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to finish building index[%s], rc:%d",
                   ic->getObj().getIndexName().str(), rc);
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
      SDB_ASSERT(NULL != buildingContext, "can not be null");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexMergingRecordList mrl;
      indexConsole console;
      console.init(_record.mbID, &is);

      while (!buildingContext->endToBuildCurrentRange(mrl))
      {
         SDB_ASSERT(FALSE, "TODO");
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 collection::mergeSorterAndContextIntoIndex(requestContext *context,
                                                    indexContext *ic,
                                                    _dmsIxmKeySorter *sorter)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != sorter, "can not be null");
      SDB_ASSERT(NULL != ic && ic->isBuilding(), "must be building");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      console.init(_record.mbID, &is);
      buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext *>
                                              (ic->getUnstatbleContext());
      if (OSS_UNLIKELY(NULL == buildingContext))
      {
         PD_LOG(PDERROR, "failed to get building context ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      do
      {
         ixmKey key;
         dmsRecordID dmsRid;

         rc = sorter->fetch(key, dmsRid);
         if (SDB_DMS_EOC == rc)
         {
            rc = SDB_OK;
            break;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch key from sorter:%d", rc);
            goto error;
         }

         rc = console.insert(context, ic->getIndexSlot(),
                             ic->getObj(),
                             key, DPS_TRANS_ID(),
                             context->getSession()->getLastLSN(),
                             recordID(dmsRid._extent, dmsRid._offset));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert key into index[%s]:%d",
                   ic->getObj().getIndexName().str(), rc);
            goto error;
         }
      } while (TRUE);

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
                                     indexContext *ic)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != ic && ic->isBuilding(), "must be building");

      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      CHAR *fullNameBuffer = NULL;
      UINT32 fullNameBufferSize = 0;
      strSlice nameSlice;


      SDB_ASSERT(!context->getCSName().empty(), "can not be empty");
      SDB_ASSERT(!context->getCLName().empty(), "can not be empty");
      fullNameBufferSize = context->getCSName().strLen() +
                           context->getCLName().strLen() + 2;

      fullNameBuffer = context->allocateBuffer(fullNameBufferSize);
      if (NULL == fullNameBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!buildFullName(fullNameBufferSize,
                         fullNameBuffer,
                         context->getCSName(),
                         context->getCLName()))
      {
         PD_LOG(PDERROR, "failed to build full name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      nameSlice.reset(fullNameBuffer, fullNameBufferSize - 1);

      console.init(_record.mbID, &is);

      rc = commitCreateIndexEndLog(context, nameSlice,
                                   ic->getObj().getIndexName(),
                                   ic->getIndexID(),
                                   ic->getIndexSlot(),
                                   SDB_OK);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit index creating end log:%d", rc);
         ossPanic();
         goto error;
      }

      rc = console.updateIndexStatus(context, ic->getIndexID(),
                                     ic->getEntryLpid(),
                                     INDEX_STATUS_NORMAL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to update index[%s] status to normal:%d",
                ic->getObj().getIndexName().str(), rc);
         ossPanic();
         goto error;
      }

      ic->setNormalFromBuilding();
   done:
      if (NULL != fullNameBuffer)
      {
         context->releaseBuffer(fullNameBuffer, fullNameBufferSize);
      }
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(pattern.isValid(), "can not be invalid");

      duplicated = FALSE;
      for (indexContextMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         if (!itr->second->isNormal() && !itr->second->isBuilding())
         {
            continue;
         }

         if (itr->second->getObj().getIndexName() == indexName)
         {
            PD_LOG(PDINFO, "duplidated index name[%s]", indexName.str());
            duplicated = TRUE;
            goto done;
         }
         if (pattern.isCoveredBy(itr->second->getObj().getPattern()))
         {
            PD_LOG(PDINFO, "duplidated index pattern[%s]",
                   itr->second->getObj().getIndexName().str());
            duplicated = TRUE;
            goto done;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }


/*
   INT32 collection::rollbackCreatingIndexLog(requestContext *context,
                                              INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");

      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      ossPoolString fullName;
      strSlice fullNameSlice;
      indexContext *ic = NULL;

      ossRWMutexGuard(&_dmlLatch, EXCLUSIVE);

      ic = _indexes.find(indexSlot);
      if (NULL == ic)
      {
         PD_LOG(PDERROR, "index[%d] context not found", indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      console.init(_record.mbID, &is);

      fullName.append(_collectionSpace->getCSName());
      fullName.append(".");
      fullName.append(_record.name);
      fullNameSlice.reset(fullName.c_str(), fullName.size());

      rc = commitCreateIndexEndLog(context, fullNameSlice,
                                   uic->getIndexObj().getIndexName(),
                                   uic->getIndexId(),
                                   indexSlot, -1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit create index end log:%d", rc);
         ossPanic();
         goto error;
      }

      _indexContext.freeSlotAndEraseUnstableIndex(indexSlot);
      uic = NULL;

      rc = console.releaseIndexDefPage(context, indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release index def page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
   */

   INT32 collection::initIndexesWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;

      console.init(_record.mbID, &is);

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
      indexContextMap::CONST_ITERATOR itr = _indexes.begin();
      for (; itr != _indexes.end(); ++itr)
      {
         if (!itr->second->isNormal())
         {
            SDB_ASSERT(FALSE, "TODO");
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::buildDmlIndexRequests(requestContext *context,
                                           const slice &record,
                                           dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      bson::BSONObjSet keySet;

      for (indexContextMap::CONST_ITERATOR itr = _indexes.begin();
           itr != _indexes.end(); ++itr)
      {
         keySet.clear();
         indexContext *ic = itr->second;
         SDB_ASSERT(NULL != ic && ic->isValid(), "can not be invalid");
         if (!ic->isNormal() && !ic->isBuilding())
         {
            continue;
         }

         rc = keyGen(ic->getObj().getPattern().getPattern(),
                     ic->getObj().getParams().notArray,
                     record, keySet);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to generate index keys:%d", rc);
            goto error;
         }

         rc = ra.append(itr->second, keySet);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to set index request at pos[%d], rc:%d", itr->first, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::constraintCheck(dmlContext *context,
                                     const dmlIndexRequestArray &ra,
                                     utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      recordID rid;

      if (ra.withoutConstraint())
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
         
         dmlIndexRequest *req = ra.get(i);
         SDB_ASSERT(NULL != req && req->isValid(), "impossible");
         if (!req->withConstraint())
         {
            continue;
         }

         ossPoolList<bson::BSONObj>::const_iterator itr = req->getKeys().begin();
         for (; itr != req->getKeys().end(); ++itr)
         {
            rc = indexConsole::checkUniqueConstraint(context, req->getContext(), *itr, rid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find key in index:%d", rc);
               goto error;
            }

            if (rid.valid())
            {
               PD_LOG(PDDEBUG, "duplidated key[%s] found in index[%s], rid[%d,%d]",
                      itr->toString().c_str(),
                      req->getContext()->getObj().getIndexName().str(),
                      rid.getPageID(), rid.getSlotID());
               rc = SDB_IXM_DUP_KEY;
               res.incDuplicatedNum();
               if (res.isEnaleIndexErrInfo())
               {
                  const indexObject &indexObj = req->getContext()->getObj();
                  res.setIndexErrInfo(indexObj.getIndexName().str(),
                                      indexObj.getPattern().getPattern(),
                                      *itr);
               }
               goto error;
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
      console.init(_record.mbID, &is);

      rc = console.dmlInsert(context, ra);
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

   INT32 collection::insertNewKeysToBuildingContext(dmlContext *context,
                                                    dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      for (UINT32 i = 0; i < ra.getSize(); ++i)
      {
         dmlIndexRequest *ir = ra.get(i);
         SDB_ASSERT(NULL != ir && ir->isValid(), "impossible");
         if (!ir->getContext()->isBuilding())
         {
            continue;
         }

         scanEntry entry;
         BOOLEAN refused = FALSE;
         unstableIndexContext *uic = ir->getContext()->getUnstatbleContext();
         buildingIndexContext *buildingContext = dynamic_cast<buildingIndexContext*>(uic);
         if (OSS_UNLIKELY(NULL == buildingContext))
         {
            PD_LOG(PDERROR, "failed to get building context ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = buildingContext->insert(ir->getContext()->getObj().getParams().isUnique,
                                      context->getLastDmlScanEntry(),
                                      context->getLastDmlRid().getPageID(),
                                      context->getLastDmlLSN(),
                                      context->getTransID(),
                                      ir->getKeys(),
                                      refused);

         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert merging keys into context:%d", rc);
            goto error;
         }

         if (!refused)
         {
            ir->setMerged();
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::rollbackCreatingIndex(requestContext *context,
                                           INT32 indexSlot,
                                           INT32 reason)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(SDB_OK != reason, "can not be ok");
      SDB_ASSERT(!context->getCSName().empty(), "can not be empty");
      SDB_ASSERT(!context->getCLName().empty(), "can not be empty");

      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      console.init(_record.mbID, &is);
      strSlice nameSlice;
      UINT32 fullNameBufferSize = context->getCSName().strLen() +
                                  context->getCLName().strLen() + 2;

      CHAR *fullNameBuffer = context->allocateBuffer(fullNameBufferSize);
      if (NULL == fullNameBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!buildFullName(fullNameBufferSize,
                         fullNameBuffer,
                         context->getCSName(),
                         context->getCLName()))
      {
         PD_LOG(PDERROR, "failed to build full name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      nameSlice.reset(fullNameBuffer, fullNameBufferSize - 1);

      {
         ossRWMutexGuard guard(&_dmlLatch, SHARED);
         indexContext *ic = _indexes.find(indexSlot, INDEX_STATUS_BUILDING);
         if (NULL == ic)
         {
            PD_LOG(PDERROR, "failed to find index context[%d]", indexSlot);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = commitCreateIndexEndLog(context, nameSlice, ic->getObj().getIndexName(),
                                            ic->getObj().getIndexID(), indexSlot, reason);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit create index end log:%d", rc);
            goto error;
         }
      }

      rc = removeIndex(context, indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove index[%d], rc:%d", indexSlot, rc);
         goto error;
      }

      rc = truncateIndex(context, indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index[%d], rc:%d", indexSlot, rc);
         goto error;
      }

      rc = console.releaseIndexDefPage(context, indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release index[%d] def page:%d", indexSlot, rc);
         goto error;
      }

      {
         ossRWMutexGuard guard(&_dmlLatch, EXCLUSIVE);
         _indexes.erase(indexSlot);
      }
   done:
      if (NULL != fullNameBuffer)
      {
         context->releaseBuffer(fullNameBuffer, fullNameBufferSize);
      }
      return rc;
   error:
      goto done;
   }

   INT32 collection::removeIndex(requestContext *context,
                                 INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      ossRWMutexGuard guard(&_dmlLatch, EXCLUSIVE);
      console.init(_record.mbID, &is);
      indexContext *ic = _indexes.find(indexSlot);
      if (NULL == ic)
      {
         PD_LOG(PDERROR, "failed to find index context[%d]", indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = console.updateIndexStatus(context, ic->getIndexID(),
                                     ic->getEntryLpid(),
                                     INDEX_STATUS_REMOVING);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index status:%d", rc);
         goto error;
      }

      ic->setRemoving();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::truncateIndex(requestContext *context,
                                   INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      ossRWMutexGuard guard(&_dmlLatch, SHARED);
      console.init(_record.mbID, &is);
      indexContext *ic = _indexes.find(indexSlot);
      if (NULL == ic)
      {
         PD_LOG(PDERROR, "failed to find index context[%d]", indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!ic->isRemoving() || ic->isTruncating())
      {
         PD_LOG(PDERROR, "index[%d] with wrong status[%d]", ic->getStatus());
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = console.truncateIndex(context, ic);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate index[%s], rc:%d",
                ic->getObj().getIndexName().str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::_getMoreWhenIndexScan(indexScanContext *context,
                                           indexContext *ic)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && context->isCursorAttached(), "can not be invalid");
      SDB_ASSERT(NULL != ic && ic->isNormal(), "must be normal");

      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      memoryBlock mb;
      recordReader rr;
      const indexScanOptions &o = context->getOptions();
      SDB_ASSERT(0 < o.stepLength, "can not be zero");
      indexScanCursor *cursor = context->getCursor();
      indexScanner scanner;
      bson::BSONObjBuilder keyObjBuilder;

      rc = scanner.open(context, ic);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index scanner:%d", rc);
         goto error;
      }

      do
      {
         recordID rid;
         DPS_TRANS_ID transID;

         if (scanner.isPaused())
         {
            rc = scanner.resume();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to resume scanner:%d", rc);
               goto error;
            }
         }

         rc = scanner.next(rid);
         if (SDB_IXM_EOC == rc)
         {
            rc = SDB_OK;
            cursor->pushEnd();
            goto done;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next rid from scanner:%d", rc);
            goto error;
         }

         if (o.indexCoverd)
         {
            transID = scanner.getTransID();
            ixmKey key;
            scanner.getKey(key);
            bson::BSONObj keyObj;

            keyObjBuilder.reset();
            rc = key.toRecord(ic->getObj().getPattern().getPattern(), keyObjBuilder);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build key obj:%d", rc);
               goto error;
            }

            keyObj = keyObjBuilder.done();

            rc = cursor->pushDataFragments({slice(sizeof(recordID), &rid),
                                            slice(sizeof(DPS_TRANS_ID), &transID),
                                            slice(keyObj.objsize(), keyObj.objdata())});
            if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
            {
               rc = SDB_OK;
               goto done;
            }
            else if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push data into cursor:%d", rc);
               goto error;
            }
         }
         else
         {
            scanner.pause();
            
            rc = rr.read(context, rid, &mds, &mb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to read record[%d,%d], rc:%d",
                      rid.getPageID(), rid.getSlotID(), rc);
               goto error;
            }

            transID = rr.getCurrentRecordHead().getTransID();
            rc = cursor->pushDataFragments({slice(sizeof(recordID), &rid),
                                            slice(sizeof(DPS_TRANS_ID), &transID),
                                            rr.getCurrentRecordBody()});
            if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
            {
               rc = SDB_OK;
               goto done;
            }
            else if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push data fragments into cursor:%d", rc);
               goto error;
            }

            rr.fini();
         }

         context->unlockRid(rid);
      } while (cursor->isWaitingMorePushing());
      
   done:
      scanner.close();
      return rc;
   error:
      rr.fini();
      context->unlockAllRids();
      goto done;
   }
}//namespace vessel
}//namespace engine