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

   Source File Name = inMemBitmap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/inMemBitmap.h"
#include "ossLikely.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/bitmapUtils.h"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   inMemBitmap::_inMemBitPage::_inMemBitPage()
   {}

   inMemBitmap::_inMemBitPage::~_inMemBitPage()
   {
      fini();
   }

   void inMemBitmap::_inMemBitPage::fini()
   {
      _pageID = -1;
      _free = 0;
      _firstFreeBits = -1;
      if (NULL != _buf)
      {
         SDB_THREAD_FREE(_buf);
         _buf = NULL;
      }
      return;
   }

   INT32 inMemBitmap::_inMemBitPage::initWithNoFree(INT32 pageId,
                                                    UINT32 capacity)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isReady(), "do not reinit");
      UINT32 bufferSize = capacity >> 3;

      if (OSS_UNLIKELY(pageId < 0 ||
                       0 == capacity ||
                       !ossIsAligned64(capacity)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _buf = (UINT64 *)SDB_THREAD_ALLOC(bufferSize);
      if (NULL == _buf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemset(_buf, 0x0, bufferSize);
      
      _pageID = pageId;
      _free = 0;
      _firstFreeBits = -1;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 inMemBitmap::_inMemBitPage::init(INT32 pageId,
                                          UINT32 capacity)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isReady(), "do not reinit");
      UINT32 bufferSize = capacity >> 3;

      if (OSS_UNLIKELY(pageId < 0 ||
                       0 == capacity ||
                       !ossIsAligned64(capacity)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _buf = (UINT64 *)SDB_THREAD_ALLOC(bufferSize);
      if (NULL == _buf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemset(_buf, 0xFF, bufferSize);
      
      _pageID = pageId;
      _free = capacity;
      _firstFreeBits = 0;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 inMemBitmap::_inMemBitPage::initFromBuf(INT32 pageId,
                                                 UINT32 capacity,
                                                 const UINT64 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isReady(), "do not reinit");
      UINT32 bufSize = capacity >> 3;
      UINT32 count = capacity >> 6;

      if (OSS_UNLIKELY(pageId < 0 ||
                       0 == capacity ||
                       !ossIsAligned64(capacity) ||
                       NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pageID = pageId;
      _buf = (UINT64 *)SDB_THREAD_ALLOC(bufSize);
      if (NULL == _buf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemcpy(_buf, buf, bufSize);
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 freeCnt = ossGetNonZeroBitCount64(_buf[i]);
         if (0 < freeCnt && _firstFreeBits < 0)
         {
            _firstFreeBits = (INT32)i;
         }
         _free += freeCnt;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 inMemBitmap::_inMemBitPage::allocate(UINT32 capacity,
                                              UINT32 count,
                                              UINT32 *buf,
                                              UINT32 *stillFreeCount)
   {
      INT32 rc = SDB_OK;
      UINT32 allocatedCount = 0;
      UINT32 bitsCount = capacity >> 6;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == capacity ||
                            !ossIsAligned64(capacity)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_free < count)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      /// from here, do not goto error.
      SDB_ASSERT(0 <= _firstFreeBits, "impossible");

      do
      {
         UINT32 offset = 0;
         if (findAndClearFirstNonzeroBit(bitsCount,
                                         (UINT32)_firstFreeBits,
                                         _buf, offset))
         {
            buf[allocatedCount++] = offset;
            --_free;
         }
         else
         {
            SDB_ASSERT(FALSE, "free count in head is not zero, but failed to find free bit from bitmap");
            PD_LOG(PDERROR, "free count in head is not zero, but failed to find free bit from bitmap");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      } while (allocatedCount < count);

      if (0 < _free)
      {
         updateFirstFree(bitsCount, (buf[count - 1] >> 6));
      }
      else
      {
         _firstFreeBits = -1;
      }

      if (NULL != stillFreeCount)
      {
         *stillFreeCount = _free;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void inMemBitmap::_inMemBitPage::release(UINT32 capacity,
                                            UINT32 count,
                                            const UINT32 *buf)
   {
      UINT32 bitsCount = capacity >> 6;

      if (OSS_UNLIKELY(!isReady()))
      {
         SDB_ASSERT(FALSE, "invalid page");
         goto done;
      }
      if (OSS_UNLIKELY(0 == capacity ||
                       !ossIsAligned64(capacity)))
      {
         SDB_ASSERT(FALSE, "not valid capacity");
         goto done;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(capacity < (_free + count)))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = buf[i];
         if (setBitIfZeroed(bitsCount, _buf, offset))
         {
            ++_free;
            if (_firstFreeBits < 0)
            {
               _firstFreeBits = (INT32)offset;
            }
            else if ((INT32)offset < _firstFreeBits)
            {
               _firstFreeBits = (INT32)offset;
            }
         }
         else
         {
            SDB_ASSERT(FALSE, "bit to released is free");
         }
      }
   done:
      return;
   }

   void inMemBitmap::_inMemBitPage::updateFirstFree(UINT32 bitsCount, UINT32 beginBits)
   {
      SDB_ASSERT(isReady(), "impossible");
      SDB_ASSERT(0 < _free, "impossible");
      UINT32 offset = 0;
      if (findFirstNonzeroBit(bitsCount, beginBits, _buf, offset))
      {
         _firstFreeBits = offset >> 6;
      }
      else
      {
         _firstFreeBits = -1;
      }
      
   done:
      return;
   }

   INT32 inMemBitmap::_inMemBitPage::test(UINT32 capacity,
                                          UINT32 offset,
                                          BOOLEAN &isFree)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(capacity <= offset))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      isFree = testBitIsNonzero(capacity >> 6, _buf, offset);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::_inMemBitPage::occupy(UINT32 capacity,
                                            UINT32 offset,
                                            UINT32 *stillFreeCount)
   {
      INT32 rc = SDB_OK;
      UINT32 bitsCount = capacity >> 6;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(capacity <= offset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == _free))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      if (!clearBitIfNonzero(bitsCount, _buf, offset))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      SDB_ASSERT(0 < _free, "impossible");
      --_free;
      if (0 < _free)
      {
         updateFirstFree(bitsCount, _firstFreeBits);
      }
      else
      {
         _firstFreeBits = -1;
      }

      if (NULL != stillFreeCount)
      {
         *stillFreeCount = _free;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void inMemBitmap::_inMemBitPage::bitsAnd(UINT32 capacity, const UINT64 *bits)
   {
      SDB_ASSERT(0 < capacity, "can not be zero");
      SDB_ASSERT(ossIsPowerOf2(capacity), "must be power of 2");
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(isReady(), "must be ready");
      UINT32 count = capacity >> 6;

      bitsAndMerge(count, bits, _buf);

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 freeCnt = ossGetNonZeroBitCount64(_buf[i]);
         if (0 < freeCnt && _firstFreeBits < 0)
         {
            _firstFreeBits = (INT32)i;
         }
         _free += freeCnt;
      }
      return;
   }

////////////////inMemBitmap
   inMemBitmap::inMemBitmap()
   {}

   inMemBitmap::~inMemBitmap()
   {
      fini();
   }

   INT32 inMemBitmap::init(UINT32 pageCapacity,
                           const options &o)
   {
      return init(pageCapacity, o, &_innerLatch);
   }
   
   INT32 inMemBitmap::initWithNoLatch(UINT32 pageCapacity,
                                      const options &o)
   {
      return init(pageCapacity, o, NULL);
   }

   INT32 inMemBitmap::init(UINT32 pageCapacity,
                           const options &o,
                           ossSpinXLatch *latch)
   {
      INT32 rc = SDB_OK;
      fini();
      if (OSS_UNLIKELY(0 == pageCapacity ||
                       !ossIsAligned64(pageCapacity)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(pageCapacity <= o.freeBound))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(o.maxBitmapPageCount <= o.bitmapPageSkipped))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pageCapacity = pageCapacity;
      _freeBound = o.freeBound;
      _pageSkipped = o.bitmapPageSkipped;
      _pageCount = o.bitmapPageSkipped;
      _maxBitmapPageCount = o.maxBitmapPageCount;
      _latch = latch;
   done:
      return rc;
   error:
      goto done;
   }

   void inMemBitmap::fini()
   {
      if (!_pagesWithLowFreeCount.empty())
      {
         _PAGE_MAP::iterator itr = _pagesWithLowFreeCount.begin();
         for (; itr != _pagesWithLowFreeCount.end(); ++itr)
         {
            if (NULL != itr->second)
            {
               SDB_OSS_DEL(itr->second);
            }
         }
         _pagesWithLowFreeCount.clear();
      }

      if (!_pagesWithHighFreeCount.empty())
      {
         _PAGE_MAP::iterator itr = _pagesWithHighFreeCount.begin();
         for (; itr != _pagesWithHighFreeCount.end(); ++itr)
         {
            if (NULL != itr->second)
            {
               SDB_OSS_DEL(itr->second);
            }
         }
         _pagesWithHighFreeCount.clear();
      }

      _latch = NULL;
      _pageCapacity = 0;
      _freeBound = 0;
      _pageSkipped = 0;
      _maxBitmapPageCount = 0;
      _pageCount = 0;
      _totalFreeCount = 0;

      return;
   }

   INT32 inMemBitmap::incPageCount(UINT32 cnt)
   {
      INT32 rc = SDB_OK;
      if (OSS_LIKELY(isInitialized()))
      {
         ossXLatchGuard guard(_latch);
         if ((_pageCount + cnt) <= _maxBitmapPageCount)
         {
            _pageCount += cnt;
         }
         else
         {
            rc = SDB_VESSEL_OUT_OF_RESOURCE;
            goto error;
         }
      }
      else
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::allocateNewBitmapPage()
   {
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(_latch, FALSE);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      guard.lock();
      rc = _allocateNewBitmapPage();
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::_allocateNewBitmapPage()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      _inMemBitPage *page = NULL;

      if (_pageCount == _maxBitmapPageCount)
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      page = SDB_OSS_NEW _inMemBitPage();
      if (NULL == page)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = page->init(_pageCount++, _pageCapacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init bitmap page:%d", rc);
         goto error;
      }

      _pagesWithHighFreeCount[page->getPageID()] = page;
      _totalFreeCount += _pageCapacity;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 inMemBitmap::_allocateNewBitmapPages(UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(1 < count, "impossible");
      ossPoolVector<_inMemBitPage *> pages;

      if (_maxBitmapPageCount < (_pageCount + count))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      pages.reserve(count);
      for (UINT32 i = 0; i < count; ++i)
      {
         _inMemBitPage *page = SDB_OSS_NEW _inMemBitPage();
         if (NULL == page)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = page->init(_pageCount + i, _pageCapacity);
         if (SDB_OK != rc)
         {
            goto error;
         }

         pages.push_back(page);
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         _inMemBitPage *page = pages[i];
         _pagesWithHighFreeCount[page->getPageID()] = page;
         _totalFreeCount += _pageCapacity;
      }
      _pageCount += count;
   done:
      return rc;
   error:
      for (UINT32 i = 0; i < pages.size(); ++i)
      {
         SAFE_OSS_DELETE(pages[i]);
      }
      goto done;
   }

   INT32 inMemBitmap::allocateNewBitmapPages(UINT32 count)
   {
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(_latch, FALSE);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count))
      {
         return SDB_INVALIDARG;
      }

      guard.lock();
      if (1 == count)
      {
         rc = _allocateNewBitmapPage();
      }
      else
      {
         rc = _allocateNewBitmapPages(count);
      }

      if (SDB_OK != rc)
      {
         goto error;
      }
   
   done:
      return rc;
   error:
      goto done;
   }


   INT32 inMemBitmap::allocateBits(UINT32 count,
                                   UINT32 *buf,
                                   UINT32 autoExtendingCount)
   {
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(_latch, FALSE);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_pageCapacity < count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.lock();
      do
      {
         rc = _allocateBits(count, buf);
         if (SDB_OK == rc)
         {
            goto done;
         }
         else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE == rc)
         {
            if (0 == autoExtendingCount)
            {
               goto error;
            }
            else if (1 == autoExtendingCount)
            {
               rc = _allocateNewBitmapPage();
               if (SDB_OK != rc)
               {
                  goto error;
               }
            }
            else
            {
               rc = _allocateNewBitmapPages(autoExtendingCount);
               if (SDB_OK != rc)
               {
                  goto error;
               }
            }
         }
         else
         {
            goto error;
         }
      } while (TRUE);
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::ensureBitmapPageCount(UINT32 count)
   {
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(_latch, FALSE);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (count <= _pageCount)
      {
         goto done;
      }

      guard.lock();
      if (count <= _pageCount)
      {
         goto done;
      }

      if ((_pageCount + 1) == count)
      {
         rc = _allocateNewBitmapPage();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _allocateNewBitmapPages(count - _pageCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate multiple pages:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::occupy(UINT32 offset)
   {
      return occupy(1, &offset);
   }

   INT32 inMemBitmap::occupy(UINT32 count, const UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _occupy(count, buf);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::_occupy(UINT32 count, const UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");

      UINT32 occupied = 0;
      ossScopedLock guard(_latch);

      for (UINT32 i = 0; i < count; ++i)
      {
         INT32 pageId = buf[i] / _pageCapacity;
         UINT32 offset = buf[i] % _pageCapacity;
         _inMemBitPage *page = NULL;

         if ((INT32)_pageCount <= pageId)
         {
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }
         else if (pageId < (INT32)_pageSkipped)
         {
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }

         page = getFromHFC(pageId);
         if (NULL != page)
         {
            rc = occupyFromHFC(page, offset);
            if (SDB_OK != rc)
            {
               goto error;
            }
            ++occupied;
            continue;
         }
         
         page = getFromLFC(pageId);
         if (NULL != page)
         {
            rc = occupyFromLFC(page, offset);
            if (SDB_OK != rc)
            {
               goto error;
            }
            ++occupied;
            continue;
         }

         /// already been occupied.
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

   done:
      return rc;
   error:
      if (0 < occupied)
      {
         _releaseBits(occupied, buf);
      }
      goto done;
   }

   INT32 inMemBitmap::occupyFromHFC(_inMemBitPage *page,
                                    UINT32 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != page, "can not be null");

      UINT32 stillFreeCount = 0;
      rc = page->occupy(_pageCapacity, offset, &stillFreeCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      --_totalFreeCount;
      if (0 == stillFreeCount)
      {
         _pagesWithHighFreeCount.erase(page->getPageID());
         SDB_OSS_DEL page;
      }
      else if (stillFreeCount < _freeBound)
      {
         _pagesWithHighFreeCount.erase(page->getPageID());
         _pagesWithLowFreeCount[page->getPageID()] =  page;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::occupyFromLFC(_inMemBitPage *page,
                                    UINT32 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != page, "can not be null");

      UINT32 stillFreeCount = 0;
      rc = page->occupy(_pageCapacity, offset, &stillFreeCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      --_totalFreeCount;
      if (0 == stillFreeCount)
      {
         _pagesWithLowFreeCount.erase(page->getPageID());
         SDB_OSS_DEL page;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::_allocateBits(UINT32 count, UINT32 *buf)
   {
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");
      INT32 rc = SDB_OK;

      if (_totalFreeCount < (INT32)count)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      rc = allocateOnSinglePage(count, buf);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE != rc)
      {
         PD_LOG(PDERROR, "failed to allocate from bitmap:%d", rc);
         goto error;
      }
      else
      {
         rc = allocateOnMultiPages(count, buf);
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

   INT32 inMemBitmap::allocateOnSinglePage(UINT32 count, UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");

      if (_freeBound <= count || _pagesWithLowFreeCount.empty())
      {
         rc = allocateBitsFromHFC(count, buf);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = allocateBitsFromLFC(count, buf);
         if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE == rc)
         {
            rc = allocateBitsFromHFC(count, buf);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }  
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::allocateOnMultiPages(UINT32 count, UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");

      UINT32 allocated = 0;

      do
      {
         rc = allocateOnSinglePage(1, buf + allocated);
         if (SDB_OK != rc)
         {
            goto error;
         }

         ++allocated;
      } while (allocated < count);
      
   done:
      return rc;
   error:
      if (0 < allocated)
      {
         _releaseBits(allocated, buf);
      }
      goto done;
   }

   void inMemBitmap::releaseBits(UINT32 count, const UINT32 *buf)
   {
      ossXLatchGuard guard(_latch, FALSE);
      if (OSS_UNLIKELY(!isInitialized()))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         goto done;
      }

      guard.lock();
      _releaseBits(count, buf);
   done:
      return;
   }

   void inMemBitmap::release(UINT32 bitOffset)
   {
      return releaseBits(1, &bitOffset);
   }

   void inMemBitmap::_releaseBits(UINT32 count, const UINT32 *buf)
   {
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");
      static const UINT32 BATCH_SIZE = 16;
      UINT32 batch[BATCH_SIZE];
      UINT32 released = 0;

      while (released < count)
      {
         _PAGE_MAP::iterator itr;
         INT32 pageId = buf[released] / _pageCapacity;
         UINT32 currentBatchCount = 0;
         batch[currentBatchCount++] = buf[released] % _pageCapacity;
         while (currentBatchCount < BATCH_SIZE &&
                ((released + currentBatchCount) < count))
         {
            INT32 nextPage = buf[released + currentBatchCount] / _pageCapacity;
            if (pageId == nextPage)
            {
               batch[currentBatchCount] = buf[released + currentBatchCount] % _pageCapacity;
               ++currentBatchCount;
            }
            else
            {
               break;
            }
         }

         released += currentBatchCount;

         if ((INT32)_pageCount <= pageId)
         {
            SDB_ASSERT(FALSE, "invalid offset to be released");
            continue;
         }
         if ((INT32)_pageSkipped > pageId)
         {
            SDB_ASSERT(FALSE, "invalid offset to be released");
            continue;
         }

         itr = _pagesWithHighFreeCount.find(pageId);
         if (_pagesWithHighFreeCount.end() != itr)
         {
            releaseBitFromHFC(currentBatchCount, batch, itr->second);
         }
         else
         {
            itr = _pagesWithLowFreeCount.find(pageId);
            if (_pagesWithLowFreeCount.end() != itr)
            {
               releaseBitFromLFC(currentBatchCount, batch, itr->second);
            }
            else
            {
               releaseAtDestroyedPage(pageId, currentBatchCount, batch);
            }
         }
      }
   done:
      return;
   }

   void inMemBitmap::releaseBitFromHFC(UINT32 count,
                                        const UINT32 *buf,
                                        _inMemBitPage *page)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(NULL != page, "can not be null");

      UINT32 oldFree = page->getFree();
      page->release(_pageCapacity, count, buf);
      if (OSS_LIKELY(page->getFree() > oldFree))
      {
         _totalFreeCount += (page->getFree() - oldFree);
      }
      else
      {
         SDB_ASSERT(FALSE, "free count not changed");
      }
      return;
   }

   void inMemBitmap::releaseBitFromLFC(UINT32 count,
                                       const UINT32 *buf,
                                       _inMemBitPage *page)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(NULL != page, "can not be null");

      UINT32 oldFree = page->getFree();
      page->release(_pageCapacity, count, buf);

      if (OSS_LIKELY(page->getFree() > oldFree))
      {
         _totalFreeCount += (page->getFree() - oldFree);
      }
      else
      {
         SDB_ASSERT(FALSE, "free count not changed");
      }

      if (_freeBound <= page->getFree())
      {
         _pagesWithLowFreeCount.erase(page->getPageID());
         _pagesWithHighFreeCount[page->getPageID()] = page;
      }
      return ;
   }

   void inMemBitmap::bitsAndFromHFC(_inMemBitPage *page,
                                    const UINT64 *bits)
   {
      SDB_ASSERT(NULL != page, "can not be null");
      SDB_ASSERT(NULL != bits, "can not be null");
      UINT32 oldFree = page->getFree();
      UINT32 newFree = 0;
      page->bitsAnd(_pageCapacity, bits);
      newFree = page->getFree();
      SDB_ASSERT(newFree <= oldFree, "impossible");
      _totalFreeCount -= (oldFree - newFree);
      if (0 == newFree)
      {
         _pagesWithHighFreeCount.erase(page->getPageID());
         SDB_OSS_DEL page;
      }
      else if (newFree < _freeBound)
      {
         _pagesWithHighFreeCount.erase(page->getPageID());
         _pagesWithLowFreeCount[page->getPageID()] = page;
      }
      return;
   }

   void inMemBitmap::bitsAndFromLFC(_inMemBitPage *page,
                                    const UINT64 *bits)
   {
      SDB_ASSERT(NULL != page, "can not be null");
      SDB_ASSERT(NULL != bits, "can not be null");
      UINT32 oldFree = page->getFree();
      UINT32 newFree = 0;
      page->bitsAnd(_pageCapacity, bits);
      newFree = page->getFree();
      SDB_ASSERT(newFree <= oldFree, "impossible");
      _totalFreeCount -= (oldFree - newFree);
      if (0 == newFree)
      {
         _pagesWithLowFreeCount.erase(page->getPageID());
         SDB_OSS_DEL page;
      }
      return;
   }

   void inMemBitmap::releaseAtDestroyedPage(INT32 pageID, UINT32 count, const UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 <= pageID, "can not be invalid");
      SDB_ASSERT(0 < count && NULL != buf, "can not be invalid");
      SDB_ASSERT((INT32)_pageSkipped <= pageID, "impossible");
      _inMemBitPage *page = SDB_OSS_NEW _inMemBitPage();
      if (NULL == page)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         goto error;
      }

      rc = page->initWithNoFree(pageID, _pageCapacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page[%d], rc:%d", pageID, rc);
         goto error;
      }

      page->release(_pageCapacity, count, buf);
      if (OSS_UNLIKELY(0 == page->getFree()))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      if (page->getFree() < _freeBound)
      {
         _pagesWithLowFreeCount[pageID] = page;
      }
      else
      {
         _pagesWithHighFreeCount[pageID] = page;
      }

      _totalFreeCount += page->getFree();
      
   done:
      return;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 inMemBitmap::allocateBitsFromHFC(UINT32 count, UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < count && NULL != buf, "can not be invalid");
      _inMemBitPage *page = NULL;
      UINT32 stillFreeCount = 0;
      std::map<INT32, _inMemBitPage*>::iterator itr = _pagesWithHighFreeCount.begin();
      for (; itr != _pagesWithHighFreeCount.end(); ++itr)
      {
         page = itr->second;
         if (page->getFree() < count)
         {
            continue;
         }
      }

      if (NULL == page)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      rc = page->allocate(_pageCapacity, count, buf, &stillFreeCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         buf[i] = buf[i] + (_pageCapacity * page->getPageID());
      }

      _totalFreeCount -= count;

      if (0 == stillFreeCount)
      {
         _pagesWithHighFreeCount.erase(page->getPageID());
         SDB_OSS_DEL page;
      }
      else if (stillFreeCount < _freeBound)
      {
         _pagesWithLowFreeCount[page->getPageID()] = page;
         _pagesWithHighFreeCount.erase(page->getPageID());
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::allocateBitsFromLFC(UINT32 count, UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < count && NULL != buf, "can not be invalid");
      _inMemBitPage *page = NULL;
      UINT32 stillFreeCount = 0;
      std::map<INT32, _inMemBitPage*>::iterator itr = _pagesWithLowFreeCount.begin();
      for (; itr != _pagesWithLowFreeCount.end(); ++itr)
      {
         page = itr->second;
         if (page->getFree() < count)
         {
            continue;
         }
      }

      if (NULL == page)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      rc = page->allocate(_pageCapacity, count, buf, &stillFreeCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _totalFreeCount -= count;

      for (UINT32 i = 0; i < count; ++i)
      {
         buf[i] = buf[i] + (_pageCapacity * page->getPageID());
      }

      if (0 == stillFreeCount)
      {
         _pagesWithLowFreeCount.erase(page->getPageID());
         SDB_OSS_DEL page;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::testBit(UINT32 bitOffset, BOOLEAN &isFree)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      INT32 pageId = bitOffset / _pageCapacity;
      const _inMemBitPage *page = NULL;
      _PAGE_MAP::const_iterator itr;

      if ((INT32)_pageCount <= pageId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (pageId < (INT32)_pageSkipped)
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      
      itr = _pagesWithHighFreeCount.find(pageId);
      if (_pagesWithHighFreeCount.end() != itr)
      {
         page = itr->second;
      }
      else
      {
         itr = _pagesWithLowFreeCount.find(pageId);
         if (_pagesWithLowFreeCount.end() != itr)
         {
            page = itr->second;
         }
      }

      if (NULL == page)
      {
         isFree = FALSE;
         goto done;
      }

      rc = page->test(_pageCapacity,
                      (bitOffset % _pageCapacity),
                      isFree);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   inMemBitmap::_inMemBitPage *inMemBitmap::getFromHFC(INT32 pageId)const
   {
      _inMemBitPage *page = NULL;
      _PAGE_MAP::const_iterator itr = _pagesWithHighFreeCount.find(pageId);
      if (_pagesWithHighFreeCount.end() != itr)
      {
         page = itr->second;
      }
      return page;
   }

   inMemBitmap::_inMemBitPage *inMemBitmap::getFromLFC(INT32 pageId)const
   {
      _inMemBitPage *page = NULL;
      _PAGE_MAP::const_iterator itr = _pagesWithLowFreeCount.find(pageId);
      if (_pagesWithLowFreeCount.end() != itr)
      {
         page = itr->second;
      }
      return page;
   }
} // namespace vessel
} // namespace engine
