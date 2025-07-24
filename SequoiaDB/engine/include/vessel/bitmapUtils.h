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

   Source File Name = bitmapUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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


   void clearFromOffsetToTheEnd(UINT32 bitsCount,
                                UINT32 offset,
                                UINT64 *bits);

   /// clear bits between [begin, end]
   void batchClearBits(UINT32 bitsCount,
                       UINT32 begin,
                       UINT32 end,
                       UINT64 *bits);

   /// set bits between [begin, end]
   void batchSetBits(UINT32 bitsCount,
                     UINT32 begin,
                     UINT32 end,
                     UINT64 *bits);

   BOOLEAN batchTestBitsAllZeroed(UINT32 bitsCount,
                                  UINT32 begin,
                                  UINT32 end,
                                  const UINT64 *bits);

   BOOLEAN batchTestBitsNonZeroed(UINT32 bitsCount,
                                  UINT32 begin,
                                  UINT32 end,
                                  const UINT64 *bits);


   UINT32 getNonzeroBitCount(UINT32 bitsCount,
                             const UINT64 *bits,
                             UINT32 *firstPos=NULL);


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