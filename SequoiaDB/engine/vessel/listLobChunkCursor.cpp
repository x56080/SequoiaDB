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

   Source File Name = listLobChunkCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/listLobChunkCursor.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 listLobChunkCursor::getBucketPosToScan()const
   {
      return _bucketToScan.findFirst();
   }

   void listLobChunkCursor::endToScanBucket(UINT32 pos)
   {
      SDB_ASSERT(pos < lobcBucketRegionBlock::BUCKET_COUNT, "out of bound");
      BOOLEAN old = FALSE;
      _bucketToScan.clear(pos, &old);
      SDB_ASSERT(old, "invalid pos");
   }

   void listLobChunkCursor::setRegionToScan(UINT32 regionId,
                                            const lobcBucketRegionBlock &regionBlock)
   {
      _regionId = regionId;
      _bucketToScan.clearAll();
      for (UINT32 i = 0; i < lobcBucketRegionBlock::BUCKET_COUNT; ++i)
      {
         if (regionBlock.buckets[i] != INVALID_PAGE_ID)
         {
            _bucketToScan.set(i);
         }
      }
   }
} // namespace vessel

} // namespace engine