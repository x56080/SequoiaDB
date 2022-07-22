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

   Source File Name = keyStringMetaByte.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_KEY_STRING_META_BYTE_H_
#define VESSEL_KEY_STRING_META_BYTE_H_

#include "ossTypes.h"
#include "vessel/slice.h"
#include <limits>
namespace engine
{
namespace vessel
{
#pragma pack(1)
   class keyStringMetaByte
   {
      public:
         keyStringMetaByte() = default;
         keyStringMetaByte(UINT32 bytesBeforeKey, UINT32 keySize, UINT32 typeBitsSize):
         _b(0)
         {
            if (0 < bytesBeforeKey)
            {
               setHasDataBeforeKey();
            }
            if (std::numeric_limits<UINT8>::max() < keySize)
            {
               setWideKeySizeWord();
            }
            if (std::numeric_limits<UINT8>::max() < typeBitsSize)
            {
               setWideTypeBitsSizeWord();
            }
         }
         keyStringMetaByte(UINT8 b):
         _b(b){}

      public:
         enum FLAG : UINT8
         {
            HAS_DATA_BEFORE_KEY = 0x01,
            WIDE_KEY_SIZE_WORD = 0x02,
            WIDE_TYPE_BITS_SIZE_WORD = 0x04,
         };

      public:
         void init(UINT8 b) {_b = b;}

      public:
         void setHasDataBeforeKey() {OSS_BIT_SET(_b, HAS_DATA_BEFORE_KEY);}
         void setWideKeySizeWord() {OSS_BIT_SET(_b, WIDE_KEY_SIZE_WORD);}
         void setWideTypeBitsSizeWord() {OSS_BIT_SET(_b, WIDE_TYPE_BITS_SIZE_WORD);}

      public:
         BOOLEAN hasDataBeforeKey() const
         {
            return 0 != OSS_BIT_TEST(_b, HAS_DATA_BEFORE_KEY);
         }
         
         BOOLEAN isWideKeySizeWord() const
         {
            return 0 != OSS_BIT_TEST(_b, WIDE_KEY_SIZE_WORD);
         }

         BOOLEAN isWideTypeBitsSizeWord() const
         {
            return 0 != OSS_BIT_TEST(_b, WIDE_TYPE_BITS_SIZE_WORD);
         }

      public:
         UINT32 getBeforeKeySizeWordWidth()const
         {
            return (0 != OSS_BIT_TEST(_b, HAS_DATA_BEFORE_KEY)) ?
                    sizeof(UINT8) : 0;
         }
         INT32 getBeforeKeySizeWordOffset()const
         {
            return (0 != OSS_BIT_TEST(_b, HAS_DATA_BEFORE_KEY)) ?
                   0 : -1;
         }
         UINT32 getKeySizeWordWidth()const
         {
            return (0 != OSS_BIT_TEST(_b, WIDE_KEY_SIZE_WORD)) ?
                   sizeof(UINT32) : sizeof(UINT8);
         }
         UINT32 getKeySizeWordOffset()const
         {
            return getBeforeKeySizeWordWidth() + getKeySizeWordWidth();
         }
         UINT32 getTypeBitsSizeWordWidth()const
         {
            return (0 != OSS_BIT_TEST(_b, WIDE_TYPE_BITS_SIZE_WORD)) ?
                   sizeof(UINT32) : sizeof(UINT8);
         }
         UINT32 getTypeBitsSizeWordOffset()const
         {
            return getTypeBitsSizeWordWidth() +
                   getKeySizeWordOffset();
         }

         UINT32 getTotalSizeWidth()const
         {
            return getBeforeKeySizeWordWidth() +
                   getKeySizeWordWidth() +
                   getTypeBitsSizeWordWidth();
         }


         UINT8 getByte()const {return _b;}

      private:
         UINT8 _b = 0;
   };//class keyStringMetaByte

#pragma pack()

} // namespace vessel
} // namespace engine
#endif // VESSEL_KEY_STRING_META_BYTE_H_