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

   Source File Name = collectionIndexContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_INDEX_CONTEXT_H_
#define VESSEL_COLLECTION_INDEX_CONTEXT_H_

#include "vessel/unstableIndexContext.h"

namespace engine
{
namespace vessel
{
   class collectionIndexContext : public SDBObject
   {
      public:
         collectionIndexContext(){}
         ~collectionIndexContext();
         collectionIndexContext(const collectionIndexContext &) = delete;
         collectionIndexContext &operator=(const collectionIndexContext &) = delete;

      public:
         OSS_INLINE UINT32 getNextIndexId()const
         {
            return _nextIndexId;
         }
         OSS_INLINE BOOLEAN hasNoMoreIndexId()const
         {
            return INVALID_LOGICAL_INDEX_ID == _nextIndexId;
         }
         OSS_INLINE BOOLEAN hasUnstableIndexes()const
         {
            return !_unstatbleIndexMap.empty();
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == getIndexSlotBitmap();
         }

      public:
         void fini();

         OSS_INLINE UINT64 getUniqueIndexBitmap()const
         {
            return _uniqueIndexes;
         }
         OSS_INLINE UINT64 getNonuniqueIndexBitmap()const
         {
            return _nonUniqueIdexes;
         }
         OSS_INLINE UINT64 getIndexSlotBitmap()const
         {
            return (_uniqueIndexes | _nonUniqueIdexes);
         }
         void incNextIndexId();
         void setNextIndexId(UINT32 id)
         {
            _nextIndexId = id;
         }
         INT32 findFreeIndexSlot()const;
         void unfreeIndexSlot(INT32 indexSlot, BOOLEAN isUnique);

         UINT32 getUnfreeIndexSlotCount()const;
         UINT32 getNormalIndexCount()const;

      public:
         INT32 unfreeSlotAndSetBuilding(INT32 indexSlot,
                                        const indexObject &obj,
                                        unstableIndexContext **out=NULL);

         BOOLEAN freeSlotAndEraseUnstableIndex(INT32 indexSlot);

         BOOLEAN eraseUnstableIndex(INT32 indexSlot);

         unstableIndexContext *findUnstableIndex(INT32 indexSlot)const;

      private:
         INT32 insertUnstableIndex(INT32 indexSlot,
                                   INDEX_STATUS status,
                                   const indexObject &obj,
                                   unstableIndexContext **out);

      private:
         typedef ossPoolMap<INT32, unstableIndexContext*> _UNSTABLE_INDEX_MAP;
      
      private:
         UINT32 _nextIndexId = 0;
         UINT64 _uniqueIndexes = 0;
         UINT64 _nonUniqueIdexes = 0;

         _UNSTABLE_INDEX_MAP _unstatbleIndexMap;
   };//collectionIndexContext
}//namespace vessel
}//nemespace engine

#endif//VESSEL_COLLECTION_INDEX_CONTEXT_H_
