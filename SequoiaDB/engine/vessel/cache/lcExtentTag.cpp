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
   :_pageSize(0),
    _diskPagePtr(0),
    _flags(TAG_FLAG_NONE),
    _minLSN(DPS_INVALID_LSN_OFFSET),
    _maxLSN(DPS_INVALID_LSN_OFFSET),
    _lruFlags(0), _lruCnt(0),
    _lruPre(NULL), _lruNext(NULL),
    _dirtyPre(NULL), _dirtyNext(NULL)
   {

   }

   lcExtentTag::~lcExtentTag()
   {

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
      _memPage.reset();
      _flags = TAG_FLAG_NONE;

      _lruPre = NULL;
      _lruNext = NULL;
      _lruFlags = ET_LRU_FLAG_NONE;
      _lruCnt.init(0);

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

   INT32 lcExtentTag::copyDataToDisk()
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      if (0 == _diskPagePtr || !_memPage.valid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!OSS_BIT_TEST(_flags, TAG_FLAG_DIRTY))
      {
         goto done;
      }

      ossMemcpy((CHAR *)_diskPagePtr, (const CHAR *)(_memPage.buf()), _pageSize);
      OSS_BIT_CLEAR(_flags, TAG_FLAG_DIRTY);
   done:
      return rc;
   error:
      goto done;
   }

} /// end of namespace vessel
} /// end of namespace engine
