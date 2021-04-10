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

   Source File Name = inMemBitMap.cpp

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

#include "vessel/inMemBitMap.h"
#include "ossLikely.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/bitMapUtils.h"

namespace engine
{
namespace vessel
{
   inMemBitMap::_inMemBitPage::~_inMemBitPage()
   {
      fini();
   }

   INT32 inMemBitMap::_inMemBitPage::fini()
   {
      _pageID = -1;
      _free = 0;
      _firstFreeBits = -1;
      if (NULL != _buf)
      {
         SDB_THREAD_FREE(_buf);
         _buf = NULL;
      }
      return SDB_OK;
   }

   INT32 inMemBitMap::_inMemBitPage::init(INT32 pageID,
                                          UINT32 capacity,
                                          BOOLEAN noFree,
                                          UINT32 occupied)
   {
      INT32 rc = SDB_OK;
      UINT32 bufSize = 0;
      UINT32 bitsCount = 0;
      UINT32 alignedCapacity = 0;
      fini();

      if (OSS_UNLIKELY(pageID < 0))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(capacity < occupied))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      alignedCapacity = ossAlign64(capacity);
      bufSize = alignedCapacity >> 3;/// bufSize = ossAlign64(capacity) / 8;
      bitsCount = bufSize >> 3; /// bitsCount = bufSize / 8;

