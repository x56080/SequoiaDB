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
   _pageSize(0),
   _totalAllocated(0),
    _chunks(NULL),
    _size(0)
   {

   }

   lcFreeList::~lcFreeList()
   {
      fini();
   }

   INT32 lcFreeList::init(UINT32 pageSize, const liteCacheOptions::freeListOptions &options)
   {
      INT32 rc = SDB_OK;
      if (0 == options.maxChunkCount ||
          0 == options.pageCountInChunk ||
          !ossIsPowerOf2(options.pageCountInChunk) ||
          ((DMS_PAGE_SIZE32K != pageSize) && (DMS_PAGE_SIZE64K != pageSize)))
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

      _pageSize = pageSize;
      _options = options;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void lcFreeList::fini()
   {
      _free.clear();
      if (NULL != _chunks)
      {
         SDB_OSS_DEL []_chunks;
         _chunks = NULL;
      }

      _size = 0;
      _pageSize = 0;
      _options = liteCacheOptions::freeListOptions();
      return;
   }


   INT32 lcFreeList::allocate(freeListPage &page)
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock(&_latch);
      if (_free.empty())
      {
         rc = pushNewChunkIntoFreeList();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      ++_totalAllocated;
      page = _free.front();
      _free.pop_front();

   done:
      return rc;
   error:
      goto done;
   }

   void lcFreeList::releasePage(const freeListPage &page)
   {
      if (page.valid())
      {
         ossScopedLock lock(&_latch);
         _free.push_back(page);
      }
      return;
   }

   void lcFreeList::releasePages(UINT32 size, const freeListPage *pages)
   {
      if (NULL != pages)
      {
         ossScopedLock lock(&_latch);
         for (UINT32 i = 0; i < size; ++i)
         {
            if (pages[i].valid())
            {
               _free.push_back(pages[i]);
            }
         }
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
      rc = chunk->setup(_size, _options.pageCountInChunk, _pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      for (UINT32 i = 0; i < _options.pageCountInChunk; ++i)
      {
         _free.push_back(freeListPage(_size, chunk->getPagePtr(i)));
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