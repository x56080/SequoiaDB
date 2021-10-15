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

   Source File Name = btreeNodeCompressor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNodeCompressor.h"
#include "vessel/btreeNodePage.h"

namespace engine
{
namespace vessel
{
   BOOLEAN btreeNodeCompressor::compress(UINT32 prefixPos,
                                         const ixmKey &prefix,
                                         const ixmKey &key,
                                         btreeNodeCompressedKey &compressedKey)
   {
      SDB_ASSERT(prefix.isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");

      BOOLEAN r = FALSE;
      compressedKey.reset();

      _ikc.reset(prefix.data());
      if (_ikc.compress(key))
      {
         SDB_ASSERT(_ikc.isDone(), "impossible");
         ixmKey suffix;
         if (!_ikc.isPerfectlyCompressed())
         {
            _ikc.getSuffix(suffix);
         }

         compressedKey.shallowInit(prefixPos, prefix, suffix);
         r = TRUE;
      }
   done:
      return r;
   }
} // namespace vessel

} // namespace engine

