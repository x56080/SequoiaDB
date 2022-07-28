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

   Source File Name = keyStringMetaBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#include "vessel/keyStringMetaBlock.h"
#include "vessel/keyStringMetaByte.h"

namespace engine
{
namespace vessel
{
   static const UINT32 _META_BLOCK_SIZE_ARRAY[] = 
   {
      sizeof(keyStringMetaBlock<0>),
      sizeof(keyStringMetaBlock<1>),
      sizeof(keyStringMetaBlock<2>),
      sizeof(keyStringMetaBlock<3>),
      sizeof(keyStringMetaBlock<4>),
      sizeof(keyStringMetaBlock<5>),
      sizeof(keyStringMetaBlock<6>),
      sizeof(keyStringMetaBlock<7>),
   };

   UINT32 GET_META_BLOCK_SIZE(UINT8 metabyte)
   {
      return _META_BLOCK_SIZE_ARRAY[
                 keyStringMetaByte::getMetaBlockFormatNum(metabyte)];
   }
} // namespace vessel

} // namespace engine
