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

   Source File Name = deltaLpidTable.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LPID_TABLE_H_
#define VESSEL_DELTA_LPID_TABLE_H_

#include "pageDef.h"
#include "ossLatch.hpp"
#include "ossMemPool.hpp"
#include "vessel/idMapPage.h"

namespace engine
{
namespace vessel
{
   class deltaLpidTable : public SDBObject
   {
      public:
         deltaLpidTable(){}
         ~deltaLpidTable();
         deltaLpidTable(const deltaLpidTable &) = delete;
         deltaLpidTable &operator=(const deltaLpidTable &) = delete;

      public:
         OSS_INLINE BOOLEAN isInitialized()const
         {
            return 0 != _bucketCount;
         }
         INT32 init(UINT32 bucketCount, UINT32 latchCount);
         void fini();

         /// not thread safe.
         /// must ensure that on one accessing
         /// immutable buckets.
         void clearInmmutableBuckets();

         /// not thread safe.
         INT32 mergeMutablePagesIntoImmutable();

         /// find in mutable/immutable buckets.
         INT32 find(PAGE_ID lpid,
                    PAGE_ID &pid,
                    BOOLEAN &found,
                    BOOLEAN *isMutable=NULL);

         /// upsert lpid into mutable buckets.
         INT32 upsert(PAGE_ID lpid, PAGE_ID pid);
      private:
         class _cacheBucket : public SDBObject
         {
            public:
               _cacheBucket(){}
               ~_cacheBucket(){}

            public:
               void merge(const _cacheBucket &);
            public:
               /// lpid->pid
               typedef ossPoolMap<PAGE_ID, PAGE_ID> PAGE_CACHE;
               PAGE_CACHE cache;

         };//class _cacheBucket

      private:
         BOOLEAN findInMutableBuckets(PAGE_ID lpid, PAGE_ID &pid)const;
         BOOLEAN findInImmutableBuckets(PAGE_ID lpid, PAGE_ID &pid)const;

      private:
         OSS_INLINE ossSpinSLatch *getLatch(PAGE_ID lpid)
         {
            return &(_latches[getLatchHash(lpid)]);
         }
         OSS_INLINE UINT32 getBucketHash(PAGE_ID lpid)const
         {
            return lpid & (_bucketCount - 1);
         }
         OSS_INLINE UINT32 getLatchHash(PAGE_ID lpid)const
         {
            return lpid & (_latchCount - 1);
         }
         OSS_INLINE _cacheBucket *getMutableBucket(PAGE_ID lpid)
         {
            return &(_mutableBuckets[getBucketHash(lpid)]);
         }
         OSS_INLINE const _cacheBucket *getMutableBucket(PAGE_ID lpid)const
         {
            return &(_mutableBuckets[getBucketHash(lpid)]);
         }
         OSS_INLINE _cacheBucket *getImmutableBucket(PAGE_ID lpid)
         {
            return &(_immutableBuckets[getBucketHash(lpid)]);
         }
         OSS_INLINE const _cacheBucket *getImmutableBucket(PAGE_ID lpid)const
         {
            return &(_immutableBuckets[getBucketHash(lpid)]);
         }
      
      private:
         UINT32 _latchCount = 0;
         ossSpinSLatch *_latches = NULL;
         UINT32 _bucketCount = 0;
         _cacheBucket *_mutableBuckets = NULL;
         _cacheBucket *_immutableBuckets = NULL;
   };//class deltaLpidTable
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LPID_TABLE_H_