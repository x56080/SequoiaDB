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
#include "ossAtomic.hpp"

namespace engine
{
namespace vessel
{
   diskFreeSpaceMap::diskFreeSpaceMap():
   _fsmFile(NULL)
   {

   }

   diskFreeSpaceMap::~diskFreeSpaceMap()
   {

   }

   void diskFreeSpaceMap::close()
   {
      _fsmFile = NULL;
      _entry = fsmCLEntry();
      _pmapPids.clear();
      _cursor.reset();
      _stats.reset();
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

      rc = createNewBitMapPage(file, pid);
      if (SDB_OK != rc)
      {
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
         file->releasePages(1, &pid);
      }
      close();
      goto done;
   }

   INT32 diskFreeSpaceMap::open(fsmFile *file,
                                CL_MB_ID mbID,
                                UINT32 logicalID)
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

      rc = cachePMapPids();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = createStats(_stats);
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
      UINT32 pageNo = 0;
      SDB_ASSERT(PAGE_COUNT_IN_EXTENT == count, "impossible");

      if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == sequence ||
                       PAGE_COUNT_IN_EXTENT != count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      pageNo = getPageNo(sequence);
      if (0 == pageNo)
      {
         rc = addNewPagesToBitMapPage(_entry.root, sequence, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure page with no[%d] exists", pageNo, rc);
            goto error;
         }
      }
      else
      {
         UINT32 pmapCount = ((pageNo - 1) / FSM_PAGE_MAP_CAPAITY) + 1;
         rc = ensurePMapPage(pmapCount);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = addNewPagesToPageMapPage(_pmapPids.at(pmapCount - 1), sequence, count);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      _stats.totalPageCount += count;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::updatePageFreeSizeLvL(CL_PAGE_SEQ sequence,
                                                 const fsmSizeLvl &lvl)
   {
      INT32 rc = SDB_OK;
      UINT32 pageNo = 0;
      fsmBitMapPage *page = NULL;
      fsmPageMapSlot *slot = NULL;
      fsmSizeLvl old;

      /// lvl can be invalid here. which means no free space at all.
      if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == sequence))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      pageNo = getPageNo(sequence);
      rc = getBitMapPageByPageNo(pageNo, &page, &slot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = updateSizeLvl(sequence, lvl, page, old);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (lvl.getLvl() != old.getLvl())
      {
         if (old.isValid())
         {
            if (NULL != slot)
            {
               decStatWithCAS(slot->stat, old.getLvl());
            }
            decStatWithCAS(_stats, old.getLvl());
         }
         
         if (lvl.isValid())
         {
            if (NULL != slot)
            {
               incStatWithCAS(slot->stat, lvl.getLvl());
            }
            incStatWithCAS(_stats, lvl.getLvl());
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::updateSizeLvl(CL_PAGE_SEQ seq,
                                         const fsmSizeLvl &lvl,
                                         fsmBitMapPage *page,
                                         fsmSizeLvl &old)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != seq, "can not be invalid");
      SDB_ASSERT(NULL != page, "can not be null");
      
      UINT32 subPageOffset = getSubPageOffset(seq);
      UINT32 offset = seq % FSM_SEQ_RANGE_IN_SUB_PAGE;
      fsmBitMapSubPage *subPage = &(page->pages[subPageOffset]);
      INT32 oldLvl = FSM_SPACE_LVL_INVALID;
      UINT32 oldDelta = 0;
      UINT64 *bits = NULL;

      /// Usually due to dirty pages not being flushed.
      if (subPage->stat.totalPageCount < (offset +1))
      {
         PD_LOG(PDERROR, "page sequence[%d] out of bound", seq);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      
      oldDelta = getDelta(subPage->deltas, offset);
      for (UINT32 i = FSM_SPACE_LVL_MIN; i <= FSM_SPACE_LVL_MAX; ++i)
      {
         bits = subPage->getBits(i);
         if (testBitIsFree(subPage->getBitsCount(), bits, offset))
         {
            oldLvl = i;
            break;
         }
      }

      if (oldDelta != lvl.getDelta())
      {
         /// update delta
         setDeltaWithCAS(subPage->deltas, offset, lvl.getDelta());
      }

      if (lvl.getLvl() != oldLvl && FSM_SPACE_LVL_INVALID != oldLvl)
      {
         decStatWithCAS(subPage->stat, oldLvl);
         /// do not goto error when cas failed. checking it return value
         /// just for logging error message.
         if (!setNotFreeWithCAS64(subPage->getBitsCount(),
                                  subPage->getBits(oldLvl),
                                  offset, 64))
         {
            PD_LOG(PDERROR, "failed to update lvl with cas");
         }
      }

      if (lvl.getLvl() != oldLvl && lvl.isValid())
      {
         if (!setFreeWithCAS64(subPage->getBitsCount(),
                               subPage->getBits(lvl.getLvl()),
                               offset, 64))
         {
            PD_LOG(PDERROR, "failed to update lvl with cas");
         }
         incStatWithCAS(subPage->stat, lvl.getLvl());
      }

      old.reset(oldLvl, oldDelta);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::addNewPagesToPageMapPage(PAGE_ID pid,
                                                    CL_PAGE_SEQ seq,
                                                    UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != seq, "can not be invalid");
      fsmPageMapPage *page = NULL;
      fsmPageHead *head = NULL;
      fsmPageMapSlot *slot = NULL;
      UINT32 slotOffset = seq % FSM_SEQ_RANGE_IN_PAGE;

      rc = getPageHead(pid, &head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      page = (fsmPageMapPage *)head;
      slot = &(page->pages[slotOffset]);

      if (!slot->isValid())
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = createNewBitMapPage(_fsmFile, pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new bitmap page:%d", rc);
            goto error;
         }

         slot->pid = pid;
         slot->stat.reset();
      }

      /// We always think that writing 4bytes count or pid on disk is
      /// atomic. Dirty page flush missing will cause the inconsistency
      /// between head and slot. Just correct the head count.
      if (page->count < (slotOffset + 1))
      {
         page->count = slotOffset + 1;
      }

      rc = addNewPagesToBitMapPage(slot->pid, seq, count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      slot->stat.totalPageCount += count;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::addNewPagesToBitMapPage(PAGE_ID pid,
                                                   CL_PAGE_SEQ sequence,
                                                   UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != sequence, "can not be invalid");
      SDB_ASSERT(0 == (sequence & 0x07), "impossible");
      SDB_ASSERT(8 == count, "must be 8");
      UINT32 subPageOffset = getSubPageOffset(sequence);
      UINT32 offset = sequence % FSM_SEQ_RANGE_IN_SUB_PAGE;
      fsmPageHead *page = NULL;
      fsmBitMapSubPage *subPage = NULL;

      rc = getPageHead(pid, &page);
      if (SDB_OK != rc)
      {
         goto error;
      }

      subPage = (fsmBitMapSubPage *)page + subPageOffset;
      for (UINT32 i = 0; i < count; ++i)
      {
         for (UINT32 j = 0; j < FSM_SPACE_LVL_COUNT; ++j)
         {
            setNotFreeIfFree64(subPage->getBitsCount(),
                               subPage->getBits(j),
                               offset + i);
         }
         setDelta(subPage->deltas, offset + i, 0);
      }

      if (subPage->stat.totalPageCount < (offset + count))
      {
         subPage->stat.totalPageCount = (offset + count);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::ensurePMapPage(UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      PAGE_ID lastPid = _entry.root;
      fsmPageHead *last = NULL;
      PAGE_ID pid = INVALID_PAGE_ID;

      rc = getPageHead(_entry.root, &last);
      if (SDB_OK != rc)
      {
         goto error;
      }

      while (_pmapPids.size() < count)
      {
         rc = _fsmFile->allocateNewPage(pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
            goto error;
         }
         rc = initPageMapPage(pid, lastPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init first page map:%d", rc);
            goto error;
         }

         rc = _fsmFile->fsync(pid, 1, TRUE);
         if (SDB_OK != rc)
         {
            goto error;
         }

         last->nextPage = pid;
         _fsmFile->fsync(lastPid, 1, FALSE);
         _pmapPids.push_back(pid);
         lastPid = pid;
         pid = INVALID_PAGE_ID;
         rc = getPageHead(lastPid, &last);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _fsmFile->releasePages(1, &pid);
      }
      goto done;
   }

   INT32 diskFreeSpaceMap::createNewBitMapPage(fsmFile *file, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != file && file->isOpen(), "can not be closed");
      PAGE_ID bitmapPid = INVALID_PAGE_ID;

      rc = file->allocateNewPage(bitmapPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page form file:%d", rc);
         goto error;
      }

      rc = initBitMapPage(bitmapPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init new fsm page:%d", rc);
         goto error;
      }

      rc = file->fsync(bitmapPid, 1, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }

      pid = bitmapPid;
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != bitmapPid)
      {
         _fsmFile->releasePages(1, &bitmapPid);
      }
      goto done;
   }

   INT32 diskFreeSpaceMap::find(const fsmSizeLvl &lvl,
                                CL_PAGE_SEQ &seq,
                                fsmSizeLvl &realLvl)
   {
      INT32 rc = SDB_OK;
      UINT32 *cursor = NULL;
      UINT32 begin = 0;
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

      if (!isWorthToScanDisk(lvl.getLvl(), _stats.totalPageCount,
                             _stats.getLvl(FSM_SPACE_LVL0),
                             _stats.getLvl(FSM_SPACE_LVL1),
                             _stats.getLvl(FSM_SPACE_LVL2),
                             _stats.getLvl(FSM_SPACE_LVL3)))
      {
         rc = SDB_VESSEL_FSM_NO_FREE_SPACE;
         goto error;
      }

      cursor = _cursor.get(lvl.getLvl());
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
            if (!isWorthToScanDisk(lvl.getLvl(), slot->stat.totalPageCount,
                                   slot->stat.getLvl(FSM_SPACE_LVL0),
                                   slot->stat.getLvl(FSM_SPACE_LVL1),
                                   slot->stat.getLvl(FSM_SPACE_LVL2),
                                   slot->stat.getLvl(FSM_SPACE_LVL3)))
            {
               /// do not upate scanned
               ++(*cursor);
               continue;
            }
         }

         if (findFromBitMapPage(lvl, *cursor, page, candidate, candidateLvl))
         {
            if (NULL != slot)
            {
               slot->stat.decLvl(candidateLvl.getLvl());
            }
            break;
         }
         else
         {
            ++(*cursor);
            ++scanned;
            continue;
         }
      }while(scanned <= MAX_SCAN);

      if (INVALID_CL_PAGE_SEQ != candidate)
      {
         _stats.decLvl(candidateLvl.getLvl());
      }
      else
      {
         rc = SDB_VESSEL_FSM_NO_FREE_SPACE;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN diskFreeSpaceMap::findFromBitMapPage(const fsmSizeLvl &lvl,
                                                UINT32 pageNo,
                                                fsmBitMapPage *page,
                                                CL_PAGE_SEQ &candidate,
                                                fsmSizeLvl &realLvl)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(lvl.isValid() && NULL != page, "can not be invalid");
      UINT32 offset = 0;
      for (UINT32 i = 0; i < FSM_SUB_BITMAP_COUNT; ++i)
      {
         fsmBitMapSubPage &subPage = page->pages[i];
         if (findFromSubBitMapPage(lvl, &subPage, offset, realLvl))
         {
            candidate = pageNo * FSM_SEQ_RANGE_IN_PAGE +
                        i * FSM_SEQ_RANGE_IN_SUB_PAGE +
                        offset;
            break;
         }
      }

      return r;
   }

   BOOLEAN diskFreeSpaceMap::findFromSubBitMapPage(const fsmSizeLvl &lvl,
                                                   fsmBitMapSubPage *page,
                                                   UINT32 &offset,
                                                   fsmSizeLvl &realLvl)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(lvl.isValid() && NULL != page, "can not be invalid");
      UINT16 delta = lvl.getDelta();
      UINT16 realDelta = 0;

      if (0 == page->stat.totalPageCount)
      {
         goto done;
      }

      for (UINT32 i = lvl.getLvl(); i <= FSM_SPACE_LVL_MAX; ++i)
      {
         if (findFromLvLn(page, i, delta, offset, realDelta))
         {
            realLvl.reset(i, realDelta);
            r = TRUE;
            goto done;
         }
         delta = 0;
      }
   done:
      return r;
   }

   BOOLEAN diskFreeSpaceMap::findFromLvLn(fsmBitMapSubPage *page,
                                          UINT32 lvl,
                                          UINT32 delta,
                                          UINT32 &offset,
                                          UINT16 &realDelta)
   {
      SDB_ASSERT(NULL != page, "can not be null");
      SDB_ASSERT(FSM_SPACE_LVL_MIN <= lvl && lvl <= FSM_SPACE_LVL_MAX, "impossible");
      BOOLEAN r = FALSE;
      UINT64 *bits = NULL;

      UINT32 maxOffset = 0;
      INT32 bound = -1;
      UINT32 diskDelta = 0;
      INT32 lvlCnt = 0;
      INT32 originalCnt = 0;
      UINT32 bitOffset = 0;

      bits = page->getBits(lvl);
      lvlCnt = page->stat.getLvl(lvl);
      originalCnt = lvlCnt;

      if (0 == originalCnt || 0 == page->stat.totalPageCount)
      {
         goto done;
      }

      maxOffset = page->stat.totalPageCount - 1;

      while (0 < lvlCnt)
      {
         if (!upperBoundFirstFreeBitFromBit64(FSM_BITMAP_BITS_COUNT,
                                              bits, bound, maxOffset,
                                              bitOffset))
         {
            /// should alwasy find free bit when head count is not zero.
            PD_LOG(PDWARNING, "inconsistency found, reset head as %d from %d",
                   originalCnt - lvlCnt, originalCnt);
            page->stat.lvln[lvl] = originalCnt - lvlCnt;
            goto done;
         }

         diskDelta = getDelta(page->deltas, bitOffset);
         if (delta <= diskDelta)
         {
            setNotFreeIfFree64(FSM_BITMAP_BITS_COUNT,
                               bits, bitOffset);
            setDelta(page->deltas, bitOffset, 0);
            r = TRUE;
            page->stat.decLvl(lvl);
            offset = bitOffset;
            realDelta = diskDelta;
            goto done;
         }

         bound = bitOffset;
         --lvlCnt;
      }
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
      UINT32 n = offset & 0xF;
      UINT64 mask = 0xF;
      mask <<= n;
      mask = ~mask;

      UINT64 v = value & 0xF; /// remove invalid bits.
      v <<= n;
      UINT64 *delta = deltas + (offset >> 4); /// divided by 16
      *delta &= mask;
      *delta |= v;
      return;
   }

   BOOLEAN diskFreeSpaceMap::setDeltaWithCAS(UINT64 *deltas,
                                             UINT32 offset,
                                             UINT32 value)
   {
      SDB_ASSERT(NULL != deltas, "can not be null");
      SDB_ASSERT(FSM_LVL_DELTA_COUNT == 16, "impossible");
      BOOLEAN r = FALSE;
      UINT32 loopCnt = 0;
      UINT32 n = offset & 0xF;
      UINT64 mask = 0xF;
      mask <<= n;
      mask = ~mask;

      UINT64 v = value & 0xF; /// remove invalid bits.
      v <<= n;
      volatile UINT64 *delta = deltas + (offset >> 4); /// divided by 16
      UINT64 expected = *delta;
      UINT64 disired = (expected & mask) | v;

      while (!ossCompareAndSwap64(delta, expected, disired))
      {
         if (++loopCnt == 64)
         {
            PD_LOG(PDERROR, "delta cas loop over 64 times");
            goto done;
         }
         expected = *delta;
         disired = (expected & mask) | v;
      }
      r = TRUE;
   done:
      return r;
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
         if (NULL != slot)
         {
            *slot = NULL;
         }
      }
      else
      {
         UINT32 pmapNo = (pageNo - 1) / FSM_PAGE_MAP_CAPAITY;
         UINT32 slotOffset = (pageNo - 1) % FSM_PAGE_MAP_CAPAITY;
         fsmPageMapPage *pmap = NULL;
         fsmPageHead *head = NULL;

         rc = getPMap(pmapNo, &pmap);
         if (SDB_OK != rc)
         {
            goto error;
         }
         
         if (pmap->count < (slotOffset + 1))
         {
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         if (!pmap->pages[slotOffset].isValid())
         {
            rc = SDB_VESSEL_PAGE_NOT_EXISTS;
            goto error;
         }

         rc = getPageHead(pmap->pages[slotOffset].pid, &head);
         if (SDB_OK != rc)
         {
            goto error;
         }

         *page = (fsmBitMapPage *)head;
         if (NULL != slot)
         {
            *slot = &(pmap->pages[slotOffset]);
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::getPMap(UINT32 mapPageNo, fsmPageMapPage **page)
   {
      SDB_ASSERT(isOpen(), "can not be null");
      SDB_ASSERT(NULL != page, "can not be null");
      INT32 rc = SDB_OK;
      fsmPageHead *head = NULL;
      PAGE_ID pid = INVALID_PAGE_ID;
      if (_pmapPids.size() <= mapPageNo)
      {
         rc = SDB_OUT_OF_BOUND;
         goto done;
      }

      rc = getPageHead(_pmapPids.at(mapPageNo), &head);
      if (SDB_OK != rc)
      {
         goto done;
      }

      *page = (fsmPageMapPage *)head;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::getRootBitMap(fsmBitMapPage **page)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != page, "can not be null");
      fsmPageHead *head = NULL;
      rc = getPageHead(_entry.root, &head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      *page = (fsmBitMapPage *)head;
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
      const fsmPageHead *ph = NULL;
      rc = getPageHead(pid, &ph);
      if (SDB_OK != rc)
      {
         goto error;
      }

      createStats(ph, stats);

      for (UINT32 i = 0; i < _pmapPids.size(); ++i)
      {
         pid = _pmapPids.at(i);
         rc = getPageHead(pid, &ph);
         if (SDB_OK != rc)
         {
            goto error;
         }
         createStats(ph, stats);
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
            stats.merge(subPage.stat);
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
            stats.merge(slot.stat);
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

   INT32 diskFreeSpaceMap::cachePMapPids()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      const fsmPageHead *page = NULL;
      PAGE_ID pid = INVALID_PAGE_ID;
      rc = getPageHead(_entry.root, &page);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get root page:%d", rc);
         goto error;
      }

      while (INVALID_PAGE_ID != page->nextPage)
      {
         _pmapPids.push_back(page->nextPage);
         pid = page->nextPage;
         rc = getPageHead(pid, &page);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get root page:%d", rc);
            goto error;
         }
      }
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

      ossMemset(page, 0, FSM_PAGE_SIZE);
      page = (fsmPageMapPage *)ptr;
      page->head.version = FSM_PAGE_VERSION;
      page->head.type = FSM_PAGE_TYPE_PAGEMAP;
      page->head.flags = 0;
      page->head.prePage = pre;
      page->head.nextPage = INVALID_PAGE_ID;

      page->flags = 0;
      page->count = 0;

      for (UINT32 i = 0; i < FSM_PAGE_MAP_CAPAITY; ++i)
      {
         fsmPageMapSlot &slot = page->pages[i];
         slot.pid = INVALID_PAGE_ID;
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

   void diskFreeSpaceMap::decStatWithCAS(fsmStats &stats, UINT32 lvl)
   {
      SDB_ASSERT(FSM_SPACE_LVL_MIN <= lvl && lvl <= FSM_SPACE_LVL_MAX, "impossible");
      ossFetchAndDecrement32(&(stats.lvln[lvl]));
      return;
   }

   void diskFreeSpaceMap::incStatWithCAS(fsmStats &stats, UINT32 lvl)
   {
      SDB_ASSERT(FSM_SPACE_LVL_MIN <= lvl && lvl <= FSM_SPACE_LVL_MAX, "impossible");
      ossFetchAndIncrement32(&(stats.lvln[lvl]));
      return;
   }

}//namespace vessel
}//namespace engine