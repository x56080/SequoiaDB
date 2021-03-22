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

   Source File Name = diskFreeSpaceMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A
/
   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/diskFreeSpaceMap.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/bitMapUtils.h"

namespace engine
{
namespace vessel
{
   diskFreeSpaceMap::diskFreeSpaceMap():
   _fsmFile(NULL),
   _firstPMapPid(INVALID_PAGE_ID)
   {

   }

   diskFreeSpaceMap::~diskFreeSpaceMap()
   {

   }

   void diskFreeSpaceMap::close()
   {
      _fsmFile = NULL;
      _entry = fsmCLEntry();
      _cursor.reset();
      _firstPMapPid = INVALID_PAGE_ID;
      return;
   }

   INT32 diskFreeSpaceMap::create(fsmFile *file,
                                  CL_MB_ID mbID,
                                  UINT32 logicalID)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;

      SDB_ASSERT(NULL == _fsmFile, "already open");
      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen() ||
                       INVALID_CL_MB_ID == mbID ||
                       DMS_INVALID_LOGICCLID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _fsmFile = file;

      rc = _fsmFile->allocateNewPage(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page form file:%d", rc);
         goto error;
      }

      rc = initBitMapPage(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init new fsm page:%d", rc);
         goto error;
      }

      _entry.root = pid;
      _entry.logicalID = logicalID;

      rc = updateEntrySlot(mbID, _entry, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update entry slot:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != _fsmFile && INVALID_PAGE_ID != pid)
      {
         _fsmFile->releasePages(1, &pid);
      }
      close();
      goto done;
   }

   INT32 diskFreeSpaceMap::open(fsmFile *file,
                                CL_MB_ID mbID,
                                UINT32 logicalID,
                                fsmStats &stats)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");

      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen() ||
                       INVALID_CL_MB_ID == mbID ||
                       DMS_INVALID_LOGICCLID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      PD_LOG(PDDEBUG, "openning fsm for mbid[%d], lid[%d]", mbID, logicalID);

