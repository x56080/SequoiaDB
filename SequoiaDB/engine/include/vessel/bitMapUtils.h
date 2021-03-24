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

   Source File Name = bitMapUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BIT_MAP_UTILS_H_
#define VESSEL_BIT_MAP_UTILS_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   void resetBitMap32(UINT32 count, UINT32 *bits, BOOLEAN allFree);

   BOOLEAN findFirstFreeFromBitMap32(UINT32 totalCount,
                                     const UINT32 *bits,
                                     INT32 searchBegin,
                                     UINT32 &offset);

   BOOLEAN findFirstFreeBitFromBit32(UINT32 bitsCount,
                                     INT32 beginBits,
                                     INT32 maxOffset,
                                     UINT32 *bits,
                                     BOOLEAN clear,
                                     UINT32 &offset);

   BOOLEAN findFirstFreeBitsFromBitMap32(UINT32 totalCount,
                                         const UINT32 *bits,
                                         INT32 searchBegin,
                                         INT32 &bitsOffset);

   BOOLEAN allocateFromBitMap32(UINT32 count, UINT32 *bits, UINT32 &offset);

   BOOLEAN setFreeIfNotFree32(UINT32 count, UINT32 *bits, UINT32 offset);

   BOOLEAN setNotFreeIfFree32(UINT32 count, UINT32 *bits, UINT32 offset);


   BOOLEAN findFirstFreeBitFromBit64(UINT32 bitsCount,
                                     INT32 beginBits,
                                     const UINT64 *bits,
                                     UINT32 &offset);

   BOOLEAN findAndClearFirstFreeBitFromBit64(UINT32 bitsCount,
                                             INT32 beginBits,
                                             UINT64 *bits,
                                             UINT32 &offset);

   /// offset between (low, high]
   BOOLEAN upperBoundFirstFreeBitFromBit64(UINT32 bitsCount,
                                           const UINT64 *bits,
                                           INT32 low,
                                           INT32 high,
                                           UINT32 &offset);

   BOOLEAN setNotFreeIfFree64(UINT32 count, UINT64 *bits, UINT32 offset);
   BOOLEAN setNotFreeWithCAS64(UINT32 count, UINT64 *bits,
                               UINT32 offset, UINT32 maxLoop);
   BOOLEAN setFreeWithCAS64(UINT32 count, UINT64 *bits,
                            UINT32 offset, UINT32 maxLoop);
   BOOLEAN setFreeIfNotFree64(UINT32 count, UINT64 *bits, UINT32 offset);
   BOOLEAN testBitIsFree(UINT32 count, const UINT64 *bits,
                         UINT32 offset);

}//namespace vessel
}//namespace engine

#endif//VESSEL_BIT_MAP_UTILS_H_