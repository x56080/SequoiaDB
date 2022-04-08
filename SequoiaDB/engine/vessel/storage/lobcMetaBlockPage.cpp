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

   Source File Name = lobcMetaBlockPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lobcMetaBlockPage.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/pageDef.h"
#include "dms.hpp"
#include "vessel/strictBuffer.h"
#include "vessel/lobMetaDataFile.h"
#include "ossMemPool.hpp"

#include <algorithm>

namespace engine
{
namespace vessel
{
   lobcMetaBlockPageAccessor::lobcMetaBlockPageAccessor(UINT32 pageSize,
                                                        void *pageBuf):
   _pageSize(pageSize),
   _header((lobcMetaBlockPage::pageHead *)pageBuf)
   {
      SDB_ASSERT(isValidPageSize(_pageSize), "can not be invalid");
      SDB_ASSERT(nullptr != _header, "can not be invalid");
   }

   lobcMetaBlockPageAccessor::lobcMetaBlockPageAccessor(lobMetaDataFile *mfile,
                                                        PAGE_ID pid)
   {
      init(mfile, pid);
   }

   lobcMetaBlockPageAccessor::~lobcMetaBlockPageAccessor()
   {

   }

   FLOAT32 lobcMetaBlockPageAccessor::getFreePct()const
   {
      FLOAT32 freePct = 0.0f;
      if (isValid())
      {
         freePct = static_cast<FLOAT32>(_header->totalFreeSize) / _pageSize;
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid page");
      }
      return freePct;
   }

   UINT32 lobcMetaBlockPageAccessor::getMaxItemCount()const
   {
      if (OSS_LIKELY(isValid()))
      {
         return (_pageSize - lobcMetaBlockPage::HEAD_SIZE) / getItemAndSlotSize();
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         return 0;
      }
   }

   void lobcMetaBlockPageAccessor::init(UINT32 pageSize, void *pageBuf)
   {
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(nullptr != pageBuf, "can not be invalid");
      reset();
      _pageSize = pageSize;
      _header = (lobcMetaBlockPage::pageHead *)pageBuf;
      return;
   }

