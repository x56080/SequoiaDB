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

   Source File Name = partialImpCache.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/partialImpCache.h"

namespace engine
{
namespace vessel
{
   partialImpCache::partialImpCache()
   {

   }

   partialImpCache::~partialImpCache()
   {}

   void partialImpCache::reset()
   {
      _flags = 0;
      ossMemset(_buffer, 0xFF, ID_MAP_PARTIAL_PAGE_CACHE_SIZE);
      return;
   }

   void partialImpCache::copy(const void *data, UINT32 flags)
   {
      SDB_ASSERT(NULL != data, "can not be null");
      _flags = flags;
      ossMemcpy(_buffer, data, ID_MAP_PARTIAL_PAGE_CACHE_SIZE);
      return;
   }

   const idMapSlot *partialImpCache::get(UINT32 pos, BOOLEAN &isMutable)const
   {
      const idMapSlot *slot = NULL;
      if (OSS_LIKELY(pos < ID_MAP_PARTIAL_CACHE_SLOT_COUNT))
      {
         isMutable = this->isMutable(pos);
         slot = _buffer + pos;
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
      }

      return slot;
   }


   INT32 partialImpCache::upsert(UINT32 pos,
                                 const idMapSlot &slot)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(ID_MAP_PARTIAL_CACHE_SLOT_COUNT <= pos ||
                       slot.isFree()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(_buffer[pos].isFree() || !isMutable(pos), "can not update mutable page");
      _buffer[pos] = slot;
      setAsMutable(pos);
      _dirty = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 partialImpCache::remove(UINT32 pos, idMapSlot *beforeDropping)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(ID_MAP_PARTIAL_CACHE_SLOT_COUNT <= pos))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      if (_buffer[pos].isFree())
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      if (NULL != beforeDropping)
      {
         *beforeDropping = _buffer[pos];
      }

      _buffer[pos].reset();
      setAsInmmutable(pos);
      _dirty = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 partialImpCache::getMutablePageCount()const
   {
      return ossGetNonZeroBitCount32(_flags);
   }

   void partialImpCache::makeClean(UINT32 pageCountPerSeg,
                                   ossPoolSet<UINT32> *mutableSegmentIds,
                                   UINT32 *mutableCount)
   {
      if (NULL != mutableCount)
      {
         *mutableCount = ossGetNonZeroBitCount32(_flags);
      }
      if (NULL != mutableSegmentIds)
      {
         do
         {
            INT32 mutableSlot = -1;
            mutableSlot = ossGetLowestBit1From32Bits(_flags);
            if (mutableSlot < 0)
            {
               break;
            }
            
            SDB_ASSERT(!_buffer[mutableSlot].isFree(), "impossible");
            mutableSegmentIds->insert((_buffer[mutableSlot].pid / pageCountPerSeg));
            setAsInmmutable(mutableSlot);
         } while (TRUE);
      }
      else
      {
         _flags = 0;
      }

      _dirty = FALSE;
   done:
      return;
   }

   void partialImpCache::setAsInmmutable(UINT32 slotNo)
   {
      SDB_ASSERT(slotNo < ID_MAP_PARTIAL_CACHE_SLOT_COUNT, "out of bound");
      UINT32 v = ((UINT32)1 << slotNo);
      OSS_BIT_CLEAR(_flags, v);
      return;
   }

   void partialImpCache::setAsMutable(UINT32 slotNo)
   {
      SDB_ASSERT(slotNo < ID_MAP_PARTIAL_CACHE_SLOT_COUNT, "out of bound");
      UINT32 v = ((UINT32)1 << slotNo);
      OSS_BIT_SET(_flags, v);
      return;
   }

   BOOLEAN partialImpCache::isMutable(UINT32 slotNo)const
   {
      SDB_ASSERT(slotNo < ID_MAP_PARTIAL_CACHE_SLOT_COUNT, "out of bound");
      UINT32 v = ((UINT32)1 << slotNo);
      return 0 != OSS_BIT_TEST(_flags, v);
   }

}//namespace vessel
}//namespace engine