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

   Source File Name = freeSpaceMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_VESSEL_FREE_SPACE_MAP_H_
#define SDB_VESSEL_FREE_SPACE_MAP_H_

#include "vessel/diskFreeSpaceMap.h"
#include "ossLikely.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/fsmCandidateBucket.h"
#include "vessel/fsmCandidate.h"
#include "dmsStripingId.hpp"

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

         INT32 create(CL_MB_ID mbID,
                      UINT32 logicalID,
                      fsmFile *file,
                      const dmsStripingRange &range);

         INT32 open(CL_MB_ID mbID,
                    UINT32 logicalID,
                    UINT32 pageCount,
                    fsmFile *file,
                    const dmsStripingRange &range);

         void close();

         void destroy();

         void truncate();

         /// Find free space in whole map.
         /// The striping is necessary when cl is sharded.
         /// User should validate candidate again even return ok.
         /// Invalid candidate means no suitable candidate found.
         INT32 find(requestContext *context,
                    INT32 lvl,
                    const dmsStripingId &striping,
                    fsmCandidate &candidate);
         
         // only find free space on disk or new page pool,
         // and the candidate will be removed from free space map at once if found.
         INT32 findAndKick(INT32 lvl,
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

         UINT32 getBucketNoByStriping(const dmsStripingId &striping)const;
         UINT32 getBucketNoByEid(EDUID eid)const;

         OSS_INLINE ossXLatch *getBucketLatch(UINT32 bucketNo)
         {
            return _bucketLatches + (bucketNo & (_bucketLatchCount - 1));
         }

         /// get bucket latch outside first.
         INT32 upsertIntoBucketFromNewPagePool(fsmCandidateBucket &bucket,
                                               BOOLEAN &upserted);

         INT32 findFromNewPagePool(fsmCandidate &candidate);

         OSS_INLINE BOOLEAN hasStipingRange()const
         {
            return _stripingRange.isValid();
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
         dmsStripingRange _stripingRange;
         
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