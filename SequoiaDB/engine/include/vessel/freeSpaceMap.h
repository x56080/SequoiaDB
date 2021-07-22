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
#include "vessel/forwardList.hpp"

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
         freeSpaceMap(const freeSpaceMap &) = delete;
         freeSpaceMap &operator=(const freeSpaceMap &) = delete;

      public:
         class fastFindContext : public SDBObject
         {
            public:
               fastFindContext(){}
               ~fastFindContext(){}
               fastFindContext(const fastFindContext &o):
               _striping(o._striping),
               _tick(o._tick),
               _lvl(o._lvl),
               _bucket(o._bucket){}
               fastFindContext &operator=(const fastFindContext &o)
               {
                  _striping = o._striping;
                  _tick = o._tick;
                  _lvl = o._lvl;
                  _bucket = o._bucket;
                  return *this;
               }

            private:
               STRIPING_ID _striping = INVALID_STRIPING_ID;
               UINT16 _tick = 0;
               INT16 _lvl = FSM_INVALID_SPACE_LVL;
               INT16 _bucket = -1; 
         };//class fastFindContext

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _isOpen;
         }

         /// create with multiple bucket mode
         INT32 create(CL_MB_ID mbID,
                      UINT32 logicalID,
                      fsmFile *file,
                      UINT32 bucketCount,
                      STRIPING_ID min,
                      STRIPING_ID max);

         /// create with single bucket mode
         INT32 create(CL_MB_ID mbID,
                      UINT32 logicalID,
                      fsmFile *file);

         INT32 open(CL_MB_ID mbID,
                    UINT32 logicalID,
                    UINT32 pageCount,
                    fsmFile *file);

         INT32 open(CL_MB_ID mbID,
                    UINT32 logicalID,
                    UINT32 pageCount,
                    fsmFile *file,
                    UINT32 bucketCount,
                    STRIPING_ID min,
                    STRIPING_ID max);

         void close();

         /// Find free space only in buckets.
         /// The striping is necessary when cl is sharded.
         /// User should validate candidate again even return ok.
         /// Invalid candidate means no suitable candidate found.
         INT32 fastFind(INT32 lvl,
                        STRIPING_ID striping,
                        fsmCandidate &candidate,
                        UINT16 *bucketTick=NULL);

         INT32 find(INT32 lvl,
                    STRIPING_ID striping,
                    fsmCandidate &candidate,
                    const UINT16 *bucketTick=NULL);

         INT32 insertNewPages(UINT32 firstSeq,
                              const PAGE_ID *lpids,
                              UINT32 count);
      private:
         INT32 initBuckets(UINT32 bucketCount,
                           UINT32 bucketCapacity,
                           UINT32 bucketLatchCount);

         void finiBuckets();

         UINT32 getBucketNo(STRIPING_ID striping)const;

         OSS_INLINE ossXLatch *getBucketLatch(UINT32 bucketNo)
         {
            return _bucketLatches + (bucketNo & (_bucketLatchCount - 1));
         }

         INT32 findFromBucket(UINT32 bucketNo,
                              INT32 lvl,
                              fsmCandidate &candidate,
                              UINT16 *tick);

         INT32 upsertIntoBucket(UINT32 bucketNo,
                                UINT32 seq,
                                const fsmCandidate::SHARED_INFO_PTR sptr);

      private:
         OSS_INLINE BOOLEAN isSharded()const
         {
            return INVALID_STRIPING_ID != _minStriping;
         }

         INT32 findFromNewPagePool(fsmCandidate &candidate);

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
         
         UINT32 _bucketCapacity = 0;
         UINT32 _bucketCount = 0;
         UINT32 _bucketLatchCount = 0;
         fsmCandidateBucket *_buckets = NULL;
         ossSpinXLatch *_bucketLatches = NULL;
         UINT16 *_bucketTicks = NULL;

         FREE_SPACE_TUPLE_POOL _newPagePool;
         diskFreeSpaceMap _dfsm;
   };//class freeSpaceMap
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FREE_SPACE_MAP_H_