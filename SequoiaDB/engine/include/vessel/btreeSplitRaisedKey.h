/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = btreeSplitRaisedKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         btreeSplitRaisedKey(btreeSplitRaisedKey &&o)noexcept:
         entry(std::move(o.entry)),
         leftChild(o.leftChild),
         rightChild(o.rightChild),
         fromLeaf(o.fromLeaf),
         transID(o.transID)
         {
            o.reset();
         }

         btreeSplitRaisedKey &operator=(btreeSplitRaisedKey &&o)noexcept
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