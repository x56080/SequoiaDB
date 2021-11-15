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

   Source File Name = lcCacheChunk.cpp

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

#include "vessel/lcCacheChunk.h"
#include "ossMem.hpp"
#include "ossErr.h"

namespace engine
{
namespace vessel
{
   lcCacheChunk::lcCacheChunk()
   :_id(0), _pageNum(0), _pageSize(0), _pages(NULL), _pageBitmap(0)
   {}

   lcCacheChunk::~lcCacheChunk()
   {
      teardown();
   }

   INT32 lcCacheChunk::setup(UINT32 id, UINT32 pageNum, UINT32 pageSize)
   {
      INT32 rc = SDB_OK;
      if (0 == pageNum || 0 == pageSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pages = (CHAR *)SDB_OSS_MALLOC(pageNum * pageSize);
      if (NULL == _pages)
      {
         rc = SDB_OOM;
         goto error;
      }

      _pageSize = pageSize;
      _pageNum = pageNum;
      _id = id;
      _pageBitmap.resize(pageNum);
      _bitPos = 0;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcCacheChunk::teardown()
   {
      SAFE_OSS_FREE(_pages);
      _pageNum = 0;
      _pageSize = 0;
      _id = 0;
      _pageBitmap.resetBitmap();
      _bitPos = 0;
      return SDB_OK;
   }

   ossValuePtr lcCacheChunk::getPagePtr(UINT32 pagePos)const
   {
      SDB_ASSERT(0 <= pagePos && _pageNum >= pagePos, "must in range");
      return (ossValuePtr)(_pages + (pagePos * _pageSize));
   }

   UINT32 lcCacheChunk::getPagePos(ossValuePtr pagePtr)const
   {
      SDB_ASSERT(pagePtr >= getPagePtr(0) && 
                 pagePtr <= getPagePtr(_pageNum - 1), "must in range");
      return ((CHAR *)pagePtr - _pages) / _pageSize;
   }

   BOOLEAN lcCacheChunk::allocatePage(freeListPage &page)
   {
      INT32 pos = -1;

      SDB_ASSERT(NULL != _pages, "can not be null");
      SDB_ASSERT(0 < _pageBitmap.getSize(), "impossible");
      page.reset();
      if (this->hasFreePage())
      {
         pos = _pageBitmap.nextFreeBitPos(_bitPos);
         if (pos < 0)
         {
            pos = _pageBitmap.nextFreeBitPos();
            SDB_ASSERT(0 <= pos, "has one free page at least");
         }
         _bitPos = pos;
         _pageBitmap.setBit(_bitPos);
         page.set(this->getId(), this->getPagePtr(_bitPos));
         return TRUE;
      }
      return FALSE;
   }

   void lcCacheChunk::releasePage(const freeListPage &page)
   {
      if(!_pageBitmap.isEmpty())
      {
         _pageBitmap.clearBit(getPagePos(page.getBuf()));
      }
   }
} /// end of namespace vessel
} /// end of namespace engine