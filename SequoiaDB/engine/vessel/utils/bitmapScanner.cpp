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

   Source File Name = bitmapScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

   void bitmapScanner::extend(UINT32 deltaCapacity, BOOLEAN zeroed)
   {
      SDB_ASSERT(isReady(), "must be ready");
      SDB_ASSERT(ossIsAligned64(deltaCapacity), "must be aligned");
      if (0 < deltaCapacity)
      {
         UINT32 deltaSize = (deltaCapacity >> 6);
         if (zeroed)
         {
            for (UINT32 i = 0; i < deltaSize; ++i)
            {
               UINT64 *bits = _bitmap + _size + i;
               *bits = 0;
            }
         }
         else
         {
            for (UINT32 i = 0; i < deltaSize; ++i)
            {
               UINT64 *bits = _bitmap + _size + i;
               *bits = OSS_UINT64_MAX;
            }
            _nonzeroed += deltaCapacity;
         }

         _size += deltaSize;
      }
      return;
   }

} // namespace vessel

} // namespace engine

