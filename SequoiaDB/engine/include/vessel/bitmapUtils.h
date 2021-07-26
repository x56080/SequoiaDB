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

   Source File Name = bitmapUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BITMAP_UTILS_H_
#define VESSEL_BITMAP_UTILS_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   void resetBitmap(UINT32 count, UINT64 *bits, BOOLEAN zeroed);

   /// searchBegin is offset of uint64, not offset of bit
   BOOLEAN findFirstNonzeroBit(UINT32 bitsCount,
                               UINT32 searchBegin,
                               const UINT64 *bits,
                               UINT32 &offset);


   /// searchBegin is offset of uint64, not offset of bit
   BOOLEAN findAndClearFirstNonzeroBit(UINT32 bitsCount,
                                       UINT32 searchBegin,
                                       UINT64 *bits,
                                       UINT32 &offset);


   /// find the first bit from [bitOffset, end]
   BOOLEAN lowerBoundFirstNonzeroBit(UINT32 bitsCount,
                                     UINT32 bitOffset,
                                     const UINT64 *bits,
                                     UINT32 &offset);

   /// return false if bit is zero
   BOOLEAN clearBitIfNonzero(UINT32 count,
                            UINT64 *bits,
                            UINT32 offset);

   /// return false if bit is nonzero
   BOOLEAN setBitIfZeroed(UINT32 count, UINT64 *bits, UINT32 offset);

   BOOLEAN testBitIsNonzero(UINT32 count,
                            const UINT64 *bits,
                            UINT32 offset);

   void bitsAndMerge(UINT32 count, const UINT64 *toAnd, UINT64 *bits);

   void clearFromOffsetToTheEnd(UINT32 bitsCount,
                                UINT32 offset,
                                UINT64 *bits);

   UINT32 getNonzeroBitCount(UINT32 bitsCount,
                             const UINT64 *bits);


   /// set bit at offet
   void atomicSetBitAtOffset(UINT32 count,
                             UINT64 *bits,
                             UINT32 offset,
                             BOOLEAN *zeroBeforeSet=NULL);

   /// clear bit at offset
   void atomicClearBitAtOffset(UINT32 count,
                               UINT64 *bits,
                               UINT32 offset,
                               BOOLEAN *nonzeroBeforeClear=NULL);

   /// searchBegin is offset of uint64, not offset of bit
   /// WARNING: To speed up, we will find nonzero bit in 
   /// one uint64 first. And then, try to clear it with atomic op.
   /// In the moment, new nonzeroed bit may be ignored.
   BOOLEAN atomicFindAndClearFirstNonzeroBit(UINT32 bitsCount,
                                             UINT32 searchBegin,
                                             UINT64 *bits,
                                             UINT32 &offset);

}//namespace vessel
}//namespace engine

#endif//VESSEL_BITMAP_UTILS_H_