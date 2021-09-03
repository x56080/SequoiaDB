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

   Source File Name = indexKeySuffix.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_KEY_SUFFIX_H_
#define VESSEL_INDEX_KEY_SUFFIX_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"

namespace engine
{
namespace vessel
{
   class indexKeySuffix : public SDBObject
   {
      public:
         indexKeySuffix(){}
         ~indexKeySuffix() = delete;

      public:
         /// suffix flag 8bits:
         /// bit 0 - 3: type
         /// bit 4 - 5: unused
         /// bit 6: has more suffix key
         /// bit 7: has more key
         enum TYPE
         {
            TYPE_INVALID = 0,
            TYPE_0B = 1,
            TYPE_1B = 2,
            TYPE_2B = 3,
            TYPE_4B = 4,
            TYPE_BYTES = 5, /// with 1 byte size
            TYPE_LONG_BYTES = 6, /// with 2bytes size
         };//enum type

         static const UINT8 FLAG_MORE_SUFFIX_KEY = 0x40;
         static const UINT8 FALG_MORE_KEY = 0x80;

         static OSS_INLINE BOOLEAN hasMoreSuffixKey(UINT8 flag)
         {
            return 0 != OSS_BIT_TEST(flag, FLAG_MORE_SUFFIX_KEY);
         }
         static OSS_INLINE BOOLEAN hasMoreKey(UINT8 flag)
         {
            return 0 != OSS_BIT_TEST(flag, FALG_MORE_KEY);
         }
         static OSS_INLINE TYPE getSuffixType(UINT8 flag)
         {
            return (TYPE)(flag & 0xF);
         }

   };//class indexKeySuffix
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_KEY_SUFFIX_H_