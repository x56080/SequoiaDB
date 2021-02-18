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

namespace engine
{
namespace vessel
{
   lcCacheChunk::lcCacheChunk()
   :_id(0), _pageNum(0), _pageSize(0), _pages(NULL)
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
      return SDB_OK;
   }

   INT32 lcCacheChunk::transferTo(lcCacheChunk &chunk)
   {
      INT32 rc = SDB_OK;
      chunk.teardown();

      chunk = *this;
      _pages = NULL;
      _pageNum = 0;
      _pageSize = 0;
      _id = 0;
   done:
      return rc;
   error:
      goto done;
   }

   ossValuePtr lcCacheChunk::getPagePtr(UINT32 page)const
   {
      return (ossValuePtr)(_pages + (page * _pageSize));
   }
} /// end of namespace vessel
} /// end of namespace engine