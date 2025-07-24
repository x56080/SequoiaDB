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

   Source File Name = lobcMetaBlockMapping.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobcMetaBlockMapping.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/lobcMetaBlockPage.h"
#include "vessel/lobMetaDataFile.h"
#include "vessel/storageManifest.h"
#include "vessel/fclusterSpaceManager.h"
#include "dmsLobDef.hpp"

namespace engine
{
namespace vessel
{
   lobcMetaBlockMapping::lobcMetaBlockMapping(const storageUnitManifest *manifest,
                                              PAGE_ID entryPid,
                                              lobMetaDataFile *file):
   _manifest(manifest),
   _entryPid(entryPid),
   _mfile(file)
   {
      SDB_ASSERT(nullptr != _manifest && _manifest->isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != _entryPid, "can not be invalid");
      SDB_ASSERT(nullptr != _mfile && _mfile->isOpen(), "can not be invalid");
      _regionCount = file->getCommonHeadInMem().pageSize / LOBC_BUCKET_REGION_BLOCK_SIZE;
      SDB_ASSERT(0 < _regionCount, "can not be invalid");
      SDB_ASSERT(ossIsPowerOf2(_regionCount), "must be power of 2");
      _globalBucketCount = lobcBucketRegionBlock::BUCKET_COUNT * _regionCount;
      SDB_ASSERT(ossIsPowerOf2(_globalBucketCount),
                 "must be power of 2");
   }

   lobcMetaBlockMapping::~lobcMetaBlockMapping()
   {

   }

   void lobcMetaBlockMapping::truncate(UINT32 lclid, fclusterSpaceManager *smgr)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCLID != lclid, "can not be invalid");
      
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      FIXED_POSIX_S_LATCH_ARRAY &latches = tc->getEnv()->latchEnv.lobRegionLatchVec;

      for (UINT32 i = 0; i < getTotalRegionCount(); ++i)
      {
         ossSpinSLatchPOSIX *regionLock = latches.mod(i);
         regionLock->get();

         lobcBucketRegionBlock *regionBlock = getRegionBlock(i);
         lobcBucketRegion region(i, regionBlock);
         _truncate(region, lclid, smgr);
         
         regionLock->release();
      }
   
