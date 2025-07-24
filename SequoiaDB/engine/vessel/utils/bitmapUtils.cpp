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

   Source File Name = bitmapUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/bitmapUtils.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

#include <bitset>

namespace engine
{
namespace vessel
{
   constexpr UINT32 BM_UTIL_BITWISE_64 = 6;
   constexpr UINT32 BM_UTIL_BIT_AND_MOD_64 = 63;
   constexpr UINT32 BM_UTIL_BIT_COUNT_PER_WORD = 64;

   typedef std::bitset<64> BITSET_64;

   OSS_INLINE BITSET_64 *GET_BITSET_64(UINT64 *bitmap, UINT32 pos)
   {
      return (BITSET_64 *)(bitmap + pos);
   }
   OSS_INLINE const BITSET_64 *GET_BITSET_64(const UINT64 *bitmap, UINT32 pos)
   {
      return (BITSET_64 *)(bitmap + pos);
   }

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
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_AND_MOD_64);

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
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_AND_MOD_64);
      
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
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_AND_MOD_64);
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
      UINT32 n = (offset & BM_UTIL_BIT_AND_MOD_64);
      mask <<= n;
      UINT64 old = ossFetchAndAND64(bits + slot, ~mask);
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
      UINT32 n = (offset & BM_UTIL_BIT_AND_MOD_64);
      mask <<= n;
      UINT64 old = ossFetchAndOR64(bits + slot, mask);
      if (NULL != zeroBeforeSet)
      {
         *zeroBeforeSet = (0 == OSS_BIT_TEST(old, mask));
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
      UINT32 n = (offset & BM_UTIL_BIT_AND_MOD_64);
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

   void batchClearBits(UINT32 bitsCount,
                       UINT32 begin,
                       UINT32 end,
                       UINT64 *bits)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(begin <= end, "can not be invalid");
      SDB_ASSERT(end < (bitsCount << BM_UTIL_BITWISE_64), "out of bound");
      SDB_ASSERT(nullptr != bits, "can not be null");

      UINT32 pos = begin >> BM_UTIL_BITWISE_64;
      UINT32 offset = begin;
      while (0 != (offset & BM_UTIL_BIT_AND_MOD_64) &&
             offset <= end)
      {
         BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         bitset->reset(offset & BM_UTIL_BIT_AND_MOD_64);
         ++offset;
      }

      while ((offset + BM_UTIL_BIT_COUNT_PER_WORD - 1) <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         bits[pos] = 0;
         offset += BM_UTIL_BIT_COUNT_PER_WORD;
      }

      if (offset <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         do
         {
            bitset->reset(offset & BM_UTIL_BIT_AND_MOD_64);
            ++offset;
         } while (offset <= end);
      }

      return;
   }

   void batchSetBits(UINT32 bitsCount,
                     UINT32 begin,
                     UINT32 end,
                     UINT64 *bits)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(begin <= end, "can not be invalid");
      SDB_ASSERT(end < (bitsCount << BM_UTIL_BITWISE_64), "out of bound");
      SDB_ASSERT(nullptr != bits, "can not be null");

      UINT32 pos = begin >> BM_UTIL_BITWISE_64;
      UINT32 offset = begin;
      while (0 != (offset & BM_UTIL_BIT_AND_MOD_64) &&
             offset <= end)
      {
         BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         bitset->set(offset & BM_UTIL_BIT_AND_MOD_64);
         ++offset;
      }

      while ((offset + BM_UTIL_BIT_COUNT_PER_WORD - 1) <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         bits[pos] = OSS_UINT64_MAX;
         offset += BM_UTIL_BIT_COUNT_PER_WORD;
      }

      if (offset <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         do
         {
            bitset->set(offset & BM_UTIL_BIT_AND_MOD_64);
            ++offset;
         } while (offset < end);
      }

      return;
   }

   BOOLEAN batchTestBitsAllZeroed(UINT32 bitsCount,
                                  UINT32 begin,
                                  UINT32 end,
                                  const UINT64 *bits)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(begin <= end, "can not be invalid");
      SDB_ASSERT(end < (bitsCount << BM_UTIL_BITWISE_64), "out of bound");
      SDB_ASSERT(nullptr != bits, "can not be null");

      BOOLEAN r = FALSE;
      UINT32 pos = begin >> BM_UTIL_BITWISE_64;
      UINT32 offset = begin;
      while (0 != (offset & BM_UTIL_BIT_AND_MOD_64) &&
             offset <= end)
      {
         const BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         if (bitset->test(offset & BM_UTIL_BIT_AND_MOD_64))
         {
            goto done;
         }
         ++offset;
      }

      while ((offset + BM_UTIL_BIT_COUNT_PER_WORD - 1) <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         if (0 != bits[pos])
         {
            goto done;
         }
         offset += BM_UTIL_BIT_COUNT_PER_WORD;
      }

      if (offset <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         const BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         do
         {
            if (bitset->test(offset & BM_UTIL_BIT_AND_MOD_64))
            {
               goto done;
            }
            ++offset;
         } while (offset <= end);
      }

      r = TRUE;

   done:
      return r;
   }

   BOOLEAN batchTestBitsNonZeroed(UINT32 bitsCount,
                                  UINT32 begin,
                                  UINT32 end,
                                  const UINT64 *bits)
   {
      {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(begin <= end, "can not be invalid");
      SDB_ASSERT(end < (bitsCount << BM_UTIL_BITWISE_64), "out of bound");
      SDB_ASSERT(nullptr != bits, "can not be null");

      BOOLEAN r = FALSE;
      UINT32 pos = begin >> BM_UTIL_BITWISE_64;
      UINT32 offset = begin;
      while (0 != (offset & BM_UTIL_BIT_AND_MOD_64) &&
             offset <= end)
      {
         const BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         if (!bitset->test(offset & BM_UTIL_BIT_AND_MOD_64))
         {
            goto done;
         }
         ++offset;
      }

      while ((offset + BM_UTIL_BIT_COUNT_PER_WORD - 1) <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         if (OSS_UINT64_MAX != bits[pos])
         {
            goto done;
         }
         offset += BM_UTIL_BIT_COUNT_PER_WORD;
      }

      if (offset <= end)
      {
         pos = offset >> BM_UTIL_BITWISE_64;
         const BITSET_64 *bitset = GET_BITSET_64(bits, pos);
         do
         {
            if (!bitset->test(offset & BM_UTIL_BIT_AND_MOD_64))
            {
               goto done;
            }
            ++offset;
         } while (offset <= end);
      }

      r = TRUE;

   done:
      return r;
   }
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
      UINT32 n = (bitOffset & BM_UTIL_BIT_AND_MOD_64);

      if (0 < n)
      {
         UINT64 firstBits = 0;
         INT32 freeInFirstBits = -1;
         UINT64 mask = OSS_UINT64_MAX;
         mask <<= n;
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
                             const UINT64 *bits,
                             UINT32 *firstPos)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(NULL != bits, "can not be null");

      INT64 firstNonzero = -1;
      UINT32 totalCount = 0;
      for (UINT32 i = 0; i < bitsCount; ++i)
      {
         const UINT64 &n = bits[i];
         UINT32 cnt = ossGetNonZeroBitCount64(n);
         if (0 != cnt && firstNonzero < 0)
         {
            firstNonzero = i;
         }
         totalCount += cnt;
      }
      
      if (NULL != firstPos && 0 <= firstNonzero)
      {
         *firstPos = (UINT32)firstNonzero;
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