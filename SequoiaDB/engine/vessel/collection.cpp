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
#include "vessel/indexDefPage.h"
#include "vessel/indexConsole.h"
#include "vessel/redoLogUtil.h"
#include "vessel/unstableIndexContext.h"
#include "rtnIxmKeySorter.hpp"
#include "vessel/indexKeyGenerator.h"
#include "vessel/outerResource.h"
#include "vessel/recordReader.h"
#include "ixmIndexKey.hpp"

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
      return;
   }

   INT32 collection::createIndex(requestContext *context,
                                 const strSlice &indexName,
                                 const indexKeyPattern &keyPattern,
                                 const indexParameters &params,
                                 const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      UINT32 sortBufSize = 0;
      static const UINT32 _MIN_SORT_BUF_SIZE = 16;
      static const UINT32 _MAX_SORT_BUF_SIZE = 4096;
      static const UINT32 _DEFAULT_SORT_BUF_SIZE = 64;
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

      rc = _createIndex(context, indexName, keyPattern, params, indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index[%s]:%d", indexName.str(), rc);
         goto error;
      }

      sortBufSize = options.sortBufferSize;
      if (sortBufSize < _MIN_SORT_BUF_SIZE ||
          _MAX_SORT_BUF_SIZE < sortBufSize)
      {
         sortBufSize = _DEFAULT_SORT_BUF_SIZE;
      }
      sortBufSize *= 1048576; /// 1MB

      if (!options.blockDML)
      {
         rc = onlineBuildIndex(context,
                                indexSlot,
                                sortBufSize);
      }
      else
      {
         SDB_ASSERT(FALSE, "TODO");
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rebuild index[%s], rc:%d", indexName.str(), rc);
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
      indexConsole console;
      indexes.clear();

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

      console.init(&_record, &(_collectionSpace->getSU()->getIndexSpace()));
      {
      ossScopedRWLock guard(&_dmlLatch, SHARED);
      rc = console.listIndexes(context, indexes);
      if (SDB_OK != rc)
      {
         goto error;
      }
      }
   done:
      return rc;
   error:
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

      PAGE_ID lpid = getCrpLpidOfCollection(pageSize, _record.mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get lpid of mbid[%d]", _record.mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      rc = mds.getLogicalPageBuffer(context, lpid,
                                    OSS_SHARED_LATCH_MODE_EXCLUSIVE, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }
      
      csName.reset(_collectionSpace->getCSName());
      rc = accessor.createCL(context, _record, options, csName, &lpb);
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

      if (UTIL_COMPRESSOR_INVALID != _record.compressionType)
      {
         SDB_ASSERT(FALSE, "todo");
      }

      if (!isBigRecord(getDataPageSize(), context->getOriginalRecord().len()))
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
   done:
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

      rc = mds.getLogicalPageBuffer(context, lpid, OSS_SHARED_LATCH_MODE_SHARED, lpb);
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

         if (rr.isCurrentRecordIsTombstone())
         {
            SDB_ASSERT(FALSE, "TODO");
         }

         rid = rr.getCurrentRid();
         cursor->setToScanSlot(rid.getSlotID());
         transId.setNodeID(rr.getCurrentRecordHead().getTransNode());
         transId.setSN(rr.getCurrentRecordHead().getTransSN());
         record = rr.getCurrentRecordBody();

         rc = cursor->pushFragments({std::make_pair(sizeof(recordID), &rid),
                                     std::make_pair(sizeof(DPS_TRANS_ID), &transId),
                                     std::make_pair(record.len(), record.data())});
         if (SDB_OK != rc)
         {
            if (SDB_VESSEL_CURSOR_NO_SPACE != rc)
            {
               PD_LOG(PDERROR, "failed to push record to cursor:%d", rc);
            }
            goto error;
         }

         cursor->setToScanSlot(rid.getSlotID() + 1);
      } while (TRUE);
      
   done:
      rr.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertNonBigRecord(insertContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      UINT32 size = getMaxSizeOfRecordInRdp(context->getRecordToInsert().len());
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

      rc = mds.getLogicalPageBuffer(context, lpid,
                                    OSS_SHARED_LATCH_MODE_EXCLUSIVE, lpb);
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

      rc = mds.getLogicalPageBuffer(context, lvl0Lpid,
                                    OSS_SHARED_LATCH_MODE_EXCLUSIVE, lpb);
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

      rc = getCountAndLastEleInRoutePage(context, capacity,
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

      rc = getCountAndLastEleInRoutePage(context, capacity,
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

      rc = getCountAndLastEleInRoutePage(context, capacity,
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

      rc = getCountAndLastEleInRoutePage(context, capacity,
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

      rc = getCountAndLastEleInRoutePage(context, capacity,
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

      UINT32 pageSize = _collectionSpace->getSU()->getMainDataSpace().
                        getStorageCoreArgs().pageSize;
      UINT32 capacity = getCapacityOfRoutePage(pageSize);
      if (0 == capacity)
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = getCountAndLastEleInRoutePage(context, capacity, lpid,
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
                                                   UINT32 capacity,
                                                   PAGE_ID lpid,
                                                   INT32 lvl,
                                                   UINT32 &count,
                                                   PAGE_ID &element)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < capacity, "can not be zero");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(isValidRoutePageLvl(lvl), "must be valid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");

      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      mmapPagePointer ptr;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      const routePageHead *head = NULL;

      rc = mds.getPageMappingAtNonruntime(context, lpid, pid, psv, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get mapping of lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = validatePage(ptr.get(), PAGE_TYPE_ROUTE,
                        mds.getStorageCoreArgs().pageSize,
                        pid, lpid, psv);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate pid[%d], rc:%d", pid, rc);
         goto error;
      }

      head = (const routePageHead *)(ptr.get() + PAGE_HEAD_SIZE);
      if (_record.logicalCLID != head->logicalId)
      {
         PD_LOG(PDERROR, "cl logical ids not same[%d,%d]",
                _record.logicalCLID, head->logicalId);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
      else if (capacity < head->size)
      {
         PD_LOG(PDERROR, "invalid count in head:%d", head->size);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (lvl != head->lvl)
      {
         PD_LOG(PDERROR, "lvl in head [%d] is not target page lvl[%d] in page[%d]",
                head->lvl, lvl, lpid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      count = head->size;
      if (0 < count)
      {
         UINT32 offset = PAGE_HEAD_SIZE + ROUTE_PAGE_HEAD_SIZE;
         offset += (count - 1) * sizeof(PAGE_ID);
         const PAGE_ID *tmp = (const PAGE_ID *)(ptr.get() + offset);
         element = *tmp;
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

      rc = mds.getLogicalPageBuffer(context, routePgaeLpid,
                                    OSS_SHARED_LATCH_MODE_SHARED, lpb);
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

      rc = mds.getLogicalPageBuffer(context, crpLpid,
                                    OSS_SHARED_LATCH_MODE_EXCLUSIVE, lpb);
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

      context->swtichOplist(&oplist, &backup);

      rc = createNewRoutePage(context, pageLvl, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new route page:%d", rc);
         goto error;
      }

      rc = mds.getLogicalPageBuffer(context, father,
                                    OSS_SHARED_LATCH_MODE_EXCLUSIVE, lpb);
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
                                    OSS_SHARED_LATCH_MODE_SHARED, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get root lvl2:%d", rc);
         goto error;
      }

      rc = accessor.getSizeAndLast(context, &lpb, currentLvl1Count, lvl1);
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
      ossPoolString fullName;
      strSlice nameSlice;
      UINT32 indexId = INVALID_LOGICAL_INDEX_ID;
      BOOLEAN duplicated = FALSE;
      PAGE_ID crpLpid = INVALID_PAGE_ID;
      logicalPageBuffer lpb;
      crpAccessor accessor;
      mainDataSpace &mds = _collectionSpace->getSU()->getMainDataSpace();
      indexConsole console;
      console.init(&_record, &(_collectionSpace->getSU()->getIndexSpace()));
      BOOLEAN rollbackLog = FALSE;
      BOOLEAN rollbackIndex = FALSE;
      BOOLEAN rollbackUnstableIndexes = FALSE;
      unstableIndexContext *uic = NULL;
      indexObject indexObj;

      UINT64 newUniqueIndexes = 0;
      UINT64 newUonuniqueIndexes = 0;

      ossScopedRWLock guard(&_dmlLatch, EXCLUSIVE);

      indexId = _record.nextIndexId;
      if (INVALID_LOGICAL_INDEX_ID == indexId)
      {
         PD_LOG(PDERROR, "no more available logical index id");
         rc = SDB_DMS_MAX_INDEX;
         goto error;
      }

      rc = console.allocateIndexSlot(indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new index slot:%d", rc);
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

      rc = indexObj.shallowInit(indexId, indexName, pattern, params);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index obj:%d", rc);
         goto error;
      }

      rc = _unstableIndexes.insert(indexSlot, indexObj,
                                   INDEX_STATUS_BUILDING,
                                   &uic);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to register unstable index[%s], rc:%d",
                indexName.str(), rc);
         goto error;
      }
      rollbackUnstableIndexes = TRUE;

      fullName.append(_collectionSpace->getCSName());
      fullName.append(".");
      fullName.append(_record.name);
      nameSlice.reset(fullName.c_str(), fullName.size());

      rc = commitCreateIndexLog(context, nameSlice,
                               indexId, indexSlot, objSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit create index log:%d", rc);
         goto error;
      }
      rollbackLog = TRUE;

      rc = console.createIndex(context, indexSlot, indexId, objSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index on index space:%d", rc);
         goto error;
      }
      rollbackIndex = TRUE;

      if (params.isUnique)
      {
         newUniqueIndexes = (_record.uniqueIndexes | ((UINT64)1 << indexSlot));
         newUonuniqueIndexes = _record.nonUniqueIndexes;
      }
      else
      {
         newUniqueIndexes = _record.uniqueIndexes;
         newUonuniqueIndexes = (_record.nonUniqueIndexes | ((UINT64)1 << indexSlot));
      }

      crpLpid = getCrpLpidOfCollection(getDataPageSize(), _record.mbID);
      if (INVALID_PAGE_ID == crpLpid)
      {
         PD_LOG(PDERROR, "failed to get crp lpid of mbid[%d]", _record.mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = mds.getLogicalPageBuffer(context, crpLpid,
                                    OSS_SHARED_LATCH_MODE_EXCLUSIVE,
                                    lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", crpLpid, rc);
         goto error;
      }

      rc = accessor.updateIndexInfo(context, _record.mbID,
                                    newUniqueIndexes,
                                    newUonuniqueIndexes,
                                    indexId + 1, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index info:%d", rc);
         goto error;
      }

      _record.uniqueIndexes = newUniqueIndexes;
      _record.nonUniqueIndexes = newUonuniqueIndexes;
      ++_record.nextIndexId;

   done:
      lpb.fini();
      return rc;
   error:
      if (rollbackUnstableIndexes)
      {
         _unstableIndexes.erase(indexSlot);
      }

      /// rollback log first, we need a new lsn.
      if (rollbackLog)
      {
         strSlice name(fullName.c_str(), fullName.size());
         INT32 tmpRc = commitCreateIndexEndLog(context, name,
                                               indexId, indexSlot, -1);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to commit creating end log, rc:%d", tmpRc);
         }
      }

      if (rollbackIndex)
      {
         INT32 tmpRc = console.releaseIndexSlot(context, indexSlot);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to rollback index created[%d,%d], rc:%d",
                   indexSlot, indexId, tmpRc);
         }
      }
      
      indexSlot = -1;
      goto done;
   }

   INT32 collection::buildIndexBySortingAndUpdateContext(requestContext *context,
                                                         unstableIndexContext *uic,
                                                         UINT32 maxRdpCount,
                                                         memoryBlock &sortBuffer)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != uic, "can not be null");
      SDB_ASSERT(INDEX_STATUS_BUILDING == uic->getStatus(), "must be building");
      SDB_ASSERT(0 < sortBuffer.getCapacity(), "can not be zero");

      rtnIxmKeySorterCreator creator;
      dmsIxmKeySorter *sorter = NULL;
      scanEntry entry;
      orderingWrapper ow = uic->getIndexObj().getPattern().getOrdering();

      if (!uic->getNextRebuildingRangeBound(entry))
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
                                dmsIxmKeyComparer(*ow.toBsonOrdering()),
                                &sorter);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create sorter:%d", rc);
         goto error;
      }


      rc = fillSorterAndUpdateEntry(context, sorter, maxRdpCount, uic);
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

      rc = mergeSorterAndContextIntoIndex(context, sorter, uic);
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

   INT32 collection::fillSorterAndUpdateEntry(requestContext *context,
                                              _dmsIxmKeySorter *sorter,
                                              UINT32 maxRdpCount,
                                              unstableIndexContext *uic)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != uic, "can not be null");
      SDB_ASSERT(INDEX_STATUS_BUILDING == uic->getStatus(), "must be building");
      memoryBlock mb;
      mainDataSpace *mds = &(_collectionSpace->getSU()->getMainDataSpace());
      scanEntry entry;
      bson::BSONObjSet keySet;
      INDEX_KEY_GENERATOR keyGen = context->getOuterResource()->indexKeyGen;
      _ixmKeyBuilder builder(FALSE);

      if (!uic->getNextRebuildingRangeBound(entry))
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
               uic->updateRebuildingHighBound(entry);
               rr.fini();

               /// break to scan the next page.
               break;
            }

            SDB_ASSERT(!rr.isCurrentRecordIsTombstone(), "TODO");
            record = rr.getCurrentRecordBody();
            SDB_ASSERT(record.isValid(), "impossible");

            rc = keyGen(uic->getIndexObj().getPattern().getPattern(), 
                        uic->getIndexObj().getParams().notArray,
                        record,
                        &builder, keySet);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to generate index key:%d", rc);
               goto error;
            }

            rc = sorter->push(keySet, dmsRecordID(lpid, rr.getCurrentRid().getSlotID()));
            if (SDB_DMS_EOC == rc)
            {
               entry.reset(entry.getSeq(), rr.getCurrentRid().getSlotID());
               uic->updateRebuildingHighBound(entry);
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
                                      INT32 indexSlot,
                                      UINT32 sortBufferSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      SDB_ASSERT(0 < sortBufferSize, "can not be zero");

      static const UINT32 _ENDING_LOOP_RDP_COUNT = 2;
      memoryBlock mb;
      unstableIndexContext *uic = NULL;
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
         /// _totalRdpCount is not protected by latch here.
         UINT32 currentRdpCount = _totalRdpCount;
         uic = _unstableIndexes.findBuidingContext(indexSlot);
         if (NULL == uic)
         {
            PD_LOG(PDERROR, "index[%d] is not building", indexSlot);
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         if (!uic->getNextRebuildingRangeBound(entry))
         {
            PD_LOG(PDERROR, "failed to get next building range");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (currentRdpCount <= (entry.getSeq() + _ENDING_LOOP_RDP_COUNT))
         {
            break;
         }
         
         rc = buildIndexBySortingAndUpdateContext(context, uic, currentRdpCount, mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build index[%d], rc:%d", indexSlot, rc);
            goto error;
         }
      } while (TRUE);

      {
         ossRWMutexGuard guard(&_dmlLatch, EXCLUSIVE);
         uic = _unstableIndexes.findBuidingContext(indexSlot);
         if (NULL == uic)
         {
            PD_LOG(PDERROR, "index[%d] is not building", indexSlot);
            rc = SDB_VESSEL_INDEX_BUILDING_TERMINATED;
            goto error;
         }

         do
         {
            if (!uic->getNextRebuildingRangeBound(entry))
            {
               PD_LOG(PDERROR, "failed to get next building range");
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
            else if (entry.getSeq() == _totalRdpCount)
            {
               break;
            }

            rc = buildIndexBySortingAndUpdateContext(context, uic, _totalRdpCount, mb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build index[%d], rc:%d", indexSlot, rc);
               goto error;
            }
         } while(TRUE);

         rc = endToBuildIndex(context, indexSlot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to finish building index[%], rc:%d", indexSlot, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 collection::mergeSorterAndContextIntoIndex(requestContext *context,
                                                    _dmsIxmKeySorter *sorter,
                                                    unstableIndexContext *uic)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != sorter, "can not be null");
      SDB_ASSERT(NULL != uic, "can not be null");
      SDB_ASSERT(INDEX_STATUS_BUILDING == uic->getStatus(), "must be building");

      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      ossPoolList<unstableIndexContext::keyOperation> deltas;

      indexConsole console;
      rc = console.init(&_record, &is);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index console:%d", rc);
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

         rc = console.insert(context, uic->getIndexSlot(),
                             uic->getIndexObj(),
                             key, DPS_TRANS_ID(),
                             context->getSession()->getLastLSN(),
                             dmsRid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert key into index[%s]:%d",
                   uic->getName().str(), rc);
            goto error;
         }
      } while (TRUE);

      while (!uic->endToBuildCurrentRangeOrPopKeys(deltas))
      {
         SDB_ASSERT(FALSE, "todo");
      }
      
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 collection::endToBuildIndex(requestContext *context,
                                     INT32 indexSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");

      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;

      rc = console.init(&_record, &is);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init console:%d", rc);
         goto error;
      }

      rc = console.updateIndexStatus(context, indexSlot, INDEX_STATUS_NORMAL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update index[%d] status to normal:%d", indexSlot, rc);
         goto error;
      }

      _unstableIndexes.erase(indexSlot);
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(pattern.isValid(), "can not be invalid");

      duplicated = FALSE;
      indexSpace &is = _collectionSpace->getSU()->getIndexSpace();
      indexConsole console;
      UINT64 indexes = 0;

      rc = console.init(&_record, &is);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index console:%d", rc);
         goto error;
      }

      indexes = (_record.uniqueIndexes | _record.nonUniqueIndexes);
      for (INT32 i = 0; i < (INT32)MAX_INDEX_COUNT_PER_CL; ++i)
      {
         unstableIndexContext *uic = NULL;
         UINT64 mask = (UINT64)1 << i;
         if (0 == OSS_BIT_TEST(indexes, mask))
         {
            continue;
         }

         uic = _unstableIndexes.find(i);
         if (NULL == uic)
         {
            rc = console.testIfDuplicated(context, i, indexName, pattern, duplicated);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to test index slot[%d], rc:%d", i, rc);
               goto error;
            }

            if (duplicated)
            {
               goto done;
            }
         }
         else if (uic->testIfDuplicatedIfBuilding(indexName, pattern))
         {
            duplicated = TRUE;
            goto done;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine