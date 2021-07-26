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

   Source File Name = bitmapUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/bitmapUtils.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 BM_UTIL_BITWISE_64 = 6;
   static const UINT32 BM_UTIL_BIT_MOD_64 = 0x3f;

   BOOLEAN findAndClearFirstNonzeroBit(UINT32 bitsCount,
                                       UINT32 searchBegin,
                                       UINT64 *bits,
                                       UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(searchBegin < bitsCount, "impossible");
      for (UINT32 i = searchBegin; i < bitsCount; ++i)
      {
         INT32 res = ossGetLowestBit1From64Bits(bits[i]);
         if (0 <= res)
         {
            offset = (i << BM_UTIL_BITWISE_64) + res;
            r = TRUE;
            UINT64 value = 1;
            value <<= res;
            OSS_BIT_CLEAR(bits[i], value);
            goto done;
         }
      }
   done:
      return r;
   }

   void resetBitmap(UINT32 count, UINT64 *bits, BOOLEAN zeroed)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");
      ossMemset(bits, zeroed ? 0 : 0xFF, count << 3);
      return;
   }

   BOOLEAN findFirstNonzeroBit(UINT32 bitsCount,
                               UINT32 searchBegin,
                               const UINT64 *bits,
                               UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(searchBegin < bitsCount, "impossible");
      for (UINT32 i = searchBegin; i < bitsCount; ++i)
      {
         INT32 res = ossGetLowestBit1From64Bits(bits[i]);
         if (0 <= res)
         {
            offset = (i << BM_UTIL_BITWISE_64) + res;
            r = TRUE;
            goto done;
         }
      }
   done:
      return r;
   }

   BOOLEAN setBitIfZeroed(UINT32 count, UINT64 *bits, UINT32 offset)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(offset < (count << BM_UTIL_BITWISE_64), "can not be out of bound");
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);

      if (0 == OSS_BIT_TEST(bits[slot], mask))
      {
         OSS_BIT_SET(bits[slot], mask);
         r = TRUE;
      }
   done:
      return r;
   }

   BOOLEAN clearBitIfNonzero(UINT32 count, UINT64 *bits, UINT32 offset)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(offset < (count << BM_UTIL_BITWISE_64), "can not be out of bound");

      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);
      
      if (0 != OSS_BIT_TEST(bits[slot], mask))
      {
         OSS_BIT_CLEAR(bits[slot], mask);
         r = TRUE;
      }
   done:
      return r;
   }

   BOOLEAN testBitIsNonzero(UINT32 count,
                            const UINT64 *bits,
                            UINT32 offset)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(offset < (count << BM_UTIL_BITWISE_64), "can not be out of bound");
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);
      return 0 != OSS_BIT_TEST(bits[slot], mask);
   }

   void atomicClearBitAtOffset(UINT32 count,
                               UINT64 *bits,
                               UINT32 offset,
                               BOOLEAN *nonzeroBeforeClear)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(offset < (count << BM_UTIL_BITWISE_64), "can not be out of bound");
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 mask = 1;
      UINT32 n = (offset & BM_UTIL_BIT_MOD_64);
      mask <<= n;
      UINT64 old = ossFetchAndAND64(bits + offset, ~mask);
      if (NULL != nonzeroBeforeClear)
      {
         *nonzeroBeforeClear = (0 != OSS_BIT_TEST(old, mask));
      }
      return;
   }

   void atomicSetBitAtOffset(UINT32 count,
                             UINT64 *bits,
                             UINT32 offset,
                             BOOLEAN *zeroBeforeSet)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(offset < (count << BM_UTIL_BITWISE_64), "can not be out of bound");
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 mask = 1;
      UINT32 n = (offset & BM_UTIL_BIT_MOD_64);
      mask <<= n;
      UINT64 old = ossFetchAndOR64(bits + slot, mask);
      if (NULL != zeroBeforeSet)
      {
         *zeroBeforeSet = (0 == OSS_BIT_TEST(old, mask));
      }
      return;
   }

   void bitsAndMerge(UINT32 count, const UINT64 *toAnd, UINT64 *bits)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != toAnd, "can not be null");
      SDB_ASSERT(NULL != bits, "can not be null");

      for (UINT32 i = 0; i < count; ++i)
      {
         bits[i] = (bits[i] & toAnd[i]);
      }
      return;
   }

   void clearFromOffsetToTheEnd(UINT32 bitsCount,
                                UINT32 offset,
                                UINT64 *bits)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(offset < (bitsCount << 6), "can not out of bound");
      SDB_ASSERT(NULL != bits, "can not be null");

      UINT32 begin = offset >> BM_UTIL_BITWISE_64;
      UINT32 n = (offset & BM_UTIL_BIT_MOD_64);
      if (0 != n)
      {
         UINT32 totalCount = 64 - n;
         for (UINT32 i = 0; i < totalCount; ++i)
         {
            clearBitIfNonzero(bitsCount, bits, offset + i);
         }
         ++begin;
      }

      if (begin < bitsCount)
      {
         resetBitmap(bitsCount - begin, bits + begin, TRUE);
      }
      return;
   }

   BOOLEAN lowerBoundFirstNonzeroBit(UINT32 bitsCount,
                                    UINT32 bitOffset,
                                    const UINT64 *bits,
                                    UINT32 &offset)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(bitOffset < (bitsCount << 6), "can not out of bound");
      SDB_ASSERT(NULL != bits, "can not be null");
      BOOLEAN r = FALSE;
      UINT32 delta = 0;
      UINT32 begin = bitOffset >> BM_UTIL_BITWISE_64;
      UINT32 n = (bitOffset & BM_UTIL_BIT_MOD_64);

      if (0 < n)
      {
         UINT64 firstBits = 0;
         INT32 freeInFirstBits = -1;
         UINT64 mask = 1;

         for (UINT32 i = 1; i < n; ++i)
         {
            mask <<= 1;
            mask |= 1;
         }

         mask = (~mask);
         firstBits = *(bits + begin);
         firstBits = (firstBits & mask);
         freeInFirstBits = ossGetLowestBit1From64Bits(firstBits);
         if (0 <= freeInFirstBits)
         {
            r = TRUE;
            offset = bitOffset + freeInFirstBits;
            goto done;
         }

         ++begin;
         delta = 64;
      }

      if (begin < bitsCount)
      {
         if (findFirstNonzeroBit(bitsCount - begin, 0, bits + begin, offset))
         {
            r = TRUE;
            offset += delta;
         }
      }

   done:
      return r;
   }

   UINT32 getNonzeroBitCount(UINT32 bitsCount,
                             const UINT64 *bits)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");
      UINT32 totalCount = 0;
      for (UINT32 i = 0; i < bitsCount; ++i)
      {
         const UINT64 &n = bits[i];
         UINT32 cnt = ossGetNonZeroBitCount64(n);
         totalCount += cnt;
      }
      return totalCount;
   }

   BOOLEAN atomicFindAndClearFirstNonzeroBit(UINT32 bitsCount,
                                             UINT32 searchBegin,
                                             UINT64 *bits,
                                             UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != bits, "can not be null");
      SDB_ASSERT(searchBegin < bitsCount, "impossible");
      for (UINT32 i = searchBegin; i < bitsCount; ++i)
      {
         for (UINT32 loop = 0; loop < 64; ++loop)
         {
            INT32 pos = ossGetLowestBit1From64Bits(bits[i]);
            if (pos < 0)
            {
               break;
            }

            UINT64 mask = (UINT64)1 << pos;
            UINT64 newMask = ossFetchAndAND64(bits + i, ~mask);
            if (0 != OSS_BIT_TEST(mask, newMask))
            {
               r = TRUE;
               offset = (i << BM_UTIL_BITWISE_64) + pos;
               goto done;
            }
         }
      }
   done:
      return r;
   }
}//namespace vessel
}//namespace engine