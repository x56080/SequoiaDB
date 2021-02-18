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

   Source File Name = lcFreeList.cpp

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

#include "vessel/lcFreeList.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   lcFreeList::lcFreeList():
   _totalAllocated(0),
    _chunks(NULL),
    _size(0)
   {

   }

   lcFreeList::~lcFreeList()
   {
      teardown();
   }

   INT32 lcFreeList::setup(const liteCacheOptions::freeListOptions &options)
   {
      INT32 rc = SDB_OK;
      if (0 == options.maxChunkCount ||
          0 == options.pageCountInChunk ||
          0 != options.pageCountInChunk % 4 ||
          DMS_PAGE_SIZE32K != options.pageSize)
      {
         PD_LOG(PDERROR, "invalid options: %s", options.toString().c_str());
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = initChunkArray(options.maxChunkCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _options = options;
   done:
      return rc;
   error:
      teardown();
      goto done;
   }

   INT32 lcFreeList::teardown()
   {
      _free.clear();
      if (NULL != _chunks)
      {
         SDB_OSS_DEL []_chunks;
         _chunks = NULL;
      }

      _size = 0;
      _options = liteCacheOptions::freeListOptions();
      return SDB_OK;
   }

   UINT32 lcFreeList::getFreePageCount()
   {
      _mutex.lock();
      UINT32 freePageCount = _free.size() + (_options.pageCountInChunk * (_options.maxChunkCount - _size));
      _mutex.unlock();
      return freePageCount;
   }

   INT32 lcFreeList::allocatePage(lcChunkPage &page)
   {
      INT32 rc = SDB_OK;
      if (!_free.empty())
      {
         page = _free.front();
         _free.pop_front();
         goto done;
      }
      else
      {
         rc = pushNewChunkIntoFreeList();
         if (SDB_OK != rc)
         {
            goto error;
         }

         ++_totalAllocated;
         page = _free.front();
         _free.pop_front();
         goto done;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcFreeList::allocatePages(UINT32 size, lcChunkPage *pages)
   {
      INT32 rc = SDB_OK;
      _mutex.lock();
      UINT32 freePageCount = 0;
      UINT32 allocated = 0;
      if (OSS_UNLIKELY(NULL == pages || 0 == size))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      freePageCount = _free.size() + (_options.pageCountInChunk * (_options.maxChunkCount - _size));
      if (freePageCount < size)
      {
         rc = SDB_VESSEL_LC_NOT_ENOUGH_PAGES_IN_FL;
         goto error;
      }

      do
      {
         rc = allocatePage(pages[allocated]);
         if (SDB_OK != rc)
         {
            goto error;
         }
         ++allocated;
      } while (allocated < size);
   done:
      _mutex.unlock();
      return rc;
   error:
      releasePages(allocated, pages);
      goto done;
   }

   void lcFreeList::releasePage(const lcChunkPage &page)
   {
      if (page.valid())
      {
         _mutex.lock();
         _free.push_back(page);
         _mutex.unlock();
      }
      return;
   }

   void lcFreeList::releasePages(UINT32 size, lcChunkPage *pages)
   {
      if (NULL != pages)
      {
         _mutex.lock();
         for (UINT32 i = 0; i < size; ++i)
         {
            _free.push_back(pages[i]);
         }
         _mutex.unlock();
      }
      return;
   }

   INT32 lcFreeList::pushNewChunkIntoFreeList()
   {
      INT32 rc = SDB_OK;
      lcCacheChunk *chunk = NULL;
      if (_options.maxChunkCount == _size)
      {
         rc = SDB_VESSEL_LC_NOT_ENOUGH_PAGES_IN_FL;
         goto error;
      }

      
      chunk = &(_chunks[_size]);
      rc = chunk->setup(_size, _options.pageCountInChunk, _options.pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      for (UINT32 i = 0; i < _options.pageCountInChunk; ++i)
      {
         _free.push_back(lcChunkPage(_size, chunk->getPagePtr(i)));
      }

      ++_size;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcFreeList::initChunkArray(UINT32 size)
   {
      INT32 rc = SDB_OK;
      if (0 == size)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (NULL != _chunks)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _chunks = SDB_OSS_NEW lcCacheChunk[size];
      if (NULL == _chunks)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} /// end of namespace vessel
} /// end of namespace engine