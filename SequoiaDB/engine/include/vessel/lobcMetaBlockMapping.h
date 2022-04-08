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

   Source File Name = lobcMetaBlockMapping.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOBC_META_BLOCK_MAPPING_H_
#define VESSEL_LOBC_META_BLOCK_MAPPING_H_

#include "vessel/lobMetaDataFile.h"
#include "vessel/lobChunkKey.h"
#include "vessel/lobcExtentChain.h"
#include "vessel/lobcBucketRegion.h"
#include "vessel/lobChunkSearchEntry.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   class lobMetaDataFile;
   class storageUnitManifest;
   struct lobExtentMetaBlock;

   class lobcMetaBlockMapping : public SDBObject
   {
      public:
         lobcMetaBlockMapping(const storageUnitManifest *manifest,
                              PAGE_ID entryPid,
                              lobMetaDataFile *file);
         ~lobcMetaBlockMapping();
         lobcMetaBlockMapping(const lobcMetaBlockMapping &) = delete;
         lobcMetaBlockMapping &operator=(const lobcMetaBlockMapping &) = delete;

      public:
         INT32 find(const lobChunkSearchEntry &entry,
                    lobcExtentChain &chain);

         /// block must be a complete chain,
         /// which means it's chain pos is zero and tail bit is set.
         INT32 insert(const lobExtentMetaBlock *block);

         /// chain pos must be zero.
         INT32 remove(const lobChunkSearchEntry &entry,
                      lobcExtentChain *chainRemoved);

      private:
         /// return region id and position in region.
         UINT32 getBucketRegion(UINT32 lobKeyHash, UINT32 &pos)const;

         lobcBucketRegionBlock *getRegionBlock(UINT32 regionId);

         /// seek entry in buket list until hit the end.
         INT32 seek(const lobChunkSearchEntry &entry,
                    PAGE_ID bucketEntry,
                    recordID &rid);

         INT32 fillChain(const lobChunkSearchEntry &entry,
                         const recordID &pos,
                         lobcExtentChain &chain);

         INT32 insertIntoRegion(const lobExtentMetaBlock *block,
                                UINT32 beginPos,
                                lobcBucketRegion &region);

         INT32 findPageToInsert(const lobChunkSearchEntry &entry,
                                PAGE_ID bucketEntry,
                                PAGE_ID &pid,
                                BOOLEAN &freeToInsert);

         INT32 removeChainFromRegion(const lobChunkSearchEntry &entry,
                                     const recordID &rid,
                                     UINT32 chainSize,
                                     UINT32 bucketPos,
                                     lobcBucketRegion &region);

         INT32 ensureBucket(lobcBucketRegion &region,
                            UINT32 pos);

         INT32 resizeBucketsInRegion(const lobcBucketRegion::resizingStrategy &strategy,
                                     UINT32 srcPos,
                                     lobcBucketRegion &region);

         void removeTargetOwnedBlocks(const lobcBucketRegion::resizingStrategy &strategy,
                                      UINT32 pos,
                                      lobcBucketRegion &region);

         void removePageFromBucket(PAGE_ID pid,
                                   UINT32 pos,
                                   lobcBucketRegion &region);

         INT32 splitBlockPage(PAGE_ID pid);

         BOOLEAN validateExtentSize(const lobExtentMetaBlock *block);

         INT32 rebalancePagesInBucket(lobcBucketRegion &region,
                                      UINT32 bucketPos,
                                      PAGE_ID beginEntry=INVALID_PAGE_ID);

      private:
         static constexpr FLOAT32 MAX_PAGE_FREE_PCT = 0.75f;
         static constexpr FLOAT32 REBALANCED_PAGE_FREE_PCT = 0.50f;
         static_assert(REBALANCED_PAGE_FREE_PCT < MAX_PAGE_FREE_PCT, "invalid ratio");

      private:
         const storageUnitManifest *_manifest = nullptr;
         const PAGE_ID _entryPid = INVALID_PAGE_ID;
         lobMetaDataFile *_mfile = nullptr;
         UINT32 _regionCount = 0;
         UINT32 _globalBucketCount = 0;
   };//class lobcMetaBlockMapping
} // namespace vessel`

} // namespace engine


#endif//VESSEL_LOBC_META_BLOCK_MAPPING_H_