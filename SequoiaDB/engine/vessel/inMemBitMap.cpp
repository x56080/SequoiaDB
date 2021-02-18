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

namespace engine
{
namespace vessel
{
   const UINT32 BIT_COUNT_PER_UINT32 = 32;
   const UINT32 BIT_COUNT_PER_BYTE = 8;

   inMemBitMap::_inMemBitPage::~_inMemBitPage()
   {
      teardown();
   }

   INT32 inMemBitMap::_inMemBitPage::teardown()
   {
      _pageID = -1;
      _size = 0;
      _free = 0;
      _firstFreeBits = -1;
      SAFE_OSS_FREE(_bitsBuf);

      return SDB_OK;
   }

   INT32 inMemBitMap::_inMemBitPage::setup(INT32 pageID,
                                           UINT32 size,
                                           BOOLEAN noFree,
                                           UINT32 occupied)
   {
      INT32 rc = SDB_OK;
      UINT32 bufSize = 0;
      UINT32 loopCount = 0;
      UINT32 tailLoopCount = 0;
      UINT32 *tailBuf = NULL;
      if (OSS_UNLIKELY(NULL != _bitsBuf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(pageID < 0))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == size))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(size < occupied))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      bufSize = ossAlign32(size) / BIT_COUNT_PER_BYTE;

