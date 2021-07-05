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

   Source File Name = liteCachePageTag.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/liteCachePageTag.h"
#include "dpsDef.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   void liteCachePageTag::reset()
   {
      _id.reset();

      _ts.status = LC_TAG_STATUS_INVALID;
      _ts.usageCnt = 0;
      _ts.flags = 0;

      _diskPagePtr = 0;
      _flags = 0;
      _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
      _maxMemDirtyLSN = DPS_INVALID_LSN_OFFSET;
      _memPage.reset();

      _bucketItr = LC_BUCKET_INNER_INDEX_ITERATOR();
      
      _lruTouchCnt = 0;
      _lruFlags = 0;
      _lruPre = NULL;
      _lruNext = NULL;

      _dirtyPre = NULL;
      _dirtyNext = NULL;
      return;
   }

} /// end of namespace vessel
} /// end of namespace engine