      _buf = (UINT64 *)SDB_THREAD_ALLOC(bufSize);
      if (NULL == _buf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemset(_buf, 0, bufSize);

      _pageID = pageID;

      if (noFree)
      {
         _free = 0;
         _firstFreeBits = -1;
         goto done;
      }
      
      _free = capacity;
      _firstFreeBits = 0;
      resetBitMap64(bitsCount, _buf, TRUE);
      if (alignedCapacity != capacity)
      {
         UINT32 n = alignedCapacity - capacity;
         UINT64 v = OSS_UINT64_MAX;
         v = v >> n;
         _buf[bitsCount - 1] = v;
      }

      for (UINT32 i = 0; i < occupied; ++i)
      {
         UINT32 tmp = 0;
         rc = allocate(bitsCount, 1, &tmp, NULL);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 inMemBitMap::_inMemBitPage::initFromBuf(INT32 pageID,
                                                 UINT32 capacity,
                                                 UINT32 count,
                                                 const UINT64 *buf)
   {
      INT32 rc = SDB_OK;
      UINT32 alignedCapacity = 0;
      UINT32 bufSize = count << 3; /// count * 8;
      fini();
      if (OSS_UNLIKELY(pageID < 0))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      alignedCapacity = ossAlign64(capacity);
      if (count != (alignedCapacity >> 6))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _buf = (UINT64 *)SDB_THREAD_ALLOC(bufSize);
      if (NULL == _buf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _pageID = pageID;
      ossMemcpy(_buf, buf, bufSize);
      if (alignedCapacity != capacity)
      {
         UINT32 n = alignedCapacity - capacity;
         UINT64 v = OSS_UINT64_MAX;
         v = v >> n;
         _buf[count - 1] = v;
      }
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

   INT32 inMemBitMap::_inMemBitPage::allocate(UINT32 alignedBitsCount,
                                              UINT32 count,
                                              UINT32 *buf,
                                              UINT32 *stillFreeCount)
   {
      INT32 rc = SDB_OK;
      UINT32 allocatedCount = 0;
      if (OSS_UNLIKELY(0 == alignedBitsCount))
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
         if (findAndClearFirstFreeBitFromBit64(alignedBitsCount,
                                                _firstFreeBits,
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
         updateFirstFree(alignedBitsCount, (buf[count - 1] >> 6));
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

   void inMemBitMap::_inMemBitPage::free(UINT32 alignedBitsCount, UINT32 count, const UINT32 *buf)
   {
      if (OSS_UNLIKELY(0 == alignedBitsCount))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(NULL == _buf))
      {
         SDB_ASSERT(FALSE, "invalid operation");
         goto done;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = buf[i];
         if (setFreeIfNotFree64(alignedBitsCount, _buf, buf[i]))
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
      }
   done:
      return;
   }

   void inMemBitMap::_inMemBitPage::updateFirstFree(UINT32 bitsCount, UINT32 beginBits)
   {
      SDB_ASSERT(0 < _free, "impossible");
      UINT32 offset = 0;
   
      if (0 == _free)
      {
         _firstFreeBits = -1;
         goto done;
      }

      if (findFirstFreeBitFromBit64(bitsCount, (INT32)beginBits, _buf, offset))
      {
         _firstFreeBits = offset >> 6;
      }
      
   done:
      return;
   }

////////////////
   inMemBitMap::inMemBitMap():
   _bitCountInPage(0),
   _alignedBitsCount(0),
   _freeBound(0),
   _pageCount(0)
   {

   }

   inMemBitMap::~inMemBitMap()
   {
      fini();
   }

   INT32 inMemBitMap::init(UINT32 bitCountInPage, UINT32 freeBound)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == bitCountInPage))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(bitCountInPage <= freeBound))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fini();

      _bitCountInPage = bitCountInPage;
      _alignedBitsCount = (ossAlign64(_bitCountInPage)) >> 6;/// _alignedBitsCount = ossAlign64(_bitCountInPage) / 64;
      _freeBound = freeBound;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::fini()
   {
      INT32 rc = SDB_OK;

      std::map<INT32, _inMemBitPage*>::iterator itr = _pagesWithLowFreeCount.begin();
      for (; itr != _pagesWithLowFreeCount.end(); ++itr)
      {
         if (NULL != itr->second)
         {
            SDB_OSS_DEL(itr->second);
         }
      }
      _pagesWithLowFreeCount.clear();

      itr = _pagesWithHighFreeCount.begin();
      for (; itr != _pagesWithHighFreeCount.end(); ++itr)
      {
         if (NULL != itr->second)
         {
            SDB_OSS_DEL(itr->second);
         }
      }
      _pagesWithHighFreeCount.clear();

      _freeBound = 0;
      _bitCountInPage = 0;
      _alignedBitsCount = 0;
      _pageCount = 0;
   done:

      return rc;
   error:
      goto done;
   }

   void inMemBitMap::incPageCount()
   {
      ossScopedLock guard(&_mutex);
      if (isInitialized())
      {
         ++_pageCount;
      }
      return;
   }

   INT32 inMemBitMap::allocateNewBitPage(UINT32 occupied)
   {
      INT32 rc = SDB_OK;
      _inMemBitPage *page = NULL;
      ossScopedLock guard(&_mutex);
      
      if (OSS_UNLIKELY(_bitCountInPage < occupied))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      page = SDB_OSS_NEW _inMemBitPage();
      if (NULL == page)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = page->init(_pageCount, _bitCountInPage, FALSE, occupied);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ++_pageCount;
      /// do not goto error from here.
      if (OSS_UNLIKELY(_freeBound <= page->getFree()))
      {
         _pagesWithHighFreeCount[page->getPageID()] = page;
      }
      else if (0 < page->getFree())
      {
         _pagesWithLowFreeCount[page->getPageID()] = page;
      }
      
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 inMemBitMap::allocateBits(UINT32 count, UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      ossScopedLock guard(&_mutex);
      if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == _bitCountInPage))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

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

   void inMemBitMap::releaseBits(UINT32 count, const UINT32 *buf)
   {
      static const UINT32 BATCH_SIZE = PAGE_COUNT_IN_EXTENT;
      UINT32 batch[BATCH_SIZE];
      UINT32 released = 0;
      ossScopedLock guard(&_mutex);
      if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(0 == _bitCountInPage))
      {
         goto done;
      }

      while (released < count)
      {
         INT32 pageId = 0;
         UINT32 currentBatchCount = 0;

         pageId = buf[released] / _bitCountInPage;
         batch[currentBatchCount++] = buf[released] % _bitCountInPage;
         while (currentBatchCount < BATCH_SIZE &&
                (released + currentBatchCount) < count)
         {
            INT32 nextPage = buf[released + currentBatchCount] / _bitCountInPage;
            if (pageId == nextPage)
            {
               batch[currentBatchCount] = buf[released + currentBatchCount] % _bitCountInPage;
               ++currentBatchCount;
            }
            else
            {
               break;
            }
         }

         if ((INT32)_pageCount <= pageId)
         {
            SDB_ASSERT(FALSE, "invalid offset to be released");
            continue;
         }

         _PAGE_MAP::iterator itr = _pagesWithHighFreeCount.find(pageId);
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

         released += currentBatchCount;
      }
   done:
      return;
   }

   INT32 inMemBitMap::mapNewBitPage(UINT32 count, const UINT64 *bits)
   {
      INT32 rc = SDB_OK;
      _inMemBitPage *page = NULL;
      ossScopedLock lock(&_mutex);
      if (OSS_UNLIKELY(0 == count || NULL == bits))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(count != _alignedBitsCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      page = SDB_OSS_NEW _inMemBitPage();
      if (NULL == page)
      {
         PD_LOG(PDERROR, "failed to alloate mem");
         goto error;
      }

      rc = page->initFromBuf(_pageCount, _bitCountInPage, count, bits);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (0 == page->getFree())
      {
         SDB_OSS_DEL page;
      }
      else if (page->getFree() < _freeBound)
      {
         _pagesWithLowFreeCount[_pageCount] = page;
      }
      else
      {
         _pagesWithHighFreeCount[_pageCount] = page;
      }

      ++_pageCount;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   void inMemBitMap::releaseBitFromHFC(UINT32 count,
                                        const UINT32 *buf,
                                        _inMemBitPage *page)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(NULL != page, "can not be null");


      page->free(_alignedBitsCount, count, buf);
      return;
   }

   void inMemBitMap::releaseBitFromLFC(UINT32 count,
                                       const UINT32 *buf,
                                       _inMemBitPage *page)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(NULL != page, "can not be null");

      page->free(_alignedBitsCount, count, buf);

      if (_freeBound <= page->getFree())
      {
         _pagesWithLowFreeCount.erase(page->getPageID());
         _pagesWithHighFreeCount[page->getPageID()] = page;
      }
      return ;
   }

   void inMemBitMap::releaseAtDestroyedPage(INT32 pageID, UINT32 count, const UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 <= pageID, "can not be invalid");
      SDB_ASSERT(0 < count && NULL != buf, "can not be invalid");
      _inMemBitPage *page = SDB_OSS_NEW _inMemBitPage();
      if (NULL == page)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         goto done;
      }

      rc = page->init(pageID, _bitCountInPage, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page[%d], rc:%d", pageID, rc);
         goto error;
      }

      page->free(_alignedBitsCount, count, buf);

      if (page->getFree() < _freeBound)
      {
         _pagesWithLowFreeCount[pageID] = page;
      }
      else
      {
         _pagesWithHighFreeCount[pageID] = page;
      }
      
   done:
      return;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 inMemBitMap::allocateBitsFromHFC(UINT32 count, UINT32 *buf)
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

      rc = page->allocate(_alignedBitsCount, count, buf, &stillFreeCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         buf[i] = buf[i] + (_bitCountInPage * page->getPageID());
      }

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

   INT32 inMemBitMap::allocateBitsFromLFC(UINT32 count, UINT32 *buf)
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

      rc = page->allocate(_alignedBitsCount, count, buf, &stillFreeCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         buf[i] = buf[i] + (_bitCountInPage * page->getPageID());
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
} // namespace vessel
} // namespace engine