      return;
   }

   INT32 lobcMetaBlockMapping::find(const lobChunkSearchEntry &entry,
                                    lobcExtentChain &chain)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      UINT32 regionId = 0;
      ossSpinSLatchPOSIX *regionLock = nullptr;
      lobcBucketRegionBlock *regionBlock = nullptr;
      UINT32 bucketPos = 0;
      lobcBucketRegion::bucketDesc bucket;
      recordID rid;

      chain.reset();

      if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      regionId = getBucketRegion(entry.hash(), bucketPos);
      regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");
      regionLock->get_shared();

      regionBlock = getRegionBlock(regionId);
      if (OSS_UNLIKELY(nullptr == regionBlock))
      {
         PD_LOG(PDERROR, "failed to get region block[%d]", regionId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bucket = lobcBucketRegion::searchBucket(bucketPos, regionBlock);
      if (INVALID_PAGE_ID == bucket.pid)
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST;
         goto error;
      }

      rc = seek(entry, bucket.pid, rid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = fillChain(entry, rid, chain);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fill chain:%d", rc);
         goto error;
      }
   done:
      if (nullptr != regionLock)
      {
         regionLock->release_shared();
      }
      return rc;
   error:
      chain.reset();
      goto done;
   }

   INT32 lobcMetaBlockMapping::insert(const lobExtentMetaBlock *block)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      UINT32 regionId = 0;
      ossSpinSLatchPOSIX *regionLock = nullptr;
      lobcBucketRegionBlock *regionBlock = nullptr;
      UINT32 beginBucket = 0;

      if (OSS_UNLIKELY(nullptr == block ||
                       !block->isValid() ||
                       0 != block->chainPos ||
                       !block->isChainTail()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      regionId = getBucketRegion(block->hash(), beginBucket);
      regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");
      regionLock->get();

      regionBlock = getRegionBlock(regionId);
      if (OSS_UNLIKELY(nullptr == regionBlock))
      {
         PD_LOG(PDERROR, "failed to get region block[%d]", regionId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else
      {
         lobcBucketRegion region(regionId, regionBlock);
         rc = insertIntoRegion(block, beginBucket, region);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert into region[%d], rc:%d", regionId, rc);
            goto error;
         }
      }
      
   done:
      if (nullptr != regionLock)
      {
         regionLock->release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::remove(const lobChunkSearchEntry &entry,
                                      lobcExtentChain *chainRemoved)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      UINT32 regionId = 0;
      ossSpinSLatchPOSIX *regionLock = nullptr;
      lobcBucketRegionBlock *regionBlock = nullptr;
      lobcBucketRegion region;
      UINT32 bucketPos = 0;
      lobcBucketRegion::bucketDesc bucket;
      recordID rid;
      lobcExtentChain chain;
      lobcExtentChain *chainPtr = nullptr == chainRemoved ?
                                  &chain : chainRemoved;

      chainPtr->reset();

      if (OSS_UNLIKELY(!entry.isValid() || 0 != entry.getChainPos()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      chainPtr->init(_manifest->lobArgs.pageSize);
      regionId = getBucketRegion(entry.hash(), bucketPos);
      regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");
      regionLock->get();

      regionBlock = getRegionBlock(regionId);
      if (OSS_UNLIKELY(nullptr == regionBlock))
      {
         PD_LOG(PDERROR, "failed to get region block[%d]", regionId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bucket = lobcBucketRegion::searchBucket(bucketPos, regionBlock);
      if (INVALID_PAGE_ID == bucket.pid)
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST;
         goto error;
      }

      rc = seek(entry, bucket.pid, rid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = fillChain(entry, rid, *chainPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fill chain:%d", rc);
         goto error;
      }

      region = lobcBucketRegion(regionId, regionBlock);
      rc = removeChainFromRegion(entry, rid, chainPtr->getChainSize(),
                                 bucket.pos, region);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove entry[%s], rc:%d",
                  entry.getKey().toString().c_str(), rc);
         goto error;
      }
   done:
      if (nullptr != regionLock)
      {
         regionLock->release();
      }
      return rc;
   error:
      if (nullptr != chainRemoved)
      {
         chainRemoved->reset();
      }
      goto done;
   }

   INT32 lobcMetaBlockMapping::truncate(const lobChunkSearchEntry &entry,
                                        UINT32 size,
                                        UINT32 &tsize,
                                        lobcExtentChain &chain,
                                        ossPoolList<lextentDescriptor> &discarded)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      UINT32 regionId = 0;
      ossSpinSLatchPOSIX *regionLock = nullptr;
      lobcBucketRegionBlock *regionBlock = nullptr;
      lobcBucketRegion region;
      UINT32 bucketPos = 0;
      lobcBucketRegion::bucketDesc bucket;

      tsize = 0;
      chain.reset();
      discarded.clear();

      if (OSS_UNLIKELY(!entry.isValid() ||
                       0 != entry.getChainPos()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      chain.init(_manifest->lobArgs.pageSize);
      regionId = getBucketRegion(entry.hash(), bucketPos);
      regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");
      regionLock->get();

      regionBlock = getRegionBlock(regionId);
      if (OSS_UNLIKELY(nullptr == regionBlock))
      {
         PD_LOG(PDERROR, "failed to get region block[%d]", regionId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bucket = lobcBucketRegion::searchBucket(bucketPos, regionBlock);
      if (INVALID_PAGE_ID == bucket.pid)
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST;
         goto error;
      }

      region = lobcBucketRegion(regionId, regionBlock);
      rc = _truncateLobc(entry, size, bucket.pos, region,
                         tsize, chain, discarded);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate lobc:%d", rc);
         goto error;
      }
   done:
      if (nullptr != regionLock)
      {
         regionLock->release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::insertIntoRegion(const lobExtentMetaBlock *block,
                                                UINT32 beginPos,
                                                lobcBucketRegion &region)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != block && block->isValid(), "can not be invalid");
      SDB_ASSERT(region.isValid(), "can not be invalid");
      
      lobChunkSearchEntry searchEntry(block->oid, block->chunkId,
                                      block->lclid, block->chainPos);

      do
      {
         PAGE_ID candidate = INVALID_PAGE_ID;
         BOOLEAN freeToInsert = FALSE;
         lobcBucketRegion::bucketDesc bucket = region.searchBucket(beginPos);
         if (INVALID_PAGE_ID == bucket.pid)
         {
            rc = ensureBucket(region, bucket.pos);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure bucket[%d] in region, rc:%d",
                     bucket.pos, rc);
               goto error;
            }

            bucket.pid = region.getBucket(bucket.pos);
         }
         
         rc = findPageToInsert(searchEntry, bucket.pid, candidate, freeToInsert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find page to insert:%d", rc);
            goto error;
         }
         
         if (freeToInsert)
         {
            lobcMetaBlockPageAccessor accessor(_mfile, candidate);
            rc = accessor.insert(block, &searchEntry);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert block into page[%d]:%d", candidate, rc);
               goto error;
            }
            break;
         }
         else
         {
            lobcBucketRegion::resizingStrategy strategy =
                                     region.getResizingStrategy(bucket.pos);
            if (strategy.isValid())
            {
               rc = resizeBucketsInRegion(strategy, bucket.pos, region);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to create new bucket[%d] from [%d] in region:%d, rc:%d",
                        strategy.getTargetPos(), bucket.pos, region.getRegionId(), rc);
                  goto error;
               }

               continue;
            }
            else
            {
               PAGE_ID newCandidate = INVALID_PAGE_ID;
               rc = splitBlockPage(candidate);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to split page[%d], rc:%d", candidate, rc);
                  goto error;
               }

               /// refind page from candidate.
               rc = findPageToInsert(searchEntry, candidate, newCandidate, freeToInsert);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to find page to insert:%d", rc);
                  goto error;
               }

               SDB_ASSERT(freeToInsert, "must be free to insert");
               {
                  lobcMetaBlockPageAccessor accessor(_mfile, newCandidate);
                  rc = accessor.insert(block, &searchEntry);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to insert block into page[%d]:%d", candidate, rc);
                     goto error;
                  }
               }

               break;

            }
         }
      } while (TRUE);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::findPageToInsert(const lobChunkSearchEntry &entry,
                                                PAGE_ID bucketEntry,
                                                PAGE_ID &pid,
                                                BOOLEAN &freeToInsert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(entry.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != bucketEntry, "can not be invalid");
      pid = INVALID_PAGE_ID;

      PAGE_ID bucketPid = bucketEntry;

      do
      {
         INT32 cmp = 0;
         lobcMetaBlockPageAccessor accessor(_mfile, bucketPid);
         if (!accessor.isValid())
         {
            PD_LOG(PDERROR, "failed to access bucket page[%d]", bucketPid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else if (accessor.isEmpty())
         {
            SDB_ASSERT(bucketPid == bucketEntry, "must be the first page");
            pid = bucketPid;
            freeToInsert = TRUE;
            break;
         }
         else if (!accessor.hasNextPid())
         {
            pid = bucketPid;
            freeToInsert = accessor.isFreeToInsert(1);
            break;
         }

         cmp = accessor.compareWithHighKey(entry);
         if (0 <= cmp)
         {
            pid = bucketPid;
            freeToInsert = accessor.isFreeToInsert(1);
            break;
         }

         bucketPid = accessor.getNextPid();
      } while (TRUE);
      
      SDB_ASSERT(INVALID_PAGE_ID != pid, "impossible");
   done:
      return rc;
   error:
      pid = INVALID_PAGE_ID;
      goto done;
   }

   lobcBucketRegionBlock *lobcMetaBlockMapping::getRegionBlock(UINT32 regionId)
   {
      SDB_ASSERT(regionId < _regionCount, "out of bound");
      lobcBucketRegionBlock *block = nullptr;
      mmapPagePointer ptr;
      strictBuffer buffer;
      INT32 rc = _mfile->getPagePtr(_entryPid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr, rc:%d", _entryPid, rc);
         goto done;
      }

      buffer.makeWritable(_mfile->getPageSize(), ptr.getBuf());
      block = buffer.getWritableObjPtr<lobcBucketRegionBlock>
                          (regionId * LOBC_BUCKET_REGION_BLOCK_SIZE);
   done:
      return block;
   }

   UINT32 lobcMetaBlockMapping::getBucketRegion(UINT32 lobKeyHash, UINT32 &pos)const
   {
      UINT32 val = (lobKeyHash & (_globalBucketCount - 1));
      pos = (val & (lobcBucketRegionBlock::BUCKET_COUNT - 1));
      return (val >> lobcBucketRegionBlock::BUCKET_COUNT_SQUARE);
   }

   UINT32 lobcMetaBlockMapping::getTotalRegionCount()const
   {
      return _globalBucketCount >> lobcBucketRegionBlock::BUCKET_COUNT_SQUARE;
   }

   INT32 lobcMetaBlockMapping::fillChain(const lobChunkSearchEntry &entry,
                                         const recordID &pos,
                                         lobcExtentChain &chain,
                                         recordID *tailRid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(entry.isValid(), "can not be invalid");
      SDB_ASSERT(pos.isValid(), "can not be invalid");

      PAGE_ID toScan = pos.getPid();
      INT32 slotPos = pos.getPos();
      UINT16 chainPos = entry.getChainPos();
      
      chain.init(_manifest->lobArgs.pageSize);

      do
      {
         BOOLEAN scanNextPage = TRUE;
         lobcMetaBlockPageAccessor accessor(_mfile, toScan);
         if (OSS_UNLIKELY(!accessor.isValid()))
         {
            PD_LOG(PDERROR, "failed to init accessor of pid[%d]", toScan);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         while ((UINT32)slotPos < accessor.getItemCount())
         {
            const lobExtentMetaBlock *block = accessor.getExtentMetaBlock(slotPos);
            SDB_ASSERT(nullptr != block && block->isValid(), "can not be invalid");
            if (0 != block->compare(entry.getLogicalClId(),
                                    entry.getKey(),
                                    chainPos))
            {
               scanNextPage = FALSE;
               break;
            }

            if (!validateExtentSize(block))
            {
               PD_LOG(PDERROR, "invalid block size[%d] found in block[%s]",
                     block->size, block->toString().c_str());
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            rc = chain.pushBack(block->getExtentDesc());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push extent into chain:%d", rc);
               goto error;
            }

            if (block->isChainTail())
            {
               if (nullptr != tailRid)
               {
                  tailRid->setPid(toScan);
                  tailRid->setPos(slotPos);
               }
               goto done;
            }

            ++slotPos;
            ++chainPos;
         }

         if (scanNextPage)
         {
            /// continue to scan the next page.
            /// reset pos as zero.
            /// all blocks should be saved in order even span mutliple pages.
            toScan = accessor.getNextPid();
            slotPos = 0;
         }
         else
         {
            break;
         }
      }while (INVALID_PAGE_ID != toScan);


      PD_LOG(PDERROR, "chain tail of [%s] not found", entry.getKey().toString().c_str());
      rc = SDB_VESSEL_KEY_NOT_FOUND;
      goto error;

      
   done:
      return rc;
   error:
      chain.reset();
      if (nullptr != tailRid)
      {
         tailRid->reset();
      }
      goto done;
   }

   INT32 lobcMetaBlockMapping::seek(const lobChunkSearchEntry &entry,
                                    PAGE_ID bucketEntry,
                                    recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(entry.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != bucketEntry, "can not be invalid");

      PAGE_ID pidToScan = bucketEntry;
      rid.reset();

      do
      {
         lobcMetaBlockPageAccessor accessor(_mfile, pidToScan);
         if (!accessor.isValid())
         {
            PD_LOG(PDERROR, "failed to init accessor of page[%d]", pidToScan);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (accessor.isEmpty())
         {
            SDB_ASSERT(pidToScan == bucketEntry, "only the first page can be empty");
            pidToScan = accessor.getNextPid();
            continue;
         }
         else
         {
            INT32 cmp = accessor.testHashBound(entry.hash());
            if (cmp < 0)
            {
               pidToScan = accessor.getNextPid();
               continue;
            }
            else if (cmp > 0)
            {
               break;
            }
            else
            {
               INT32 slotPos = -1;
               rc = accessor.seek(entry, slotPos);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to seek entry in page[%d], rc:%d",
                         pidToScan, rc);
                  goto error;
               }

               if (0 <= slotPos)
               {
                  rid.setPid(pidToScan);
                  rid.setPos(static_cast<RECORD_SLOT_POS>(slotPos));
                  break;
               }
               else
               {
                  pidToScan = accessor.getNextPid();
                  continue;
               }
            }
         }
      } while (INVALID_PAGE_ID != pidToScan);

      if (!rid.isValid())
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::ensureBucket(lobcBucketRegion &region,
                                            UINT32 pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(region.isValid(), "can not be invalid");
      SDB_ASSERT(pos < lobcBucketRegionBlock::BUCKET_COUNT, "out of bound");
      SDB_ASSERT(nullptr != _mfile, "can not be invalid");
      PAGE_ID pid = INVALID_PAGE_ID;
      mmapPagePointer ptr;
      lobcMetaBlockPageAccessor accessor;

      if (INVALID_PAGE_ID != region.getBucket(pos))
      {
         goto done;
      }

      rc = _mfile->reservePid(pid, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve new bucket page:%d", rc);
         goto error;
      }

      accessor.init(_mfile->getPageSize(), ptr.getBuf());
      accessor.initPage();
      region.setBucketPid(pos, pid);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::resizeBucketsInRegion(const lobcBucketRegion::resizingStrategy &strategy,
                                                     UINT32 srcPos,
                                                     lobcBucketRegion &region)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(strategy.isValid(), "can not be invalid");
      SDB_ASSERT(region.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID == region.getBucket(strategy.getTargetPos()), "must be invalid");

      PAGE_ID srcBucketPid = region.getBucket(srcPos);
      SDB_ASSERT(INVALID_PAGE_ID != srcBucketPid, "can not be invalid");

      ossPoolVector<PAGE_ID> newBucketPids;
      lobcMetaBlockPageAccessor newBucketAccessor;
      PAGE_ID firstNewBucketPid = INVALID_PAGE_ID;
      mmapPagePointer ptr;
      UINT32 rowCount = 0;

      rc = _mfile->reservePid(firstNewBucketPid, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve new page:%d", rc);
         goto error;
      }
      newBucketAccessor.init(_mfile->getPageSize(), ptr.getBuf());
      newBucketAccessor.initPage();
      newBucketPids.push_back(firstNewBucketPid);

      do
      {
         lobcMetaBlockPageAccessor src(_mfile, srcBucketPid);
         if (!src.isValid())
         {
            PD_LOG(PDERROR, "failed to init accessor on page[%d]", srcBucketPid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         for (UINT32 i = 0; i < src.getItemCount(); ++i)
         {
            lobcMetaBlockPage::itemSlot slot = src.getSlot(i);
            if (!strategy.targetOwned(slot.hash))
            {
               continue;
            }

            const lobExtentMetaBlock *mb = src.getExtentMetaBlock((INT32)i);
            if (OSS_UNLIKELY(nullptr == mb))
            {
               PD_LOG(PDERROR, "failed to get block of[%d, %d]", srcBucketPid, i);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            if (!newBucketAccessor.isFreeToInsert(1))
            {
               PAGE_ID preBucketPid = newBucketPids.back();
               PAGE_ID newBucketPid = INVALID_PAGE_ID;
               rc = _mfile->reservePid(newBucketPid, &ptr);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reserve new page:%d", rc);
                  goto error;
               }

               newBucketAccessor.setNextPid(newBucketPid);
               newBucketPids.push_back(newBucketPid);

               rc = newBucketAccessor.init(_mfile, newBucketPid);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to init accessor on page[%d], rc:%d", newBucketPid, rc);
                  goto error;
               }

               newBucketAccessor.initPage();
               newBucketAccessor.setPrePid(preBucketPid);
            }

            rc = newBucketAccessor.pushBack(slot.hash, mb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push back meta block:%d", rc);
               goto error;
            }

            ++rowCount;
         }

         srcBucketPid = src.getNextPid();
      } while (INVALID_PAGE_ID != srcBucketPid);
      
      /// do not remove blocks in first loop 
      /// to ensure the operation is atomic.
      if (0 < rowCount)
      {
         removeTargetOwnedBlocks(strategy, srcPos, region);
      }

      region.setBucketPid(strategy.getTargetPos(), firstNewBucketPid);
   done:
      return rc;
   error:
      if (!newBucketPids.empty())
      {
         _mfile->freePids(newBucketPids.size(), newBucketPids.data());
      }
      goto done;
   }

   void lobcMetaBlockMapping::removeTargetOwnedBlocks(const lobcBucketRegion::resizingStrategy &strategy,
                                                      UINT32 pos,
                                                      lobcBucketRegion &region)
   {
      SDB_ASSERT(strategy.isValid(), "can not be invalid");

      ossPoolVector<PAGE_ID> emptyPids;
      PAGE_ID pid = region.getBucket(pos);
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      while (INVALID_PAGE_ID != pid)
      {
         lobcMetaBlockPageAccessor accessor(_mfile, pid);
         SDB_ASSERT(accessor.isValid(), "can not be invalid");
         accessor.removeTargetOwnedBlocks(strategy);
         PAGE_ID nextPid = accessor.getNextPid();

         if (accessor.isEmpty() && !accessor.isTheOnlyPageInBucket())
         {
            removePageFromBucket(pid, pos, region);
         }

         pid = nextPid;
      }       

      return;
   }

   void lobcMetaBlockMapping::removePageFromBucket(PAGE_ID pid,
                                                   UINT32 pos,
                                                   lobcBucketRegion &region)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != region.getBucket(pos), "can not be invalid");
      lobcMetaBlockPageAccessor accessor(_mfile, pid);
      SDB_ASSERT(accessor.isValid(), "can not be invalid");
      SDB_ASSERT(!accessor.isTheOnlyPageInBucket(), "can not be the only one in bucket");

      if (region.getBucket(pos) == pid)
      {
         lobcMetaBlockPageAccessor next(_mfile, accessor.getNextPid());
         SDB_ASSERT(next.isValid(), "can not be invalid");
         next.setPrePid(INVALID_PAGE_ID);
         region.setBucketPid(pos, accessor.getNextPid());
      }
      else if (accessor.hasPrePid() && accessor.hasNextPid())
      {
         lobcMetaBlockPageAccessor pre(_mfile, accessor.getPrePid());
         SDB_ASSERT(pre.isValid(), "can not be invalid");
         lobcMetaBlockPageAccessor next(_mfile, accessor.getNextPid());
         SDB_ASSERT(next.isValid(), "can not be invalid");
         pre.setNextPid(accessor.getNextPid());
         next.setPrePid(accessor.getPrePid());
      }
      else
      {
         SDB_ASSERT(accessor.hasPrePid() && !accessor.hasNextPid(), "must be the last one");
         lobcMetaBlockPageAccessor pre(_mfile, accessor.getPrePid());
         SDB_ASSERT(pre.isValid(), "can not be invalid");
         pre.setNextPid(INVALID_PAGE_ID);
      }

      _mfile->freePid(pid);
   }

   INT32 lobcMetaBlockMapping::splitBlockPage(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

      ///TODO: it may be a bad idea to always do half split.
      constexpr FLOAT32 _SPLIT_RATIO = 0.50f;
      PAGE_ID targetPid = INVALID_PAGE_ID;
      lobcMetaBlockPageAccessor targetAccessor;
      mmapPagePointer ptr;
      lobcMetaBlockPageAccessor accessor(_mfile, pid);
      SDB_ASSERT(accessor.isValid(), "can not be invalid");
      SDB_ASSERT(!accessor.isFreeToInsert(1), "must be full");
      INT32 beginPos = -1;
      
      rc = _mfile->reservePid(targetPid, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve new page:%d", rc);
         goto error;
      }

      targetAccessor.init(_mfile->getPageSize(), ptr.getBuf());
      targetAccessor.initPage();
      beginPos = accessor.getItemCount() * _SPLIT_RATIO;

      for (INT32 i = beginPos; i < (INT32)(accessor.getItemCount()); ++i)
      {
         lobcMetaBlockPage::itemSlot slot = accessor.getSlot(i);
         const lobExtentMetaBlock *mb = accessor.getExtentMetaBlock(i);
         if (OSS_UNLIKELY(nullptr == mb))
         {
            PD_LOG(PDERROR, "failed to get block[%d] ad pid[%d]", i, pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = targetAccessor.pushBack(slot.hash, mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push block into new page:%d", rc);
            goto error;
         }
      }

      if (accessor.hasNextPid())
      {
         lobcMetaBlockPageAccessor next(_mfile, accessor.getNextPid());
         SDB_ASSERT(next.isValid(), "can not be invalid");
         next.setPrePid(targetPid);
         targetAccessor.setNextPid(accessor.getNextPid());
      }
      targetAccessor.setPrePid(pid);
      accessor.setNextPid(targetPid);
      accessor.truncate(beginPos);
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != targetPid)
      {
         _mfile->freePid(targetPid);
      }
      goto done;
   }

   BOOLEAN lobcMetaBlockMapping::validateExtentSize(const lobExtentMetaBlock *block)
   {
      SDB_ASSERT(nullptr != block && block->isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != _manifest && _manifest->isValid(), "can not be invalid");
      UINT32 blockSize = block->pcnt * _manifest->lobArgs.pageSize;
      if (block->isChainTail())
      {
         return block->size <= blockSize;
      }
      else
      {
         return blockSize == block->size;
      }
   }

   INT32 lobcMetaBlockMapping::removeChainFromRegion(const lobChunkSearchEntry &entry,
                                                     const recordID &rid,
                                                     UINT32 chainSize,
                                                     UINT32 bucketPos,
                                                     lobcBucketRegion &region)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(entry.isValid() && 0 == entry.getChainPos(), "can not be invalid");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(region.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != region.getBucket(bucketPos), "can not be invalid");
      SDB_ASSERT(0 < chainSize, "can not be zero");

      PAGE_ID pid = rid.getPid();
      UINT32 pos = rid.getPos();
      UINT32 removed = 0;
      PAGE_ID firstPidToRebalance = INVALID_PAGE_ID;

      do
      {
         UINT32 count = 0;
         UINT32 countToRemove = chainSize - removed;
         PAGE_ID nextPid = INVALID_PAGE_ID;
         lobcMetaBlockPageAccessor accessor(_mfile, pid);
         if (!accessor.isValid())
         {
            PD_LOG(PDERROR, "failed to init accessor of pid[%d]", pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else if (accessor.getItemCount() <= pos)
         {
            PD_LOG(PDERROR, "invalid pos[%d] to remove", pos);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         count = (pos + countToRemove) <= accessor.getItemCount() ?
                 countToRemove : (accessor.getItemCount() - pos);
         for (UINT32 i = pos; i < count; ++i)
         {
            const lobExtentMetaBlock *block = accessor.getExtentMetaBlock(i);
            if (OSS_UNLIKELY(nullptr == block))
            {
               PD_LOG(PDERROR, "failed to get block[%d] ptr in page[%d]", i, pid);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            if (0 != block->compare(entry.getLogicalClId(),
                                    entry.getKey(),
                                    removed + i))
            {
               PD_LOG(PDERROR, "failed to validate block to be removed");
               rc = SDB_INVALID_OPERATION;
               goto error;
            }
         }

         rc = accessor.remove(pos, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove blocks on page[%d], rc:%d", pid, rc);
            goto error;
         }

         removed += count;
         pos = 0;
         nextPid = accessor.getNextPid();

         if (accessor.isEmpty() && !accessor.isTheOnlyPageInBucket())
         {
            removePageFromBucket(pid, bucketPos, region);
         }
         else if (MAX_PAGE_FREE_PCT < accessor.getFreePct() &&
                  INVALID_PAGE_ID == firstPidToRebalance)
         {
            firstPidToRebalance = pid;
         }

         pid = nextPid;

      } while (removed < chainSize && INVALID_PAGE_ID != pid);

      if (INVALID_PAGE_ID != firstPidToRebalance)
      {
         rebalancePagesInBucket(region, bucketPos, firstPidToRebalance);
      }

      if (removed != chainSize)
      {
         PD_LOG(PDERROR, "failed to remove whole chain of lobc[%s]",
                entry.getKey().toString().c_str());
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::rebalancePagesInBucket(lobcBucketRegion &region,
                                                      UINT32 bucketPos,
                                                      PAGE_ID beginEntry)
   {
      SDB_ASSERT(region.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != region.getBucket(bucketPos), "can not be invalid");
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID == beginEntry ?
                    region.getBucket(bucketPos) : beginEntry;

      lobcMetaBlockPageAccessor accessor(_mfile, pid);
      if (!accessor.isValid())
      {
         PD_LOG(PDERROR, "failed to init accessor of page[%d]", pid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      while (accessor.hasNextPid())
      {
         lobcMetaBlockPageAccessor next(_mfile, accessor.getNextPid());
         if (!next.isValid())
         {
            PD_LOG(PDERROR, "failed to init accessor of pid[%d]", accessor.getNextPid());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else if (accessor.getFreePct() <= MAX_PAGE_FREE_PCT ||
                  0 == accessor.getItemCountToFit(REBALANCED_PAGE_FREE_PCT))
         {
            accessor = next;
            continue;
         }
         else
         {
            UINT32 moved = 0;
            UINT32 count = accessor.getItemCountToFit(REBALANCED_PAGE_FREE_PCT);
            if (next.getItemCount() < count)
            {
               count = next.getItemCount();
            }

            for (UINT32 i = 0; i < count; ++i)
            {
               lobcMetaBlockPage::itemSlot slot = next.getSlot(i);
               const lobExtentMetaBlock *block = next.getExtentMetaBlock(i);
               if (OSS_UNLIKELY(nullptr == block))
               {
                  PD_LOG(PDERROR, "failed to get block ptr[%d] on page[%d]",
                         i, accessor.getNextPid());
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  goto error;
               }

               rc = accessor.pushBack(slot.hash, block);
               if (SDB_OK != rc)
               {
                  if (0 < moved)
                  {
                     next.remove(0, moved);
                  }
                  PD_LOG(PDERROR, "failed to push block into accessor:%d", rc);
                  goto error;
               }

               ++moved;
            }

            SDB_ASSERT(0 < moved, "impossible");
            next.remove(0, moved);
            if (next.isEmpty())
            {
               next.reset();
               removePageFromBucket(accessor.getNextPid(), bucketPos, region);
               /// accessor.next updated
               if (accessor.hasNextPid())
               {
                  PAGE_ID nextPid = accessor.getNextPid();
                  rc = next.init(_mfile, nextPid);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to init accessor of page[%d], rc:%d",
                            nextPid, rc);
                     goto error;
                  }
               }
               else
               {
                  break;
               }
            }
            
            accessor = next;
         }
      }

   done:
      return rc;
   error:
      goto done;  
   }

   void lobcMetaBlockMapping::_truncate(lobcBucketRegion &region,
                                        UINT32 lclid,
                                        fclusterSpaceManager *smgr)
   {
      SDB_ASSERT(region.isValid(), "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != lclid, "can not be invalid");

      for (UINT32 bucketPos = 0; bucketPos < lobcBucketRegionBlock::BUCKET_COUNT; ++bucketPos)
      {
         PAGE_ID pid = region.getBucket(bucketPos);
         while (INVALID_PAGE_ID != pid)
         {
            PAGE_ID nextPid = INVALID_PAGE_ID;
            lobcMetaBlockPageAccessor accessor(_mfile, pid);
            if (!accessor.isValid())
            {
               PD_LOG(PDERROR, "failed to init accessor of pid[%d]", pid);
               goto done;
            }

            nextPid = accessor.getNextPid();

            for (UINT32 pos = 0; pos < accessor.getItemCount();)
            {
               const lobExtentMetaBlock *block = accessor.getExtentMetaBlock(pos);
               if (OSS_UNLIKELY(nullptr == block || !block->isValid()))
               {
                  PD_LOG(PDERROR, "failed to get block[%d] at page[%d]", pos, pid);
                  ++pos;
               }
               else if (block->lclid == lclid)
               {
                  lextentDescriptor desc = block->getExtentDesc();
                  INT32 rc = accessor.remove(pos, 1);
                  if (OSS_UNLIKELY(SDB_OK != rc))
                  {
                     PD_LOG(PDERROR, "failed to remove item[%d] at page[%d], rc:%d",
                            pos, pid, rc);
                     ++pos;
                  }

                  if (nullptr != smgr)
                  {
                     smgr->releaseExtent(desc.pid, desc.pcnt);
                  }
               }
               else
               {
                  ++pos;
               }
            }//for (UINT32 pos = 0; pos < accessor.getItemCount();)

            if (accessor.isEmpty() && !accessor.isTheOnlyPageInBucket())
            {
               removePageFromBucket(pid, bucketPos, region);
            }

            pid = nextPid;
         }//while (INVALID_PAGE_ID != pid)
      }

   done:
      return;
   }

   INT32 lobcMetaBlockMapping::appendBlockToChain(const lobExtentMetaBlock *block)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      UINT32 regionId = 0;
      ossSpinSLatchPOSIX *regionLock = nullptr;
      lobcBucketRegionBlock *regionBlock = nullptr;
      UINT32 beginBucket = 0;
      lobcBucketRegion::bucketDesc bucket;
      lobChunkSearchEntry entry;
      recordID rid, tailRid;
      lobcExtentChain chain;

      if (OSS_UNLIKELY(nullptr == block ||
                       !block->isValid() ||
                       0 == block->chainPos || /// impossible to be zero.
                       !block->isChainTail()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      regionId = getBucketRegion(block->hash(), beginBucket);
      regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");
      regionLock->get();

      regionBlock = getRegionBlock(regionId);
      if (OSS_UNLIKELY(nullptr == regionBlock))
      {
         PD_LOG(PDERROR, "failed to get region block[%d]", regionId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bucket = lobcBucketRegion::searchBucket(beginBucket, regionBlock);
      if (INVALID_PAGE_ID == bucket.pid)
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST;
         goto error;
      }

      entry.set(block->oid, block->chunkId, block->lclid);
      rc = seek(entry, bucket.pid, rid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = fillChain(entry, rid, chain, &tailRid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fill extent chain:%d", rc);
         goto error;
      }

      if (chain.getChainSize() != block->chainPos)
      {
         PD_LOG(PDERROR, "invalid block with chain pos[%d] to append", block->chainPos);
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (MAX_LOB_CHUNK_SIZE < (block->size + chain.getCurrentCapacity()))
      {
         PD_LOG(PDERROR, "out of lob chunk size");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = _appendBlockToChain(tailRid, block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append new tail block:%d", rc);
         goto error;
      }
   done:
      if (nullptr != regionLock)
      {
         regionLock->release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::_appendBlockToChain(const recordID &currentTailRid,
                                                   const lobExtentMetaBlock *block)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(currentTailRid.isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != block && block->isValid(), "can not be invalid");

      UINT32 tailPos = currentTailRid.getPos();
      PAGE_ID pid = currentTailRid.getPid();
      lobcMetaBlockPageAccessor accessor(_mfile, pid);
      if (OSS_UNLIKELY(!accessor.isValid()))
      {
         PD_LOG(PDERROR, "failed to init accessor of page[%d]", pid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!accessor.isFreeToInsert(1))
      {
         rc = splitBlockPage(pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split block page[%d], rc:%d", pid, rc);
            goto error;
         }

         if (accessor.getItemCount() <= tailPos)
         {
            tailPos -= accessor.getItemCount();
            pid = accessor.getNextPid();
            accessor.init(_mfile, pid);
            SDB_ASSERT(accessor.isValid(), "impossible to be invalid");
         }
      }

      rc = accessor.addNewTailToChain(tailPos, _manifest->lobArgs.pageSize, block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append new tail to chain at page[%d], rc:%d",
                pid, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::extendLastBlockSize(const lobChunkSearchEntry &entry,
                                                   UINT32 deltaSize)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      UINT32 regionId = 0;
      ossSpinSLatchPOSIX *regionLock = nullptr;
      lobcBucketRegionBlock *regionBlock = nullptr;
      UINT32 beginBucket = 0;
      lobcBucketRegion::bucketDesc bucket;
      recordID rid, tailRid;
      lobcExtentChain chain;
      
      if (OSS_UNLIKELY(!entry.isValid() ||
                       0 == deltaSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      regionId = getBucketRegion(entry.hash(), beginBucket);
      regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");

      /// should we get_shared() here?
      regionLock->get();

      regionBlock = getRegionBlock(regionId);
      if (OSS_UNLIKELY(nullptr == regionBlock))
      {
         PD_LOG(PDERROR, "failed to get region block[%d]", regionId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bucket = lobcBucketRegion::searchBucket(beginBucket, regionBlock);
      if (INVALID_PAGE_ID == bucket.pid)
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST;
         goto error;
      }

      rc = seek(entry, bucket.pid, rid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = fillChain(entry, rid, chain, &tailRid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fill extent chain:%d", rc);
         goto error;
      }

      if (chain.getFreeSizeInLastExtent() < deltaSize)
      {
         PD_LOG(PDERROR, "delta size[%d] is out of extent space", deltaSize);
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = _extendBlockSize(tailRid, deltaSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend the tail extent:%d", rc);
         goto error;
      }
   done:
      if (nullptr != regionLock)
      {
         regionLock->release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::_extendBlockSize(const recordID &rid,
                                                UINT32 deltaSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      lobcMetaBlockPageAccessor accessor(_mfile, rid.getPid());
      if (!accessor.isValid())
      {
         PD_LOG(PDERROR, "failed to init accessor of page[%d]", rid.getPid());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = accessor.extendBlockSize(rid.getPos(), _manifest->lobArgs.pageSize, deltaSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend block size at[%s], rc:%d",
                rid.toString().c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::_truncateLobc(const lobChunkSearchEntry &entry,
                                             UINT32 size,
                                             UINT32 bucketPos,
                                             lobcBucketRegion &region,
                                             UINT32 &tsize,
                                             lobcExtentChain &chain,
                                             ossPoolList<lextentDescriptor> &discarded)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(discarded.empty(), "must be empty");
      recordID rid;
      PAGE_ID entryPid = region.getBucket(bucketPos);
      PAGE_ID pid = INVALID_PAGE_ID;
      INT32 pos = 0;
      INT32 removingPos = -1;
      PAGE_ID removingPid = INVALID_PAGE_ID;
      tsize = 0;

      chain.init(_manifest->lobArgs.pageSize);

      rc = seek(entry, entryPid, rid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      pid = rid.getPid();
      pos = rid.getPos();
      do
      {
         BOOLEAN truncated = FALSE;
         lobcMetaBlockPageAccessor accessor(_mfile, pid);
         if (OSS_UNLIKELY(!accessor.isValid()))
         {
            PD_LOG(PDERROR, "failed to init accessor of page[%d]", pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         for (INT32 i = pos; i < (INT32)accessor.getItemCount(); ++i)
         {
            BOOLEAN isChainTail = FALSE;
            lobExtentMetaBlock *block = accessor.getExtentMetaBlock(i);
            if (nullptr == block || !block->isValid())
            {
               PD_LOG(PDERROR, "failed to get block at[%d, %d]", pid, i);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            if (0 != block->compare(entry.getLogicalClId(), entry.getKey()))
            {
               PD_LOG(PDERROR, "unexpected block found at [%d, %d]", pid, i);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            isChainTail = block->isChainTail();
            SDB_ASSERT(isChainTail ||
                       (_manifest->lobArgs.pageSize * block->pcnt == block->size),
                       "invalid block size");
            
            if ((chain.getChunkSize() + block->size) <= size)
            {
               rc = chain.pushBack(block->getExtentDesc());
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  PD_LOG(PDERROR, "failed to push desc into chain:%d", rc);
                  goto error;
               }

               if (size == chain.getChunkSize())
               {
                  if (block->isChainTail())
                  {
                     /// nothing changed.
                     goto done;
                  }
                  else
                  {
                     truncated = TRUE;
                     block->setAsTail();
                     removingPos = i + 1;
                     removingPid = pid;
                     break;
                  }
               }
            }
            else
            {
               truncated = TRUE;

               /// tsize also may be increased when removing.
               tsize = chain.getChunkSize() + block->size - size;
               block->size -= tsize;
               if (!isChainTail)
               {
                  removingPos = i + 1;
                  removingPid = pid;
                  block->setAsTail();
               }

               rc = chain.pushBack(block->getExtentDesc());
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  block->size += tsize;
                  if (!isChainTail)
                  {
                     block->clearTail();
                  }
                  PD_LOG(PDERROR, "failed to push desc into chain:%d", rc);
                  goto error;
               }

               break;
            }

            if (isChainTail)
            {
               break;
            }
         }//for (INT32 i = rid.getPos(); i < (INT32)accessor.getItemCount(); ++i)

         if (truncated)
         {
            if (INVALID_PAGE_ID != removingPid &&
                removingPos == (INT32)accessor.getItemCount())
            {
               /// reset removing pos to next page.
               removingPos = 0;
               removingPid = accessor.getNextPid();
               SDB_ASSERT(INVALID_PAGE_ID != removingPid, "block tail missed"); 
            }
            break;
         }
         else
         {
            pid = accessor.getNextPid();
            pos = 0;
         }
      } while (INVALID_PAGE_ID != pid);

      /// clear discarded blocks, do not goto error from here.
      while (INVALID_PAGE_ID != removingPid)
      {
         PAGE_ID nextPid = INVALID_PAGE_ID;
         BOOLEAN hitRemovingTail = FALSE;
         UINT32 count = 0;
         lobcMetaBlockPageAccessor accessor(_mfile, removingPid);
         if (OSS_UNLIKELY(!accessor.isValid()))
         {
            PD_LOG(PDSEVERE, "failed to init accessor of page[%d]", removingPid);
            ///WARNING: invalid blocks remained.
            goto done;
         }

         for (UINT32 i = (UINT32)removingPos; i < accessor.getItemCount(); ++i)
         {
            const lobExtentMetaBlock *block = accessor.getExtentMetaBlock(i);
            if (nullptr == block || !block->isValid())
            {
               PD_LOG(PDERROR, "failed to get block at[%d, %d]", pid, i);
               SDB_ASSERT(FALSE, "invalid block");
               goto done;
            }

            if (0 != block->compare(entry.getLogicalClId(), entry.getKey()))
            {
               PD_LOG(PDSEVERE, "unexpected block found at [%d, %d]", removingPid, i);
               SDB_ASSERT(FALSE, "invalid block");
               goto done;
            }

            discarded.push_back(block->getExtentDesc());
            ++count;
            tsize += block->size;
            if (block->isChainTail())
            {
               hitRemovingTail = TRUE;
               break;
            }
         }//for (UINT32 i = (UINT32)removingPos; i < accessor.getItemCount(); ++i)

         SDB_ASSERT(0 < count, "should not be zero");
         accessor.remove(removingPos, count);

         nextPid = hitRemovingTail ? INVALID_PAGE_ID : accessor.getNextPid();

         if (accessor.isEmpty() && !accessor.isTheOnlyPageInBucket())
         {
            removePageFromBucket(removingPid, bucketPos, region);
         }

         removingPid = nextPid;
         removingPos = 0;
      }
   done:
      return rc;
   error:
      tsize = 0;
      chain.reset();
      discarded.clear();
      goto done;
   }

   INT32 lobcMetaBlockMapping::list(listLobChunkCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != cursor && !cursor->isClosed(), "can not be invalid");
      UINT32 count = 0;

      if (cursor->getRegionId() < 0)
      {
         _initRegionToList(0, cursor);
      }

      do
      {
         rc = _listInRegion(cursor, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to list chunks in region:%d, rc:%d",
                   cursor->getRegionId(), rc);
            goto error;
         }

         if (0 < count || 
             ((cursor->getRegionId() + 1) == (INT32)_regionCount))
         {
            break;
         }
         else
         {
            _initRegionToList(cursor->getRegionId() + 1, cursor);
         }
      } while (TRUE);

      if (0 == count)
      {
         cursor->setEOC();
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void lobcMetaBlockMapping::_initRegionToList(UINT32 regionId,
                                                listLobChunkCursor *cursor)
   {
      SDB_ASSERT(regionId < _regionCount, "out of bound");
      SDB_ASSERT(nullptr != cursor && !cursor->isClosed(), "can not be invalid");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      ossSpinSLatchPOSIX *regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");
      regionLock->get_shared();
      lobcBucketRegionBlock *regionBlock = getRegionBlock(regionId);
      cursor->setRegionToScan(regionId, *regionBlock);
      regionLock->release_shared();
      return;
   }

   INT32 lobcMetaBlockMapping::_listInRegion(listLobChunkCursor *cursor, UINT32 &count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != cursor, "can not be invalid");
      INT32 regionId = cursor->getRegionId();
      SDB_ASSERT(0 <= regionId, "invalid region id");
      count = 0;

      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      ossSpinSLatchPOSIX *regionLock = tc->getEnv()->latchEnv.lobRegionLatchVec.mod(regionId);
      SDB_ASSERT(nullptr != regionLock, "can not be null");
      ossSLatchGuard guard(regionLock, SHARED);

      lobcBucketRegionBlock *regionBlock = getRegionBlock(regionId);

      do
      {
         INT32 bucketToScan = cursor->getBucketPosToScan();
         if (bucketToScan < 0)
         {
            break;
         }
         else if (INVALID_PAGE_ID == regionBlock->buckets[bucketToScan])
         {
            SDB_ASSERT(FALSE, "impossible to get invalid pos");
            cursor->endToScanBucket(bucketToScan);
            continue;
         }
         else
         {
            _listInBucket(regionBlock->buckets[bucketToScan], cursor, count);
            cursor->endToScanBucket(bucketToScan);
            if (0 < count)
            {
               break;
            }
            else
            {
               guard.unlock();
               guard.lock();
               continue;
            }
         }
      } while (TRUE);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockMapping::_listInBucket(PAGE_ID entryPid,
                                             listLobChunkCursor *cursor,
                                             UINT32 &count)
   {
      INT32 rc = SDB_OK;
      UINT32 lclid = cursor->getCollectionId().getCLLid();
      SDB_ASSERT(DMS_INVALID_LOGICCLID != lclid, "can not be invalid");
      INT32 chunkId = cursor->getOptions().chunkId;
      PAGE_ID pid = entryPid;
      dmsLobChunkInfo info;
      count = 0;

      while (INVALID_PAGE_ID != pid)
      {
         lobcMetaBlockPageAccessor accessor(_mfile, pid);
         if (OSS_UNLIKELY(!accessor.isValid()))
         {
            PD_LOG(PDERROR, "failed to init accessor of page[%d]", pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto done;
         }

         for (UINT32 i = 0; i < accessor.getItemCount(); ++i)
         {
            const lobExtentMetaBlock *block = accessor.getExtentMetaBlock(i);
            if (nullptr == block || !block->isValid())
            {
               PD_LOG(PDERROR, "invalid block found at[%d, %d]", pid, i);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            if (info.oid.isSet())
            {
               if (block->oid != info.oid ||
                   block->chunkId != info.chunkId ||
                   block->chainPos != info.profile.chainSize)
               {
                  PD_LOG(PDERROR, "abnormal chain(tail missed) found at[%d, %d]", pid, i);
                  info.profile.setAbnormal();
                  rc = cursor->pushData(sizeof(dmsLobChunkInfo), (const CHAR *)(&info));
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to push data into cursor:%d", rc);
                     goto error;
                  }
                  info = dmsLobChunkInfo();
                  ++count;
               }
               else
               {
                  info.profile.chunkSize += block->size;
                  ++info.profile.chainSize;
               }
            }

            /// not else if
            if (!info.oid.isSet())
            {
               if (block->lclid != lclid ||
                   (0 <= chunkId && static_cast<UINT32>(chunkId) != block->chunkId))
               {
                  continue;
               }

               info.oid = block->oid;
               info.chunkId = block->chunkId;
               info.profile.chunkSize = block->size;
               info.profile.chainSize = 1;

               if (0 != block->chainPos)
               {
                  info.profile.setAbnormal();
                  PD_LOG(PDERROR, "abnormal chain(header missed) found at[%d, %d]", pid, i);
               }
            }

            if (block->isChainTail())
            {
               rc = cursor->pushData(sizeof(dmsLobChunkInfo), (const CHAR *)(&info));
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push data into cursor:%d", rc);
                  goto error;
               }
               info = dmsLobChunkInfo();
               ++count;
            }
         } //for (UINT32 i = 0; i < accessor.getItemCount(); ++i)

         pid = accessor.getNextPid();
      } //while (INVALID_PAGE_ID != pid)

      if (info.oid.isSet())
      {
         info.profile.setAbnormal();
         rc = cursor->pushData(sizeof(dmsLobChunkInfo), (const CHAR *)(&info));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push data into cursor:%d", rc);
            goto error;
         }
         ++count;
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
