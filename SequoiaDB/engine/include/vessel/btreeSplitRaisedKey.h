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

   Source File Name = btreeSplitRaisedKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_SPLIT_RAISED_KEY_H_
#define VESSEL_BTREE_SPLIT_RAISED_KEY_H_

#include "vessel/recordID.h"
#include "vessel/btreeKeyStringEntry.h"

namespace engine
{
namespace vessel
{
   class btreeSplitRaisedKey : public SDBObject
   {
      public:
         btreeSplitRaisedKey() = default;
         ~btreeSplitRaisedKey() = default;
         btreeSplitRaisedKey(const btreeSplitRaisedKey &) = delete;
         btreeSplitRaisedKey &operator=(const btreeSplitRaisedKey &) = delete;
         btreeSplitRaisedKey(btreeSplitRaisedKey &&o):
         entry(std::move(o.entry)),
         leftChild(o.leftChild),
         rightChild(o.rightChild),
         fromLeaf(o.fromLeaf),
         transID(o.transID)
         {
            o.reset();
         }

         btreeSplitRaisedKey &operator=(btreeSplitRaisedKey &&o)
         {
            reset();
            entry = std::move(o.entry);
            leftChild = o.leftChild;
            rightChild = o.rightChild;
            fromLeaf = o.fromLeaf;
            transID = o.transID;
            o.reset();
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return entry.isValid() &&
                   INVALID_PAGE_ID != leftChild &&
                   INVALID_PAGE_ID != rightChild;
         }
         
         OSS_INLINE void reset()
         {
            entry.reset();
            leftChild = INVALID_PAGE_ID;
            rightChild = INVALID_PAGE_ID;
            fromLeaf = TRUE;
            transID.reset();
            return;
         }

         void shallowCopy(const btreeSplitRaisedKey &o)
         {
            reset();
            entry = o.entry;
            leftChild = o.leftChild;
            rightChild = o.rightChild;
            fromLeaf = o.fromLeaf;
            transID = o.transID;
            return;
         }

      public:
         btreeKeyStringEntry entry;
         PAGE_ID leftChild = INVALID_PAGE_ID;
         PAGE_ID rightChild = INVALID_PAGE_ID;
         BOOLEAN fromLeaf = TRUE;
         DPS_TRANS_ID transID;
   };//class btreeSplitRaisedKey
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_SPLIT_RAISED_KEY_H_