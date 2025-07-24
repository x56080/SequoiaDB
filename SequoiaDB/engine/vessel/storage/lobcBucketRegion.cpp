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

   Source File Name = lobcBucketRegion.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobcBucketRegion.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lobcBucketRegion::lobcBucketRegion(UINT32 regionId,
                                      lobcBucketRegionBlock *block):
   _regionId(regionId),
   _block(block)
   {
      SDB_ASSERT(nullptr != _block, "can not be null");

#if defined (_DEBUG)
      SDB_ASSERT(lobcBucketRegionBlock::BUCKET_COUNT
                 == std::pow(2, lobcBucketRegionBlock::BUCKET_COUNT_SQUARE), "must be same");
#endif//_DEBUG
   }

   lobcBucketRegion::~lobcBucketRegion()
   {
      
   }

   lobcBucketRegion::bucketDesc lobcBucketRegion::searchBucket(UINT32 beginPos,
                                                               const lobcBucketRegionBlock *rblock)
   {
      SDB_ASSERT(nullptr != rblock, "can not be invalid");
      SDB_ASSERT(beginPos < lobcBucketRegionBlock::BUCKET_COUNT, "out of bound");

      bucketDesc desc;
      for (INT32 i = (INT32)beginPos; i >= 0; --i)
      {
         if (INVALID_PAGE_ID != rblock->buckets[i])
         {
            desc.pid = rblock->buckets[i];
            desc.pos = (UINT32)i;
            break;
         }
      }

      return desc;
   }

   lobcBucketRegion::bucketDesc lobcBucketRegion::searchBucket(UINT32 beginPos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return searchBucket(beginPos, _block);
   }

   void lobcBucketRegion::setBucketPid(UINT32 pos, PAGE_ID pid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < lobcBucketRegionBlock::BUCKET_COUNT, "out of bound");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      _block->buckets[pos] = pid;
   }

   PAGE_ID lobcBucketRegion::getBucket(UINT32 pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < lobcBucketRegionBlock::BUCKET_COUNT, "out of bound");
      return _block->buckets[pos];
   }

   lobcBucketRegion::resizingStrategy lobcBucketRegion::getResizingStrategy(UINT32 pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < lobcBucketRegionBlock::BUCKET_COUNT, "out of bound");
      SDB_ASSERT(INVALID_PAGE_ID != _block->buckets[pos], "can not be invalid");

      UINT32 step = 2;
      UINT32 nextPos = 0;
      resizingStrategy strategy;

      if (0 != (pos & 1) || INVALID_PAGE_ID != _block->buckets[pos + 1])
      {
         goto done;
      }

      strategy._range = lobcBucketRegionBlock::BUCKET_COUNT;
      strategy._targetPos = pos + 1;
      
      nextPos = pos + step;
      while (nextPos < lobcBucketRegionBlock::BUCKET_COUNT)
      {
         if (INVALID_PAGE_ID != _block->buckets[nextPos])
         {
            break;
         }
         else
         {
            strategy._targetPos = nextPos;
          
            step <<= 1;
            nextPos = pos + step;
         }
      }

   done:
      return strategy;
   }
} // namespace vessel

} // namespace engine

