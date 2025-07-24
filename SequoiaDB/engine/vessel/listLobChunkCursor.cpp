/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = listLobChunkCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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