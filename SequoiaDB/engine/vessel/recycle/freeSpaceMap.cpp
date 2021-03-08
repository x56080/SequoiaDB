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

   Source File Name = freeSpaceMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/freeSpaceMap.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/stripingGroupFileDef.h"
#include "vessel/bitMapFile.h"
#include "utilStr.hpp"
#include "vessel/bitMapFileDef.h"
#include "vessel/bitMapUtils.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   freeSpaceMap::freeSpaceMap()
   {

   }

   freeSpaceMap::~freeSpaceMap()
   {

   }

   BOOLEAN freeSpaceMap::isOpen()const
   {
      return _sgFile.isOpen();
   }

   INT32 freeSpaceMap::create(requestContext *context,
                              const CHAR * dir,
                              UINT32 secretValue,
                              UINT32 logicalCS)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;

      if (NULL == context ||
          NULL == dir ||
          DMS_INVALID_LOGICCSID == logicalCS)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (INVALID_SPACE_ID == context->getSpaceID())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = createSGFile(context, dir, secretValue, logicalCS);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollback = TRUE;

      rc = createBitMapFile(context, dir, secretValue, logicalCS);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         _sgFile.destroy();
      }
      goto done;
   }

   INT32 freeSpaceMap::createBitMapFile(requestContext *context,
                                        const CHAR * dir,
                                        UINT32 secretValue,
                                        UINT32 logicalCS)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != context->getSpaceID(), "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalCS, "can not be invalid");
      storageFileOptions options;
      storageFileName fn;
      storageCoreArgs args(DMS_PAGE_SIZE4K, 1024, 256);
      BOOLEAN rollback = FALSE;
      
      rc = fn.build(SPACE_TYPE_FSM_BITMAP, context->getSpaceID(), 0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      options.dir = dir;
      options.name = fn.getName();
      options.secretValue = secretValue;
      options.spaceID = context->getSpaceID();
      options.args = &args;
      options.sequence = 0;
      options.logicalCS = logicalCS;

      rc = _bitmap.create(options);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollback = TRUE;

      rc = _bitmap.allocateNewSegment(NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < FSM_SMP_COUNT; ++i)
      {
         rc = initBitMapSMP(i);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = _bitmap.fsync(0, FSM_SMP_COUNT, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         _bitmap.destroy();
      }
      goto done;
   }

   INT32 freeSpaceMap::initBitMapSMP(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      ossValuePtr ptr = 0;
      bitMapFileSMPHead *head = NULL;
      UINT32 capacity = getBitMapFileSMPCapacity();
      UINT32 *bits = NULL;
      UINT64 *tail = NULL;

      rc = _bitmap.getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemset((CHAR *)ptr, 0, DMS_PAGE_SIZE4K);
      head = (bitMapFileSMPHead *)ptr;
      head->version = BIT_MAP_SMP_VERSION;
      head->pid = pid;
      head->flags = 0;
      head->capacity = capacity;
      head->free = capacity;
      head->possibleFree = -1;
      head->lsn = 1;
      head->pad = 0;

      bits = (UINT32*)(ptr + sizeof(bitMapFileSMPHead));
      resetBitMap(capacity/32, bits, (UINT32)(-1));
      
      tail = (UINT64*)(ptr + DMS_PAGE_SIZE4K - sizeof(UINT64));
      *tail = head->lsn = 1;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::createSGFile(requestContext *context,
                                    const CHAR * dir,
                                    UINT32 secretValue,
                                    UINT32 logicalCS)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != context->getSpaceID(), "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalCS, "can not be invalid");
      storageFileOptions options;
      storageFileName fn;
      storageCoreArgs args(DMS_PAGE_SIZE4K, 32, 256);
      
      rc = fn.build(SPACE_TYPE_FSM_SG, context->getSpaceID(), 0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      options.dir = dir;
      options.name = fn.getName();
      options.secretValue = secretValue;
      options.spaceID = context->getSpaceID();
      options.args = &args;
      options.sequence = 0;
      options.logicalCS = logicalCS;

      rc = _sgFile.create(options);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::open(requestContext *context,
                            const CHAR *dir)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      storageFileName fn;
      BOOLEAN rollback = FALSE;

      if (NULL == context ||
          NULL == dir ||
          INVALID_SPACE_ID == context->getSpaceID())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = fn.build(SPACE_TYPE_FSM_SG, context->getSpaceID(), 0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = utilBuildFullPath(dir, fn.getName(), OSS_MAX_PATHSIZE,
                             fullPath) ;

      if (SDB_OK != rc)
      {
         PD_LOG ( PDERROR, "Path+filename are too long: %s; %s", dir,
                  fn.getName()) ;
         goto error ;
      }

      rc = _sgFile.open(fullPath, fn);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rollback = TRUE;

      rc = fn.build(SPACE_TYPE_FSM_BITMAP, context->getSpaceID(), 0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = utilBuildFullPath(dir, fn.getName(), OSS_MAX_PATHSIZE,
                             fullPath) ;

      if (SDB_OK != rc)
      {
         PD_LOG ( PDERROR, "Path+filename are too long: %s; %s", dir,
                  fn.getName()) ;
         goto error ;
      }

      rc = _bitmap.open(fullPath, fn);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         _sgFile.close();
      }
      goto done;
   }

   INT32 freeSpaceMap::close()
   {
      return SDB_OK;
   }

   INT32 freeSpaceMap::createCL(requestContext *context,
                                UINT32 clLogicalID)
   {
      return createCL(context, clLogicalID, 0, 0);
   }

   INT32 freeSpaceMap::createCL(requestContext *context,
                                UINT32 clLogicalID,
                                UINT16 minStripingId,
                                UINT16 maxStripingId)
   {
      INT32 rc = SDB_OK;
      UINT32 segmentCount = 0;
      UINT32 minSegmentCount = 0;
      CL_MB_ID mbid = INVALID_CL_MB_ID;
   
      if (NULL == context ||
          INVALID_CL_MB_ID == context->getMBID()||
          DMS_INVALID_LOGICCLID == clLogicalID ||
          INVALID_STRIPING_ID == minSegmentCount ||
          INVALID_STRIPING_ID == maxStripingId ||
          maxStripingId < minStripingId)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      mbid = context->getMBID();
      minSegmentCount = getMinSegmentCount(mbid);
      segmentCount = _sgFile.getSegmentCount();

      if (segmentCount < minSegmentCount)
      {
         rc = extendSGFile(minSegmentCount);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = initSGPage(mbid, clLogicalID, minStripingId, maxStripingId);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::find(requestContext *context,
                            UINT32 logicalID,
                            UINT32 lvl,
                            candidates &c)
   {
      return find(context, logicalID, 0, lvl, c);
   }

   INT32 freeSpaceMap::find(requestContext *context,
                            UINT32 logicalID,
                            STRIPING_ID striping,
                            UINT32 lvl,
                            candidates &c)
   {
      INT32 rc = SDB_OK;
      stripingGroup sg;
      CL_MB_ID mbid = INVALID_CL_MB_ID;
      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       INVALID_STRIPING_ID == striping))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      c.reset();
      mbid = context->getMBID();
      rc = findSG(mbid, logicalID, striping, sg);
      if (SDB_OK != rc)
      {
         goto error;
      }

      c.setSG(sg);
      rc = findCandidates(context, logicalID, sg, lvl, c);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::getStripingGroup(requestContext *context,
                                        UINT32 logicalID,
                                        UINT16 slot,
                                        stripingGroupOnDisk &sg)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       DMS_INVALID_LOGICCLID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSG(context->getMBID(), logicalID, slot, sg);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::getSG(CL_MB_ID mbid,
                             UINT32 logicalID,
                             UINT16 slot,
                             stripingGroupOnDisk &sg)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      ossValuePtr ptr = 0;
      const stripingGroupPageHead *head = NULL;
      const stripingGroupOnDisk *sgOnDisk = NULL;

      rc = getSGPagePtr(mbid, ptr);
      if (SDB_VESSEL_PAGE_NOT_EXISTS == rc)
      {
         PD_LOG(PDERROR, "mbid[%d] not exists in fsm", mbid);
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get sg page:%d, rc:%d", mbid, rc);
         goto error;
      }

      head = (const stripingGroupPageHead *)ptr;
      if (sgpCrashed(ptr))
      {
         PD_LOG(PDERROR, "striping group page crashed, mbid[%d]", mbid);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (head->isSame(mbid, logicalID))
      {
         PD_LOG(PDERROR, "mbid[%d] logicalID[%d] not exists in fsm", mbid, logicalID);
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }
      else if (head->currentSGCount < slot)
      {
         PD_LOG(PDERROR, "slot[%d] is out of range[%d]", slot, head->currentSGCount);
         rc = SDB_VESSEL_SG_NOT_FOUND;
         goto error;
      }

      sgOnDisk = (const stripingGroupOnDisk *)
            (ptr + SG_PAGE_HEAD_SIZE + slot * sizeof(stripingGroupOnDisk));
      if (!sgOnDisk->valid())
      {
         PD_LOG(PDERROR, "striping group[%d] is invalid, which does not match the page head[%d]",
                head->currentSGCount);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      sg = *sgOnDisk;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::findSG(CL_MB_ID mbid,
                              UINT32 logicalID,
                              STRIPING_ID striping,
                              stripingGroup &sg)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      SDB_ASSERT(INVALID_STRIPING_ID != striping, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      ossValuePtr ptr = 0;
      const stripingGroupPageHead *head = NULL;

      rc = getSGPagePtr(mbid, ptr, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }

      head = (const stripingGroupPageHead *)ptr;
      if (sgpCrashed(ptr))
      {
         PD_LOG(PDERROR, "striping group page crashed, mbid[%d]", mbid);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (head->isSame(mbid, logicalID))
      {
         PD_LOG(PDERROR, "mbid[%d] logicalID[%d] not exists in fsm", mbid, logicalID);
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }
      else if (striping < head->minStripingId ||
               striping > head->maxStripingId)
      {
         PD_LOG(PDERROR, "striping[%d] out of range, min/max[%d, %d]",
                striping, head->minStripingId, head->maxStripingId);
         rc = SDB_VESSEL_STRIPING_NOT_FOUND;
         goto error;
      }

      if (!search(striping, head->currentSGCount,
                  (const stripingGroupOnDisk *)(ptr + SG_PAGE_HEAD_SIZE),
                  sg))
      {
         PD_LOG(PDERROR, "striping[%d] not found, mbid[%d]", striping, mbid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN freeSpaceMap::search(STRIPING_ID striping,
                                UINT32 count,
                                const stripingGroupOnDisk *begin,
                                stripingGroup &sg)
   {
      SDB_ASSERT(INVALID_STRIPING_ID != striping, "can not be invalid");
      SDB_ASSERT(0 < count, "must be over zero");
      SDB_ASSERT(NULL != begin, "can not be null");

      static const UINT32 linearSearchCount = MAX_SG_COUNT / 2;
      UINT16 slot = 0;
      UINT16 searchCount = count;
      BOOLEAN found = FALSE;
      if (linearSearchCount < count)
      {
         slot = count / 2;
         const stripingGroupOnDisk &tmp = begin[slot];
         if (striping < tmp.minStripingId)
         {
            slot = 0;
            searchCount = slot;
         }
         else if (tmp.maxStripingId > striping)
         {
            searchCount = count - slot - 1;
            ++slot;
         }
         else
         {
            sg.sgOnDisk = tmp;
            sg.slot = slot;
            found = TRUE;
            goto done;
         }
      }

      for (UINT32 i = 0; i < searchCount; ++i)
      {
         const stripingGroupOnDisk &tmp = begin[slot];
         if (tmp.minStripingId <= striping &&
             tmp.maxStripingId >= striping)
         {
            sg.sgOnDisk = tmp;
            sg.slot = slot;
            found = TRUE;
            break;
         }
         ++slot;
      }

   done:
      return found;
   }

   INT32 freeSpaceMap::findCandidates(requestContext *context,
                                      UINT32 logicalID,
                                      const stripingGroup &sg,
                                      UINT32 lvl,
                                      candidates &c)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_CL_MB_ID != context->getMBID(), "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      SDB_ASSERT(sg.valid(), "can not be invalid");

      PAGE_ID pid = sg.sgOnDisk.lastExtent;
      const static UINT32 count = DMS_PAGE_SIZE4K / BIT_MAP_PAGE_SIZE;
      while (INVALID_PAGE_ID != pid)
      {
         ossValuePtr ptr = 0;
         rc = _bitmap.getPagePtr(pid, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get bm extent:%d", rc);
            goto error;
         }

         for (UINT32 i = 0; i < count; ++i)
         {
            rc = findCandidates(context->getMBID(), logicalID,
                                context->getSession()->getSessionID(),
                                (ptr + i * BIT_MAP_PAGE_SIZE),
                                lvl, c);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::findCandidates(CL_MB_ID mbid,
                                      UINT32 logicalID,
                                      UINT64 seed,
                                      ossValuePtr ptr,
                                      UINT32 lvl,
                                      candidates &c)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      SDB_ASSERT(0 != ptr, "can not be null");
      SDB_ASSERT(!c.needNoMore(), "can not be need no more");

      UINT32 clvl = lvl;
      const UINT32 *bits = NULL;
      UINT32 count = 0;
      UINT16 freeCount = 0;
      const bitMapPageHead *head = (const bitMapPageHead *)ptr;
      const UINT32 *w = (const UINT32 *)(ptr + BIT_MAP_PAGE_SIZE - sizeof(UINT32));
      if (head->crashed(*w))
      {
         PD_LOG(PDERROR, "page crashed, mbid[%d], logicalID[%d]", mbid, logicalID);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (!head->isSame(mbid, logicalID))
      {
         PD_LOG(PDERROR, "target:[%d:%d], on disk[%d:%d]",
                mbid, logicalID, head->mbid, head->cllid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (0 == head->count)
      {
         goto done;
      }

      do
      {
         if (BIT_MAP_BIT_LVL0 == clvl)
         {
            freeCount = head->lvl0Free;
            bits = getLvl0Bits(ptr);
         }
         else if (BIT_MAP_BIT_LVL1 = clvl)
         {
            freeCount = head->lvl1Free;
            bits = getLvl1Bits(ptr);
         }
         else if (BIT_MAP_BIT_LVL2 = clvl)
         {
            freeCount = head->lvl2Free;
            bits = getLvl2Bits(ptr);
         }
         else if (BIT_MAP_BIT_LVL3 = clvl)
         {
            freeCount = head->lvl3Free;
            bits = getLvl3Bits(ptr);
         }

         if (0 == freeCount)
         {
            if (clvl < BIT_MAP_MAX_LEVEL)
            {
               ++clvl;
               continue;
            }
            else
            {
               break;
            }
         }
      }while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::dropCL(requestContext *context)
   {
      INT32 rc = SDB_OK;
      UINT32 segmentCount = 0;
      UINT32 minSegmentCount = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      CL_MB_ID mbid = INVALID_CL_MB_ID;

      if (NULL == context ||
          INVALID_CL_MB_ID == context->getMBID())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      mbid = context->getMBID();
      minSegmentCount = getMinSegmentCount(mbid);
      segmentCount = _sgFile.getSegmentCount();

      if (segmentCount < minSegmentCount)
      {
         PD_LOG(PDERROR, "mbid[%d] not exists", mbid);
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      rc = resetSGPage(mbid);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::addNewPages(CL_MB_ID mbid,
                                   STRIPING_ID striping,
                                   UINT32 count,
                                   PAGE_ID lpid,
                                   UINT32 lvl)

   INT32 freeSpaceMap::extendSGFile(UINT32 minSegCount)
   {
      INT32 rc = SDB_OK;
      UINT32 count = 0;
      _sgExtendingLatch.get();
      count = _sgFile.getSegmentCount();

      while (count < minSegCount)
      {
         rc = _sgFile.allocateNewSegment(NULL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new sg file segment:%d", rc);
            goto error;
         }
      }
   done:
      _sgExtendingLatch.release();
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::initSGPage(CL_MB_ID mbid,
                                  UINT32 clLogicalID,
                                  UINT16 minStripingId,
                                  UINT16 maxStripingId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clLogicalID, "can not be invalid");
      SDB_ASSERT(INVALID_STRIPING_ID != minStripingId, "can not be invalid");
      SDB_ASSERT(INVALID_STRIPING_ID != maxStripingId, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");

      UINT32 sgpPerPage = getSGPCountPerFilePage();
      PAGE_ID pid = INVALID_PAGE_ID;
      ossValuePtr ptr = 0;
      stripingGroupPageHead *head = NULL;
      stripingGroupOnDisk *sg = NULL;
      UINT64 *tail = NULL;

      rc = getSGPagePtr(mbid, ptr, &pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemset((CHAR*)ptr, 0, SG_PAGE_SIZE);
      head = (stripingGroupPageHead*)ptr;
      head->version = SG_PAGE_VERSION;
      head->mbid = mbid;
      head->clLogicalID = clLogicalID;
      head->flags = 0;
      head->maxSGCount = MAX_SG_COUNT;
      head->currentSGCount = 1;
      head->minStripingCountPerSG = (maxStripingId - minStripingId + 1) / MAX_SG_COUNT;
      if (0 == head->minStripingCountPerSG)
      {
         head->minStripingCountPerSG = 1;
      }
      head->minStripingId = minStripingId;
      head->maxStripingId = maxStripingId;
      head->lsn = 1;

      for (UINT32 i = 0; i < head->maxSGCount; ++i)
      {
         sg = (stripingGroupOnDisk *)(ptr + SG_PAGE_HEAD_SIZE + sizeof(stripingGroupOnDisk) * i);
         sg->minStripingID = INVALID_STRIPING_ID;
         sg->maxStripingID = INVALID_STRIPING_ID;
         sg->extentCount = 0;
         sg->lastExtent = INVALID_PAGE_ID;
      }

      sg = (stripingGroupOnDisk *)(ptr + SG_PAGE_HEAD_SIZE);
      sg->minStripingID = head->minStripingId;
      sg->maxStripingID = head->maxStripingId;

      tail = (UINT64 *)(ptr + SG_PAGE_SIZE - sizeof(UINT64));
      *tail = head->lsn;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::resetSGPage(CL_MB_ID mbid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      ossValuePtr ptr = 0;
      stripingGroupPageHead *head = NULL;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT64 *tail = NULL;

      SDB_ASSERT(FALSE, "todo");

      rc = getSGPagePtr(mbid, ptr, &pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemset((CHAR *)(ptr), 0, SG_PAGE_SIZE);
      head = (stripingGroupPageHead *)ptr;
      *head = stripingGroupPageHead();
      tail = (UINT64 *)(ptr + SG_PAGE_SIZE - sizeof(UINT64));
      *tail = DPS_INVALID_LSN_OFFSET;
   done:
      return rc;
   error:
      goto done;
   }

   

   BOOLEAN freeSpaceMap::smpCrashed(ossValuePtr ptr)
   {
      SDB_ASSERT(0 != ptr, "can not be null");
      const bitMapFileSMPHead *head = (const bitMapFileSMPHead *)ptr;
      UINT64 *tail = (UINT64 *)(ptr + DMS_PAGE_SIZE4K - sizeof(UINT64));
      return head->lsn != *tail;
   }

   BOOLEAN freeSpaceMap::sgpCrashed(ossValuePtr ptr)
   {
      const stripingGroupPageHead *head = (const stripingGroupPageHead *)ptr;
      const UINT64 *tail = (UINT64 *)(ptr+SG_PAGE_SIZE-sizeof(UINT64));
      return head->crashed(*tail);
   }

   UINT32 freeSpaceMap::getMinSegmentCount(CL_MB_ID mbid)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      SDB_ASSERT(0 != _sgFile.getMaxPageCountPerSeg(), "can not be zero");
      return mbid / getSGPCountPerFilePage() / _sgFile.getMaxPageCountPerSeg() + 1;
   }

   UINT32 freeSpaceMap::getSGPCountPerFilePage()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return _sgFile.getPageSize() / SG_PAGE_SIZE;
   }

   INT32 freeSpaceMap::getSGPagePtr(CL_MB_ID mbid,
                                    ossValuePtr &ptr,
                                    PAEG_ID *pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      UINT32 sgpPerPage = getSGPCountPerFilePage();
      SDB_ASSERT(0 < sgpPerPage, "impossible");

      PAGE_ID pidOfFile = mbid / sgpPerPage;
      rc = _sgFile.getPagePtr(pidOfFile, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ptr += (mbid % sgpPerPage) * SG_PAGE_SIZE;

      if (NULL != pid)
      {
         *pid = pidOfFile;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine