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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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

   INT32 collection::initWhenOpen(const collectionRecord &record,
                                  collectionSpace *cs)
   {
      INT32 rc = SDB_OK;
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
      
      _record = record;
      _collectionSpace = cs;
   done:
      return rc;
   error:
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
      BOOLEAN rollback = FALSE;
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

      rollback = TRUE;
      _record.version = COLLECTION_RECORD_VERSION;
      _record.mbID = context->getMBID();
      _record.innerID = innerID;
      _record.type = options.type;
      _record.logicalCLID = logicalID;
      ossStrncpy(_record.name, clName.str(), clName.strLen());
      _record.maxSGCount = options.maxStripingGroupCount;
      _record.compressionType = options.compressionType;
      _collectionSpace = cs;

      rc = saveOnDiskWhenCreating(context);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         fini();
      }
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

      rc = _collectionSpace->getLpidOfClRecord(getMBID(), lpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lpid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE);
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

      rc = saveCLRecordWhenCreating(context, pid);
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
                            utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      storageUnit *su = NULL;
      UINT32 pageSize = 0;
      UINT32 originalRecordSize = 0;
      ossScopedLock guard(&_ddlSLatch, SHARED);

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->clInfoIsValid() ||
                            !context->getRecord().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      su = _collectionSpace->getSU();
      rc = su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      originalRecordSize = context->getRecord().getSlice().len();
      if (!isBigRecordInRdp(pageSize, originalRecordSize))
      {
         rc = insertNonBigRecord(context, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert record:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::insertNonBigRecord(insertContext *context,
                                        utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      UINT32 recordSize = 0;

      if (UTIL_COMPRESSOR_INVALID == context->getCompressionType())
      {
         recordSize = context->getRecord().getSlice().len();
      }
      else if (UTIL_COMPRESSOR_LZW == context->getCompressionType())
      {
         SDB_ASSERT(FALSE, "todo");
         recordSize = context->getCompressedRecordSize();
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         rc = findFreePageForRecord(static_cast<requestContext*>(context),
                                    recordSize, context->getStriping(),
                                    context->getCandidate());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free space for record:%d", rc);
            goto error;
         }
      }while(TRUE);
      
   done:
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
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY |
                     PAGE_ACCESSOR_FLAG_OPLIST_TAIL;
      lpidLockHelper lh;
      routePageAccessor accessor;

      rc = _collectionSpace->getSU()->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }
      capacity = getCapacityOfRoutePage(pageSize);
      if (OSS_UNLIKELY(0 == capacity))
      {
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

      rc = _collectionSpace->preallocateDataPages(context, count, lpids, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate pages:%d", rc);
         goto error;
      }

      rollbackPre = TRUE;
      rc = initNewRecordDataPages(context, count, lpids, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init pages:%d", rc);
         goto error;
      }

      rc = _collectionSpace->allocateDataPages(context, SPACE_TYPE_RECORD_D,
                                               count, lpids, pids, args, &lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate data pages:%d", rc);
         goto error;
      }

      rollbackPre = FALSE;
      rollbackPages = TRUE;

      rc = lh.lock(context, SPACE_TYPE_RECORD_D, lvl0Lpid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid[%d] lock:%d", lvl0Lpid, rc);
         goto error;
      }

      rc = _collectionSpace->getDataPhyPidInIdMapToWrite(context, lvl0Lpid, lvl0Pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lvl0 pid of[%d], rc:%d", lvl0Lpid, lvl0Pid);
         goto error;
      }

      rc = accessor.init(context, SPACE_TYPE_RECORD_D, lvl0Pid, flags,
                         _collectionSpace->getSU(), lsn);
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
      lh.unlock();

      firstSeq = _pageCntInRoutePages;
      _pageCntInRoutePages += count;
   done:
      return rc;
   error:
      accessor.fini(context);
      lh.unlock();
      if (rollbackPre)
      {
         _collectionSpace->releaseDataPagesPreallocated(context, count, lpids, pids);
      }
      if (rollbackPages)
      {
         INT32 tmpRc = _collectionSpace->releaseDataPages(context, count, lpids, lsn);
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

   INT32 collection::initNewRecordDataPages(requestContext *context,
                                            UINT32 count,
                                            const PAGE_ID *lpids,
                                            const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");
      UINT32 pageSize = 0;
      ossValuePtr ptr = 0;
      storageUnit *su = NULL;
      CL_PAGE_SEQ first = _pageCntInRoutePages;
      rdpAccessor accessor;

      su = _collectionSpace->getSU();
      rc = su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
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

         rc = su->getPagePtr(SPACE_TYPE_RECORD_D, pids[i], ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page ptr of[%d], rc:%d", pids[i], rc);
            goto error;
         }

         rc = accessor.initWithDirectMode(context, SPACE_TYPE_RECORD_D,
                                          pids[i], pageSize, ptr, FALSE, FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         rc = accessor.initRdp(context, lpids[i], _record.logicalCLID, first++);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init record data page[%d], rc:%d", pids[i], rc);
            goto error;
         }

         accessor.fini(context);
      }
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 collection::getLvl0RoutePage(requestContext *context,
                                      UINT32 capacity,
                                      UINT32 lvl0No,
                                      PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      PAGE_ID lvl1Pid = INVALID_PAGE_ID;
      lpidLockHelper lh;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 slot = 0;
      routePageAccessor accessor;

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
         lvl1Pid = (lvl0No < (1 + capacity)) ?
                    _record.routePages[COLLECTION_FIRST_ROOT_LVL1] :
                    _record.routePages[COLLECTION_SECOND_ROOT_LVL1];
      }
      else if (lvl0No < getMaxLvl0Cnt(capacity))
      {
         PAGE_ID lvl2Pid = _record.routePages[COLLECTION_ROOT_LVL2];
         if (INVALID_PAGE_ID == lvl2Pid)
         {
            rc = SDB_VESSEL_PAGE_NOT_EXISTS;
            goto error;
         }

         rc = lh.lock(context, SPACE_TYPE_RECORD_D, lvl2Pid, SHARED);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid[%d] lock:%d", lvl2Pid, rc);
            goto error;
         }

         rc = _collectionSpace->getDataPhyPidInIdMapToRead(context, lvl2Pid, pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get pid of lpid[%d], rc:%d", lvl2Pid, pid);
            goto error;
         }

         rc = accessor.init(context, SPACE_TYPE_RECORD_D,
                            pid, 0, _collectionSpace->getSU());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor:%d", rc);
            goto error;
         }

         slot = (lvl0No - 1 - capacity - capacity) / capacity;
         rc = accessor.readSlot(context, slot, lvl1Pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read slot[%d] of lvl2:%d", slot, rc);
            goto error;
         }

         accessor.fini(context);
         lh.unlock();
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INVALID_PAGE_ID == lvl1Pid)
      {
         rc = SDB_VESSEL_PAGE_NOT_EXISTS;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_RECORD_D, lvl1Pid, SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid[%d] lock:%d", lvl1Pid, rc);
         goto error;
      }

      rc = _collectionSpace->getDataPhyPidInIdMapToRead(context, lvl1Pid, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get pid of lpid[%d], rc:%d", lvl1Pid, pid);
         goto error;
      }

      rc = accessor.init(context, SPACE_TYPE_RECORD_D,
                           pid, 0, _collectionSpace->getSU());
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
      lh.unlock();

      if (INVALID_PAGE_ID == lpid)
      {
         rc = SDB_VESSEL_PAGE_NOT_EXISTS;
         goto error;
      }
   done:
      return rc;
   error:
      accessor.fini(context);
      lh.unlock();
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

      rc = _collectionSpace->getSU()->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
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
      PAGE_ID pid = INVALID_PAGE_ID;
      storageUnit *su = _collectionSpace->getSU();
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY |
                     PAGE_ACCESSOR_FLAG_OPLIST_TAIL;
      lpidLockHelper lh;
      /// we are holding ddl s latch and page allocating x latch.
      /// all columns in _record are unchangeable now.
      collectionRecord record = _record;
      crpAccessor accessor;
      UINT64 mask = COLLECTION_UPDATE_MASK_ROUTE_PAGES;

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

      rc = _collectionSpace->getLpidOfClRecord(getMBID(), crpLpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lpid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_RECORD_D, crpLpid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", crpLpid, rc);
         goto error;
      }

      rc = _collectionSpace->getDataPhyPidInIdMapToWrite(context, crpLpid, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure crp[%d] allocated:%d", crpLpid, rc);
         goto error;
      }

      record.routePages[rootSlot] = lpidOfRP;
      rc = accessor.init(context, SPACE_TYPE_RECORD_D, pid,
                         flags, su, lsn);
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
      lh.unlock();
      _record.routePages[rootSlot] = lpidOfRP;
   done:
      return rc;
   error:
      accessor.fini(context);
      lh.unlock();
      if (INVALID_PAGE_ID != lpidOfRP)
      {
         INT32 tmpRc = _collectionSpace->releaseDataPages(context, 1, &lpidOfRP, lsn);
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
      lpidLockHelper lh;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY |
                     PAGE_ACCESSOR_FLAG_OPLIST_TAIL;
      routePageAccessor accessor;

      rc = createNewRoutePage(context, newLpid, &oplist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new route page:%d", rc);
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = _collectionSpace->getDataPhyPidInIdMapToWrite(context, lpid, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get phy pid of lpid[%d], rc:%d", lpid, pid);
         goto error;
      }

      rc = accessor.init(context, SPACE_TYPE_RECORD_D,
                         pid, flags,
                         _collectionSpace->getSU(),
                         oplist);
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
      lh.unlock();
      lpidOfRP = newLpid;
   done:
      return rc;
   error:
      accessor.fini(context);
      lh.unlock();
      if (INVALID_PAGE_ID != newLpid)
      {
         INT32 tmpRc = _collectionSpace->releaseDataPages(context, 1, &newLpid, oplist);
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
      lpidLockHelper lh;
      PAGE_ID pid = INVALID_PAGE_ID;
      routePageAccessor accessor;
      PAGE_ID lvl2Lpid = _record.routePages[COLLECTION_ROOT_LVL2];
      if (INVALID_PAGE_ID == lvl2Lpid)
      {
         PD_LOG(PDERROR, "lvl2 page does not exist");
         rc = SDB_INVALIDARG;
      }

      rc = lh.lock(context, SPACE_TYPE_RECORD_D, lvl2Lpid, SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lvl2Lpid, rc);
         goto error;
      }

      rc = _collectionSpace->getDataPhyPidInIdMapToRead(context, lvl2Lpid, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get pid of lvl2[%d], rc:%d", lvl2Lpid, rc);
         goto error;
      }

      rc = accessor.init(context, SPACE_TYPE_RECORD_D, pid, 0, _collectionSpace->getSU());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.readSlot(context, slot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read slot[%d] on route page:%d", slot, rc);
         goto error;
      }

      accessor.fini(context);
      lh.unlock();

      if (INVALID_PAGE_ID != lpid)
      {
         goto done;
      }

      rc = createNonRootRoutePage(context, lvl2Lpid, slot, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faile to create new route page:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      accessor.fini(context);
      lh.unlock();
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
      storageUnit *su = NULL;
      slice s;
      UINT32 logicalId = _record.logicalCLID;
      routePageAccessor accessor;

      su = _collectionSpace->getSU();
      rc = su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = _collectionSpace->preallocateDataPages(context, 1, &lpid, &pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate data page:%d", rc);
         goto error;
      }

      rc = su->getPagePtr(SPACE_TYPE_RECORD_D, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr, rc:%d", pid, rc);
         goto error;
      }

      rc = accessor.initWithDirectMode(context, SPACE_TYPE_RECORD_D,
                                       pid, pageSize, ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page accessor:%d", rc);
         goto error;
      }

      rc = accessor.initPage(context, lpid, logicalId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init route page:%d", rc);
         goto error;
      }

      accessor.fini(context);

      s.reset(sizeof(logicalId), (const CHAR *)(&logicalId));

      rc = _collectionSpace->allocateDataPages(context, PAGE_TYPE_ROUTE,
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
      accessor.fini(context);
      if (INVALID_PAGE_ID != pid)
      {
         _collectionSpace->releaseDataPagesPreallocated(context, 1, &lpid, &pid);
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
      SDB_ASSERT(context->testLpidLockMode(SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE), "must holding lock");

      rc = _collectionSpace->getDataPhyPidInIdMapToWrite(context, lpid, pid);
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
      SDB_ASSERT(context->testLpidLockMode(SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE), "must holding lock");

      rc = preallocateCLRecordPage(context, lpid, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _collectionSpace->allocateDataPages(context, PAGE_TYPE_COLLECTION_RECORD,
                                               1, &lpid, &pid, slice());
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
         _collectionSpace->releasePhyPagesPreallocated(context, 1, &pid);
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
      ossValuePtr pagePtr = 0;
      crpAccessor crp;
      storageUnit *su = _collectionSpace->getSU();
      
      pid = INVALID_PAGE_ID;
   
      rc = su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _collectionSpace->preallocatePhyPagesInDFile(context, 1, &pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

      rc = su->getPagePtr(SPACE_TYPE_RECORD_D, pid, pagePtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = crp.initWithDirectMode(context, SPACE_TYPE_RECORD_D,
                                  pid, pageSize, pagePtr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = crp.initPage(context, lpid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      crp.fini(context);

      rc = su->fsync(SPACE_TYPE_RECORD_D, pid, 1, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      crp.fini(context);
      if (INVALID_PAGE_ID != pid)
      {
         _collectionSpace->releasePhyPagesPreallocated(context, 1, &pid);
         pid = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 collection::saveCLRecordWhenCreating(requestContext *context, PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      crpAccessor accessor;
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY;
      rc = accessor.init(context,
                          SPACE_TYPE_RECORD_D,
                          pid, flags, _collectionSpace->getSU());
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