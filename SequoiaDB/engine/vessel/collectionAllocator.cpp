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

   Source File Name = collectionAllocator.cpp

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

#include "vessel/collectionAllocator.h"
#include "ossLikely.hpp"
#include "vessel/collection.h"

namespace engine
{
namespace vessel
{
   collectionAllocator::collectionAllocator()
   :_pageCount(0)
   {

   }

   collectionAllocator::~collectionAllocator()
   {
      fini();
   }

   INT32 collectionAllocator::fini()
   {
      INT32 rc = SDB_OK;
      _notFullPages.clear();
      PAGE_MAP::iterator itr = _pageMap.begin();

      for (; itr != _pageMap.end(); ++itr)
      {
         SAFE_OSS_DELETE(itr->second);
      }
      _pageMap.clear();
      _pageCount = 0;
   done:
      return rc;
   error:
      goto done;
   }



   INT32 collectionAllocator::allocateNewMB(CL_MB_ID &mbID, collectionHolder **holder)
   {
      INT32 rc = SDB_OK;
      ossScopedLock(&_mutex, EXCLUSIVE);

      if (_notFullPages.empty())
      {
         rc = allocateNewPage();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = allocateMBFromNotFullPages(mbID, holder);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionAllocator::occupyMB(CL_MB_ID mbID, collectionHolder **holder)
   {
      INT32 rc = SDB_OK;
      UINT32 pageID = mbID / CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT32 count = pageID + 1;
      UINT32 pos = mbID % CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT64 bit = 1ull << pos;
      PAGE_MAP::iterator itr;
      _collectionCachePage *page = NULL;
      collectionHolder *tmpHolder = NULL;

      ossScopedLock lock(&_mutex, EXCLUSIVE);
      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = _pageCount; i < count; ++i)
      {
         rc = allocateNewPage();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      itr = _notFullPages.find(pageID);
      if (OSS_UNLIKELY(_notFullPages.end() == itr))
      {
         PD_LOG(PDERROR, "mb id is in used:%d", mbID);
         rc = SDB_VESSEL_OP_ON_MBID_BANNED;
         goto error;
      }
      page = itr->second;

      if (!OSS_BIT_TEST(page->bitMap, bit))
      {
         PD_LOG(PDERROR, "mb id is in used:%d", mbID);
         rc = SDB_VESSEL_OP_ON_MBID_BANNED;
         goto error;
      }
      else
      {
         tmpHolder = &(page->slots[pos]);
         if (!tmpHolder->isFree())
         {
            PD_LOG(PDERROR, "wrong bitmap data, mbid:%d", mbID);
            SDB_ASSERT(FALSE, "impossible");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (NULL == tmpHolder->allocateCLObj())
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         OSS_BIT_CLEAR(page->bitMap, bit);
         if (0 == page->bitMap)
         {
            _notFullPages.erase(pageID);
         }

         if (NULL != holder)
         {
            *holder = tmpHolder;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionAllocator::releaseMB(CL_MB_ID mbID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");

      UINT32 pageID = mbID / CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT32 count = pageID + 1;
      UINT32 pos = mbID % CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT64 bit = 1ull << pos;
      PAGE_MAP::iterator itr;
      _collectionCachePage *page = NULL;
      BOOLEAN moveToNotFull = FALSE;
      collectionHolder *holder = NULL;

      ossScopedLock lock(&_mutex, EXCLUSIVE);

      if (INVALID_CL_MB_ID == mbID)
      {
         goto done;
      }
      if (OSS_UNLIKELY(_pageCount < count))
      {
         rc = SDB_VESSEL_OP_ON_MBID_BANNED;
         PD_LOG(PDERROR, "can not release the mbid[%d] on not allocated page", mbID);
         goto error;
      }

      itr = _pageMap.find(pageID);
      if (_pageMap.end() == itr)
      {
         SDB_ASSERT(FALSE, "impossible");
         PD_LOG(PDERROR, "mb id not allocated:%d", mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      page = itr->second;
      if (OSS_BIT_TEST(page->bitMap, bit))
      {
         PD_LOG(PDERROR, "can not release the mbid[%d] not allocated", mbID);
         rc = SDB_VESSEL_OP_ON_MBID_BANNED;
         goto error;
      }

      holder = &(page->slots[pos]);
      holder->releaseCLObj();
      moveToNotFull = (0 == page->bitMap);

      OSS_BIT_SET(page->bitMap, bit);
      if (moveToNotFull)
      {
         _notFullPages[pageID] = page;
      }
   done:
      return rc;
   error:
      goto done;
   
   }

   INT32 collectionAllocator::allocateNewPage()
   {
      INT32 rc = SDB_OK;
      _collectionCachePage *page = NULL;
      CL_MB_ID mbid = CL_MAP_TUPLE_COUNT_PER_PAGE * _pageCount;
      if (MAX_CL_MB_COUNT <= _pageCount * CL_MAP_TUPLE_COUNT_PER_PAGE)
      {
         rc = SDB_DMS_NOSPC;
         goto error;
      }

      page = SDB_OSS_NEW _collectionCachePage();
      if (NULL == page)
      {
         PD_LOG(PDERROR, "failed to alloate mem");
         rc = SDB_OOM;
         goto error;
      }

      page->pageID = _pageCount++;
      page->bitMap = OSS_UINT64_MAX;
      _pageMap[page->pageID] = page;
      _notFullPages[page->pageID] = page;
      for (UINT32 i = 0; i < CL_MAP_TUPLE_COUNT_PER_PAGE; ++i)
      {
         page->slots[i]._mbID = mbid++;
      }

      /// max mb count is 65535, not 65536. the last slot can not be allocated.
      if (MAX_CL_MB_COUNT <= _pageCount * CL_MAP_TUPLE_COUNT_PER_PAGE)
      {
         page->bitMap = page->bitMap >> 1;
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 collectionAllocator::allocateMBFromNotFullPages(CL_MB_ID &mbID, collectionHolder **holder)
   {
      INT32 rc = SDB_OK;
      _collectionCachePage *page = NULL;
      collectionHolder *tmpHolder = NULL;
      CL_MB_ID tmpID = INVALID_CL_MB_ID;
      PAGE_MAP::iterator itr = _notFullPages.begin();
      INT32 bit = -1;
      if (OSS_UNLIKELY(_notFullPages.end() == itr))
      {
         rc = SDB_DMS_NOSPC;
         goto error;
      }

      page = itr->second;
      SDB_ASSERT(NULL != page, "can not be null");

      bit = ossGetLowestBit1From64Bits(page->bitMap);
      if (OSS_UNLIKELY(bit < 0))
      {
         PD_LOG(PDERROR, "cl page[%d] has no free holder but exists in index", page->pageID);
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      tmpID = page->pageID * CL_MAP_TUPLE_COUNT_PER_PAGE + bit;
      tmpHolder = &(page->slots[bit]);
      if (OSS_UNLIKELY(!tmpHolder->isFree()))
      {
         PD_LOG(PDERROR, "wrong bitmap data, mbid:%d", tmpID);
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (NULL == tmpHolder->allocateCLObj())
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      OSS_BIT_CLEAR(page->bitMap, (1ull << bit));
      if (0 == page->bitMap)
      {
         _notFullPages.erase(itr);
      }
      
      mbID = tmpID;
      if (NULL != holder)
      {
         *holder = tmpHolder;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionAllocator::getHolder(CL_MB_ID mbID, collectionHolder *&holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      _collectionCachePage *page = NULL;
      PAGE_MAP::const_iterator itr;
      UINT32 pos = mbID % CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT64 bit = 1ull << pos;
      UINT32 pageID = mbID/CL_MAP_TUPLE_COUNT_PER_PAGE;
      collectionHolder *tmpHolder = NULL;

      ossScopedLock lock(&_mutex, SHARED);

      itr = _pageMap.find(pageID);
      if (_pageMap.end() == itr)
      {
         PD_LOG(PDERROR, "page of mbid[%d] has not been allocated", mbID);
         rc = SDB_VESSEL_OP_ON_MBID_BANNED;
         goto error;
      }

      page = itr->second;
      if (OSS_BIT_TEST(page->bitMap, bit))
      {
         PD_LOG(PDERROR, "mbid[%d] has not been allocated", mbID);
         rc = SDB_VESSEL_OP_ON_MBID_BANNED;
         goto error;
      }

      tmpHolder = &(page->slots[pos]);
      if (tmpHolder->isFree())
      {
         PD_LOG(PDERROR, "wrong bitmap data, mbid:%d", mbID);
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      holder = tmpHolder;
   done:
      return rc;
   error:
      goto done;
   }

/////collectionAllocator::collectionHolder
   void collectionAllocator::collectionHolder::releaseCLObj()
   {
      SAFE_OSS_DELETE(_cl);
      return;
   }

   collection *collectionAllocator::collectionHolder::allocateCLObj()
   {
      SDB_ASSERT(isFree(), "must be free");
      _cl = SDB_OSS_NEW collection();
      return _cl;
   }

}//namespace vessel
}//namespace engine