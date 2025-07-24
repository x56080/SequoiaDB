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

   Source File Name = lobcMetaBlockMapping.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_META_BLOCK_MAPPING_H_
#define VESSEL_LOBC_META_BLOCK_MAPPING_H_

#include "vessel/lobMetaDataFile.h"
#include "vessel/lobChunkKey.h"
#include "vessel/lobcExtentChain.h"
#include "vessel/lobcBucketRegion.h"
#include "vessel/lobChunkSearchEntry.h"
#include "vessel/recordID.h"
#include "vessel/listLobChunkCursor.h"

namespace engine
{
namespace vessel
{
   class lobMetaDataFile;
   class storageUnitManifest;
   struct lobExtentMetaBlock;
   class fclusterSpaceManager;

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

         INT32 truncate(const lobChunkSearchEntry &entry,
                        UINT32 size,
                        UINT32 &tsize,
                        lobcExtentChain &chain,
                        ossPoolList<lextentDescriptor> &discarded);

         /// deltaSize can not be over the free size of current tail extent.
         INT32 extendLastBlockSize(const lobChunkSearchEntry &entry,
                                   UINT32 deltaSize);

         /// current tail extent will be resized to clear whole.
         /// chain pos of new tail block must be set correctly.
         INT32 appendBlockToChain(const lobExtentMetaBlock *block);

         void truncate(UINT32 lclid, fclusterSpaceManager *smgr=nullptr);

         INT32 list(listLobChunkCursor *cursor);

      private:
         /// return region id and position in region.
         UINT32 getBucketRegion(UINT32 lobKeyHash, UINT32 &pos)const;

         UINT32 getTotalRegionCount()const;

         lobcBucketRegionBlock *getRegionBlock(UINT32 regionId);

         /// seek entry in buket list until hit the end.
         INT32 seek(const lobChunkSearchEntry &entry,
                    PAGE_ID bucketEntry,
                    recordID &rid);

         INT32 fillChain(const lobChunkSearchEntry &entry,
                         const recordID &pos,
                         lobcExtentChain &chain,
                         recordID *tailRid=nullptr);

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

         void _truncate(lobcBucketRegion &region,
                        UINT32 lclid,
                        fclusterSpaceManager *smgr);

         INT32 _appendBlockToChain(const recordID &currentTailRid,
                                   const lobExtentMetaBlock *block);

         INT32 _extendBlockSize(const recordID &rid,
                                UINT32 deltaSize);

         INT32 _truncateLobc(const lobChunkSearchEntry &entry,
                             UINT32 size,
                             UINT32 bucketPos,
                             lobcBucketRegion &region,
                             UINT32 &tsize,
                             lobcExtentChain &chain,
                             ossPoolList<lextentDescriptor> &discarded);

         void _initRegionToList(UINT32 regionId, listLobChunkCursor *cursor);
         INT32 _listInRegion(listLobChunkCursor *cursor, UINT32 &count);
         INT32 _listInBucket(PAGE_ID entryPid,
                             listLobChunkCursor *cursor,
                             UINT32 &count);

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