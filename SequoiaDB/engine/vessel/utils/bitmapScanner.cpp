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

   Source File Name = bitmapScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/bitmapScanner.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/bitmapUtils.h"

namespace engine
{
namespace vessel
{
   void bitmapScanner::init(UINT64 *bitmap,
                            UINT32 capacity,
                            BOOLEAN zeroed)
   {
      SDB_ASSERT(NULL != bitmap, "can not be null");
      SDB_ASSERT(0 < capacity, "can not be zero");
      SDB_ASSERT(ossIsAligned64(capacity), "must be 64 aligned");
      SDB_ASSERT(capacity <= OSS_SINT32_MAX, "out of range");

      _bitmap = bitmap;
      _size = capacity >> 6;
      _nonzeroed = zeroed ? 0 : capacity;
      _pos = 0;
      ossMemset(_bitmap, zeroed ? 0x00 : 0xFF, (_size << 3));
      return;
   }

   void bitmapScanner::load(UINT64 *bitmap,
                           UINT32 capacity)
   {
      SDB_ASSERT(NULL != bitmap, "can not be null");
      SDB_ASSERT(0 < capacity, "can not be zero");
      SDB_ASSERT(ossIsAligned64(capacity), "must be 64 aligned");
      SDB_ASSERT(capacity <= OSS_SINT32_MAX, "out of range");

      _bitmap = bitmap;
      _size = capacity >> 6;
      _pos = 0;
      _nonzeroed = getNonzeroBitCount(_size, _bitmap, &_pos);
      return;
   }

   void bitmapScanner::reset()
   {
      _bitmap = NULL;
      _size = 0;
      _nonzeroed = 0;
      _pos = 0;
      return;
   }

   BOOLEAN bitmapScanner::moveToNextUnzeroPos(INT32 &offset)
   {
      SDB_ASSERT(NULL != _bitmap, "can not be null");

      UINT32 bitOffset = 0;
      offset = -1;
      if (0 == _nonzeroed)
      {
         goto done;
      }

      if (findFirstNonzeroBit(_size, _pos, _bitmap, bitOffset))
      {
         offset = (INT32)bitOffset;
         _pos = bitOffset >> 6;
         goto done;
      }
      else if (0 < _pos)
      {
         if (findFirstNonzeroBit(_pos, 0, _bitmap, bitOffset))
         {
            offset = (INT32)bitOffset;
            _pos = bitOffset >> 6;
            goto done;
         }
      }

      SDB_ASSERT(FALSE, "counter is not zero but not found in bitmap");
   done:
      return 0 <= offset;
   }

   BOOLEAN bitmapScanner::findAndClearNext(INT32 &offset)
   {
      SDB_ASSERT(NULL != _bitmap, "can not be null");
      offset = -1;
      UINT32 bitOffset = 0;

      if (0 == _nonzeroed)
      {
         goto done;
      }

      SDB_ASSERT(_pos < _size, "impossible");
      if (findAndClearFirstNonzeroBit(_size, _pos, _bitmap, bitOffset))
      {
         offset = (INT32)bitOffset;
         _pos = bitOffset >> 6;
         --_nonzeroed;
         goto done;
      }
      else if (0 < _pos)
      {
         if (findAndClearFirstNonzeroBit(_pos, 0, _bitmap, bitOffset))
         {
            offset = (INT32)bitOffset;
            _pos = bitOffset >> 6;
            --_nonzeroed;
            goto done;
         }
      }
      
      SDB_ASSERT(FALSE, "counter is not zero but not found in bitmap");

   done:
      return 0 <= offset;
   }

   void bitmapScanner::setBit(UINT32 offset, BOOLEAN resetPosToBackward)
   {
      UINT32 pos = offset >> 6;

      if (getCapacity() <= offset)
      {
         SDB_ASSERT(FALSE, "out of bound");
         goto done;
      }

      if (!setBitIfZeroed(_size, _bitmap, offset))
      {
         goto done;
      }

      ++_nonzeroed;
      if (1 == _nonzeroed)
      {
         _pos = pos;
      }
      else if (resetPosToBackward &&
               pos < _pos)
      {
         _pos = pos;
      }

   done:
      return;
   }

   BOOLEAN bitmapScanner::testBit(UINT32 offset)const
   {
      SDB_ASSERT(offset < getCapacity(), "out of bound");
      return testBitIsNonzero(_size, _bitmap, offset);
   }

   BOOLEAN bitmapScanner::clearBit(UINT32 offset)
   {
      SDB_ASSERT(offset < getCapacity(), "out of bound");
      BOOLEAN r = FALSE;
      if (clearBitIfNonzero(_size, _bitmap, offset))
      {
         --_nonzeroed;
         r = TRUE;
      }

      return r;
   }

} // namespace vessel

} // namespace engine

