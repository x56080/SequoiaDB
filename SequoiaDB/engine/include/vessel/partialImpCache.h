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

   Source File Name = partialImpCache.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_PARTIAL_IMP_CACHE_H_
#define VESSEL_PARTIAL_IMP_CACHE_H_

#include "utilGlobalPooledObject.hpp"
#include "vessel/idMapPage.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 ID_MAP_PARTIAL_PAGE_CACHE_SIZE = 256;
   constexpr UINT32 ID_MAP_PARTIAL_CACHE_SLOT_COUNT = ID_MAP_PARTIAL_PAGE_CACHE_SIZE / sizeof(idMapSlot);
   constexpr UINT32 ID_MAP_PARTIAL_CACHE_COUNT_PER_IMP =
                                  ID_MAP_FILE_PAGE_SIZE / ID_MAP_PARTIAL_PAGE_CACHE_SIZE;

   class partialImpCache : public utilGlobalPooledObject
   {
      public:
         partialImpCache();
         ~partialImpCache();
         partialImpCache(const partialImpCache &) = delete;
         partialImpCache &operator=(const partialImpCache &) = delete;

      public:
         static const UINT32 SLOT_COUNT = ID_MAP_PARTIAL_PAGE_CACHE_SIZE/sizeof(idMapSlot);

      public:
         void reset();
         void copy(const void *data, UINT32 flags);

         /// WARNGING: User should always validate if slot is free when return ok.
         INT32 get(UINT32 slotNo, idMapSlot &slot, BOOLEAN &isMutable)const;
         const idMapSlot *get(UINT32 pos, BOOLEAN &isMutable)const;
         INT32 upsert(UINT32 pos,
                      const idMapSlot &slot);
         INT32 remove(UINT32 pos, idMapSlot *beforeDropping=NULL);

         UINT32 getMutablePageCount()const;

         void setAllPageImmutable(UINT32 pageCountPerSeg,
                                  ossPoolSet<UINT32> *mutableSegmentIds,
                                  UINT32 *mutableCount=NULL);
         UINT64 getFlags()const
         {
            return _flags;
         }

         OSS_INLINE const CHAR *getBuffer()const
         {
            return (const CHAR *)_buffer;
         }
      private:
         void setAsInmmutable(UINT32 slotNo);
         void setAsMutable(UINT32 slotNo);
         BOOLEAN isMutable(UINT32 slotNo)const;

      private:
         UINT32 _flags = 0;
         idMapSlot _buffer[ID_MAP_PARTIAL_CACHE_SLOT_COUNT];

   };//class partialImpCache
}//namespace vessel
}//namespace engine

#endif//VESSEL_PARTIAL_IMP_CACHE_H_