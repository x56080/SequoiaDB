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

*******************************************************************************/
#include "vessel/diskFreeSpaceMap.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/bitmapUtils.h"
#include "ossAtomic.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   constexpr UINT32 SUPER_BITMAP_EXTENDING_CAPACITY = 64;

   diskFreeSpaceMap::diskFreeSpaceMap()
   {
   }

   diskFreeSpaceMap::~diskFreeSpaceMap()
   {
      close();
   }

   void diskFreeSpaceMap::close()
   {
      _fsmFile = NULL;
      _logicalId = DMS_INVALID_LOGICCLID;
      _mbID = INVALID_CL_MB_ID;
      _totalDataPageCount = 0;
      for (_BITMAP_OBJ_MAP::iterator itr = _bitmaps.begin();
           itr != _bitmaps.end(); ++itr)
      {
         if (NULL != itr->second)
         {
            SDB_OSS_DEL itr->second;
         }
      }
      _bitmaps.clear();
      _bitmapOwners.clear();

      for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         _superBitmap[i].release();
      }
      return;
   }

   void diskFreeSpaceMap::destroy()
   {
      if (isOpen())
      {
         ossPoolVector<PAGE_ID> pids;
         INT32 rc = destroyEntrySlot(_mbID);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy entry slot of mbid[%d], rc:%d",
                   _mbID, rc);    
         }

         for (UINT32 i = 0; i < _bitmapOwners.size(); ++i)
         {
            pids.push_back(_bitmapOwners.at(i));
         }
         for (_BITMAP_OBJ_MAP::const_iterator itr = _bitmaps.begin();
              itr != _bitmaps.end(); ++itr)
         {
            pids.push_back(itr->second->getPid());
         }

         rc = _fsmFile->releasePages(pids.size(), pids.data());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to release pids on smp:%d", rc);
         }

         close();
      }

      return;
   }

   INT32 diskFreeSpaceMap::truncate()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(!_bitmaps.empty(), "can not be empty");
      ossPoolVector<PAGE_ID> pids;
      PAGE_ID firstPid = INVALID_PAGE_ID;
      fsmBitmapPageObject *obj = NULL;
      fsmFile *file = NULL;
      UINT32 lid = DMS_INVALID_LOGICCLID;
      CL_MB_ID mbID = INVALID_CL_MB_ID;
      ossValuePtr ptr = 0;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      file = _fsmFile;
      lid = _logicalId;
      mbID = _mbID;

      /// keep the first page and reinit it.
      /// release others.
      SDB_ASSERT(!_bitmaps.empty(), "can not be empty");
      SDB_ASSERT(0 == _bitmaps.begin()->first, "must be the first page");
      obj = _bitmaps.begin()->second;
      firstPid = obj->getPid();
      _bitmaps.erase(_bitmaps.begin());
      SAFE_OSS_DELETE(obj);
      for (_BITMAP_OBJ_MAP::const_iterator itr = _bitmaps.begin();
           itr != _bitmaps.end(); ++itr)
      {
         pids.push_back(itr->second->getPid());
      }

      for (UINT32 i = 0; i < _bitmapOwners.size(); ++i)
      {
         pids.push_back(_bitmapOwners.at(i));
      }

      close();
      file->getPagePtr(firstPid, ptr);
      initBitmapPageBuffer(ptr, lid);
      file->fsyncPage(firstPid, TRUE);
      file->releasePages(pids.size(), pids.data());

      rc = open(file, mbID, lid, 0, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reopen disk fsm:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::create(fsmFile *file,
                                  CL_MB_ID mbID,
                                  UINT32 logicalID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      PAGE_ID pid = INVALID_PAGE_ID;
      fsmBitmapPageObject *obj = NULL;
      fsmCLEntry entry;

      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen() ||
                       INVALID_CL_MB_ID == mbID ||
                       DMS_INVALID_LOGICCLID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _fsmFile = file;
      _logicalId = logicalID;
      _mbID = mbID;

      rc = createNewBitmapObj(0, &obj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      pid = obj->getPid();
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      entry.root = pid;
      entry.logicalID = logicalID;

      rc = updateEntrySlot(mbID, entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update entry slot:%d", rc);
         goto error;
      }

      rc = createSuperBitmap();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create super bitmaps:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         file->releasePages(1, &pid);
      }
      close();
      goto done;
   }

   INT32 diskFreeSpaceMap::open(fsmFile *file,
                                CL_MB_ID mbID,
                                UINT32 logicalID,
                                UINT32 dataPageCount,
                                BOOLEAN autoRecreateEntry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      fsmCLEntry entry;

      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen() ||
                       INVALID_CL_MB_ID == mbID ||
                       DMS_INVALID_LOGICCLID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _fsmFile = file;
      _logicalId = logicalID;
      _mbID = mbID;
      _totalDataPageCount = dataPageCount;

      rc = readEntrySlot(mbID, logicalID, entry);
      if (SDB_VESSEL_FSM_ENTRY_BROKEN == rc)
      {
         PD_LOG(PDERROR, "entry slot[%d,%d] is broken", mbID, logicalID);
         if (autoRecreateEntry)
         {
            rc = SDB_OK;
            close();
            rc = create(file, mbID, logicalID);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to recreate entry slot:%d", rc);
               goto error;
            }
         }
         else
         {
            goto error;
         }
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read root page:%d", rc);
         goto error;
      }

      rc = buildBitmapObjs(entry.root, dataPageCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build bitmap objs:%d", rc);
         goto error;
      }

      rc = createSuperBitmap();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create super bitmaps:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 diskFreeSpaceMap::incDataPageCount(UINT32 count)
   {
      INT32 rc = SDB_OK;
      UINT32 minBitmapNo = 0;
      UINT32 maxBitmapNo = 0;
      fsmBitmapPageObject *obj = NULL;
      ossSLatchGuard guard(&_latch, EXCLUSIVE, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.lock();

      minBitmapNo = getBitmapPageNo(_totalDataPageCount);
      maxBitmapNo = getBitmapPageNo(_totalDataPageCount + count - 1);
      /// We will not rollback _dataPageCount,
      /// which to keep _dataPageCount as same as cl meta data.
      _totalDataPageCount += count;

      rc = ensureSuperBitmapSize(maxBitmapNo + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to ensure super bitmap size:%d", rc);
         goto error;
      }

      for (UINT32 i = minBitmapNo; i < maxBitmapNo; ++i)
      {
         rc = ensureBitmapObj(i, &obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure bitmap obj[%d]:%d", i, rc);
            goto error;
         }

         obj->resetSizeIfHigher(FSM_BITMAP_PAGE_CAPACITY);
      }

      rc = ensureBitmapObj(maxBitmapNo, &obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure bitmap obj[%d]:%d", maxBitmapNo, rc);
         goto error;
      }

      obj->resetSizeIfHigher((_totalDataPageCount - maxBitmapNo * FSM_BITMAP_PAGE_CAPACITY));

      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::upgradePageSpaceLvl(UINT32 seq,
                                               INT32 lvl)
   {
      INT32 rc = SDB_OK;
      UINT32 bitmapNo = 0;
      fsmBitmapPageObject *obj = NULL;
      BOOLEAN upgraded = FALSE;
      ossSLatchGuard guard(&_latch, SHARED);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isValidFsmLvL(lvl))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_totalDataPageCount <= seq)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      bitmapNo = getBitmapPageNo(seq);
      obj = getBitmapPageObj(bitmapNo);
      if (NULL == obj)
      {
         PD_LOG(PDERROR, "failed to get bitmap obj[%d]", bitmapNo);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = obj->upgradePageSpaceLvl(seq, lvl, upgraded);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upgrade size lvl in obj[%d], rc:%d",
                bitmapNo, rc);
         goto error;
      }

      if (upgraded)
      {
         atomicSetSuperBitmap(lvl, bitmapNo);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::downgradePgaeSpaceLvl(UINT32 seq,
                                                 INT32 lvl)
   {
      INT32 rc = SDB_OK;
      UINT32 bitmapNo = 0;
      fsmBitmapPageObject *obj = NULL;
      ossSLatchGuard guard(&_latch, SHARED);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isValidFsmLvL(lvl))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_totalDataPageCount <= seq)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      bitmapNo = getBitmapPageNo(seq);
      obj = getBitmapPageObj(bitmapNo);
      if (NULL == obj)
      {
         PD_LOG(PDERROR, "failed to get bitmap obj[%d]", bitmapNo);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = obj->downgradePageSpaceLvl(seq, lvl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upgrade size lvl in obj[%d], rc:%d",
                bitmapNo, rc);
         goto error;
      }

      /// Do not test stats in obj when downgrade.
      /// We only update super bitmap when finding page.
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::ensureBitmapObj(UINT32 bitmapNo,
                                           fsmBitmapPageObject **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      fsmBitmapPageObject *obj = getBitmapPageObj(bitmapNo);
      if (NULL == obj)
      {
         fsmPageHead *head = NULL;
         PAGE_ID ownerPid = INVALID_PAGE_ID;
         UINT32 ownerPageNo = 0;
         UINT32 pos = 0;

         if (0 == bitmapNo)
         {
            PD_LOG(PDERROR, "root bitmap obj not found");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         ownerPageNo = (bitmapNo - 1) / FSM_BITMAP_OWNER_PAGE_CAPACITY;
         pos = (bitmapNo - 1) % FSM_BITMAP_OWNER_PAGE_CAPACITY;

         rc = ensureOwnerPage(ownerPageNo + 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure ower page:%d", rc);
            goto error;
         }

         ownerPid = _bitmapOwners.at(ownerPageNo);

         rc = getFsmPageHead(ownerPid,
                             FSM_FILE_PAGE_TYPE_BITMAP_OWNER,
                             &head);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d], rc:%d",
                   ownerPid, rc);
            goto error;
         }

         rc = createNewBitmapObj(bitmapNo, &obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new bitmap obj:%d", rc);
            goto error;
         }

         ((fsmBitmapOwnerPage *)head)->pages[pos] = obj->getPid();
         _fsmFile->fsyncPage(ownerPid, FALSE);
      }


      if (NULL != out)
      {
         *out = obj;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::ensureOwnerPage(UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(0 < count, "can not be zero");
      PAGE_ID prePid = INVALID_PAGE_ID;
      fsmPageHead *lastHead = NULL;
      PAGE_ID pid = INVALID_PAGE_ID;
      fsmBitmapPageObject *obj = NULL;

      if (count <= _bitmapOwners.size())
      {
         goto done;
      }

      if (_bitmapOwners.empty())
      {
         obj = getBitmapPageObj(0);
         if (NULL == obj)
         {
            PD_LOG(PDERROR, "root bitmap page does not exist");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = getFsmPageHead(obj->getPid(), FSM_FILE_PAGE_TYPE_BITMAP,
                             &lastHead);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page head:%d", rc);
            goto error;
         }

         prePid = obj->getPid();
      }
      else
      {
         rc = getFsmPageHead(_bitmapOwners.back(),
                             FSM_FILE_PAGE_TYPE_BITMAP_OWNER,
                             &lastHead);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page head:%d", rc);
            goto error;
         }
         prePid = _bitmapOwners.back();
      }


      while (_bitmapOwners.size() < count)
      {
         rc = createNewOwnerPage(prePid, pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new pmap page:%d", rc);
            goto error;
         }

         lastHead->next = pid;
         _fsmFile->fsyncPage(prePid, TRUE);
         _bitmapOwners.push_back(pid);
         prePid = pid;
         pid = INVALID_PAGE_ID;

         rc = getFsmPageHead(prePid, FSM_FILE_PAGE_TYPE_BITMAP_OWNER, &lastHead);
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

   INT32 diskFreeSpaceMap::createNewBitmapObj(UINT32 pageNo,
                                              fsmBitmapPageObject **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      PAGE_ID bitmapPid = INVALID_PAGE_ID;
      fsmBitmapPageObject *obj = NULL;
      fsmPageHead *head = NULL;

      if (0 < _bitmaps.count(pageNo))
      {
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      obj = SDB_OSS_NEW fsmBitmapPageObject();
      if (OSS_UNLIKELY(NULL == obj))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _fsmFile->allocateNewPage(bitmapPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page form file:%d", rc);
         goto error;
      }

      rc = initBitmapPage(bitmapPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init new fsm page:%d", rc);
         goto error;
      }

      rc = getFsmPageHead(bitmapPid, FSM_FILE_PAGE_TYPE_BITMAP, &head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", bitmapPid, rc);
         goto error;
      }

      rc = obj->initWhenCreate(pageNo, bitmapPid, (fsmBitmapPage *)head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init bitmap obj:%d", rc);
         goto error;
      }

      rc = _fsmFile->fsyncPage(bitmapPid, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync page[%d], rc:%d", bitmapPid, rc);
         goto error;
      }

      _bitmaps.insert(std::make_pair(pageNo, obj));

      if (NULL != out)
      {
         *out = obj;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != bitmapPid)
      {
         _fsmFile->releasePages(1, &bitmapPid);
      }
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   fsmBitmapPageObject *diskFreeSpaceMap::getBitmapPageObj(UINT32 i)
   {
      fsmBitmapPageObject *obj = NULL;
      _BITMAP_OBJ_MAP::const_iterator itr = _bitmaps.find(i);
      if (_bitmaps.end() != itr)
      {
         obj = itr->second;
      }
      return obj;
   }

   INT32 diskFreeSpaceMap::buildBitmapObj(UINT32 pageNo,
                                          PAGE_ID pid,
                                          UINT32 dataPageCount,
                                          fsmBitmapPage *page)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(NULL != page, "can nto be null");

      UINT32 abnormalCount = 0;
      fsmBitmapPageObject *obj = NULL;

      if (0 < _bitmaps.count(pageNo))
      {
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      obj = SDB_OSS_NEW fsmBitmapPageObject();
      if (NULL == obj)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = obj->initWhenOpen(pageNo, pid, dataPageCount,
                             page, abnormalCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rebuild page obj[%d,%d], rc:%d",
                pageNo, pid, rc);
         goto error;
      }
      else if (0 < abnormalCount)
      {
         PD_LOG(PDWARNING, "[%d] abnormal data pages found when rebuilding [%d,%d]",
                abnormalCount, pageNo, pid);
      }

      _bitmaps.insert(std::make_pair(pageNo, obj));
      
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   INT32 diskFreeSpaceMap::createNewOwnerPage(PAGE_ID pre, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(INVALID_PAGE_ID != pre, "can not be invalid");
      PAGE_ID ownerPid = INVALID_PAGE_ID;

      rc = _fsmFile->allocateNewPage(ownerPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page form file:%d", rc);
         goto error;
      }

      rc = initOwnerPage(ownerPid, pre);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init new fsm page:%d", rc);
         goto error;
      }

      rc = _fsmFile->fsyncPage(ownerPid, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync page[%d], rc:%d", ownerPid, rc);
         goto error;
      }

      pid = ownerPid;

   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != ownerPid)
      {
         _fsmFile->releasePages(1, &ownerPid);
      }
      goto done;
   }

   INT32 diskFreeSpaceMap::find(INT32 targetLvl,
                                BOOLEAN &found,
                                UINT32 &seq,
                                INT32 &lvl)
   {
      INT32 rc = SDB_OK;
      UINT32 bitmapNo = 0;
      fsmBitmapPageObject *obj = NULL;
      ossSLatchGuard guard(&_latch, SHARED, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidFsmLvL(targetLvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.lock();
      do
      {
         found = FALSE;
         seq = 0;
         lvl = FSM_INVALID_SPACE_LVL;

         if (0 == _totalDataPageCount ||
             !findBitmapFromSuperBitmap(targetLvl, bitmapNo))
         {
            goto done;
         }

         obj = getBitmapPageObj(bitmapNo);
         if (NULL == obj)
         {
            PD_LOG(PDERROR, "failed to get bitmap obj[%d]", bitmapNo);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = obj->findAndClear(targetLvl, found, seq, lvl);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free space from obj:%d", rc);
            goto error;
         }

         if (found)
         {
            break;
         }

         /// The page found in super bitmap but nothing found in bitmap obj.
         /// Which means no more resources at target or upper lvls exist. 
         /// clear it in super bitmaps.
         /// WARNING: In the moment, if some one is upgrading lvl on this page,
         /// super bitmap may be overwritten by mistake.
         atomicUnsetSuperBitmapFromLvl(targetLvl, bitmapNo);
      } while (TRUE);
   
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN diskFreeSpaceMap::findBitmapFromSuperBitmap(INT32 targetLvl,
                                                       UINT32 &bitmapNo)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(0 != _superBitmap[0].getSize(), "can not be empty");
      SDB_ASSERT(FSM_INVALID_SPACE_LVL != targetLvl, "can not be invalid");
      UINT32 bitsCount = _superBitmap[0].getSize() >> 3;
      UINT32 offset = 0;
      for (INT32 i = targetLvl; i < (INT32)FSM_SPACE_LVL_COUNT; ++i)
      {
         if (findFirstNonzeroBit(bitsCount, 0,
                                 (const UINT64 *)(_superBitmap[i].getBuffer()),
                                 offset))
         {
            r = TRUE;
            bitmapNo = offset;
            break;
         }
      }

      return r;
   }

   void diskFreeSpaceMap::atomicSetSuperBitmap(INT32 lvl, UINT32 bitmapNo)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      UINT32 bitsCount = _superBitmap[lvl].getSize() >> 3;
      atomicSetBitAtOffset(bitsCount,
                           (UINT64 *)(_superBitmap[lvl].getBuffer()),
                           bitmapNo);
   }

   void diskFreeSpaceMap::atomicUnsetSuperBitmap(INT32 lvl, UINT32 bitmapNo)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      UINT32 bitsCount = _superBitmap[lvl].getSize() >> 3;
      atomicClearBitAtOffset(bitsCount,
                             (UINT64 *)(_superBitmap[lvl].getBuffer()),
                             bitmapNo);
   }


   INT32 diskFreeSpaceMap::readEntrySlot(CL_MB_ID mbID,
                                         UINT32 logicalID,
                                         fsmCLEntry &entry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      SDB_ASSERT(NULL != _fsmFile, "can not be null");
      
      fsmCLEntry *slot = _fsmFile->getEntrySlotPtr(mbID);
      SDB_ASSERT(NULL != slot, "can not be null");
      if (!isValidFsmEntry(*slot) || logicalID != slot->logicalID)
      {
         PD_LOG(PDERROR, "entry slot of [%d] is broken", mbID);
         rc = SDB_VESSEL_FSM_ENTRY_BROKEN;
         goto error;
      }
      entry = *slot;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::buildBitmapObjs(PAGE_ID root, UINT32 totalPageCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(INVALID_PAGE_ID != root, "can not be invalid");
      SDB_ASSERT(_bitmaps.empty(), "must be empty");
      fsmPageHead *head = NULL;
      PAGE_ID ownerPid = INVALID_PAGE_ID;
      UINT32 pageCountInBitmap = 0;
      UINT32 totalCount = totalPageCount;

      rc = getFsmPageHead(root, FSM_FILE_PAGE_TYPE_BITMAP, &head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get root page:%d", rc);
         goto error;
      }

      pageCountInBitmap = totalCount < FSM_BITMAP_PAGE_CAPACITY ?
                          totalCount : FSM_BITMAP_PAGE_CAPACITY;

      rc = buildBitmapObj(0, root, pageCountInBitmap, (fsmBitmapPage *)head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rebuild bitmap obj:%d", rc);
         goto error;
      }
      totalCount -= pageCountInBitmap;

      ownerPid = head->next;
      while (INVALID_PAGE_ID != ownerPid)
      {
         UINT32 bitmapPageNo = _bitmapOwners.size() * FSM_BITMAP_OWNER_PAGE_CAPACITY + 1;
         fsmBitmapOwnerPage *owner = NULL;
         _bitmapOwners.push_back(ownerPid);

         rc = getFsmPageHead(ownerPid, FSM_FILE_PAGE_TYPE_BITMAP_OWNER, &head);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get owner page[%d], rc:%d", ownerPid, rc);
            goto error;
         }

         owner = (fsmBitmapOwnerPage *)head;
         for (UINT32 i = 0; i < FSM_BITMAP_OWNER_PAGE_CAPACITY; ++i)
         {
            fsmPageHead *bitmapPageHead = NULL;
            pageCountInBitmap = totalCount < FSM_BITMAP_PAGE_CAPACITY ?
                                totalCount : FSM_BITMAP_PAGE_CAPACITY;
            PAGE_ID bitmapPid = owner->pages[i];
            if (INVALID_PAGE_ID == bitmapPid)
            {
               if (0 == totalCount)
               {
                  break;
               }

               PD_LOG(PDWARNING, "failed to get bitmap pid at slot[%d], recreate it", i);
               fsmBitmapPageObject *newObj = NULL;
               rc = createNewBitmapObj(bitmapPageNo + i, &newObj);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to recreate bitmap page[%d], rc:%d",
                         bitmapPageNo + i, rc);
                  goto error;
               }

               owner->pages[i] = newObj->getPid();
               _fsmFile->fsyncPage(ownerPid, TRUE);

               newObj->resetSizeIfHigher(pageCountInBitmap);

               totalCount -= pageCountInBitmap;
               continue;
            }

            rc = getFsmPageHead(bitmapPid, FSM_FILE_PAGE_TYPE_BITMAP,
                                &bitmapPageHead);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get bitmap page[%d], rc:%d",
                      bitmapPid, rc);
               goto error;
            }

            rc = buildBitmapObj(bitmapPageNo + i,
                                bitmapPid,
                                pageCountInBitmap,
                                (fsmBitmapPage *)bitmapPageHead);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build bitmap obj[%d], rc:%d",
                      bitmapPageNo + i, rc);
               goto error;
            }

            totalCount -= pageCountInBitmap;
         }

         ownerPid = owner->head.next;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::updateEntrySlot(CL_MB_ID mbID,
                                           const fsmCLEntry &entry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(isValidFsmEntry(entry), "must be valid");
      SDB_ASSERT(NULL != _fsmFile, "can not be null");

      fsmCLEntry *slot = _fsmFile->getEntrySlotPtr(mbID);
      SDB_ASSERT(NULL != slot, "can not be null");
      *slot = entry;
      rc = _fsmFile->fsyncEntry(mbID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync entry slot:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::destroyEntrySlot(CL_MB_ID mbID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      fsmCLEntry *slot = _fsmFile->getEntrySlotPtr(mbID);
      SDB_ASSERT(NULL != slot, "can not be null");
      slot->logicalID = DMS_INVALID_LOGICCLID;
      slot->root = INVALID_PAGE_ID;
      rc = _fsmFile->fsyncEntry(mbID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync entry slot:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::getFsmPageHead(PAGE_ID pid, UINT16 type,
                                          fsmPageHead **head)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      ossValuePtr ptr = 0;
      const fsmPageHead *tmp = NULL;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       NULL == head))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
         goto error;
      }

      tmp = (const fsmPageHead *)ptr;
      if (!isValidFsmPageHead(*tmp))
      {
         PD_LOG(PDERROR, "page[%d] head is broken", pid);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (_logicalId != tmp->clLogicalId)
      {
         PD_LOG(PDERROR, "logical id[%d] does not match the one on disk[%d]",
                _logicalId, tmp->clLogicalId);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
      else if (type != tmp->type)
      {
         PD_LOG(PDERROR, "type[%d] does not match the one on disk[%d]",
                type, tmp->type);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      *head = (fsmPageHead *)ptr;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::initBitmapPage(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      CHAR *buffer = NULL;
      fsmPageHead *head = NULL;
      ossValuePtr ptr = 0;
      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      buffer = (CHAR *)SDB_THREAD_ALLOC(FSM_BITMAP_PAGE_SIZE);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemset(buffer, 0, FSM_BITMAP_PAGE_SIZE);
      head = (fsmPageHead *)buffer;
      
      head->version = FSM_FILE_PAGE_VERSION;
      head->type = FSM_FILE_PAGE_TYPE_BITMAP;
      head->clLogicalId = _logicalId;
      head->flags = 0;
      head->pre = INVALID_PAGE_ID;
      head->next = INVALID_PAGE_ID;
      head->pad = 0;

      ossMemcpy((CHAR *)ptr, buffer, FSM_BITMAP_PAGE_SIZE);
   done:
      if (NULL != buffer)
      {
         SDB_THREAD_FREE(buffer);
      }
      return rc;
   error:
      goto done;
   }

   void diskFreeSpaceMap::initBitmapPageBuffer(ossValuePtr ptr,
                                               UINT32 logicalId,
                                               PAGE_ID pre,
                                               PAGE_ID next)
   {
      SDB_ASSERT(0 != ptr, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalId, "can not be invalid");
      static_assert(FSM_BITMAP_PAGE_SIZE <= 65536, "can not be too large");
      CHAR buffer[FSM_BITMAP_PAGE_SIZE] = {};
      fsmPageHead *head = (fsmPageHead *)buffer;

      head->version = FSM_FILE_PAGE_VERSION;
      head->type = FSM_FILE_PAGE_TYPE_BITMAP;
      head->clLogicalId = logicalId;
      head->flags = 0;
      head->pre = pre;
      head->next = next;
      head->pad = 0;

      ossMemcpy((void *)ptr, buffer, FSM_BITMAP_PAGE_SIZE);
      return;
   }

   INT32 diskFreeSpaceMap::initOwnerPage(PAGE_ID pid, PAGE_ID pre)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pre, "can not be invalid");

      CHAR *buffer = NULL;
      fsmBitmapOwnerPage *page = NULL;
      ossValuePtr ptr = 0;

      rc = _fsmFile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      buffer = (CHAR *)SDB_THREAD_ALLOC(FSM_FILE_PAGE_SIZE);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }
      
      page = (fsmBitmapOwnerPage *)buffer;
      page->head.version = FSM_FILE_PAGE_VERSION;
      page->head.type = FSM_FILE_PAGE_TYPE_BITMAP_OWNER;
      page->head.clLogicalId = _logicalId;
      page->head.flags = 0;
      page->head.pre = pre;
      page->head.next = INVALID_PAGE_ID;
      page->head.pad = 0;

      page->flags = 0;
      ossMemset(page->pages, 0xFF, sizeof(page->pages));
      ossMemcpy((void *)ptr, buffer, FSM_FILE_PAGE_SIZE);
   done:
      if (NULL != buffer)
      {
         SDB_THREAD_FREE(buffer);
      }
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::createSuperBitmap()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");

      if (_bitmaps.empty())
      {
         goto done;
      }

      rc = ensureSuperBitmapSize(_bitmaps.size());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure super bitmaps size:%d", rc);
         goto error;
      }

      for (_BITMAP_OBJ_MAP::const_iterator itr = _bitmaps.begin();
           itr != _bitmaps.end(); ++itr)
      {
         SDB_ASSERT(NULL != itr->second, "can not be null");
         if (OSS_UNLIKELY(_bitmaps.size() <= itr->first))
         {
            PD_LOG(PDERROR, "unexpected bitmap no[%d]", itr->first);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         for (INT32 i = 0; i < (INT32)FSM_SPACE_LVL_COUNT; ++i)
         {
            if (0 < itr->second->getStats(i))
            {
               setBitIfZeroed((itr->first >> 6) + 1,
                               (UINT64 *)(_superBitmap[i].getBuffer()),
                              itr->first);

            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskFreeSpaceMap::ensureSuperBitmapSize(UINT32 bitmapPageCount)
   {
      SDB_ASSERT(64 == SUPER_BITMAP_EXTENDING_CAPACITY, "must be 64");
      INT32 rc = SDB_OK;
      UINT32 newBlockSize = (ossAlign64(bitmapPageCount) / SUPER_BITMAP_EXTENDING_CAPACITY)
                            * sizeof(UINT64);
      UINT32 currentSize = _superBitmap[0].getSize();
      memoryBlock mbs[FSM_SPACE_LVL_COUNT];

      if (newBlockSize <= currentSize)
      {
         goto done;
      }

      for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         rc = mbs[i].resize(newBlockSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to resize memory block:%d", rc);
            goto error;
         }

         if (0 != currentSize)
         {
            ossMemcpy(mbs[i].getBuffer(),
                      _superBitmap[i].getBuffer(),
                      currentSize);
         }
      }


      for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         _superBitmap[i] = std::move(mbs[i]);
      }
   done:
      return rc;
   error:
      goto done;
   }
   
   void diskFreeSpaceMap::atomicUnsetSuperBitmapFromLvl(INT32 lvl, UINT32 bitmapNo)
   {
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      for (INT32 i = lvl; i < (INT32)FSM_SPACE_LVL_COUNT; ++i)
      {
         atomicUnsetSuperBitmap(i, bitmapNo);
      }
      return;
   }


}//namespace vessel
}//namespace engine