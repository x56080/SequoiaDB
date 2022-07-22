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

   Source File Name = keyStringDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_KEY_STRING_DEF_H_
#define VESSEL_KEY_STRING_DEF_H_

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   enum KEY_STRING_VERSION : UINT8
   {
      KEY_STRING_VERSION_INVALID = 0x0,
      KEY_STRING_VERSION_1 = 0x01,
   };

   enum class EncodedType : UINT8
   {
      minKey = 10,
      undefined = 15,
      nullish = 20,
      numeric = 30,
      numericNaN = numeric + 0,
      numericNegativeLargeMagnitude = numeric + 1,
      numericNegative8ByteInt = numeric + 2,
      numericNegative7ByteInt = numeric + 3,
      numericNegative6ByteInt = numeric + 4,
      numericNegative5ByteInt = numeric + 5,
      numericNegative4ByteInt = numeric + 6,
      numericNegative3ByteInt = numeric + 7,
      numericNegative2ByteInt = numeric + 8,
      numericNegative1ByteInt = numeric + 9,
      numericNegativeSmallMagnitude = numeric + 10,
      numericZero = numeric + 11,
      numericPositiveSmallMagnitude = numeric + 12,
      numericPositive1ByteInt = numeric + 13,
      numericPositive2ByteInt = numeric + 14,
      numericPositive3ByteInt = numeric + 15,
      numericPositive4ByteInt = numeric + 16,
      numericPositive5ByteInt = numeric + 17,
      numericPositive6ByteInt = numeric + 18,
      numericPositive7ByteInt = numeric + 19,
      numericPositive8ByteInt = numeric + 20,
      numericPositiveLargeMagnitude = numeric + 21,
      stringLike = 60,
      object = 70,
      array = 80,
      binData = 90,
      oid = 100,
      boolean = 110,
      booleanFalse = boolean + 0,
      booleanTrue = boolean + 1,
      date = 120,
      timestamp = 130,
      regEx = 140,
      dbRef = 150,
      code = 160,
      codeWithScope = 170,
      maxKey = 240
   };

   static_assert(EncodedType::numericPositiveLargeMagnitude <
                     EncodedType::stringLike,
                 "NumericPositiveLargeMagnitude must be less than StringLike");

   enum class DecimalContinuationMarker : UINT8
   {
      hasNoContinuation = 0b0,
      hasContinuation = 0b1,
   };

   enum class typeBitsType : UINT8
   {
      STRING = 0b0,
      SYMBOL = 0b1,

      INT = 0b00,
      LONG = 0b01,
      DOUBLE = 0b10,
      DECIMAL = 0b11,
      POSITIVE_ZERO = 0b0,
      NEGATIVE_ZERO = 0b1
   };
   constexpr UINT32 KEY_STRING_MIN_META_BLOCK_SIZE = 4;

   enum class Discriminator : UINT8
   {
      INCLUSIVE,
      EXCLUSIVE_BEFORE,
      EXCLUSIVE_AFTER
   };

   enum class DiscriminatorValue : UINT8
   {
      LESS = 1,
      GREATER = 254,
      END = 4
   };
} // namespace vessel

} // namespace engine

#endif // VESSEL_KEY_STRING_DEF_H_
