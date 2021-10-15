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

   Source File Name = btreeNodeCompressor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_COMPRESSOR_H_
#define VESSEL_BTREE_NODE_COMPRESSOR_H_

#include "ixmKey.hpp"
#include "vessel/btreeNodeCompressedKey.h"

namespace engine
{
namespace vessel
{
   class btreeNodeCompressor : public SDBObject
   {
      public:
         btreeNodeCompressor(){}
         ~btreeNodeCompressor(){}
         btreeNodeCompressor(const btreeNodeCompressor &) = delete;
         btreeNodeCompressor &operator=(const btreeNodeCompressor &) = delete;

      public:
         ///WARNING: do not release compressor before get compressedKey suffix owned.
         BOOLEAN compress(UINT32 prefixPos,
                          const ixmKey &prefix,
                          const ixmKey &key,
                          btreeNodeCompressedKey &compressedKey);

      private:
         ixmKeyCompressor _ikc;
   };//class btreePrefixCompression
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_COMPRESSOR_H_