      _fsmFile = file;
      rc = readEntrySlot(mbID, logicalID, _entry);
      if (SDB_VESSEL_FSM_ENTRY_BROKEN == rc)
      {
         PD_LOG(PDERROR, "[%d:%d] entry slot is broken, will recreate it", mbID, logicalID);
         close();
         rc = create(file, mbID, logicalID);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to recreate entry slot:%d", rc);
            goto error;
         }
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read root page:%d", rc);
         goto error;
      }

      rc = readFirstPmapPid(_firstPMapPid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = createStats(stats);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 diskFreeSpaceMap::addNewPages(CL_PAGE_SEQ sequence,
                                       UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(8 == count, "must be eight");
      UINT32 pageNo = 0;

      if (INVALID_CL_PAGE_SEQ == sequence ||
          8 != count)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      pageNo = sequence / FSM_SEQ_RANGE_IN_PAGE;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::find(const fsmSizeLvl &lvl,
                                BOOLEAN &found,
                                CL_PAGE_SEQ &seq,
                                fsmSizeLvl &realLvl)
   {
      INT32 rc = SDB_OK;
      UINT32 *cursor = NULL;
      UINT32 begin = 0;
      fsmStats stat;
      UINT32 loop = 0;
      UINT32 scanned = 0;
      fsmPageMapSlot *slot = NULL;
      CL_PAGE_SEQ candidate = INVALID_CL_PAGE_SEQ;
      fsmSizeLvl candidateLvl;
      const static UINT32 MAX_SCAN = 8;

      if (!lvl.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(FSM_SPACE_LVL_MIN <= lvl.getLvl() && lvl.getLvl() <= FSM_SPACE_LVL_MAX, "impossible");
      cursor = _cursor.get(lvl.getLvl() - 1);
      begin = *cursor;

      do
      {
         fsmBitMapPage *page = NULL;
         if (0 != loop++ && begin == *cursor)
         {
            break;
         }

         rc = getBitMapPageByPageNo(*cursor, &page, &slot);
         if (SDB_OUT_OF_BOUND == rc)
         {
            rc = SDB_OK;
            *cursor = 0;
            continue;
         }
         else if (SDB_VESSEL_PAGE_CRASHED == rc)
         {
            rc = SDB_OK;
            ++(*cursor);
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d], rc:%d", cursor, rc);
            goto error;
         }

         if (NULL != slot)
         {
            stat.totalPageCount = slot->count;
            stat.lvl1 = slot->lvl1Count;
            stat.lvl2 = slot->lvl2Count;
            stat.lvl3 = slot->lvl3Count;
            stat.lvl4 = slot->lvl4Count;
            if (!isWorthToScanDisk(lvl.getLvl(), stat.totalPageCount, stat.lvl1,
                                stat.lvl2, stat.lvl3, stat.lvl4))
            {
               /// do not upate scanned
               ++(*cursor);
               continue;
            }
         }

         if (findFromBitMapPage(lvl, *cursor, page, slot, candidate, candidateLvl))
         {
            found = TRUE;
            seq = candidate;
            realLvl = candidateLvl;
            break;
         }
         else
         {
            ++(*cursor);
            ++scanned;
            continue;
         }
      }while(scanned <= MAX_SCAN);

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN diskFreeSpaceMap::findFromBitMapPage(const fsmSizeLvl &lvl,
                                              UINT32 pageNo,
                                              fsmBitMapPage *page,
                                              fsmPageMapSlot *slot,
                                              CL_PAGE_SEQ &candidate,
                                              fsmSizeLvl &realLvl)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(FSM_SUB_BITMAP_COUNT == 8, "impossible");
      SDB_ASSERT(lvl.isValid() && NULL != page, "can not be invalid");
      for (UINT32 i = 0; i < FSM_SUB_BITMAP_COUNT; ++i)
      {
         fsmBitMapSubPage &subPage = page->pages[i];
         UINT32 subNo = (pageNo << 3) + i;/// subNo = pageNo * 8 + i
         if (findFromSubBitMapPage(lvl, subNo, &subPage, candidate, realLvl))
         {
            r = TRUE;
            if (NULL != slot)
            {
               decStatInPageSlot(slot, realLvl.getLvl());
            }
            break;
         }
      }
   done:
      return r;
   }

   BOOLEAN diskFreeSpaceMap::findFromSubBitMapPage(const fsmSizeLvl &lvl,
                                                   UINT32 subPageNo,
                                                   fsmBitMapSubPage *page,
                                                   CL_PAGE_SEQ &candidate,
                                                   fsmSizeLvl &realLvl)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(lvl.isValid() && NULL != page, "can not be invalid");
      UINT32 offset = 0;
      UINT16 delta = lvl.getDelta();
      UINT16 realDelta = 0;

      if (0 == page->count)
      {
         goto done;
      }

      for (UINT32 i = lvl.getLvl(); i <= FSM_SPACE_LVL_MAX; ++i)
      {
         r = findFromLvLBits(page, i, delta, offset, realDelta);
         if (r)
         {
            realLvl.reset(i, realDelta);
            candidate = subPageNo * FSM_SEQ_RANGE_IN_SUB_PAGE + offset; 
            goto done;
         }
         delta = 0;
      }
   done:
      return r;
   }

   BOOLEAN diskFreeSpaceMap::findFromLvLBits(fsmBitMapSubPage *page,
                                             UINT32 lvl,
                                             UINT32 delta,
                                             UINT32 &offset,
                                             UINT16 &realDelta)
   {
      SDB_ASSERT(NULL != page, "can not be null");
      SDB_ASSERT(0 < page->count, "can not be zero");
      SDB_ASSERT(FSM_SPACE_LVL_MIN <= lvl && lvl <= FSM_SPACE_LVL_MAX, "impossible");
      BOOLEAN r = FALSE;
      UINT64 *bits = NULL;
      UINT16 *count = NULL;
      UINT32 maxOffset = 0;
      INT32 bound = -1;
      UINT32 diskDelta = 0;
      UINT16 lastCount = 0;

      bits = getBitMapAndCnt(page, lvl, &count);
      SDB_ASSERT(NULL != bits, "can not be null");
      if (0 == *count)
      {
         goto done;
      }

      lastCount = *count;
      maxOffset = page->count - 1;

      do
      {
         if (!upperBoundFirstFreeBitFromBit64(FSM_BITMAP_BITS_COUNT,
                                              bits, bound, maxOffset,
                                              offset))
         {
            if (lastCount == *count)
            {
               PD_LOG(PDWARNING, "current count in head is %d, reset it as zero", *count);
               /// count in head is not zero but no page was found in the first loop.
               *count = 0;
            }
            goto done;
         }

         diskDelta = getDelta(page->deltas, offset);
         if (delta <= diskDelta)
         {
            realDelta = diskDelta;
            setNotFreeIfFree64(FSM_BITMAP_BITS_COUNT,
                               bits, offset);
            --(*count);
            setDelta(page->deltas, offset, 0);
            r = TRUE;
            goto done;
         }

         bound = offset;
         --lastCount;
      } while (0 < lastCount);
   done:
      return r;
   }

   UINT32 diskFreeSpaceMap::getDelta(const UINT64 *deltas,
                                     UINT32 offset)
   {
      SDB_ASSERT(NULL != deltas, "can not be null");
      SDB_ASSERT(FSM_LVL_DELTA_COUNT == 16, "impossible");
      UINT64 delta = *(deltas + (offset >> 4)); /// divided by 16
      UINT32 deltaOffset = offset & 0xF; /// mod 16
      delta = delta >> deltaOffset;
      return delta & 0xF;
      
   }

   void diskFreeSpaceMap::setDelta(UINT64 *deltas,
                                   UINT32 offset,
                                   UINT32 value)
   {
      SDB_ASSERT(NULL != deltas, "can not be null");
      SDB_ASSERT(FSM_LVL_DELTA_COUNT == 16, "impossible");
      UINT64 v = value & 0xF; /// remove invalid bits.
      v <<= (offset & 0xF);
      UINT64 *delta = deltas + (offset >> 4); /// divided by 16
      *delta |= v;
      return;
   }

   INT32 diskFreeSpaceMap::getBitMapPageByPageNo(UINT32 pageNo,
                                                 fsmBitMapPage **page,
                                                 fsmPageMapSlot **slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != page, "can not be null");
      
      if (0 == pageNo)
      {
         fsmPageHead *head = NULL;
         rc = getPageHead(_entry.root, &head);
         if (SDB_OK != rc)
         {
            goto error;
         }

         *page = (fsmBitMapPage *)head;
      }
      else
      {
         fsmPageMapSlot *slotPtr = NULL;
         fsmPageHead *head = NULL;
         fsmPageMapPage *pageMap = NULL;
         fsmPageHead *bmHead = NULL;
         /// the first page is not managed by map page.
         UINT32 slotNo = (pageNo - 1) % FSM_PAGE_MAP_CAPAITY;
         UINT32 skipPage = (pageNo - 1) / FSM_PAGE_MAP_CAPAITY;
         UINT32 skipped = 0;
         PAGE_ID pid = _firstPMapPid;
         
         do
         {
            if (INVALID_PAGE_ID == pid)
            {
               rc = SDB_OUT_OF_BOUND;
               goto error;
            }

            rc = getPageHead(pid, &head);
            if (SDB_OK != rc)
            {
               goto error;
            }

            pid = head->nextPage;
         }while (skipped++ < skipPage);

         pageMap = (fsmPageMapPage *)head;
         if (pageMap->count <= slotNo)
         {
            if (INVALID_PAGE_ID == head->nextPage)
            {
               rc = SDB_OUT_OF_BOUND;
            }
            else
            {
               rc = SDB_VESSEL_PAGE_CRASHED;
            }
            goto error;
         }
         
         slotPtr = &(pageMap->pages[slotNo]);

         /// page head is incorrect. just skip it.
         if (!slotPtr->isValid())
         {
            rc = SDB_VESSEL_PAGE_CRASHED;
            goto error;
         }

         rc = getPageHead(slotPtr->pid, &bmHead);
         if (SDB_OK != rc)
         {
            goto error;
         }

         *page = (fsmBitMapPage *)bmHead;
         if (NULL != slot)
         {
            *slot = slotPtr;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::createStats(fsmStats &stats)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      SDB_ASSERT(isOpen(), "can not be closed");

      pid = _entry.root;
      while (INVALID_PAGE_ID != pid)
      {
         const fsmPageHead *ph = NULL;
         rc = getPageHead(pid, &ph);
         if (SDB_OK != rc)
         {
            goto error;
         }

         createStats(ph, stats);
         pid = ph->nextPage;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::getPageHead(PAGE_ID pid, fsmPageHead **head)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(NULL != head, "can not be invalid");
      SDB_ASSERT(isOpen(), "can not be closed");
      ossValuePtr ptr = 0;
      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get ptr of pid[%d], rc:%d", pid, rc);
         goto error;
      }

      *head = (fsmPageHead *)ptr;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::getPageHead(PAGE_ID pid, const fsmPageHead **head)
   {
      fsmPageHead *h = NULL;
      INT32 rc = getPageHead(pid, &h);
      if (SDB_OK != rc)
      {
         goto error;
      }

      *head = h;
   done:
      return rc;
   error:
      goto done;
   }

   void diskFreeSpaceMap::createStats(const fsmPageHead *head, fsmStats &stats)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      if (FSM_PAGE_TYPE_BITMAP == head->type)
      {
         const fsmBitMapPage *page = (const fsmBitMapPage *)head;
         for (UINT32 i = 0; i < FSM_SUB_BITMAP_COUNT; ++i)
         {
            const fsmBitMapSubPage &subPage = page->pages[i];
            stats.totalPageCount += subPage.count;
            stats.lvl1 += subPage.lvl1Cnt;
            stats.lvl2 += subPage.lvl2Cnt;
            stats.lvl3 += subPage.lvl3Cnt;
            stats.lvl4 += subPage.lvl4Cnt;
         }
      }
      else if (FSM_PAGE_TYPE_PAGEMAP == head->type)
      {
         const fsmPageMapPage *page = (const fsmPageMapPage *)head;
         for (UINT32 i = 0; i < page->count; ++i)
         {
            const fsmPageMapSlot &slot = page->pages[i];
            if (!slot.isValid())
            {
               continue;
            }
            stats.totalPageCount += slot.count;
            stats.lvl1 += slot.lvl1Count;
            stats.lvl2 += slot.lvl2Count;
            stats.lvl3 += slot.lvl3Count;
            stats.lvl4 += slot.lvl4Count;
         }
      }
      return;
   }

   INT32 diskFreeSpaceMap::readEntrySlot(CL_MB_ID mbID,
                                         UINT32 logicalID,
                                         fsmCLEntry &entry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      SDB_ASSERT(NULL != _fsmFile, "can not be null");
      UINT32 pageSize = _fsmFile->getPageSize();
      SDB_ASSERT(FSM_PAGE_SIZE == pageSize, "must be same");
      PAGE_ID pid = INVALID_PAGE_ID;
      ossValuePtr ptr = 0;
      const fsmCLEntry *slot = NULL; 

      pid = getEntryPid(mbID);
      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] from fsm file, rc:%d", pid, rc);
         goto error;
      }

      slot = getEntryFromPagePtr(ptr, mbID);
      SDB_ASSERT(NULL != slot, "can not be null");
      if (!slot->isValid() || logicalID != slot->logicalID)
      {
         PD_LOG(PDERROR, "entry slot of [%d,%d] is broken", mbID, logicalID);
         rc = SDB_VESSEL_FSM_ENTRY_BROKEN;
         goto error;
      }
      entry = *slot;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::readFirstPmapPid(PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      const fsmPageHead *root = NULL;
      rc = getPageHead(_entry.root, &root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get root page:%d", rc);
         goto error;
      }

      pid = root->nextPage;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::updateEntrySlot(CL_MB_ID mbID,
                                       const fsmCLEntry &entry,
                                       BOOLEAN fsync)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(entry.isValid(), "must be valid");
      SDB_ASSERT(NULL != _fsmFile, "can not be null");
      ossValuePtr ptr = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      fsmCLEntry *slot = NULL;

      pid = getEntryPid(mbID);
      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] from fsm file, rc:%d", pid, rc);
         goto error;
      }

      slot = getEntryFromPagePtr(ptr, mbID);
      SDB_ASSERT(NULL != slot, "can not be null");
      *slot = entry;

      if (fsync)
      {
         rc = _fsmFile->fsync(pid, 1, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fsync page[%d], rc:%d", pid, rc);
            goto error;
         }
      }      
   done:
      return rc;
   error:
      goto done;
   }

   PAGE_ID diskFreeSpaceMap::getEntryPid(CL_MB_ID mbID)
   {
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      return mbID / FSM_ENTRY_SLOT_COUNT;
   }

   fsmCLEntry *diskFreeSpaceMap::getEntryFromPagePtr(ossValuePtr ptr,
                                                     CL_MB_ID mbID)
   {
      SDB_ASSERT(0 != ptr, "can not be null");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(4096 == FSM_ENTRY_SLOT_COUNT, "must be 4096");
      fsmCLEntry *slot = (fsmCLEntry *)ptr;
      return slot + (mbID & 0xfff);/// mod 4096
   }

   INT32 diskFreeSpaceMap::initBitMapPage(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(sizeof(fsmBitMapPage) == FSM_PAGE_SIZE, "must be same");
      fsmBitMapSubPage *page = NULL;
      ossValuePtr ptr = 0;
      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      /// only init the first sub page head!
      ossMemset((CHAR*)ptr, 0, FSM_PAGE_SIZE);
      page = (fsmBitMapSubPage *)ptr;
      
      page->head.version = FSM_PAGE_VERSION;
      page->head.type = FSM_PAGE_TYPE_BITMAP;
      page->head.flags = 0;
      page->head.prePage = INVALID_PAGE_ID;
      page->head.nextPage = INVALID_PAGE_ID;

      rc = _fsmFile->fsync(pid, 1, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::initPageMapPage(PAGE_ID pid, PAGE_ID pre)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pre, "can not be invalid");
      SDB_ASSERT(sizeof(fsmPageMapPage) == FSM_PAGE_SIZE, "must be same");
      fsmPageMapPage *page = NULL;
      ossValuePtr ptr = 0;
      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      page = (fsmPageMapPage *)ptr;
      page->head.version = FSM_PAGE_VERSION;
      page->head.type = FSM_PAGE_TYPE_PAGEMAP;
      page->head.flags = 0;
      page->head.prePage = pre;
      page->head.nextPage = INVALID_PAGE_ID;
      ossMemset(page->head.pad, 0, sizeof(page->head.pad));
      page->flags = 0;
      page->count = 0;
      page->pad = 0;

      for (UINT32 i = 0; i < FSM_PAGE_VERSION; ++i)
      {
         fsmPageMapSlot &slot = page->pages[i];
         slot.pid = INVALID_PAGE_ID;
         slot.flags = 0;
         slot.count = 0;
         slot.lvl1Count = 0;
         slot.lvl2Count = 0;
         slot.lvl3Count = 0;
         slot.lvl4Count = 0;
      }

      rc = _fsmFile->fsync(pid, 1, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }


   UINT64 *diskFreeSpaceMap::getBitMapAndCnt(fsmBitMapSubPage *page,
                                             UINT32 lvl,
                                             UINT16 **cnt)
   {
      SDB_ASSERT(NULL != page, "can not be null");
      SDB_ASSERT(NULL != cnt, "can not be null");
      SDB_ASSERT(FSM_SPACE_LVL_MIN <= lvl && lvl <= FSM_SPACE_LVL_MAX, "impossible");
      SDB_ASSERT(NULL != cnt, "can not be null");
      UINT64 *bits = NULL;
      switch (lvl)
      {
      case FSM_SPACE_LVL1:
         bits = page->lvl1;
         *cnt = &(page->lvl1Cnt);
         break;
      case FSM_SPACE_LVL2:
         bits = page->lvl2;
         *cnt = &(page->lvl3Cnt);
         break;
      case FSM_SPACE_LVL3:
         bits = page->lvl3;
         *cnt = &(page->lvl3Cnt);
         break;
      case FSM_SPACE_LVL4:
         bits = page->lvl4;
         *cnt = &(page->lvl4Cnt);
         break;
      default:
         SDB_ASSERT(FALSE, "invalid lvl");
         break;
      }
      return bits;
   }

   void diskFreeSpaceMap::decStatInPageSlot(fsmPageMapSlot *slot,
                                            UINT32 lvl)
   {
      SDB_ASSERT(NULL != slot, "can not be null");
      SDB_ASSERT(FSM_SPACE_LVL_MIN <= lvl && lvl <= FSM_SPACE_LVL_MAX, "impossible");
      if (FSM_SPACE_LVL1 == lvl && 0 < slot->lvl1Count)
      {
         --slot->lvl1Count;
      }
      else if (FSM_SPACE_LVL2 == lvl && 0 < slot->lvl2Count)
      {
         --slot->lvl2Count;
      }
      else if (FSM_SPACE_LVL3 == lvl && 0 < slot->lvl3Count)
      {
         --slot->lvl3Count;
      }
      else if (FSM_SPACE_LVL4 == lvl && 0 < slot->lvl4Count)
      {
         --slot->lvl4Count;
      }
      
      return;
   }

}//namespace vessel
}//namespace engine