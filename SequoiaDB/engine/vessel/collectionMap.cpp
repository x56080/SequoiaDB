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

   Source File Name = collectionMap.cpp

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

#include "vessel/collectionMap.h"
#include "ossLikely.hpp"
#include "vessel/collection.h"

namespace engine
{
namespace vessel
{
   collectionMap::collectionMap()
   :_pageCount(0)
   {

   }

   collectionMap::~collectionMap()
   {}

   INT32 collectionMap::collectionHolder::releaseCLObj()
   {
      SAFE_OSS_DELETE(_cl);
      return SDB_OK;
   }

   INT32 collectionMap::collectionHolder::allocateCLObj()
   {
      INT32 rc = SDB_OK;
      if (NULL == _cl)
      {
         _cl = SDB_OSS_NEW collection();
         if (NULL == _cl)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionMap::teardown()
   {
      INT32 rc = SDB_OK;
      _notFullPages.clear();
      _idIndex.clear();
      _nameIndex.clear();
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

   BOOLEAN collectionMap::clExists(const strSlice &clName, UINT32 logicalID)
   {
      INT32 rc = SDB_OK;
      ossScopedLock(&_mutex, SHARED);

      return 0 < _nameIndex.count(clName.str()) ||
             0 < _idIndex.count(logicalID);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionMap::allocateMBID(CL_MB_ID &mbID, collectionHolder **holder)
   {
      INT32 rc = SDB_OK;;
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

   INT32 collectionMap::occupyMBID(CL_MB_ID mbID, collectionHolder **holder)
   {
      INT32 rc = SDB_OK;
      UINT32 pageID = mbID / CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT32 count = pageID + 1;
      UINT32 pos = mbID % CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT64 bit = 1ull << pos;
      PAGE_MAP::iterator itr;
      _collectionCachePage *page = NULL;

      ossScopedLock(&_mutex, EXCLUSIVE);
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
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      page = itr->second;

      if (OSS_UNLIKELY(0 == page->bitMap))
      {
         PD_LOG(PDERROR, "page is full:%d", pageID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (!OSS_BIT_TEST(page->bitMap, bit))
      {
         PD_LOG(PDERROR, "mb id is in used:%d", mbID);
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         OSS_BIT_CLEAR(page->bitMap, bit);
         if (NULL != holder)
         {
            *holder = &(page->slots[pos]);
         }
         if (0 == page->bitMap)
         {
            _notFullPages.erase(pageID);
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionMap::releaseMBID(CL_MB_ID mbID)
   {
      INT32 rc = SDB_OK;
      UINT32 pageID = mbID / CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT32 count = pageID + 1;
      UINT32 pos = mbID % CL_MAP_TUPLE_COUNT_PER_PAGE;
      UINT64 bit = 1ull << pos;
      PAGE_MAP::iterator itr;
      _collectionCachePage *page = NULL;
      BOOLEAN moveToNotFull = FALSE;

      ossScopedLock(&_mutex, EXCLUSIVE);
      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_pageCount < count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      itr = _pageMap.find(pageID);
      if (_pageMap.end() == itr)
      {
         PD_LOG(PDERROR, "mb id not allocated:%d", mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      page = itr->second;
      SDB_ASSERT(!OSS_BIT_TEST(page->bitMap, bit), "impossible");
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

   INT32 collectionMap::createCLObject(const strSlice &clName,
                                       UINT32 logicalID,
                                       CL_MB_ID mbID,
                                       const createCLOptions &options,
                                       collectionSpace *cs,
                                       collectionHolder &holder)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollbackIndex = FALSE;
      collection *obj = NULL;
      if (OSS_UNLIKELY(clName.empty() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       INVALID_CL_MB_ID == mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!holder.isFree()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = holder.allocateCLObj();
      if (SDB_OK != rc)
      {
         goto error;
      }

      obj = holder.getCollection();

      rc = obj->setup(clName, logicalID, mbID, options, cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = insertIntoIndex(obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollbackIndex = TRUE;
   done:
      return rc;
   error:
      if (rollbackIndex)
      {
         eraseFromIndex(obj);
      }
      if (NULL != obj)
      {
         holder.releaseCLObj();
      }
      goto done;
   }

   INT32 collectionMap::destoryCLObject(collectionHolder &holder)
   {
      if (holder.isFree())
      {
         goto done;
      }

      if (INVALID_CL_MB_ID != holder.getCollection()->getMBID())
      {
         eraseFromIndex(holder.getCollection());
      }

      holder.releaseCLObj();
   done:
      return SDB_OK;
   }

   INT32 collectionMap::initObjWhenStartup(collectionSpace *cs,
                                           const collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      collectionHolder *holder = NULL;
      collection *obj = NULL;

      if (OSS_UNLIKELY(INVALID_CL_MB_ID == record.mbID))
      {
         PD_LOG(PDERROR, "invalid mbid");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = occupyMBID(record.mbID, &holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = holder->allocateCLObj();
      if (SDB_OK != rc)
      {
         goto error;
      }

      obj = holder->getCollection();

      rc = obj->setup(record, cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = insertIntoIndex(obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != holder)
      {
         holder->releaseCLObj();
         releaseMBID(record.mbID);
      }
      goto done;
   }

   INT32 collectionMap::upperBound(UINT32 logicalID,
                                   CL_MB_ID &nextMB,
                                   UINT32 &nextLogicalID,
                                   collectionHolder **holder)
   {
      INT32 rc = SDB_OK;
      collectionHolder *tmp = NULL;
      ossScopedLock(&_mutex, SHARED);

      if (DMS_INVALID_LOGICCLID == logicalID)
      {
         if (_idIndex.empty())
         {
            rc = SDB_DMS_NOTEXIST;
            goto error;
         }
         else
         {
            nextLogicalID = _idIndex.begin()->first;
            nextMB = _idIndex.begin()->second;
         }
      }
      else
      {
         ID_INDEX::const_iterator itr = _idIndex.upper_bound(logicalID);
         if (_idIndex.end() == itr)
         {
            rc = SDB_DMS_NOTEXIST;
            goto error;
         }
         nextLogicalID = itr->first;
         nextMB = itr->second;
      }

      rc = getHolder(nextMB, tmp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "logaical id[%d} exists index, but no holder exists");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (NULL != holder)
      {
         *holder = tmp;
      }
   done:
      return rc;
   error:
      nextMB = INVALID_CL_MB_ID;
      nextLogicalID = DMS_INVALID_LOGICCLID;
      if (NULL != holder)
      {
         *holder = NULL;
      }
      goto done;
   }


   INT32 collectionMap::allocateNewPage()
   {
      INT32 rc = SDB_OK;
      _collectionCachePage *page = NULL;
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

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(page);
      goto done;
   }

   INT32 collectionMap::allocateMBFromNotFullPages(CL_MB_ID &mbID, collectionHolder **holder)
   {
      INT32 rc = SDB_OK;
      _collectionCachePage *page = NULL;
      UINT32 id = INVALID_CL_MB_ID;
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
      SDB_ASSERT(0 <= bit, "impossible");
      id = page->pageID * CL_MAP_TUPLE_COUNT_PER_PAGE + bit;

      /// some slots in the last page can not be allocated.
      if (OSS_UNLIKELY(INVALID_CL_MB_ID <= id))
      {
         rc = SDB_DMS_NOSPC;
         goto error;
      }
      
      OSS_BIT_CLEAR(page->bitMap, (1ull << bit));
      if (0 == page->bitMap)
      {
         _notFullPages.erase(itr);
      }
      
      mbID = id;
      if (NULL != holder)
      {
         *holder = &(page->slots[bit]);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionMap::insertIntoIndex(collection *clObj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != clObj, "can not be null");
      const CHAR *name = clObj->getName();
      UINT32 logicalID = clObj->getLogicalID();
      CL_MB_ID mbID = clObj->getMBID();
      SDB_ASSERT(NULL != name &&
                 DMS_INVALID_LOGICCLID != logicalID &&
                 INVALID_CL_MB_ID != mbID, "can not be invalid");
      ossScopedLock(&_mutex, EXCLUSIVE);
      if (!_nameIndex.insert(std::make_pair(name, mbID)).second)
      {
         PD_LOG(PDERROR, "duplicated collection name:%s", name);
         rc = SDB_DMS_EXIST;
         goto error;
      }
 
      if (!_idIndex.insert(std::make_pair(logicalID, mbID)).second)
      {
         PD_LOG(PDERROR, "duplicated collection id:%d", logicalID);
         _nameIndex.erase(name);
         rc = SDB_DMS_EXIST;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionMap::eraseFromIndex(collection *clObj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != clObj, "can not be null");
      const CHAR *name = clObj->getName();
      UINT32 logicalID = clObj->getLogicalID();
      CL_MB_ID mbID = clObj->getMBID();
      SDB_ASSERT(NULL != name &&
                 DMS_INVALID_LOGICCLID != logicalID &&
                 INVALID_CL_MB_ID != mbID, "can not be invalid");
      ossScopedLock(&_mutex, EXCLUSIVE);

      NAME_INDEX::iterator itr = _nameIndex.find(name);
      if (itr != _nameIndex.end() && mbID == itr->second)
      {
         _nameIndex.erase(itr);
      }
      ID_INDEX::iterator itr2 = _idIndex.find(logicalID);
      if (itr2 != _idIndex.end() && mbID == itr2->second)
      {
         _idIndex.erase(itr2);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionMap::getHolder(CL_MB_ID mbID, collectionHolder *&holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      _collectionCachePage *page = NULL;
      PAGE_MAP::const_iterator itr;

      itr = _pageMap.find(mbID/CL_MAP_TUPLE_COUNT_PER_PAGE);
      if (_pageMap.end() == itr)
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      page = itr->second;
      if (OSS_BIT_TEST(page->bitMap, (mbID % CL_MAP_TUPLE_COUNT_PER_PAGE)))
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      holder = &(page->slots[mbID % CL_MAP_TUPLE_COUNT_PER_PAGE]);
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine