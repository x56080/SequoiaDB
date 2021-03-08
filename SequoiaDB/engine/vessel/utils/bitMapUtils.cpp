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

namespace engine
{
namespace vessel
{
   static const UINT32 BM_UTIL_BIT_CNT_32 = 32;
   static const UINT32 BM_UTIL_BIT_MOD_32 = 0x1f;

   void resetBitMap32(UINT32 count, UINT32 *bits, BOOLEAN allFree)
   {
      UINT32 *p = bits;
      for (UINT32 i = 0; i < count; ++i)
      {
         *p++ = allFree ? UINT32(-1) : 0;
      }
      return;
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
            offset = 32 * i + res;
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
      UINT32 slot = offset / BM_UTIL_BIT_CNT_32;
      UINT32 bit = (UINT32)1 << (offset & BM_UTIL_BIT_MOD_32);
      UINT32 *bitsSlot = NULL;
      if (slot <= count)
      {
         bitsSlot = bits + slot;
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
      UINT32 slot = offset / BM_UTIL_BIT_CNT_32;
      UINT32 bit = (UINT32)1 << (offset & BM_UTIL_BIT_MOD_32);
      UINT32 *bitsSlot = NULL;
      if (slot <= count)
      {
         bitsSlot = bits + slot;
         if (OSS_BIT_TEST(*bitsSlot, bit))
         {
            OSS_BIT_CLEAR(*bitsSlot, bit);
            r = TRUE;
         }
      }
   done:
      return r;
   }
}//namespace vessel
}//namespace engine