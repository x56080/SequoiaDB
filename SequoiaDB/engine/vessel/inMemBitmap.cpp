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

   Source File Name = inMemBitmap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
      _scanner.reset();
      if (NULL != _buf)
      {
         SDB_THREAD_FREE(_buf);
         _buf = NULL;
      }

      _next = NULL;
      _pre = NULL;
      _inList = FALSE;
      return;
   }

   INT32 inMemBitmap::_inMemBitPage::init(INT32 pageId,
                                          UINT32 capacity,
                                          BOOLEAN noFree)
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
      
      _pageID = pageId;
      _scanner.init(_buf, capacity, noFree);
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 inMemBitmap::_inMemBitPage::allocate(UINT32 count,
                                              UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      UINT32 allocatedCount = 0;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_scanner.getNonZeroedCount() < count)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      do
      {
         INT32 offset = -1;
         if (_scanner.findAndClearNext(offset))
         {
            buf[allocatedCount++] = (UINT32)offset;
         }
         else
         {
            PD_LOG(PDERROR, "free count in scanner is not zero, but failed to find free bit from bitmap");
            SDB_ASSERT(FALSE, "impossible");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      } while (allocatedCount < count);
      
   done:
      return rc;
   error:
      for (UINT32 i = 0; i < allocatedCount; ++i)
      {
         _scanner.setBit(buf[i], FALSE);
      }
      goto done;
   }

   void inMemBitmap::_inMemBitPage::release(UINT32 count,
                                            const UINT32 *buf)
   {
      if (OSS_UNLIKELY(!isReady()))
      {
         SDB_ASSERT(FALSE, "invalid page");
         goto done;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         SDB_ASSERT(FALSE, "invalid releasing");
         goto done;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         _scanner.setBit(buf[i]);
      }
   done:
      return;
   }

   INT32 inMemBitmap::_inMemBitPage::test(UINT32 offset,
                                          BOOLEAN &isFree)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(_scanner.getCapacity() <= offset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      isFree = _scanner.testBit(offset);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitmap::_inMemBitPage::occupy(UINT32 offset)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(_scanner.getCapacity() <= offset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!_scanner.clearBit(offset))
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
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
      else if (OSS_UNLIKELY(o.maxBitmapPageCount <= o.bitmapBeginPage))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pageCapacity = pageCapacity;
      _o = o;
      _latch = latch;
      _totalFreeBitCount = 0;
   done:
      return rc;
   error:
      goto done;
   }

   void inMemBitmap::fini()
   {
     clearFreeList();

      for (UINT32 i = 0; i < _pages.size(); ++i)
      {
         if (NULL != _pages[i])
         {
            SDB_OSS_DEL _pages[i];
         }
      }
      _pages.clear();

      _latch = NULL;
      _pageCapacity = 0;
      _o = options();
      _totalFreeBitCount = 0;

      return;
   }

   INT32 inMemBitmap::_allocateNewBitmapPages(UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 < count, "impossible");
      INT32 nextPageId = getNextPageId();
      UINT32 pushed = 0;

      if (_o.maxBitmapPageCount < (getCustomizedPageCount() + count))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      _pages.reserve(count);

      for (UINT32 i = 0; i < count; ++i)
      {
         _inMemBitPage *page = SDB_OSS_NEW _inMemBitPage();
         if (NULL == page)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = page->init(nextPageId++, _pageCapacity, FALSE);
         if (SDB_OK != rc)
         {
            SDB_OSS_DEL page;
            page = NULL;
            PD_LOG(PDERROR, "failed to init page:%d", rc);
            goto error;
         }

         _pages.push_back(page);
         pushBackToFL(page);
         ++pushed;
      }

      _totalFreeBitCount += (count * _pageCapacity);
   done:
      return rc;
   error:
      for (UINT32 i = 0; i < pushed; ++i)
      {
         _inMemBitPage *page = _pages.back();
         _pages.pop_back();
         eraseFromFL(page);
         SDB_OSS_DEL page;
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
      rc = _allocateNewBitmapPages(count);
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

      {
         ossXLatchGuard guard(_latch);
         if (_totalFreeBitCount < count)
         {
            if (0 < autoExtendingCount)
            {
               rc = _allocateNewBitmapPages(autoExtendingCount);
               if (SDB_OK != rc)
               {
                  goto error;
               }
            }
            else
            {
               rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
               goto error;
            }
         }

         rc = _allocateBits(count, buf);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      {
         UINT32 minOffset = _o.bitmapBeginPage * _pageCapacity;
         for (UINT32 i = 0; i < count; ++i)
         {
            SDB_ASSERT(minOffset <= buf[i], "out of bound");
         }
      }
   
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
      else if (count <= getCustomizedPageCount())
      {
         /// pages never shrink
         goto done;
      }

      guard.lock();
      if (count <= getCustomizedPageCount())
      {
         goto done;
      }

      rc = _allocateNewBitmapPages(count - getCustomizedPageCount());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate multiple pages:%d", rc);
         goto error;
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

      {
         ossScopedLock guard(_latch);
         rc = _occupy(count, buf);
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

   INT32 inMemBitmap::test(UINT32 offset, BOOLEAN &isFree)const
   {
      INT32 rc = SDB_OK;
      INT32 pageId = -1;
      UINT32 offsetInPage = 0;
      _inMemBitPage *page = NULL;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      pageId = offset / _pageCapacity;
      offsetInPage = offset % _pageCapacity;

      {
         ossScopedLock guard(_latch);
         if (pageId < (INT32)(_o.bitmapBeginPage))
         {
            rc = SDB_INVALID_OPERATION;
            goto error;
         }
         else if (_pages.size() <= getRealPos(pageId))
         {
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         page = _pages[getRealPos(pageId)];
         if (NULL == page)
         {
            isFree = FALSE;
         }
         else
         {
            rc = page->test(offsetInPage, isFree);
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

   INT32 inMemBitmap::_occupy(UINT32 count, const UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 < count && NULL != buf, "can not be invalid");

      UINT32 occupied = 0;
      
      for (UINT32 i = 0; i < count; ++i)
      {
         INT32 pageId = buf[i] / _pageCapacity;
         UINT32 offset = buf[i] % _pageCapacity;
         _inMemBitPage *page = NULL;

         if (pageId < (INT32)(_o.bitmapBeginPage))
         {
            rc = SDB_INVALID_OPERATION;
            goto error;
         }
         else if (_pages.size() <= getRealPos(pageId))
         {
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         page = _pages[getRealPos(pageId)];
         if (NULL == page)
         {
            PD_LOG(PDERROR, "page obj is not valid at[%d]", pageId);
            rc = SDB_INVALID_OPERATION;
            goto error;
         }

         rc = page->occupy(offset);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (page->isInFreeList())
         {
            --_totalFreeBitCount;
         }

         if (0 == page->getFree())
         {
            if (page->isInFreeList())
            {
               eraseFromFL(page);
            }

            if (!_o.keepEmptyPageInMem)
            {
               _pages[getRealPos(pageId)] = NULL;
               SDB_OSS_DEL page;
            }
         }

         ++occupied;
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

   INT32 inMemBitmap::_allocateBits(UINT32 count, UINT32 *buf)
   {
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT((INT64)count <= _totalFreeBitCount, "not enough resource");
      INT32 rc = SDB_OK;
      UINT32 allocated = 0;

      do
      {
         if (isFreeListEmpty())
         {
            break;
         }

         UINT32 need = count - allocated;
         _inMemBitPage *page = _head;
         if (page->getFree() < need)
         {
            need = page->getFree();
         }
         for (UINT32 i = 0; i < need; ++i)
         {
            rc = page->allocate(1, buf + allocated);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to allocate bit from page[%d]", page->getPageID());
               goto error;
            }

            /// transfer to global offset
            buf[allocated] += (page->getPageID() * _pageCapacity);
            ++allocated;
            --_totalFreeBitCount;
         }

         if (0 == page->getFree())
         {
            SDB_ASSERT(page->isInFreeList(), "impossible");
            eraseFromFL(page);

            if (!_o.keepEmptyPageInMem)
            {
               _pages[getRealPos(page->getPageID())] = NULL;
               SDB_OSS_DEL page;
            }
         }

      } while (allocated < count);
   
      if (count != allocated)
      {
         PD_LOG(PDERROR, "unexpected failure when allocating");
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
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
      if (OSS_UNLIKELY(!isInitialized()))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         goto done;
      }

      {
         ossXLatchGuard guard(_latch);
         _releaseBits(count, buf);
      }
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
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 bitOffset = buf[i] % _pageCapacity;
         INT32 pageId = buf[i] / _pageCapacity;
         UINT32 oldFreeCount = 0;
         _inMemBitPage *page = NULL;

         if (pageId < (INT32)_o.bitmapBeginPage)
         {
            PD_LOG(PDERROR, "invalid offset to release:%d", buf[i]);
            continue;
         }
         else if (_pages.size() <= getRealPos(pageId))
         {
            PD_LOG(PDERROR, "invalid offset to release:%d", buf[i]);
            continue;
         }

         page = _pages[getRealPos(pageId)];
         if (NULL == page)
         {
            page = SDB_OSS_NEW _inMemBitPage();
            if (OSS_UNLIKELY(NULL == page))
            {
               PD_LOG(PDERROR, "failed to allocate mem when releasing:%d", buf[i]);
               continue;
            }

            INT32 rc = page->init(pageId, _pageCapacity, TRUE);
            if (SDB_OK != rc)
            {
               SDB_OSS_DEL page;
               PD_LOG(PDERROR, "failed to init page when releasing:%d, rc:%d", buf[i], rc);
               continue;
            }

            _pages[getRealPos(pageId)] = page;
         }

         oldFreeCount = page->getFree();
         page->release(1, &bitOffset);

         if (page->isInFreeList())
         {
            _totalFreeBitCount += page->getFree() - oldFreeCount;
         }
         else if ((_o.percentFreeReused * _pageCapacity) <= page->getFree())
         {
            pushBackToFL(page);
            _totalFreeBitCount += page->getFree();
         }
         else
         {
            /// do nothing.
         }
      }

      return;
   }

   void inMemBitmap::clearFreeList()
   {
      _lsize = 0;
      _head = NULL;
      _tail = NULL;
      return;
   }

   BOOLEAN inMemBitmap::isFreeListEmpty()const
   {
      return 0 == _lsize;
   }

   void inMemBitmap::pushBackToFL(_inMemBitPage *page)
   {
      SDB_ASSERT(NULL != page, "can not be null");
      SDB_ASSERT(!page->isInFreeList(), "already in list");
      SDB_ASSERT(NULL == page->_pre && NULL == page->_next, "must be null");
      SDB_ASSERT(0 < page->getFree(), "no free resource");

      if (isFreeListEmpty())
      {
         SDB_ASSERT(NULL == _head && NULL == _tail, "must be null");
         _lsize = 1;
         _head = page;
         _tail = page;
      }
      else
      {
         SDB_ASSERT(NULL != _head && NULL != _tail, "must be null");
         page->_pre = _tail;
         _tail->_next = page;
         _tail = page;
         ++_lsize;
      }

      page->_inList = TRUE;
      return;
   }

   void inMemBitmap::pushFrontToFL(_inMemBitPage *page)
   {
      SDB_ASSERT(NULL != page, "can not be null");
      SDB_ASSERT(!page->isInFreeList(), "already in list");
      SDB_ASSERT(NULL == page->_pre && NULL == page->_next, "must be null");
      SDB_ASSERT(0 < page->getFree(), "no free resource");

      if (isFreeListEmpty())
      {
         SDB_ASSERT(NULL == _head && NULL == _tail, "must be null");
         _lsize = 1;
         _head = page;
         _tail = page;
      }
      else
      {
         SDB_ASSERT(NULL != _head && NULL != _tail, "must be null");
         page->_next = _head;
         _head->_pre = page;
         _head = page;
         ++_lsize;
      }

      page->_inList = TRUE;
      return;
   }

    void inMemBitmap::popFrontFromFL()
    {
       if (!isFreeListEmpty())
       {
          _inMemBitPage *next = _head->_next;
          _head->_pre = NULL;
          _head->_next = NULL;
          _head->_inList = FALSE;

          if (NULL != next)
          {
             next->_pre = NULL;
             _head = next;
          }
          else
          {
             SDB_ASSERT(1 == _lsize, "must be the last one");
             _head = NULL;
             _tail = NULL;
          }
          
          --_lsize;
       }
       return;
    }

   void inMemBitmap::eraseFromFL(_inMemBitPage *page)
   {
      SDB_ASSERT(NULL != page && page->isInFreeList(), "can not be invalid");
      SDB_ASSERT(!isFreeListEmpty(), "can not be empty");
      _inMemBitPage *pre = page->_pre;
      _inMemBitPage *next = page->_next;

      page->_pre = NULL;
      page->_next = NULL;
      page->_inList = FALSE;
      --_lsize;

      if (NULL != pre)
      {
         pre->_next = next;
      }
      else
      {
         _head = next;
      }

      if (NULL != next)
      {
         next->_pre = pre;
      }
      else
      {
         _tail = pre;
      }

      return;
   }
} // namespace vessel
} // namespace engine
