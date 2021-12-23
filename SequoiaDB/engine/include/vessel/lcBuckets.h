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
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class lcBuckets : public SDBObject
   {
      public:
         lcBuckets(){}
         ~lcBuckets();

         lcBuckets(const lcBuckets &) = delete;
         lcBuckets &operator=(const lcBuckets &) = delete;

      public:
         INT32 init(UINT32 bucketCount,
                     UINT32 latchCount);

         void fini();

         INT32 ensureTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                    const mmapPagePointer &ptr,
                                    lcPageTagHolder &holder,
                                    BOOLEAN &isNewTag);

         BOOLEAN getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                   lcPageTagHolder &holder);

         void discardAndPinTags(SPACE_ID sid,
                                ossPoolList<liteCachePageTag *> &tags);

      private:
         void getBucketAndLatch(const GLOBAL_PAGE_ID &id,
                                _ossSpinXLatch *&latch,
                                lcBucket *&bucket);
      private:
         UINT32 _bucketCount = 0;
         lcBucket* _buckets = NULL;
         UINT32 _latchCount = 0;
         _ossSpinXLatch *_latches = NULL;
   };
} /// end of namespace vessel
} /// end of namespace engine

#endif