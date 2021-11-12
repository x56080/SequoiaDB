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
#include "vessel/fsmCandidateBucket.h"
#include "vessel/fsmCandidate.h"

namespace engine
{
namespace vessel
{
   class fsmFile;
   class requestContext;

   class freeSpaceMap : public SDBObject
   {
      public:
         
      public:
         freeSpaceMap();
         ~freeSpaceMap();
         freeSpaceMap(const freeSpaceMap &) = delete;
         freeSpaceMap &operator=(const freeSpaceMap &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _isOpen;
         }

         /// min/max both be valid or invalid
         INT32 create(CL_MB_ID mbID,
                      UINT32 logicalID,
                      fsmFile *file,
                      STRIPING_ID min,
                      STRIPING_ID max);

         INT32 open(CL_MB_ID mbID,
                    UINT32 logicalID,
                    UINT32 pageCount,
                    fsmFile *file,
                    STRIPING_ID min,
                    STRIPING_ID max);

         void close();

         void destroy();

         /// Find free space in whole map.
         /// The striping is necessary when cl is sharded.
         /// User should validate candidate again even return ok.
         /// Invalid candidate means no suitable candidate found.
         INT32 find(requestContext *context,
                    INT32 lvl,
                    STRIPING_ID striping,
                    fsmCandidate &candidate);

         INT32 insertNewPages(UINT32 firstSeq,
                              const PAGE_ID *lpids,
                              UINT32 count);

         INT32 upgradePageSpaceLvl(UINT32 seq,
                                   INT32 lvl);

         INT32 downgradePgaeSpaceLvl(UINT32 seq,
                                     INT32 lvl);
      private:
         INT32 initBuckets(UINT32 bucketCount,
                           UINT32 bucketCapacity,
                           UINT32 bucketLatchCount);

         UINT32 getBucketNo(STRIPING_ID striping)const;

         OSS_INLINE ossXLatch *getBucketLatch(UINT32 bucketNo)
         {
            return _bucketLatches + (bucketNo & (_bucketLatchCount - 1));
         }

         /// get bucket latch outside first.
         INT32 upsertIntoBucketFromNewPagePool(fsmCandidateBucket &bucket,
                                               BOOLEAN &upserted);

         INT32 findFromNewPagePool(fsmCandidate &candidate);

         OSS_INLINE BOOLEAN isSharded()const
         {
            return INVALID_STRIPING_ID != _minStriping;
         }

         INT32 _find(UINT32 bucketNo,
                     INT32 lvl,
                     fsmCandidate &candidate);
                     
         INT32 findFromDiskMap(INT32 lvl,
                               fsmCandidate &candidate);
         void fini();
         void addNewPagesToPool(UINT32 count,
                                UINT32 firstSeq,
                                const PAGE_ID *lpids);
         void savePagesInPool();
         void savePagesInBuckets();
 
         
      private:
         BOOLEAN _isOpen = FALSE;
         STRIPING_ID _minStriping = INVALID_STRIPING_ID;
         STRIPING_ID _maxStriping = INVALID_STRIPING_ID;
         
         UINT32 _bucketCount = 0;
         UINT32 _bucketLatchCount = 0;
         fsmCandidateBucket *_buckets = NULL;
         ossSpinXLatch *_bucketLatches = NULL;

         FREE_SPACE_TUPLE_POOL _newPagePool;
         diskFreeSpaceMap _dfsm;
   };//class freeSpaceMap
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FREE_SPACE_MAP_H_