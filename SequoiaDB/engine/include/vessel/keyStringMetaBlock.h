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

   Source File Name = keyStringMetaBlock.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_KEY_STRING_META_BLOCK_H_
#define VESSEL_KEY_STRING_META_BLOCK_H_

#include "ossTypes.h"
#include "vessel/slice.h"
namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct keyStringMetaBlock : public SDBObject
   {
      keyStringMetaBlock() = default;
      ~keyStringMetaBlock() = default;
      keyStringMetaBlock(const keyStringMetaBlock &b) = default;
      keyStringMetaBlock &operator=(const keyStringMetaBlock &b) = default;

      INT32 init(const slice &s);
      void reset();

      OSS_INLINE BOOLEAN isValid() const
      {
         return 0 != version && 0 != blockSize;
      }
      
      UINT8 version = 0;
      UINT8 metaByte = 0;
      UINT8 blockSize = 0;
      UINT8 sizeBeforeKey = 0;
      UINT32 keySize = 0;
      UINT32 sizeAfterKey = 0;
      UINT32 typeBitsSize = 0;
   }; // struct keyStringMetaBlock
#pragma pack()

} // namespace vessel
} // namespace engine
#endif // VESSEL_KEY_STRING_META_BLOCK_H_