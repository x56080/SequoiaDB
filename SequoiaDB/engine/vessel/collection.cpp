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
#include "vessel/impAccessor.h"
#include "vessel/idMapPage.h"
#include "vessel/storageUnit.h"
#include "dpsLogRecord.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/instanceEnv.h"
#include "vessel/crpAccessor.h"
#include "vessel/collectionSpace.h"
#include "dpsLogRecord.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/outerResource.h"
#include "vessel/lpidLockHelper.h"
#include "vessel/smpAccessor.h"
#include "vessel/insertContext.h"
#include "vessel/routePage.h"
#include "vessel/routePageAccessor.h"
#include "vessel/rdpAccessor.h"
#include "vessel/scanCLCursor.h"
#include "vessel/scanCLContext.h"

namespace engine
{
namespace vessel
{
   collection::collection():
   _collectionSpace(NULL),
   _maxPageCntInRoutePages(0),
   _pageCntInRoutePages(0)
   {
      
   }

   collection::~collection()
   {

   }

   INT32 collection::initWhenOpen(requestContext *context,
                                  const collectionRecord &record,
                                  collectionSpace *cs)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _collectionSpace, "do not reinit");
      UINT32 pageSize = 0;
      fsmFile *file = NULL;

      if (OSS_UNLIKELY(NULL == cs))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(COLLECTION_RECORD_VERSION != record.version))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_MB_ID == record.mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = cs->getMainDataSpace()->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = cs->ensureFsmFile(context, &file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure fsm file:%d", rc);
         goto error;
      }
      
      _record = record;
      _collectionSpace = cs;

      rc = initPageSequenceWhenOpen(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page sequence:%d", rc);
         goto error;
      }

      rc = _fsm.open(file, record.mbID,
                     record.logicalCLID, pageSize,
                     record.freeSizeReserved, 1 < record.maxSGCount,
                     record.minStriping, record.maxStriping);
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
      UINT32 pageSize = 0;
      SDB_ASSERT(NULL == _collectionSpace, "do not reinit");

      if (OSS_UNLIKELY(NULL == context ||
                       !context->mbLocked() ||
                       EXCLUSIVE != context->getMBLockMode() ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       clName.empty() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       NULL == cs ||
                       !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL != _collectionSpace)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = cs->getMainDataSpace()->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      fini();

      rc = cs->ensureFsmFile(context, &fsm);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create fsm file:%d", rc);
         goto error;
      }

      _record.version = COLLECTION_RECORD_VERSION;
      _record.mbID = context->getMBID();
      _record.innerID = innerID;
      _record.type = options.type;
      _record.logicalCLID = logicalID;
      _record.freeSizeReserved = options.freeSizeReserved;
      _record.minStriping = options.minStriping;
      _record.maxStriping = options.maxStriping;
      ossMemcpy(_record.name, clName.str(), clName.strLen());
      _record.maxSGCount = options.multiStripingBucket ? 32 : 1;
      _record.compressionType = options.compressionType;
      _collectionSpace = cs;


      rc = _fsm.create(fsm, context->getMBID(), logicalID,
                       pageSize, options.freeSizeReserved,
                       options.multiStripingBucket,
                       options.minStriping,
                       options.maxStriping);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create free space map of mb[%d], rc:%d",
                context->getMBID(), rc);
         goto error;
      }

      rc = saveOnDiskWhenCreating(context);
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
      _maxPageCntInRoutePages = 0;
      _pageCntInRoutePages = 0;
      _fsm.close();
      return;
   }

   INT32 collection::createIndex(requestContext *context,
                                 const strSlice &indexName,
                                 const indexKeyPattern &keyPattern,
                                 const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      ossScopedLock guard(&_ddlSLatch, EXCLUSIVE);
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::saveOnDiskWhenCreating(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->getSpaceIDLocked(), "must be locked");
      SDB_ASSERT(context->getSpaceID() == _collectionSpace->getSpaceID(), "must be same");
      SDB_ASSERT(INVALID_CL_MB_ID != getMBID(), "can not be invlalid");
      PAGE_ID lpid = INVALID_PAGE_ID;
      PAGE_ID pid = INVALID_PAGE_ID;
      lpidLockHelper lh;

      rc = _collectionSpace->getMainDataSpace()->getLpidOfCollectionRecord(getMBID(), lpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lpid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, FILE_TYPE_DD, lpid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = ensureCLRecordPageAllocated(context, lpid, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = saveCLRecordWhenCreating(context, lpid);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      lh.unlock();
      return rc;
   error:
      goto done;
   }

   INT32 collection::dump(requestContext *context,
                          listCollectionsRecord &record)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(context->mbLocked(), "must locked");
      SDB_ASSERT(context->getMBID() == _record.mbID, "must be same");

      record.version = _record.version;
      ossMemcpy(record.name, _record.name, ossStrlen(_record.name) + 1);
      record.csUniqueID = _collectionSpace->getUniqueID();
      record.clInnerID = _record.innerID;
      record.clLogicalID = _record.logicalCLID;
      record.spaceID = _collectionSpace->getSpaceID();
      record.mbID = _record.mbID;
      record.maxSGCount = _record.maxSGCount;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insert(insertContext *context,
                            const recordData &record,
                            const DPS_TRANS_ID &transID,
                            STRIPING_ID striping,
                            const insertOptions *options,
                            utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      UINT32 recordSize = 0;
      UINT32 pageSize = 0;

      ossScopedLock guard(&_ddlSLatch, SHARED);

      rc = _collectionSpace->getMainDataSpace()->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      context->initNewRequest(_collectionSpace, this, record, transID, striping);
      if (NULL != options)
      {
         context->setOptions(*options);
      }

      if (UTIL_COMPRESSOR_INVALID != _record.compressionType)
      {
         SDB_ASSERT(FALSE, "todo");
      }

      recordSize = context->getRecord().getSlice().len();
      if (!isBigRecordInRdp(pageSize, recordSize))
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

   INT32 collection::getRecordCount(requestContext *context,
                                    IQueryFilter *filter,
                                    UINT64 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      count = 0;
      ISession *session = context->getSession();
      const static UINT32 _QUIT_CHECK = 16 - 1;

      for (UINT32 i = 0; i < _pageCntInRoutePages; ++i)
      {
         PAGE_ID lpid = INVALID_PAGE_ID;
         UINT32 cnt = 0;
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

         rc = getRecordCountOfPage(context, lpid, filter, cnt);
         if (SDB_OK != rc)
         {
            goto error;
         }

         count += cnt;
      }
   done:
      return rc;
   error:
      count = 0;
      goto done;
   }

   INT32 collection::getMoreWhenScan(scanCLContext *context,
                                     scanCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != cursor, "can not be null");
      SDB_ASSERT(cursor->isOpen(), "must be open");
      SDB_ASSERT(cursor->getHandle().getCLLId() == _record.logicalCLID, "must be same");

      do
      {
         if (_pageCntInRoutePages <= cursor->getPageSeq())
         {
            cursor->pushEnd();
            goto done;
         }

         rc = getMoreFromSeqInCursor(context, cursor);
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

         cursor->incPageSeqAndResetRid();

      } while(TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::getRecordCountOfPage(requestContext *context,
                                          PAGE_ID lpid,
                                          IQueryFilter *filter,
                                          UINT32 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      pageAccessor::options o;
      o.cacheMode = TRUE;
      rdpAccessor accessor;
      rc = accessor.init(context, lpid, o,
                         _collectionSpace->getMainDataSpace());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rdp accessor:%d", rc);
         goto error;
      }

      if (NULL == filter)
      {
         rc = accessor.getRecordCount(context, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get record count of page[%d], rc:%d", lpid, rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "TODO");
      }

      accessor.fini(context);
   done:
      return rc;
   error:
      accessor.fini(context);
      count = 0;
      goto done;
   }

   INT32 collection::getMoreFromSeqInCursor(scanCLContext *context,
                                            scanCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      rdpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      PAGE_ID lpid = INVALID_PAGE_ID;
      lpid = cursor->getLpid();

      if (INVALID_PAGE_ID == lpid)
      {
         rc = getLpidBySequence(context, cursor->getPageSeq(), lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid of seq[%d], rc:%d",
                   cursor->getPageSeq(), rc);
            goto error;
         }

         cursor->setLpid(lpid);
      }

      rc = accessor.init(context, lpid, o,
                         _collectionSpace->getMainDataSpace());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init accessor of lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = accessor.getMoreWhenScan(context, cursor);
      if (SDB_OK != rc)
      {
         goto error;
      }

      accessor.fini(context);
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 collection::insertNonBigRecord(insertContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      UINT32 recordSize = context->getRecord().getSlice().len();

      do
      {
         fsmCandidate &candidate = context->getCandidate();
         context->setLastFreeSize(0);
         rc = findFreePageForRecord(static_cast<requestContext*>(context),
                                    recordSize, context->getStriping(),
                                    candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free space for record:%d", rc);
            goto error;
         }

         if (INVALID_PAGE_ID == candidate.lpid)
         {
            rc = getLpidBySequence(context, candidate.seq, candidate.lpid);
            if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS == rc)
            {
               PD_LOG(PDERROR, "sequence[%d] not exists in collection", candidate.seq);
               rc = SDB_OK;
               _fsm.updateBucket(candidate.seq, candidate.lpid, candidate.bucket,
                                 candidate.free, 0, TRUE);
               continue;
            }
            else if (SDB_OK != rc)
            {
               goto error;
            }
         }

         rc = insertNonBigRecordToCandidate(context);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            PD_LOG(PDWARNING, "page seq[%] free size may be not correct", candidate.seq);
            if (candidate.testFeedback())
            {
               _fsm.updateBucket(candidate.seq, candidate.lpid, candidate.bucket,
                                 candidate.free, context->getLastFreeSize(), TRUE);
            }
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert record to page:%d", rc);
            goto error;
         }

         if (candidate.testFeedback())
         {
            _fsm.updateBucket(candidate.seq, candidate.lpid, candidate.bucket,
                              candidate.free, context->getLastFreeSize(), FALSE);
         }

         break;
      }while(TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertNonBigRecordToCandidate(insertContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      const fsmCandidate &candidate = context->getCandidate();
      SDB_ASSERT(candidate.isValid(), "must be valid");
      SDB_ASSERT(INVALID_PAGE_ID != candidate.lpid, "can not be invalid");
      rdpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;

      rc = accessor.init(context, candidate.lpid, o,
                         _collectionSpace->getMainDataSpace());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor of lpid[%d], rc:%d",
                candidate.lpid, rc);
         goto error;
      }

      rc = accessor.insertNormalRecord(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert record to lpid[%d], rc:%d",
                candidate.lpid, rc);
         goto error;
      }

      
   done:
      accessor.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collection::findFreePageForRecord(requestContext *context,
                                           UINT32 recordSize,
                                           STRIPING_ID striping,
                                           fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < recordSize, "impossible");
      candidate.reset();

      rc = _fsm.fastFind(striping, recordSize, candidate);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_FSM_NO_FREE_SPACE != rc)
      {
         PD_LOG(PDERROR, "failed to find free space for record:%d", rc);
         goto error;
      }
      else
      {
         UINT32 flags = freeSpaceMap::FIND_ALL;
         ossScopedLock guard(&_pageAllocLatch);
         do
         {
            rc = _fsm.findInWholeMap(striping, recordSize,
                                     flags, candidate);
            if (SDB_VESSEL_FSM_NO_FREE_SPACE == rc)
            {
               PAGE_ID lpids[PAGE_COUNT_IN_EXTENT] = {INVALID_PAGE_ID};
               CL_PAGE_SEQ firstSeq = INVALID_CL_PAGE_SEQ;
               rc = allocateNewRecordDataPages(context, PAGE_COUNT_IN_EXTENT,
                                               firstSeq, lpids);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
                  goto error;
               }

               rc = _fsm.addNewPages(firstSeq, lpids, PAGE_COUNT_IN_EXTENT);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to add new pages to fsm:%d", rc);
                  goto error;
               }

               flags = freeSpaceMap::FIND_NEW_POOL;
               continue;
            }
            else if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find free space from fsm:%d", rc);
               goto error;
            }

            break;
         }while (TRUE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::allocateNewRecordDataPages(requestContext *context,
                                                UINT32 count,
                                                CL_PAGE_SEQ &firstSeq,
                                                PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(8 == count, "capacity of route page is 32bytes alienged");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(_pageCntInRoutePages <= _maxPageCntInRoutePages, "impossible");

      UINT32 capacity = 0;
      PAGE_ID lvl0Lpid = INVALID_PAGE_ID;
      PAGE_ID lvl0Pid = INVALID_PAGE_ID;
      UINT32 lvl0No = 0;
      PAGE_ID pids[PAGE_COUNT_IN_EXTENT] = {INVALID_PAGE_ID};
      UINT32 pageSize = 0;
      BOOLEAN rollbackPre = FALSE;
      BOOLEAN rollbackPages = FALSE;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      slice args;
      routePageAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();

      rc = _collectionSpace->getMainDataSpace()->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }
      capacity = getCapacityOfRoutePage(pageSize);
      if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (_maxPageCntInRoutePages == _pageCntInRoutePages)
      {
         rc = extendRoutePageMap(context, &lvl0Lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new route page:%d", rc);
            goto error;
         }
      }
      else
      {
         lvl0No = _pageCntInRoutePages / capacity;
         rc = getLvl0RoutePage(context, capacity, lvl0No, lvl0Lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lvl0[%d] page, rc:%d", lvl0No, rc);
            goto error;
         }
      }

      rc = ms->preallocatePages(context, count, lpids, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate pages:%d", rc);
         goto error;
      }

      rollbackPre = TRUE;
      rc = initNewRecordDataPages(context, count, _pageCntInRoutePages, lpids, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init pages:%d", rc);
         goto error;
      }

      args.reset(sizeof(_pageCntInRoutePages), (const CHAR *)(&_pageCntInRoutePages));
      rc = ms->allocatePages(context, PAGE_TYPE_RECORD,
                             count, lpids, pids, args, &lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate data pages:%d", rc);
         goto error;
      }

      rollbackPre = FALSE;
      rollbackPages = TRUE;

      rc = accessor.init(context, lvl0Lpid, o, ms, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ini accessor:%d", rc);
         goto error;
      }

      rc = accessor.appendSlots(context, _record.logicalCLID,
                                 _pageCntInRoutePages %capacity,
                                 count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append new page to lvl0 page[%d], rc:%d",
                lvl0Pid, rc);
         goto error;
      }

      accessor.fini(context);

      firstSeq = _pageCntInRoutePages;
      _pageCntInRoutePages += count;
   done:
      return rc;
   error:
      accessor.fini(context);
      if (rollbackPre)
      {
         ms->releasePagesPreallocated(context, count, lpids, pids);
      }
      if (rollbackPages)
      {
         INT32 tmpRc = ms->releasePages(context, count, lpids, lsn);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to rollback allocating, rc:%d", rc);
            IRedoLogger *logger = context->getOuterResource()->logger;
            logger->abortOplist(context->getSession(), lsn);
         }
      }
      ossMemset(lpids, 0xFF, (count << 2));
      goto done;
   }
   

   INT32 collection::initPageSequenceWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_CL_MB_ID != _record.mbID, "can not be invalid");
      
      UINT32 capacity = 0;
      UINT32 pageSize = 0;
      UINT32 max = 0;
      UINT32 count = 0;
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();
      rc = ms->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      capacity = getCapacityOfRoutePage(pageSize);
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (INVALID_PAGE_ID == _record.routePages[COLLECTION_ROOT_LVL0])
      {
         /// do nothing.
      }
      else if (INVALID_PAGE_ID == _record.routePages[COLLECTION_FIRST_ROOT_LVL1])
      {
         rc = getPageCntOfRoutePage(context, TRUE, capacity,
                                    _record.routePages[COLLECTION_ROOT_LVL0],
                                    COLLECTION_ROUTE_PAGE_LVL0,
                                    max, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page count:%d", rc);
            goto error;
         }
      }
      else if (INVALID_PAGE_ID == _record.routePages[COLLECTION_SECOND_ROOT_LVL1])
      {
         rc = getPageCntOfRoutePage(context, TRUE, capacity,
                                    _record.routePages[COLLECTION_FIRST_ROOT_LVL1],
                                    COLLECTION_ROUTE_PAGE_LVL1,
                                    max, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page count:%d", rc);
            goto error;
         }
         max += capacity;
         count += capacity;
      }
      else if (INVALID_PAGE_ID == _record.routePages[COLLECTION_ROOT_LVL2])
      {
         UINT32 delta = (1 + capacity) * capacity;
         rc = getPageCntOfRoutePage(context, TRUE, capacity,
                                    _record.routePages[COLLECTION_SECOND_ROOT_LVL1],
                                    COLLECTION_ROUTE_PAGE_LVL1,
                                    max, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page count:%d", rc);
            goto error;
         }

         
         max += delta;
         count += delta;
      }
      else
      {
         UINT32 delta = (1 + capacity + capacity) * capacity;
         rc = getPageCntOfRoutePage(context, TRUE, capacity,
                                    _record.routePages[COLLECTION_ROOT_LVL2],
                                    COLLECTION_ROUTE_PAGE_LVL2,
                                    max, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page count:%d", rc);
            goto error;
         }

         max += delta;
         count += delta;
      }

      _maxPageCntInRoutePages = max;
      _pageCntInRoutePages = count;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::getPageCntOfRoutePage(requestContext *context,
                                           BOOLEAN direct,
                                           UINT32 capacity,
                                           PAGE_ID lpid,
                                           UINT32 lvl,
                                           UINT32 &maxPageCnt,
                                           UINT32 &pageCnt)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 != capacity, "can not be zero");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(COLLECTION_MIN_ROUTE_LVL <= lvl, "can not be invalid");
      SDB_ASSERT(lvl <= COLLECTION_MAX_ROUTE_LVL, "can not be invalid");
      UINT32 max = 0;
      UINT32 count = 0;
      UINT32 subMax = 0;
      UINT32 subCount = 0;
      PAGE_ID last = INVALID_PAGE_ID;
      UINT32 slot = 0;

      maxPageCnt = 0;
      pageCnt = 0;

      if (COLLECTION_ROUTE_PAGE_LVL0 == lvl)
      {
         maxPageCnt = capacity;
      }

      rc = getLastElementInRoutePage(context, direct, lpid, last, slot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get last element:%d", rc);
         goto error;
      }

      if (INVALID_PAGE_ID == last)
      {
         goto done;
      }

      if (COLLECTION_ROUTE_PAGE_LVL0 == lvl)
      {
         pageCnt = slot + 1;
         goto done;
      }
      else if (COLLECTION_ROUTE_PAGE_LVL1 == lvl)
      {
         max = capacity * slot;
         count = max;
      }
      else if (COLLECTION_ROUTE_PAGE_LVL2 == lvl)
      {
         max = capacity * capacity * slot;
         count = max;
      }

      rc = getPageCntOfRoutePage(context, direct, capacity, last,
                                 lvl - 1, subMax, subCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page count of sub page:%d", rc);
         goto error;
      }

      maxPageCnt = max + subMax;
      pageCnt = count + subCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::initNewRecordDataPages(requestContext *context,
                                            UINT32 count,
                                            CL_PAGE_SEQ firstSeq,
                                            const PAGE_ID *lpids,
                                            const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != firstSeq, "can not be invalid");
      UINT32 pageSize = 0;
      ossValuePtr ptr = 0;
      storageUnit *su = _collectionSpace->getMainDataSpace()->getSU();

      rc = _collectionSpace->getMainDataSpace()->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         if (OSS_UNLIKELY(INVALID_PAGE_ID == pids[i] ||
                          INVALID_PAGE_ID == lpids[i]))
         {
            PD_LOG(PDERROR, "invalid page id");
            goto error;
         }

         rc = su->getPagePtr(FILE_TYPE_DD, pids[i], ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page ptr of[%d], rc:%d", pids[i], rc);
            goto error;
         }

         initRecordDataPage(pageSize, lpids[i], getLogicalID(),
                            firstSeq + i, (void *)ptr);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::getLpidBySequence(requestContext *context,
                                       CL_PAGE_SEQ sequence,
                                       PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != sequence, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      UINT32 pageSize = 0;
      UINT32 capacity = 0;
      UINT32 lvl0Id = 0;
      PAGE_ID lvl0Lpid = INVALID_PAGE_ID;
      routePageAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();

      lpid = INVALID_PAGE_ID;

      if (_pageCntInRoutePages <= sequence)
      {
         PD_LOG(PDERROR, "invalid page sequence[%d], current max page count[%d]",
                sequence, _pageCntInRoutePages);
         rc = SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS;
         goto error;
      }

      rc = ms->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      capacity = getCapacityOfRoutePage(pageSize);
      lvl0Id = sequence / capacity;

      rc = getLvl0RoutePage(context, capacity, lvl0Id, lvl0Lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lvl0 page[%d], rc:%d", lvl0Lpid, rc);
         goto error;
      }

      rc = accessor.init(context, lvl0Lpid, o, ms);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init route accessor of lpid[%d], rc:%d", lvl0Lpid, rc);
         goto error;
      }

      rc = accessor.readSlot(context, sequence % capacity, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read slot of sequence[%d], rc:%d", sequence, rc);
         goto error;
      }

      accessor.fini(context);
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 collection::getLastElementInRoutePage(requestContext *context,
                                               BOOLEAN direct,
                                               PAGE_ID lpid,
                                               PAGE_ID &element,
                                               UINT32 &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      pageAccessor::options o;
      o.cacheMode = !direct;
      routePageAccessor accessor;
      rc = accessor.init(context, lpid, o,
                         _collectionSpace->getMainDataSpace());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.readLastSlot(context, element, slot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read last slot of lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      
      
   done:
      accessor.fini(context);
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
      PAGE_ID lvl1Lpid = INVALID_PAGE_ID;
      UINT32 slot = 0;
      routePageAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;

      lpid = INVALID_PAGE_ID;
      if (0 == lvl0No)
      {
         lpid = _record.routePages[COLLECTION_ROOT_LVL0];
         if (INVALID_PAGE_ID == lpid)
         {
            rc = SDB_VESSEL_PAGE_NOT_EXISTS;
            goto error;
         }
         goto done;
      }
      else if (lvl0No < (1 + capacity + capacity))
      {
         lvl1Lpid = (lvl0No < (1 + capacity)) ?
                    _record.routePages[COLLECTION_FIRST_ROOT_LVL1] :
                    _record.routePages[COLLECTION_SECOND_ROOT_LVL1];
         if (INVALID_PAGE_ID == lvl1Lpid)
         {
            rc = SDB_VESSEL_PAGE_NOT_EXISTS;
            goto error;
         }
      }
      else if (lvl0No < getMaxLvl0Cnt(capacity))
      {
         PAGE_ID lvl2Pid = _record.routePages[COLLECTION_ROOT_LVL2];
         if (INVALID_PAGE_ID == lvl2Pid)
         {
            rc = SDB_VESSEL_PAGE_NOT_EXISTS;
            goto error;
         }

         rc = accessor.init(context, lvl2Pid, o,
                            _collectionSpace->getMainDataSpace());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         slot = (lvl0No - 1 - capacity - capacity) / capacity;
         rc = accessor.readSlot(context, slot, lvl1Lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read slot[%d] of lvl2:%d", slot, rc);
            goto error;
         }

         accessor.fini(context);
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = accessor.init(context, lvl1Lpid, o,
                         _collectionSpace->getMainDataSpace());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      slot = (lvl0No - 1) % capacity;
      rc = accessor.readSlot(context, slot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read slot[%d] of lvl2:%d", slot, rc);
         goto error;
      }

      accessor.fini(context);
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 collection::extendRoutePageMap(requestContext *context,
                                        PAGE_ID *newLvl0)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(_maxPageCntInRoutePages == _pageCntInRoutePages, "impossible");

      UINT32 pageSize = 0;
      UINT32 capacity = 0;
      UINT32 lvl0Cnt = 0;
      UINT32 maxLvl0Cnt = 0;
      PAGE_ID newLvl0Lpid = INVALID_PAGE_ID;
      rc = _collectionSpace->getMainDataSpace()->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get page size:%d", rc);
         goto error;
      }

      capacity = getCapacityOfRoutePage(pageSize);
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get route page capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(0 == _maxPageCntInRoutePages % capacity, "impossible");
      lvl0Cnt = _maxPageCntInRoutePages / capacity;
      maxLvl0Cnt = getMaxLvl0Cnt(capacity);
      
      if (0 == lvl0Cnt)
      {
         rc = createRootRoutePage(context, COLLECTION_ROOT_LVL0);
         if (SDB_OK != rc)
         {
            goto error;
         }
         newLvl0Lpid = _record.routePages[COLLECTION_ROOT_LVL0];
      }
      else if (lvl0Cnt < (1 + capacity))
      {
         if (INVALID_PAGE_ID == _record.routePages[COLLECTION_FIRST_ROOT_LVL1])
         {
            rc = createRootRoutePage(context, COLLECTION_FIRST_ROOT_LVL1);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }

         rc = createNonRootRoutePage(context,
                                     _record.routePages[COLLECTION_FIRST_ROOT_LVL1],
                                     (lvl0Cnt - 1 ) % capacity,
                                     newLvl0Lpid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (lvl0Cnt < (1 + capacity + capacity))
      {
         if (INVALID_PAGE_ID == _record.routePages[COLLECTION_SECOND_ROOT_LVL1])
         {
            rc = createRootRoutePage(context, COLLECTION_SECOND_ROOT_LVL1);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
         rc = createNonRootRoutePage(context,
                                     _record.routePages[COLLECTION_SECOND_ROOT_LVL1],
                                     (lvl0Cnt - 1 ) % capacity,
                                     newLvl0Lpid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (lvl0Cnt < maxLvl0Cnt)
      {
         PAGE_ID lvl1Lpid = INVALID_PAGE_ID;
         UINT32 slot = 0;
         if (INVALID_PAGE_ID == _record.routePages[COLLECTION_ROOT_LVL2])
         {
            rc = createRootRoutePage(context, COLLECTION_ROOT_LVL2);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }

         slot = (lvl0Cnt - 1 - capacity - capacity) / capacity;
         rc = ensureNonRootLvl1RoutePage(context, slot, lvl1Lpid);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = createNonRootRoutePage(context,
                                     lvl1Lpid,
                                     (lvl0Cnt - 1 ) % capacity,
                                     newLvl0Lpid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (lvl0Cnt == maxLvl0Cnt)
      {
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }
      else
      {
         PD_LOG(PDERROR, "lvl0 count[%d] in chaos", lvl0Cnt);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (NULL != newLvl0)
      {
         *newLvl0 = newLvl0Lpid;
      }

      _maxPageCntInRoutePages += capacity;

   done:
      return rc;
   error:
      goto done;
   }


   INT32 collection::createRootRoutePage(requestContext *context,
                                         UINT32 rootSlot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(COLLECTION_MIN_ROUTE_ROOT <= rootSlot &&
                 rootSlot <= COLLECTION_MAX_ROUTE_ROOT, "impossible");
      SDB_ASSERT( INVALID_PAGE_ID == _record.routePages[rootSlot], "must be invalid");
      SDB_ASSERT(0 == _maxPageCntInRoutePages, "must be zero");
      PAGE_ID lpidOfRP = INVALID_PAGE_ID;
      PAGE_ID crpLpid = INVALID_PAGE_ID;

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      /// we are holding ddl s latch and page allocating x latch.
      /// all columns in _record are unchangeable now.
      collectionRecord record = _record;
      crpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;
      o.oplistTail = TRUE;
      UINT64 mask = COLLECTION_UPDATE_MASK_ROUTE_PAGES;
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();

      if(OSS_UNLIKELY(rootSlot < COLLECTION_MIN_ROUTE_ROOT ||
                      COLLECTION_MAX_ROUTE_ROOT < rootSlot ||
                      INVALID_PAGE_ID != _record.routePages[rootSlot]))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = createNewRoutePage(context, lpidOfRP, &lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create root lvl0 page:%d", rc);
         goto error;
      }
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");

      rc = ms->getLpidOfCollectionRecord(getMBID(), crpLpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lpid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      record.routePages[rootSlot] = lpidOfRP;
      rc = accessor.init(context, crpLpid, o, ms, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init crp accessor:%d", rc);
         goto error;
      }

      rc = accessor.update(context, LOG_TYPE_DUMMY, mask, record, slice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update cl record[%d], rc:%d", record.mbID, rc);
         goto error;
      }

      accessor.fini(context);
      _record.routePages[rootSlot] = lpidOfRP;
   done:
      return rc;
   error:
      accessor.fini(context);
      if (INVALID_PAGE_ID != lpidOfRP)
      {
         INT32 tmpRc = ms->releasePages(context, 1, &lpidOfRP, lsn);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to rollback allocating[%d], rc:%d", lpidOfRP, rc);
            IRedoLogger *logger = context->getOuterResource()->logger;
            logger->abortOplist(context->getSession(), lsn);
         }
      }
      goto done;
   }

   INT32 collection::createNonRootRoutePage(requestContext *context,
                                            PAGE_ID lpid,
                                            UINT32 slot,
                                            PAGE_ID &lpidOfRP)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      PAGE_ID newLpid = INVALID_PAGE_ID;
      DPS_LSN_OFFSET oplist = DPS_INVALID_LSN_OFFSET;
      routePageAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();

      rc = createNewRoutePage(context, newLpid, &oplist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new route page:%d", rc);
         goto error;
      }

      rc = accessor.init(context, newLpid, o, ms, oplist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.appendSlots(context, _record.logicalCLID,
                                slot, 1, &newLpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append page[%d] to route page:%d", newLpid, rc);
         goto error;
      }

      accessor.fini(context);
      lpidOfRP = newLpid;
   done:
      return rc;
   error:
      accessor.fini(context);
      if (INVALID_PAGE_ID != newLpid)
      {
         INT32 tmpRc = ms->releasePages(context, 1, &newLpid, oplist);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to rollback allocating[%d], rc:%d", newLpid, rc);
            IRedoLogger *logger = context->getOuterResource()->logger;
            logger->abortOplist(context->getSession(), oplist);
         }
      }
      goto done;
   }

   INT32 collection::ensureNonRootLvl1RoutePage(requestContext *context,
                                                UINT32 slot,
                                                PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      routePageAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;

      PAGE_ID lvl2Lpid = _record.routePages[COLLECTION_ROOT_LVL2];
      if (INVALID_PAGE_ID == lvl2Lpid)
      {
         PD_LOG(PDERROR, "lvl2 page does not exist");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = accessor.init(context, lvl2Lpid, o,
                         _collectionSpace->getMainDataSpace());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.readSlot(context, slot, lpid);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS == rc)
      {
         ///create new one.
         rc = SDB_OK;
      }
      else
      {
         PD_LOG(PDERROR, "failed to read slot[%d] on route page:%d", slot, rc);
         goto error;
      }

      accessor.fini(context);

      rc = createNonRootRoutePage(context, lvl2Lpid, slot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faile to create new route page:%d", rc);
         goto error;
      }

   done:
      accessor.fini(context);
      return rc;
   error:
      lpid = INVALID_PAGE_ID;
      goto done;
   }

   INT32 collection::createNewRoutePage(requestContext *context,
                                        PAGE_ID &lpidOfRP,
                                        DPS_LSN_OFFSET *lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID lpid = INVALID_PAGE_ID;
      ossValuePtr ptr = 0;
      UINT32 pageSize = 0;
      slice s;
      UINT32 logicalId = _record.logicalCLID;
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();

      rc = ms->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = ms->preallocatePages(context, 1, &lpid, &pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate data page:%d", rc);
         goto error;
      }

      rc = ms->getSU()->getPagePtr(FILE_TYPE_DD, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr, rc:%d", pid, rc);
         goto error;
      }

      if (!initRoutePage(pageSize, lpid, logicalId, (void *)ptr))
      {
         PD_LOG(PDERROR, "failed to init route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      s.reset(sizeof(logicalId), (const CHAR *)(&logicalId));

      rc = ms->allocatePages(context, PAGE_TYPE_ROUTE,
                             1, &lpid, &pid, s, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate pages:%d", rc);
         goto error;
      }

      lpidOfRP = lpid;
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         ms->releasePagesPreallocated(context, 1, &lpid, &pid);
      }
      goto done;
   }

   INT32 collection::ensureCLRecordPageAllocated(requestContext *context,
                                                 PAGE_ID lpid,
                                                 PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(context->testLpidLockMode(FILE_TYPE_DD, lpid, EXCLUSIVE), "must holding lock");

      mainDataSpace *ms = _collectionSpace->getMainDataSpace();
      rc = ms->getPhysicalPid(context, lpid, pid, NULL);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_LOGICAL_PAGE_UNMAPPED == rc)
      {
         rc = allocatePageForCLRecord(context, lpid, pid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         PD_LOG(PDERROR, "failed to get phy pid of lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::allocatePageForCLRecord(requestContext *context,
                                             PAGE_ID lpid,
                                             PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(context->testLpidLockMode(FILE_TYPE_DD, lpid, EXCLUSIVE),
                 "must holding lock");
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();
      rc = preallocateCLRecordPage(context, lpid, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = ms->allocatePages(context, PAGE_TYPE_COLLECTION_RECORD,
                             1, &lpid, &pid, slice(), NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate data pages:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         ms->releasePhysicalPidsPreallocated(context, 1, &pid);
         pid = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 collection::preallocateCLRecordPage(requestContext *context,
                                             PAGE_ID lpid,
                                             PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");

      UINT32 pageSize = 0;
      ossValuePtr ptr = 0;
      pid = INVALID_PAGE_ID;
      mainDataSpace *ms = _collectionSpace->getMainDataSpace();

      rc = ms->getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = ms->preallocatePhysicalPids(context, 1, &pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate pid:%d", rc);
         goto error;
      }

      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

      rc = ms->getSU()->getPagePtr(FILE_TYPE_DD, pid, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!initCollectionRecordPage(pageSize, lpid, (void *)ptr))
      {
         PD_LOG(PDERROR, "failed to init crp page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = ms->getSU()->fsync(FILE_TYPE_DD, pid, 1, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         ms->releasePhysicalPidsPreallocated(context, 1, &pid);
         pid = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 collection::saveCLRecordWhenCreating(requestContext *context, PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      crpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;
      rc = accessor.init(context, lpid, o,
                         _collectionSpace->getMainDataSpace());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup crp accessor:%d", rc);
         goto error;
      }

      rc = accessor.createCL(context, _record);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      accessor.fini(context);
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine