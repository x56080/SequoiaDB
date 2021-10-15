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

   Source File Name = btreeNodeCompressedKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_COMPRESSED_KEY_H
#define VESSEL_BTREE_NODE_COMPRESSED_KEY_H

#include "ixmKey.hpp"
#include "../bson/util/builder.h"

namespace engine
{
namespace vessel
{
   class btreeNodeCompressedKey : public SDBObject
   {
      public:
         btreeNodeCompressedKey(){}
         ~btreeNodeCompressedKey();
         btreeNodeCompressedKey(const btreeNodeCompressedKey &) = delete;
         btreeNodeCompressedKey &operator=(const btreeNodeCompressedKey &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return 0 <= _prefixPos;
         }
         OSS_INLINE INT32 getPrefixSlotPos()const
         {
            return _prefixPos;
         }
         OSS_INLINE const ixmKey &getPrefix()const
         {
            return _prefix;
         }
         OSS_INLINE const ixmKey &getSuffix()const
         {
            return _suffix;
         }
         
         void shallowInit(UINT32 prefixPos,
                          const ixmKey &prefix,
                          const ixmKey &suffix);
         void reset();

      public:/// must be valid first
         BOOLEAN isSuffixOwned()const;
         INT32 getSuffixOwned();
         BOOLEAN hasSuffix()const;
         UINT32 getSuffixSize()const;

      private:
         INT32 _prefixPos = -1;
         ixmKey _prefix;
         ixmKey _suffix;
         CHAR *_ownedSuffix = NULL;
         bson::StackAllocator _allocator;
   };//class btreeNodeKeyPrefix
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_COMPRESSED_KEY_H