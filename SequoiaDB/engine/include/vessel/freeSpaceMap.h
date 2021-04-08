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

   Source File Name = freeSpaceMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_FREE_SPACE_MAP_H_
#define SDB_VESSEL_FREE_SPACE_MAP_H_

#include "vessel/diskFreeSpaceMap.h"
#include "ossLikely.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "ossMemPool.hpp"
#include "vessel/fsmCandidateBuckets.h"

namespace engine
{
namespace vessel
{
   class fsmFile;
   class fsmCandidateBucket;

   class freeSpaceMap : public SDBObject
   {
      public:
         
      public:
         freeSpaceMap();
         ~freeSpaceMap();

      public:
         BOOLEAN isOpen()const
         {
            return 0 != _pageSize;
         }

         INT32 create(fsmFile *file,
                     CL_MB_ID mbID,
                     UINT32 logicalID,
                     UINT32 pageSize,
                     UINT32 minFreeSize,
                     BOOLEAN bucketMode = FALSE,
                     STRIPING_ID min=INVALID_STRIPING_ID,
                     STRIPING_ID max=INVALID_STRIPING_ID);

         INT32 open(fsmFile *file,
                    CL_MB_ID mbID,
                    UINT32 logicalID,
                    UINT32 pageSize,
                    UINT32 minFreeSize,
                    BOOLEAN bucketMode = FALSE,
                    STRIPING_ID min=INVALID_STRIPING_ID,
                    STRIPING_ID max=INVALID_STRIPING_ID);

         void close();

         /// find free space only in buckets.
         INT32 fastFind(STRIPING_ID striping,
                        UINT32 originalRecordSize,
                        fsmCandidate &candidate);

         INT32 findInWholeMap(STRIPING_ID striping,
                              UINT32 originalRecordSize,
                              UINT32 flags,/// 0 means all
                              fsmCandidate &candidate);

         ///count should alwasy be eight now.
         INT32 addNewPages(CL_PAGE_SEQ firstSeq,
                           const PAGE_ID *lpids,
                           UINT32 count);

         INT32 updateBucket(CL_PAGE_SEQ seq,
                            PAGE_ID lpid,
                            UINT32 bucketNo,
                            UINT16 freeSizeFromBucket,
                            UINT16 currentFreeSize,
                            BOOLEAN failure);

         /// reorg page
         INT32 incPageFreeSize(CL_PAGE_SEQ sequence,
                               PAGE_ID lpid,
                               STRIPING_ID minStriping,
                               UINT16 newFreeSize,
                               UINT16 delta);/// newFreeSize - delta == oldFreeSize

         /// in-page moved when update record.
         INT32 decPageFreeSize(CL_PAGE_SEQ sequence,
                               PAGE_ID lpid,
                               STRIPING_ID minStriping,
                               UINT16 newFreeSize,
                               UINT16 delta);/// newFreeSize + delta == oldFreeSize

      public:
         static const UINT32 FIND_BUCKET;
         static const UINT32 FIND_NEW_POOL;
         static const UINT32 FIND_DISK_MAP;
         static const UINT32 FIND_ALL;

      private:
         struct _pageSAndL
         {
            OSS_INLINE _pageSAndL():
            seq(INVALID_CL_PAGE_SEQ),
            lpid(INVALID_PAGE_ID){}

            OSS_INLINE _pageSAndL(CL_PAGE_SEQ s, PAGE_ID p):
            seq(s),lpid(p){}

            OSS_INLINE ~_pageSAndL(){}

            OSS_INLINE _pageSAndL &operator=(const _pageSAndL &p)
            {
               seq = p.seq;
               lpid = p.lpid;
               return *this;
            }

            OSS_INLINE BOOLEAN isValid()const
            {
               return INVALID_CL_PAGE_SEQ != seq;
            }

            CL_PAGE_SEQ seq;
            PAGE_ID lpid;
         };//struct _pageSAndL
         typedef ossPoolList<_pageSAndL> _NEW_PAGE_POOL;

      private:
         UINT32 getBucketNo(STRIPING_ID striping);

         BOOLEAN findFromBucket(UINT32 bucketNo,
                                UINT32 size,
                                fsmCandidate &candidate);

         UINT32 estimateMaxUpdatingCount(UINT32 bucketNo);

         /// WARNING: should always do finding in bucket first.
         BOOLEAN findPageFromPoolAndUpdateBucket(UINT32 bucketNo,
                                                 UINT16 size,
                                                 fsmCandidate &candidate);

         /// WARNING: should always do finding in pool first.
         INT32 findPageFromDiskMapAndUpdateBucket(UINT32 bucketNo,
                                                  UINT16 size,
                                                  fsmCandidate &candidate);
      private:
         void fini();
         void addNewPagesToPool(UINT32 count,
                                CL_PAGE_SEQ seq,
                                const PAGE_ID *lpids);
         void savePagesInPool();
         void savePagesInBuckets();
 
         
      private:
         STRIPING_ID _minStriping;
         STRIPING_ID _maxStriping;
         UINT32 _pageSize;
         UINT32 _maxFreeSize;
         UINT32 _minFreeSize;
         UINT16 _bucketCount;
         UINT16 _bucketCapacity;
         fsmCandidateBuckets _buckets;

         ossSpinSLatch _latch;
         _NEW_PAGE_POOL _newPagePool;
         diskFreeSpaceMap _dfsm;
   };//class freeSpaceMap
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FREE_SPACE_MAP_H_