   INT32 lobcMetaBlockPageAccessor::init(lobMetaDataFile *mfile, PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      mmapPagePointer ptr;
      reset();
      if (nullptr == mfile || INVALID_PAGE_ID == pid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      rc = mfile->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }

      _pageSize = mfile->getPageSize();
      _header = (lobcMetaBlockPage::pageHead *)(ptr.getBuf());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockPageAccessor::seek(const lobChunkSearchEntry &entry,
                                         INT32 &pos)const
   {
      INT32 rc = SDB_OK;
      lobcMetaBlockPage::itemSlot target(entry.hash(), 0);
      const lobcMetaBlockPage::itemSlot *first = nullptr;
      const lobcMetaBlockPage::itemSlot *end = nullptr;
      const lobcMetaBlockPage::itemSlot *slot = nullptr;
      
      pos = -1;

      if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (0 == _header->totalItemCount)
      {
         goto done;
      }

      first = getSlotPtr(0);
      if (OSS_UNLIKELY(nullptr == first))
      {
         PD_LOG(PDERROR, "failed to get first slot ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      end = first + _header->totalItemCount;

      slot = std::lower_bound(first, end, target);
      if (slot == end)
      {
         goto done;
      }

      while (slot != end)
      {
         SDB_ASSERT(entry.hash() <= slot->hash, "impossible");
         if (slot->hash != entry.hash())
         {
            break;
         }
         else
         {
            const lobExtentMetaBlock *block = _getExtentMetaBlock(slot->offset);
            if (OSS_UNLIKELY(nullptr == block))
            {
               PD_LOG(PDERROR, "failed to get block at offset[%d]", slot->offset);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
            else
            {
               INT32 cmp = block->compare(entry.getLogicalClId(),
                                          entry.getKey(),
                                          entry.getChainPos());
               if (cmp < 0)
               {
                  ++slot;
                  continue;
               }
               else if (cmp > 0)
               {
                  break;
               }
               else
               {
                  pos = (INT32)(slot - first);
                  break;
               }
            }
         }
      }

   done:
      return rc;
   error:
      pos = -1;
      goto done;
   }

   INT32 lobcMetaBlockPageAccessor::compareWithHighKey(const lobChunkSearchEntry &entry)const
   {
      INT32 cmp = 0;
      if (0 == _header->totalItemCount)
      {
         cmp = -1;
      }
      else
      {
         lobcMetaBlockPage::itemSlot slot = getSlot(_header->totalItemCount - 1);
         if (slot.hash < entry.hash())
         {
            cmp = -1;
         }
         else if (slot.hash > entry.hash())
         {
            cmp = 1;
         }
         else
         {
            const lobExtentMetaBlock *block = _getExtentMetaBlock(slot.offset);
            cmp = block->compare(entry.getLogicalClId(),
                                 entry.getKey(),
                                 entry.getChainPos());
         }
      }

      return cmp;
   }

   INT32 lobcMetaBlockPageAccessor::testHashBound(UINT32 hash)const
   {
      INT32 res = 0;
      if (OSS_LIKELY(!isEmpty()))
      {
         lobcMetaBlockPage::itemSlot low = getSlot(0);
         lobcMetaBlockPage::itemSlot high = getSlot(_header->totalItemCount - 1);
         if (hash < low.hash)
         {
            res = 1;
         }
         else if (high.hash < hash)
         {
            res = -1;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be empty");
         res = -1;
      }

      return res;
   }

   lobcMetaBlockPage::itemSlot lobcMetaBlockPageAccessor::getSlot(UINT32 pos)const
   {
      lobcMetaBlockPage::itemSlot slot;
      const lobcMetaBlockPage::itemSlot *ptr = getSlotPtr(pos);
      if (OSS_LIKELY(nullptr != ptr))
      {
         slot = *ptr;
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
      }
      return slot;
   }

   const lobExtentMetaBlock *lobcMetaBlockPageAccessor::getExtentMetaBlock(INT32 pos)const
   {
      SDB_ASSERT(0 <= pos && (UINT32)pos < _header->totalItemCount, "out of bound");
      const lobExtentMetaBlock *block = nullptr;
      if (0 <= pos && (UINT32)pos < _header->totalItemCount)
      {
         const lobcMetaBlockPage::itemSlot *slot = getSlotPtr(pos);
         if (nullptr != slot)
         {
            block = _getExtentMetaBlock(slot->offset);
         }
      }

      return block;
   }

   const lobExtentMetaBlock *lobcMetaBlockPageAccessor::_getExtentMetaBlock(UINT32 offset)const
   {
      SDB_ASSERT(lobcMetaBlockPage::HEAD_SIZE < offset, "invalid offset");
      strictBuffer buffer;
      buffer.reset(_pageSize, _header);
      return buffer.getReadableObjPtr<lobExtentMetaBlock>(offset);
   }

   lobExtentMetaBlock *lobcMetaBlockPageAccessor::_getExtentMetaBlock(UINT32 offset)
   {
      SDB_ASSERT(lobcMetaBlockPage::HEAD_SIZE < offset, "invalid offset");
      strictBuffer buffer;
      buffer.makeWritable(_pageSize, _header);
      return buffer.getWritableObjPtr<lobExtentMetaBlock>(offset);
   }

   lobcMetaBlockPage::itemSlot *lobcMetaBlockPageAccessor::getSlotPtr(UINT32 pos)
   {
      if (OSS_UNLIKELY(_header->totalItemCount < pos))
      {
         return nullptr;
      }
      else
      {
         strictBuffer buffer;
         buffer.makeWritable(_pageSize, _header);
         UINT32 offset = lobcMetaBlockPage::HEAD_SIZE +
                         (pos * sizeof(lobcMetaBlockPage::itemSlot));
         return buffer.getWritableObjPtr<lobcMetaBlockPage::itemSlot>(offset);
      }
   }

   const lobcMetaBlockPage::itemSlot *lobcMetaBlockPageAccessor::getSlotPtr(UINT32 pos)const
   {
      if (OSS_UNLIKELY(_header->totalItemCount < pos))
      {
         return nullptr;
      }
      else
      {
         strictBuffer buffer;
         buffer.reset(_pageSize, _header);
         UINT32 offset = lobcMetaBlockPage::HEAD_SIZE +
                         (pos * sizeof(lobcMetaBlockPage::itemSlot));
         return buffer.getReadableObjPtr<lobcMetaBlockPage::itemSlot>(offset);
      }
   }

   UINT32 lobcMetaBlockPageAccessor::getFrontOffset()const
   {
      return lobcMetaBlockPage::HEAD_SIZE +
             (_header->totalItemCount * sizeof(lobcMetaBlockPage::itemSlot));
   }

   BOOLEAN lobcMetaBlockPageAccessor::isFreeToInsert(UINT32 blkCount,
                                                     BOOLEAN *compaction)const
   {
      SDB_ASSERT(0 < blkCount, "can not be zero");

      UINT32 totalSize = blkCount * getItemAndSlotSize();
      BOOLEAN r = totalSize <= _header->totalFreeSize;
      if (r && nullptr != compaction)
      {
         UINT32 frontOffset = getFrontOffset();
         *compaction = (frontOffset + totalSize) > _header->backOffset;
      }
      return r;
   }

   INT32 lobcMetaBlockPageAccessor::insert(const lobExtentMetaBlock *block,
                                           const lobChunkSearchEntry *entry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      lobChunkSearchEntry blockEntry;
      const lobChunkSearchEntry *entryPtr = nullptr;
      BOOLEAN compaction = FALSE;
      UINT32 slotPos = 0;
      strictBuffer buffer;
      lobcMetaBlockPage::itemSlot *slotPtr = nullptr;

      if (OSS_UNLIKELY(nullptr == block || !block->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!isFreeToInsert(1, &compaction))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }
      else if (compaction)
      {
         rc = compact();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact page:%d", rc);
            goto error;
         }
      }

      if (nullptr == entry || !entry->isValid())
      {
         blockEntry = lobChunkSearchEntry(block->oid, block->chunkId,
                                          block->lclid, block->chainPos);
         entryPtr = &blockEntry;
      }
      else
      {
#if defined (_DEBUG)
         SDB_ASSERT(0 == block->compare(entry->getLogicalClId(),
                                        entry->getKey(),
                                        entry->getChainPos()), "must be same");
#endif//_DEBUG
         entryPtr = entry;
      }

      rc = findSlotPosToInsert(*entryPtr, slotPos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find slot to insert:%d", rc);
         goto error;
      }

      SDB_ASSERT(slotPos <= _header->totalItemCount, "impossible");
      
      if (slotPos < _header->totalItemCount)
      {
         UINT32 moveSize = (_header->totalItemCount - slotPos) *
                           sizeof(lobcMetaBlockPage::itemSlot);
         lobcMetaBlockPage::itemSlot *mvPtr = getSlotPtr(slotPos);
         ossMemmove(mvPtr + 1, mvPtr, moveSize);
      }

      /// add slot count first and then we can access slot ptr.
      ++_header->totalItemCount;
      _header->totalFreeSize -= getItemAndSlotSize();
      _header->backOffset -= LOB_EXTENT_META_BLOCK_SIZE;
      buffer.makeWritable(_pageSize, _header);
      buffer.write(_header->backOffset, LOB_EXTENT_META_BLOCK_SIZE, block);
      slotPtr = getSlotPtr(slotPos);
      slotPtr->hash = entryPtr->hash();
      slotPtr->offset = _header->backOffset;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockPageAccessor::pushBack(UINT32 hash, const lobExtentMetaBlock *block)
   {
      INT32 rc = SDB_OK;
      BOOLEAN compaction = FALSE;
      strictBuffer buffer;
      UINT32 pos = 0;

      if (OSS_UNLIKELY(nullptr == block && !block->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isFreeToInsert(1, &compaction))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }
      else if (compaction)
      {
         rc = compact();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact page:%d", rc);
            goto error;
         }
      }


      if (0 < _header->totalItemCount)
      {
         const lobcMetaBlockPage::itemSlot *highSlot =
                                       getSlotPtr(_header->totalItemCount - 1);
         const lobExtentMetaBlock *highBlock = _getExtentMetaBlock(highSlot->offset);
         if (OSS_UNLIKELY(nullptr == highBlock))
         {
            PD_LOG(PDERROR, "failed to get block ptr[%d]", _header->totalItemCount - 1);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (hash < highSlot->hash)
         {
            PD_LOG(PDERROR, "invalid hash code[%d] to push back of [%d]",
                   hash, highSlot->hash);
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
         else if (0 <= highBlock->compare(*block))
         {
            PD_LOG(PDERROR, "invalid block[%s] to push back", block->toString().c_str());
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }

      pos = _header->totalItemCount++;
      _header->totalFreeSize -= getItemAndSlotSize();
      _header->backOffset -= LOB_EXTENT_META_BLOCK_SIZE;
      buffer.makeWritable(_pageSize, _header);
      buffer.write(_header->backOffset, LOB_EXTENT_META_BLOCK_SIZE, block);
      getSlotPtr(pos)->hash = hash;
      getSlotPtr(pos)->offset = _header->backOffset;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockPageAccessor::remove(UINT32 pos, UINT32 count)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_header->totalItemCount < (pos + count))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if ((pos + count) < _header->totalItemCount)
      {
         lobcMetaBlockPage::itemSlot *slotPtr = getSlotPtr(pos);
         UINT32 moveSize = sizeof(lobcMetaBlockPage::itemSlot) *
                           (_header->totalItemCount - (pos + count));
         ossMemmove(slotPtr, slotPtr + count, moveSize);
      }

      _header->totalItemCount -= count;
      _header->totalFreeSize += getItemAndSlotSize() * count;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcMetaBlockPageAccessor::findSlotPosToInsert(const lobChunkSearchEntry &entry,
                                                        UINT32 &slotPos)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(entry.isValid(), "can not be invalid");

      lobcMetaBlockPage::itemSlot target(entry.hash(), 0);
      const lobcMetaBlockPage::itemSlot *first = nullptr;
      const lobcMetaBlockPage::itemSlot *slot = nullptr;

      if (0 == _header->totalItemCount)
      {
         slotPos = 0;
         goto done;
      }

      first = getSlotPtr(0);
      if (OSS_UNLIKELY(nullptr == first))
      {
         PD_LOG(PDERROR, "failed to get first slot ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// we usually insert extents of same lob chunk ordered by chain pos.
      /// if the chain is very long, it is too slow to seek from the head.
      /// so just seek the next hash and go backward.
      slot = std::upper_bound(first, first + _header->totalItemCount, target);

      /// WARNING: slot may be out of bound currently(which points to the end).
      /// do not access it's value.

      while (slot != first)
      {
         const lobcMetaBlockPage::itemSlot *preSlot = slot - 1;

         if (preSlot->hash != entry.hash())
         {
            break;
         }
         else
         {
            INT32 cmp = 0;
            const lobExtentMetaBlock *block = _getExtentMetaBlock(preSlot->offset);
            if (OSS_UNLIKELY(nullptr == block))
            {
               PD_LOG(PDERROR, "failed to get block at offset[%d]", preSlot->offset);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            cmp = block->compare(entry.getLogicalClId(),
                                 entry.getKey(),
                                 entry.getChainPos());
            if (cmp < 0)
            {
               break;
            }
            else if (cmp > 0)
            {
               --slot;
               continue;
            }
            else
            {
               PD_LOG(PDERROR, "duplicated lob key:[%d:%s:%d]",
                        entry.getLogicalClId(),
                        entry.getKey().toString().c_str(), entry.getChainPos());
               rc = SDB_VESSEL_DUPLICATED_KEY;
               goto error;
            }
         }
      }

      slotPos = static_cast<UINT32>(slot - first);

   done:
      return rc;
   error:
      goto done;
   }

   void lobcMetaBlockPageAccessor::initPage()
   {
      strictBuffer buffer;
      buffer.makeWritable(_pageSize, _header);
      buffer.setBuffer(0x0);
      _header->version = lobcMetaBlockPage::HEAD_VERSION;
      _header->totalFreeSize = _pageSize - lobcMetaBlockPage::HEAD_SIZE;
      _header->totalItemCount = 0;
      _header->backOffset = _pageSize;
      _header->prePid = INVALID_PAGE_ID;
      _header->nextPid = INVALID_PAGE_ID;
   }

   INT32 lobcMetaBlockPageAccessor::compact()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");

      CHAR *compactionBuffer = nullptr;
      lobcMetaBlockPageAccessor compacter;
      strictBuffer buffer;
      
      if (0 == _header->totalItemCount)
      {
         _header->backOffset = _pageSize;
         goto done;
      }
      
      compactionBuffer = (CHAR *)SDB_THREAD_ALLOC(_pageSize);
      if (nullptr == compactionBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      compacter.init(_pageSize, compactionBuffer);
      for (UINT32 i = 0; i < _header->totalItemCount; ++i)
      {
         const lobExtentMetaBlock *block = nullptr;
         const lobcMetaBlockPage::itemSlot *slot = getSlotPtr(i);
         if (OSS_UNLIKELY(nullptr == slot))
         {
            PD_LOG(PDERROR, "failed to get slot[%d] ptr", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         block = _getExtentMetaBlock(slot->offset);
         if (OSS_UNLIKELY(nullptr == block))
         {
            PD_LOG(PDERROR, "failed to get block[%d] ptr", slot->offset);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = compacter.pushBack(slot->hash, block);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append block:%d", rc);
            goto error;
         }
      }

      SDB_ASSERT(_header->totalFreeSize == compacter._header->totalFreeSize, "must be same");
      buffer.makeWritable(_pageSize, _header);
      buffer.write(lobcMetaBlockPage::HEAD_SIZE,
                   compacter._header->totalItemCount * sizeof(lobcMetaBlockPage::itemSlot),
                   compactionBuffer + lobcMetaBlockPage::HEAD_SIZE);
      buffer.write(compacter._header->backOffset,
                   _pageSize - compacter._header->backOffset,
                   compactionBuffer + compacter._header->backOffset);
      _header->backOffset = compacter._header->backOffset;
   done:
      if (nullptr != compactionBuffer)
      {
         SDB_THREAD_FREE(compactionBuffer);
      }
      return rc;
   error:
      goto done;
   }

   void lobcMetaBlockPageAccessor::setNextPid(PAGE_ID pid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      _header->nextPid = pid;
   }

   void lobcMetaBlockPageAccessor::setPrePid(PAGE_ID pid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      _header->prePid = pid;
   }

   void lobcMetaBlockPageAccessor::removeTargetOwnedBlocks(const lobcBucketRegion::resizingStrategy &strategy)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(strategy.isValid(), "can not be invalid");
      ossPoolList<lobcMetaBlockPage::itemSlot> slots;
      UINT32 pos = 0;

      for (UINT32 i = 0; i < _header->totalItemCount; ++i)
      {
         const lobcMetaBlockPage::itemSlot *slotPtr = getSlotPtr(i);
         if (!strategy.targetOwned(slotPtr->hash))
         {
            slots.push_back(*slotPtr);
         }
      }

      for (ossPoolList<lobcMetaBlockPage::itemSlot>::const_iterator itr = slots.begin();
           itr != slots.end(); ++itr)
      {
         lobcMetaBlockPage::itemSlot *ptr = getSlotPtr(pos++);
         ptr->hash = itr->hash;
         ptr->offset = itr->offset;
      }

      _header->totalItemCount = slots.size();
      _header->totalFreeSize = _pageSize - lobcMetaBlockPage::HEAD_SIZE -
                               (slots.size() * getItemAndSlotSize());
   }

   BOOLEAN lobcMetaBlockPageAccessor::isTheOnlyPageInBucket()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return !hasPrePid() && !hasNextPid();
   }

   void lobcMetaBlockPageAccessor::truncate(UINT32 newItemCount)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(newItemCount < _header->totalItemCount, "invalid count");

      _header->totalItemCount = newItemCount;
      _header->totalFreeSize = _pageSize - lobcMetaBlockPage::HEAD_SIZE -
                               (newItemCount * getItemAndSlotSize());
   }

   UINT32 lobcMetaBlockPageAccessor::getItemCountToFit(FLOAT32 freePct)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 count = 0;
      if (getFreePct() > freePct)
      {
         count = (_pageSize * (getFreePct() - freePct)) / getItemAndSlotSize();
      }
      return count;
   }

} // namespace vessel

} // namespace engine

