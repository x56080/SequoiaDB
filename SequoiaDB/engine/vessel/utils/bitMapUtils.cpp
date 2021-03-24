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

   Source File Name = bitMapUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/bitMapUtils.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 BM_UTIL_BITWISE_32 = 5;
   static const UINT32 BM_UTIL_BIT_MOD_32 = 0x1f;

   static const UINT32 BM_UTIL_BITWISE_64 = 6;
   static const UINT32 BM_UTIL_BIT_MOD_64 = 0x3f;

   void resetBitMap32(UINT32 count, UINT32 *bits, BOOLEAN allFree)
   {
      ossMemset(bits, allFree ? 0xFF : 0, count << 2);
      return;
   }

   BOOLEAN findFirstFreeFromBitMap32(UINT32 totalCount,
                                     const UINT32 *bits,
                                     INT32 searchBegin,
                                     UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(searchBegin < totalCount, "searchBegin out of range");
      UINT32 i = 0;
      if (0 < searchBegin)
      {
         i = searchBegin;
      }
      for (; i < totalCount; ++i)
      {
         INT32 res = ossGetLowestBit1From32Bits(bits[i]);
         if (0 <= res)
         {
            offset = (i << BM_UTIL_BITWISE_32) + res;
            r = TRUE;
            goto done;
         }
      }
   done:
      return r;
   }

   BOOLEAN findFirstFreeBitFromBit32(UINT32 bitsCount,
                                     INT32 beginBits,
                                     INT32 maxOffset,
                                     UINT32 *bits,
                                     BOOLEAN clear,
                                     UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != bits, "can not be null");
      UINT32 begin = 0 < beginBits ? beginBits : 0;
      UINT32 endCount = bitsCount;
      if (0 <= maxOffset)
      {
         UINT32 specifiedCount = maxOffset >> BM_UTIL_BITWISE_32;
         if (specifiedCount < endCount)
         {
            endCount = specifiedCount;
         }
      }
      UINT32 i = begin;

      for (; i < endCount; ++i)
      {
         INT32 res = ossGetLowestBit1From32Bits(bits[i]);
         if (0 <= res)
         {
            if (clear)
            {
               OSS_BIT_CLEAR(bits[i], ((UINT32)1 << res));
            }
            offset = (i << BM_UTIL_BITWISE_32) + res;
            r = TRUE;
            goto done;
         }
      }

      if (0 < maxOffset &&
          0 != (maxOffset & BM_UTIL_BIT_MOD_32) && /// maxOffset % 32
          i < bitsCount)
      {
         INT32 res = ossGetLowestBit1From32Bits(bits[i]);
         if (0 <= res)
         {
            UINT32 tmp = (i << BM_UTIL_BITWISE_32) + res;
            if (tmp <= (UINT32)maxOffset)
            {
               if (clear)
               {
                  OSS_BIT_CLEAR(bits[i], ((UINT32)1 << res));
               }
               offset = tmp;
               r = TRUE;
            }
         }
      }
   done:
      return r;
   }

   BOOLEAN findFirstFreeBitsFromBitMap32(UINT32 totalCount,
                                         const UINT32 *bits,
                                         INT32 searchBegin,
                                         INT32 &bitsOffset)
   {
      BOOLEAN r = FALSE;
      bitsOffset = -1;
      UINT32 begin = 0 < searchBegin ? searchBegin : 0;
      UINT32 i = begin;
      for (; i < totalCount; ++i)
      {
         INT32 res = ossGetLowestBit1From32Bits(bits[i]);
         if (0 <= res)
         {
            bitsOffset = i;
            r = TRUE;
            break;
         }
      }
   done:
      return r;
   }

   BOOLEAN allocateFromBitMap32(UINT32 count, UINT32 *bits, UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      for (UINT32 i = 0; i < count; ++i)
      {
         INT32 res = ossGetLowestBit1From32Bits(bits[i]);
         if (0 <= res)
         {
            UINT32 bit = 1;
            bit = bit << res;
            OSS_BIT_CLEAR(bits[i], bit);
            offset = (i << BM_UTIL_BITWISE_32) + res;
            r = TRUE;
            goto done;
         }
      }

   done:
      return r;
   }

   

   BOOLEAN setFreeIfNotFree32(UINT32 count, UINT32 *bits, UINT32 offset)
   {
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_32;
      UINT32 bit = (UINT32)1 << (offset & BM_UTIL_BIT_MOD_32);
      UINT32 *bitsSlot = NULL;
      if (slot < count)
      {
         bitsSlot = &(bits[slot]);
         if (!OSS_BIT_TEST(*bitsSlot, bit))
         {
            OSS_BIT_SET(*bitsSlot, bit);
            r = TRUE;
         }
      }
   done:
      return r;
   }

   BOOLEAN setNotFreeIfFree32(UINT32 count, UINT32 *bits, UINT32 offset)
   {
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_32;
      UINT32 bit = (UINT32)1 << (offset & BM_UTIL_BIT_MOD_32);
      UINT32 *bitsSlot = NULL;
      if (slot < count)
      {
         bitsSlot = &(bits[slot]);
         if (OSS_BIT_TEST(*bitsSlot, bit))
         {
            OSS_BIT_CLEAR(*bitsSlot, bit);
            r = TRUE;
         }
      }
   done:
      return r;
   }

   BOOLEAN upperBoundFirstFreeBitFromBit64(UINT32 bitsCount,
                                           const UINT64 *bits,
                                           INT32 low,
                                           INT32 high,
                                           UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != bits, "can not be null");
      INT32 i = 0;
      INT32 begin = 0;
      INT32 loopEnd = bitsCount;
      UINT64 lowMask = OSS_UINT64_MAX;
      INT32 found = -1;

      if (0 <= low)
      {
         UINT32 value = (low & BM_UTIL_BIT_MOD_64);
         if (value < BM_UTIL_BIT_MOD_64)
         {
            lowMask <<= (value + 1);
         }

         begin = (low + 1) >> BM_UTIL_BITWISE_64;
      }
      
      if (0 <= high)
      {
         UINT32 count = (high >> BM_UTIL_BITWISE_64) + 1;
         if (count < loopEnd)
         {
            loopEnd = count;
         }
      }

      i = begin;
      if (i < loopEnd)
      {
         INT32 res = ossGetLowestBit1From64Bits((bits[i] & lowMask));
         if (0 <= res)
         {
            found = (i << BM_UTIL_BITWISE_64) + res;
            if (found <= high)
            {
               r = TRUE;
               offset = found;
            }
            goto done;
         }
      }

      ++i;
      for (; i < loopEnd; ++i)
      {
         INT32 res = ossGetLowestBit1From64Bits(bits[i]);
         if (0 <= res)
         {
            found = (i << BM_UTIL_BITWISE_64) + res;
            if (found <= high)
            {
               r = TRUE;
               offset = found;
            }
            goto done;
         }
      }
   done:
      return r;
   }

   BOOLEAN findAndClearFirstFreeBitFromBit64(UINT32 bitsCount,
                                             INT32 beginBits,
                                             UINT64 *bits,
                                             UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != bits, "can not be null");
      UINT32 i = 0 < beginBits ? beginBits : 0;
      for (; i < bitsCount; ++i)
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

   BOOLEAN findFirstFreeBitFromBit64(UINT32 bitsCount,
                                     INT32 beginBits,
                                     const UINT64 *bits,
                                     UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != bits, "can not be null");
      UINT32 i = 0 < beginBits ? beginBits : 0;
      for (; i < bitsCount; ++i)
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

   BOOLEAN setFreeIfNotFree64(UINT32 count, UINT64 *bits, UINT32 offset)
   {
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 bit = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);
      UINT64 *bitsSlot = NULL;
      if (slot < count)
      {
         bitsSlot = &(bits[slot]);
         if (!OSS_BIT_TEST(*bitsSlot, bit))
         {
            OSS_BIT_SET(*bitsSlot, bit);
            r = TRUE;
         }
      }
   done:
      return r;
   }

   BOOLEAN setNotFreeIfFree64(UINT32 count, UINT64 *bits, UINT32 offset)
   {
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 bit = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);
      UINT64 *bitsSlot = NULL;
      if (slot < count)
      {
         bitsSlot = &(bits[slot]);
         if (OSS_BIT_TEST(*bitsSlot, bit))
         {
            OSS_BIT_CLEAR(*bitsSlot, bit);
            r = TRUE;
         }
      }
   done:
      return r;
   }

   BOOLEAN testBitIsFree(UINT32 count, const UINT64 *bits, UINT32 offset)
   {
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 bit = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);
      const UINT64 *bitsSlot = NULL;
      if (slot < count)
      {
         bitsSlot = &(bits[slot]);
         r = OSS_BIT_TEST(*bitsSlot, bit);
      }
      return r;
   }

   BOOLEAN setNotFreeWithCAS64(UINT32 count, UINT64 *bits,
                               UINT32 offset, UINT32 maxLoop)
   {
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);
      mask = ~mask;
      volatile UINT64 *bitsSlot = NULL;
      UINT64 expected = 0;
      UINT64 disired = 0;
      UINT32 loop = 0;

      if (slot < count)
      {
         bitsSlot = &(bits[slot]);
         expected = *bitsSlot;
         disired = expected & mask;
         while (!ossCompareAndSwap64(bitsSlot, expected, disired))
         {
            if (maxLoop <= ++loop)
            {
               break;
            }

            expected = *bitsSlot;
            disired = expected & mask;
         }
      }
   done:
      return r;
   }

   BOOLEAN setFreeWithCAS64(UINT32 count, UINT64 *bits,
                            UINT32 offset, UINT32 maxLoop)
   {
      BOOLEAN r = FALSE;
      UINT32 slot = offset >> BM_UTIL_BITWISE_64;
      UINT64 mask = (UINT64)1 << (offset & BM_UTIL_BIT_MOD_64);
      volatile UINT64 *bitsSlot = NULL;
      UINT64 expected = 0;
      UINT64 disired = 0;
      UINT32 loop = 0;

      if (slot < count)
      {
         bitsSlot = &(bits[slot]);
         expected = *bitsSlot;
         disired = expected | mask;
         while (!ossCompareAndSwap64(bitsSlot, expected, disired))
         {
            if (maxLoop <= ++loop)
            {
               break;
            }

            expected = *bitsSlot;
            disired = expected | mask;
         }
      }
   done:
      return r;
   }
}//namespace vessel
}//namespace engine