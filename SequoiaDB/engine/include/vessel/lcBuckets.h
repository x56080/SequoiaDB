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

   Source File Name = lcBuckets.h

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

#ifndef VESSEL_LC_BUCKETS_H_
#define VESSEL_LC_BUCKETS_H_

#include "vessel/lcBucket.h"

namespace engine
{
namespace vessel
{
   class storageUnit;
   

   class lcBuckets : public SDBObject
   {
      public:
         lcBuckets();
         ~lcBuckets();

      public:
         INT32 init(UINT32 bucketCount,
                     UINT32 latchCount,
                     UINT32 minRecycleCount);

         INT32 fini();

         INT32 ensureTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                    UINT32 pageSize,
                                    lcExtentTagHolder &holder,
                                    BOOLEAN &newTagInBucket);

         INT32 getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                 lcExtentTagHolder &holder);

         INT32 releaseRemovedTag(lcExtentTag *tag);

         INT32 releaseRemovedTags(UINT32 num, lcExtentTag *tags[]);

      private:
         void getBucketAndLatch(const PHY_EXTENT_ID &id,
                                _ossSpinSLatch *&latch,
                                lcBucket *&bucket);
      private:
         UINT32 _minRecycleCount;
         UINT32 _bucketCount;
         lcBucket* _buckets;
         UINT32 _latchCount;
         _ossSpinSLatch *_latches;
   };
} /// end of namespace vessel
} /// end of namespace engine

#endif