      _bitsBuf = (UINT32 *)SDB_OSS_MALLOC(bufSize);
      if (NULL == _bitsBuf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemset((CHAR*)_bitsBuf, 0, bufSize);

      _pageID = pageID;
      _size = size;

      if (noFree)
      {
         _free = 0;
         _firstFreeBits = -1;
         goto done;
      }
      
      _free = _size;
      _firstFreeBits = 0;

      loopCount = size / BIT_COUNT_PER_UINT32;
      for (UINT32 i = 0; i < loopCount; ++i)
      {
         UINT32 *bits = _bitsBuf + i;
         *bits = UINT32(-1);
      }

      tailLoopCount = size & 0x1f; // mod 32
      if (0  < tailLoopCount)
      {
         tailBuf = _bitsBuf + loopCount;
         for (UINT32 i = 0; i < tailLoopCount; ++i)
         {
            OSS_BIT_SET(*tailBuf, ((UINT32)1 << i));
         }
      }

      for (UINT32 i = 0; i < occupied; ++i)
      {
         UINT32 tmp = 0;
         rc = allocate(1, &tmp, NULL);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

   done:
      return rc;
   error:
      teardown();
      goto done;
   }

   INT32 inMemBitMap::_inMemBitPage::setup(INT32 pageID, UINT32 size, UINT32 free, INT32 firstFree, const CHAR *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _bitsBuf, "must be null");
      SDB_ASSERT(0 < size, "can not be 0");
      UINT32 bufSize = ossAlign32(size) / BIT_COUNT_PER_BYTE;

      _bitsBuf = (UINT32 *)SDB_OSS_MALLOC(bufSize);
      if (NULL == _bitsBuf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _pageID = pageID;
      _size = size;
      _free = free;
      _firstFreeBits = firstFree;
      ossMemcpy(_bitsBuf, buf, bufSize);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::_inMemBitPage::allocate(UINT32 count, UINT32 *buf, UINT32 *stillFreeCount)
   {
      INT32 rc = SDB_OK;
      UINT32 allocatedCount = 0;
      if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_free < count)
      {
         rc = SDB_VESSEL_SMP_NO_FREE;
         goto error;
      }

      /// from here, do not goto error.
      SDB_ASSERT(0 <= _firstFreeBits, "impossible");

      do
      {
         UINT32 *bits = _bitsBuf + _firstFreeBits;
         INT32 bit = ossGetLowestBit1From32Bits(*bits);
         if (0 <= bit)
         {
            OSS_BIT_CLEAR(*bits, ((UINT32)1 << bit));
            buf[allocatedCount++] = (_firstFreeBits * BIT_COUNT_PER_UINT32) + bit;
            --_free;
         }
         else
         {
            ++_firstFreeBits;
            SDB_ASSERT((UINT32)_firstFreeBits < _size, "impossible");
         }
      } while (allocatedCount < count);

      updateFirstFreeAfterAllocating();

      if (NULL != stillFreeCount)
      {
         *stillFreeCount = _free;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::_inMemBitPage::free(UINT32 count, const UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      BOOLEAN everError = FALSE;

      if (OSS_UNLIKELY(0 == count || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto  error;
      }
      else if (OSS_UNLIKELY(_size < (count + _free)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 id = buf[i];
         UINT32 pos = 0;
         UINT32 bit = 0;
         UINT32 *bits = NULL;

         if (OSS_UNLIKELY(_size <= id))
         {
            PD_LOG(PDERROR, "invalid id[%d] to free, current size[%d]", id, _size);
            everError = TRUE;
            continue;
         }

         pos = id / BIT_COUNT_PER_UINT32;
         bit = id % BIT_COUNT_PER_UINT32;
         bits = _bitsBuf + pos;
         if (OSS_BIT_TEST(*bits, ((UINT32)1 << bit)))
         {
            PD_LOG(PDERROR, "id[%d] is not allocated", id);
            everError = TRUE;
            continue;
         }

         OSS_BIT_SET(*bits, ((UINT32)1 << bit));
         ++_free;

         if (0 <= _firstFreeBits)
         {
            if (pos < (UINT32)_firstFreeBits)
            {
               _firstFreeBits = pos;
            }
         }
         else
         {
            _firstFreeBits = pos;
         }
      }

      if (everError)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void inMemBitMap::_inMemBitPage::updateFirstFreeAfterAllocating()
   {
      SDB_ASSERT(0 <= _firstFreeBits, "impossible");
      const UINT32 *bits = NULL;
      UINT32 begin = _firstFreeBits;
   
      if (0 == _free)
      {
         _firstFreeBits = -1;
         goto done;
      }

      do
      {
         bits = (_bitsBuf + begin);
         if (0 != *bits)
         {
            _firstFreeBits = begin;
            goto done;
         }
         else
         {
            ++begin;
            SDB_ASSERT(begin < _size, "impossible");
         }
      } while (TRUE);
      
   done:
      return;
   }

////////////////
   inMemBitMap::inMemBitMap():
   _flags(0),
   _bitCountInPage(0),
   _freeBound(0),
   _modified(FALSE),
   _pageCount(0)
   {

   }

   inMemBitMap::~inMemBitMap()
   {
      teardown();
   }

   INT32 inMemBitMap::setup(UINT32 bitCountInPage, UINT32 freeBound)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == bitCountInPage))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 != _bitCountInPage))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(bitCountInPage <= freeBound))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _bitCountInPage = bitCountInPage;
      _freeBound = freeBound;
      _modified = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::teardown()
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

      _flags = 0;
      _freeBound = 0;
      _bitCountInPage = 0;
      _pageCount = 0;
      _modified = FALSE;
   done:

      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::allocateNewBitPage(UINT32 occupied)
   {
      INT32 rc = SDB_OK;
      _inMemBitPage *page = NULL;
      _mutex.get();
      
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

      rc = page->setup(_pageCount, _bitCountInPage, FALSE, occupied);
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
      else
      {
         _pagesWithLowFreeCount[page->getPageID()] = page;
      }

      _modified = TRUE;
      
   done:
      _mutex.release();
      return rc;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 inMemBitMap::allocateBits(UINT32 count, UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      _mutex.get();
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
         if (SDB_VESSEL_SMP_NO_FREE == rc)
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

      _modified = TRUE;
   done:
      _mutex.release();
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::releaseBits(UINT32 count, const UINT32 *buf)
   {
      INT32 rc = SDB_OK;
      BOOLEAN everError = FALSE;
      _mutex.get();
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
      
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 pageID = buf[i] / _bitCountInPage;
         UINT32 offset = buf[i] % _bitCountInPage;
         if (_pageCount <= pageID)
         {
            PD_LOG(PDERROR, "invalid bits id:%d", buf[i]);
            everError = FALSE;
            continue;
         }

         std::map<INT32, _inMemBitPage*>::iterator itr = _pagesWithHighFreeCount.find(pageID);
         if (_pagesWithHighFreeCount.end() != itr)
         {
            releaseBitFromHFC(offset, itr->second);
         }
         else
         {
            itr = _pagesWithLowFreeCount.find(pageID);
            if (_pagesWithLowFreeCount.end() != itr)
            {
               releaseBitFromLFC(offset, itr->second);
            }
            else
            {
               releaseAtDestroyedPage(pageID, offset);
            }
         }
      }

      _modified = TRUE;

      if (everError)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      _mutex.release();
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::dumpToFile(requestContext *context,
                                 const CHAR *fullPath,
                                 BOOLEAN onlyWhenModified,
                                 BOOLEAN replace)
   {
      INT32 rc = SDB_OK;
      OSSFILE file;
      CHAR *buffer = NULL;
      UINT32 bufferSize = 32768;
      UINT32 dumpSize = 0;
      UINT32 dumpSizePerLoop = 0;
      UINT32 i = 0;

      _mutex.get();

      if (OSS_UNLIKELY(NULL == context || NULL == fullPath))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (onlyWhenModified && !_modified)
      {
         goto done;
      }

      buffer = context->allocateBuffer(bufferSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      
      rc = ossOpen(fullPath,
                   (replace ? OSS_REPLACE:OSS_CREATEONLY) | OSS_READWRITE | OSS_EXCLUSIVE,
                   OSS_DEFAULTFILE,
                   file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%s, %d", fullPath, rc);
         goto error;
      }

      rc = dumpHead(bufferSize, buffer);
      if (SDB_OK != rc)
      {
         goto error;
      }

      dumpSize = getDumpHeadSize();

      while (i < _pageCount)
      {
         rc = dumpPage(i, bufferSize - dumpSize, buffer + dumpSize, dumpSizePerLoop);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (0 < dumpSizePerLoop)
         {
            dumpSize += dumpSizePerLoop;
            dumpSizePerLoop = 0;
            ++i;
            continue;
         }
         else
         {
            rc = ossWriteN(&file, buffer, dumpSize);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to write file:%d", rc);
               goto error;
            }
            dumpSizePerLoop = 0;
            dumpSize = 0;
         }
      }

      if (0 < dumpSize)
      {
         rc = ossWriteN(&file, buffer, dumpSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write file:%d", rc);
            goto error;
         }
      }

      rc = ossFsync(&file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync name file:%s, %d", fullPath, rc);
         goto error;
      }
  
      ossClose(file);
      ossChmod(fullPath, OSS_RU);
      _modified = FALSE;
   done:
      _mutex.release();
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      } 
      return rc;
   error:
      if (file.isOpened())
      {
         ossClose(file);
         ossDelete(fullPath);
      }
      goto done;
   }

   INT32 inMemBitMap::loadFromFile(requestContext *context,
                                   const CHAR *fullPath)
   {
      INT32 rc = SDB_OK;
      OSSFILE file;
      CHAR *buffer = NULL;
      const UINT32 *content = NULL;
      UINT32 bufferSize = 32768;
      SINT64 read = 0;
      BOOLEAN rollback = TRUE;

      if (OSS_UNLIKELY(NULL == context || NULL == fullPath))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 != _bitCountInPage))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      buffer = context->allocateBuffer(bufferSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = ossOpen(fullPath,
                   OSS_READONLY | OSS_EXCLUSIVE,
                   OSS_DEFAULTFILE,
                   file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%s, %d", fullPath, rc);
         goto error;
      }

      rc = ossRead(&file, buffer, getDumpHeadSize(), &read);
      if (SDB_OK != rc)
      {
         goto error;
      }
      else if (read < getDumpHeadSize())
      {
         PD_LOG(PDERROR, "invalid size of file:%s", fullPath);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      rollback = TRUE;
      content = (const UINT32 *)(buffer + sizeof(UINT32)); /// skip version
      _pageCount = *content++;
      _flags = *content++;
      _bitCountInPage = *content++;
      _freeBound = *content;
      _modified = FALSE;

      if (0 == _bitCountInPage)
      {
         PD_LOG(PDERROR, "invalid bit count:%s", fullPath);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      for (UINT32 i = 0; i < _pageCount; ++i)
      {
         _inMemBitPage * page = NULL;
         INT32 pageID = 0;
         UINT32 size = 0;
         UINT32 free = 0;
         INT32 firstFree = 0;

         rc = ossRead(&file, buffer, getDumpPageHeadSize(), &read);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read page head, file:%s, rc:%d", fullPath, rc);
            goto error;
         }
         else if (read < getDumpPageHeadSize())
         {
            PD_LOG(PDERROR, "invalid size of file:%s", fullPath);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         content = (const UINT32 *)buffer;
         pageID = (INT32)(*content++);
         size = *content++;
         free = *content++;
         firstFree = (INT32)(*content);
         if (pageID != (INT32)i)
         {
            PD_LOG(PDERROR, "invalid page id:%d, i:%d", pageID, i);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (size != _bitCountInPage)
         {
            PD_LOG(PDERROR, "invalid size:%d", size);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (0 == free)
         {
            continue;
         }

         rc = ossRead(&file, buffer, getPageDumpSize(), &read);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read page body, file:%s, rc:%d", fullPath, rc);
            goto error;
         }
         else if (read < getPageDumpSize())
         {
            PD_LOG(PDERROR, "invalid size of file:%s", fullPath);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         page = SDB_OSS_NEW _inMemBitPage();
         if (NULL == page)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = page->setup(pageID, size, free, firstFree, buffer);
         if (SDB_OK != rc)
         {
            SDB_OSS_DEL page;
            goto error;
         }

         if (free < _freeBound)
         {
            _pagesWithLowFreeCount[pageID] = page;
         }
         else
         {
            _pagesWithHighFreeCount[pageID] = page;
         }
      }
      
      ossClose(file);
   done:
      return rc;
   error:
      if (file.isOpened())
      {
         ossClose(file);
      }
      if (rollback)
      {
         teardown();
      }
      goto done;
   }

   INT32 inMemBitMap::dumpHead(UINT32 bufferSize, void *buf)const
   {
      INT32 rc = SDB_OK;
      UINT32 *ptr = (UINT32*)buf;
      if (bufferSize < getDumpHeadSize() || NULL == buf)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      *ptr++ = 0;
      *ptr++ = _pageCount;
      *ptr++ = _flags;
      *ptr++ = _bitCountInPage;
      *ptr = _freeBound;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::dumpPage(UINT32 pageID,
                               UINT32 bufferSize,
                               void *buf,
                               UINT32 &size)const
   {
      INT32 rc = SDB_OK;
      const _inMemBitPage *page = NULL;
      _PAGE_MAP::const_iterator itr;
      UINT32 *ptr = (UINT32*)buf;
      if (NULL == buf || _pageCount <= pageID)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      size = 0;
      itr = _pagesWithLowFreeCount.find((INT32)pageID);
      if (_pagesWithLowFreeCount.end() == itr)
      {
         itr = _pagesWithHighFreeCount.find((INT32)pageID);
         if (_pagesWithLowFreeCount.end() == itr)
         {
            /// dump page head only
            if (bufferSize < getDumpPageHeadSize())
            {
               goto done;
            }
            else
            {
               *ptr++ = pageID;
               *ptr++ = _bitCountInPage;
               *ptr++ = 0;
               *ptr = -1;
               size = getDumpPageHeadSize();
               goto done;
            }
         }
      }

      page = itr->second;
      SDB_ASSERT(NULL != page, "can not be null");
      if (bufferSize < (getDumpPageHeadSize() + getPageDumpSize()))
      {
         goto done;
      }

      *ptr++ = (UINT32)(page->getPageID());
      *ptr++ = page->getSize();
      *ptr++ = page->getFree();
      *ptr++ = page->getFirstFree();
      ossMemcpy(ptr, page->getBuf(), page->getBufSize());
      size = getDumpPageHeadSize() + getPageDumpSize();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::releaseBitFromHFC(UINT32 offset, _inMemBitPage *page)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != page, "can not be null");

      rc = page->free(1, &offset);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::releaseBitFromLFC(UINT32 offset, _inMemBitPage *page)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != page, "can not be null");

      rc = page->free(1, &offset);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (_freeBound == page->getFree())
      {
         _pagesWithLowFreeCount.erase(page->getPageID());
         _pagesWithHighFreeCount[page->getPageID()] = page;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 inMemBitMap::releaseAtDestroyedPage(INT32 pageID, UINT32 offset)
   {
      INT32 rc = SDB_OK;
      _inMemBitPage *page = SDB_OSS_NEW _inMemBitPage();
      if (NULL == page)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to allocate mem");
         goto error;
      }

      rc = page->setup(pageID, _bitCountInPage, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = page->free(1, &offset);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (page->getFree() < _freeBound)
      {
         _pagesWithLowFreeCount[pageID] = page;
      }
      else
      {
         _pagesWithHighFreeCount[pageID] = page;
      }
      
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 inMemBitMap::allocateBitsFromHFC(UINT32 count, UINT32 *buf)
   {
      INT32 rc = SDB_OK;
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
         rc = SDB_VESSEL_SMP_NO_FREE;
         goto error;
      }

      rc = page->allocate(count, buf, &stillFreeCount);
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
         rc = SDB_VESSEL_SMP_NO_FREE;
         goto error;
      }

      rc = page->allocate(count, buf, &stillFreeCount);
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
