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

   Source File Name = lcExtentTag.cpp

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

#include "vessel/lcExtentTag.h"
#include "dpsDef.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   lcExtentTag::lcExtentTag()
   :_pageNum(0),
    _pageSize(0),
    _diskPagePtr(0),
    _flags(TAG_FLAG_NONE),
    _minLSN(DPS_INVALID_LSN_OFFSET),
    _maxLSN(DPS_INVALID_LSN_OFFSET),
     _pages(NULL),
    _lruFlags(0), _lruCnt(0),
    _lruPre(NULL), _lruNext(NULL),
    _dirtyPre(NULL), _dirtyNext(NULL)
   {

   }

   lcExtentTag::~lcExtentTag()
   {
      if (NULL != _pages && _pages != _sPages)
      {
         SDB_OSS_DEL []_pages;
         _pages = NULL;
      }
   }

   void lcExtentTag::reset()
   {
      _id.reset();
      _pageSize = 0;
      _ts.state = ET_STATE_INVALID;
      _ts.usageCnt = 0;
      _ts.flags = ET_FLAG_NONE;
      _minLSN = DPS_INVALID_LSN_OFFSET;
      _maxLSN = DPS_INVALID_LSN_OFFSET;
      _diskPagePtr = 0;
      if (NULL != _pages && _pages != _sPages)
      {
         SDB_OSS_DEL []_pages;
         _pages = NULL;
      }
      for (UINT32 i = 0; i < PT_STATIC_PB_CNT; ++i)
      {
         _sPages[i].reset();
      }
      _pageNum = 0;
      _flags = TAG_FLAG_NONE;

      _lruPre = NULL;
      _lruNext = NULL;
      _lruFlags = ET_LRU_FLAG_NONE;
      _lruCnt = 0;

      _dirtyPre = NULL;
      _dirtyNext = NULL;
      return;
   }

   void lcExtentTag::decUsageCnt()
   {
      _spinLatch.lock();
      SDB_ASSERT(ET_STATE_INVALID != _ts.state,
                 "should not inc invalid tag's usage cnt");
      SDB_ASSERT(0 != _ts.usageCnt, "can not be zero");
      --_ts.usageCnt;
      _spinLatch.unlock();
      return;
   }

   BOOLEAN lcExtentTag::incUsageCnt()
   {
      _spinLatch.lock();
      BOOLEAN r = FALSE;
      SDB_ASSERT(ET_STATE_INVALID != _ts.state,
                 "should not inc invalid tag's usage cnt");
      if (ET_STATE_NORMAL == _ts.state)
      {
         ++_ts.usageCnt;
         SDB_ASSERT(UINT32(-1) != _ts.usageCnt,
                 "should not hit max uint32");
         r = TRUE;
      }

      _spinLatch.unlock();
      return r;
   }

   void lcExtentTag::setNoChunkPages()
   {
      SDB_ASSERT(!isDirty(), "can not be dirty");
      if (NULL != _pages)
      {
         if (_pages != _sPages)
         {
            SDB_OSS_DEL []_pages;
            _pages = NULL;
         }

         for (UINT32 i = 0; i < PT_STATIC_PB_CNT; ++i)
         {
            _sPages[i].reset();
         }
      }
      
      return;
   }

   INT32 lcExtentTag::copyDataToDisk()
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      if (0 == _diskPagePtr || NULL == _pages || 0 == _pageNum)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!OSS_BIT_TEST(_flags, TAG_FLAG_DIRTY))
      {
         goto done;
      }

      for (UINT32 i = 0; i < _pageNum; ++i)
      {
         ptr = _diskPagePtr + (i * _pageSize);
         ossMemcpy((CHAR *)ptr, (const CHAR *)(_pages[i].buf()), _pageSize);
      }

      OSS_BIT_CLEAR(_flags, TAG_FLAG_DIRTY);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcExtentTag::setChunkPages(const lcChunkPage *pages)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == pages))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL != _pages))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_pageNum <= PT_STATIC_PB_CNT)
      {
         for (UINT32 i = 0; i < _pageNum; ++i)
         {
            _sPages[i] = *(pages + i);
         }
         _pages = _sPages;
      }
      else
      {
         _pages = SDB_OSS_NEW lcChunkPage[_pageNum];
         if (OSS_UNLIKELY(NULL == _pages))
         {
            rc = SDB_OOM;
            goto error;
         }

         for (UINT32 i = 0; i < _pageNum; ++i)
         {
            _pages[i] = pages[i];
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcExtentTag::validateDiskPageAndCacheLSN()
   {
      INT32 rc = SDB_OK;
      const pageHead *head = NULL;
      UINT64 tail = DPS_INVALID_LSN_OFFSET;
      if (OSS_UNLIKELY(0 == _diskPagePtr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET != _minLSN))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = (const pageHead *)_diskPagePtr;
      tail = *((const UINT64 *)(_diskPagePtr + getDiskPageSize() - sizeof(UINT64)));
      if (!validatePage(head, tail))
      {
         OSS_BIT_SET(_flags, TAG_FLAG_DISK_PAGE_NOT_READY);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      setMinAndMaxLSN(head->lsn);
   done:
      return rc;
   error:
      goto done;
   }

} /// end of namespace vessel
} /// end of namespace engine
