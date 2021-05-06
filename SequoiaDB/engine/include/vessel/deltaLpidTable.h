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
         void clearInmmutablePages();

         /// not thread safe.
         /// will clear old immutable pages.
         void changeMutablePagesIntoImmutable();

         BOOLEAN find(PAGE_ID lpid, idMapSlot &slot);
         BOOLEAN insert(PAGE_ID lpid, const idMapSlot &slot);
         

      private:
         class _cacheBucket : public SDBObject
         {
            public:
               _cacheBucket(){}
               ~_cacheBucket(){}
            public:
               typedef ossPoolMap<PAGE_ID, idMapSlot> PAGE_CACHE;
               PAGE_CACHE cache;

         };//class _cacheBucket

      private:
         OSS_INLINE _cacheBucket *getMutableBuckets()
         {
            return 0 == _mutable ? _buckets0 : _buckets1;
         }
         OSS_INLINE _cacheBucket *getImmutableBuckets()
         {
            return 0 == _mutable ? _buckets1 : _buckets0;
         }
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
         OSS_INLINE void changeMutableBuckets()
         {
            _mutable = 1 - _mutable;
         }
      
      private:
         UINT32 _latchCount = 0;
         ossSpinSLatch *_latches = NULL;
         UINT32 _bucketCount = 0;
         _cacheBucket *_buckets0 = NULL;
         _cacheBucket *_buckets1 = NULL;
         UINT32 _mutable = 0;
   };//class deltaLpidTable
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LPID_TABLE_H_