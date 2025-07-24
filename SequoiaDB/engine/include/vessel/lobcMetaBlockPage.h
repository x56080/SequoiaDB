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

   Source File Name = lobcMetaBlockPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_META_BLOCK_PAGE_H_
#define VESSEL_LOBC_META_BLOCK_PAGE_H_

#include "core.hpp"
#include "oss.hpp"
#include "vessel/pageIdentifier.h"
#include "vessel/lobChunkKey.h"
#include "vessel/lobExtentMetaBlock.h"
#include "vessel/lobChunkSearchEntry.h"
#include "vessel/lobcBucketRegion.h"
#include "dmsLobDef.hpp"

namespace engine
{
namespace vessel
{
   class lobMetaDataFile;

   class lobcMetaBlockPage
   {
      public:
         static constexpr UINT32 HEAD_VERSION = 1;
         struct pageHead
         {
            UINT32 version;
            UINT32 totalFreeSize;
            UINT32 totalItemCount;
            UINT32 backOffset;
            UINT32 prePid;
            UINT32 nextPid;
            CHAR reserved[16];
         };//struct pageHead

         static constexpr UINT32 HEAD_SIZE = sizeof(pageHead);

         struct itemSlot
         {
            itemSlot(){}
            itemSlot(UINT32 h, UINT32 o):
            hash(h), offset(o){}

            OSS_INLINE BOOLEAN operator<(const itemSlot &o)const
            {
               return hash < o.hash;
            }
            OSS_INLINE BOOLEAN isValid()const
            {
               return HEAD_SIZE < offset;
            }

            UINT32 hash = 0;
            UINT32 offset = 0;
         };//struct itemSlot
   };

   class lobcMetaBlockPageAccessor : public SDBObject
   {
      public:
         lobcMetaBlockPageAccessor(){}
         lobcMetaBlockPageAccessor(const lobcMetaBlockPageAccessor &o) = default;
         lobcMetaBlockPageAccessor(UINT32 pageSize, void *pageBuf);
         lobcMetaBlockPageAccessor(lobMetaDataFile *mfile, PAGE_ID pid);
         ~lobcMetaBlockPageAccessor();
         lobcMetaBlockPageAccessor &operator=(const lobcMetaBlockPageAccessor &o) = default;

      public:
         
         OSS_INLINE BOOLEAN isValid()const {return nullptr != _header;}
         OSS_INLINE UINT32 getItemCount()const
         {
            return _header->totalItemCount;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _header->totalItemCount;
         }
         OSS_INLINE PAGE_ID getNextPid()const
         {
            return _header->nextPid;
         }
         OSS_INLINE BOOLEAN hasNextPid()const
         {
            return INVALID_PAGE_ID != _header->nextPid;
         }
         OSS_INLINE PAGE_ID getPrePid()const
         {
            return _header->prePid;
         }
         OSS_INLINE BOOLEAN hasPrePid()const
         {
            return INVALID_PAGE_ID != _header->prePid;
         }

         FLOAT32 getFreePct()const;

         UINT32 getMaxItemCount()const;

         OSS_INLINE UINT32 getItemAndSlotSize()const
         {
            return sizeof(lobcMetaBlockPage::itemSlot) + LOB_EXTENT_META_BLOCK_SIZE;
         }
      public:
         void reset()
         {
            _pageSize = 0;
            _header = nullptr;
         }
         void init(UINT32 pageSize, void *pageBuf);

         INT32 init(lobMetaDataFile *mfile, PAGE_ID pid);

         BOOLEAN isFreeToInsert(UINT32 blkCount,
                                BOOLEAN *compaction=nullptr)const;

         INT32 insert(const lobExtentMetaBlock *block,
                      const lobChunkSearchEntry *entry=nullptr);

         /// insert block with no ordering, only push it to back of page.
         INT32 pushBack(UINT32 hash, const lobExtentMetaBlock *block);

         INT32 remove(UINT32 pos, UINT32 count);

         /// pos is -1 if not found.
         INT32 seek(const lobChunkSearchEntry &entry,
                    INT32 &pos)const;

         const lobExtentMetaBlock *getExtentMetaBlock(INT32 pos)const;
         lobExtentMetaBlock *getExtentMetaBlock(INT32 pos);


         /// return -1 if page is empty.
         INT32 compareWithHighKey(const lobChunkSearchEntry &entry)const;

         /// page can not be empty
         /// return 1 if low key hash > hash
         /// return -1 if high key hash < hash
         /// otherwise return 0
         INT32 testHashBound(UINT32 hash)const;

         lobcMetaBlockPage::itemSlot getSlot(UINT32 pos)const;

         void initPage();

         void setNextPid(PAGE_ID pid);
         void setPrePid(PAGE_ID pid);

         BOOLEAN isTheOnlyPageInBucket()const;

         void removeTargetOwnedBlocks(const lobcBucketRegion::resizingStrategy &strategy);

         void truncate(UINT32 newItemCount);

         UINT32 getItemCountToFit(FLOAT32 freePct)const;

         INT32 addNewTailToChain(UINT32 oldTailPos,
                                 UINT32 lobdPageSize,
                                 const lobExtentMetaBlock *block);

         INT32 extendBlockSize(UINT32 pos, UINT32 lobdPageSize, UINT32 deltaSize);

      private:

         lobcMetaBlockPage::itemSlot *getSlotPtr(UINT32 pos);
         const lobcMetaBlockPage::itemSlot *getSlotPtr(UINT32 pos)const;
         
         lobExtentMetaBlock *_getExtentMetaBlock(UINT32 offset);
         const lobExtentMetaBlock *_getExtentMetaBlock(UINT32 offset)const;
         UINT32 getFrontOffset()const;

         INT32 findSlotPosToInsert(const lobChunkSearchEntry &entry,
                                   UINT32 &slotPos)const;

         INT32 compact();

         void _insertToPos(UINT32 pos,
                           UINT32 hash,
                           const lobExtentMetaBlock *block);

      private:
         UINT32 _pageSize = 0;
         lobcMetaBlockPage::pageHead *_header = nullptr;
   };
} // namespace vessel
  
} // namespace engine


#endif//VESSEL_LOBC_META_BLOCK_PAGE_H_