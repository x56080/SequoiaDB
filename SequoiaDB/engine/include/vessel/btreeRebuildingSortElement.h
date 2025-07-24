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

   Source File Name = btreeRebuildingSortElement.h

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_REBUILDING_SORT_ELEMENT_H_
#define VESSEL_BTREE_REBUILDING_SORT_ELEMENT_H_

#include "ixmKey.hpp"
#include "vessel/recordID.h"
#include "dpsTransID.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/bufferOwnedSortor.hpp"
#include "../../bson/util/builder.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class btreeRebuildingSortElement : public SDBObject
   {
      public:
         btreeRebuildingSortElement(){}
         btreeRebuildingSortElement(const CHAR *buf)
         {
            assign(buf);
         }
         btreeRebuildingSortElement(const btreeRebuildingSortElement &) = delete;
         btreeRebuildingSortElement &operator=(const btreeRebuildingSortElement &) = delete;

      public:
         class comparer
         {
            public:
               orderingWrapper ow;

               BOOLEAN operator()(const CHAR *l, const CHAR *r)const
               {
                  ixmKey lk(l);
                  ixmKey rk(r);
                  return lk.woCompare(rk, ow.toBsonOrdering()) < 0;
               }
         };

      public:
         void assign(const CHAR *buf)
         {
            SDB_ASSERT(NULL != buf, "can not be null");
            _key.assign(buf);
            const recordID *rid = (const recordID *)(buf + _key.dataSize());
            _rid = *rid;
            const DPS_TRANS_ID *transID = (const DPS_TRANS_ID *)
                                          (buf + _key.dataSize() + sizeof(recordID));
            _transID = *transID;
            return;
         }

         void reset()
         {
            _key.assign(NULL);
            _rid = recordID();
            _transID.reset();
         }

         OSS_INLINE const ixmKey &getKey()const
         {
            return _key;
         }
         OSS_INLINE slice getKeySlice()const
         {
            return slice(_key.dataSize(), _key.data());
         }
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE slice getRidSlice()const
         {
            return slice(sizeof(recordID), &_rid);
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const
         {
            return _transID;
         }
         OSS_INLINE slice getTransIDSlice()const
         {
            return slice(sizeof(DPS_TRANS_ID), &_transID);
         }

         OSS_INLINE void set(const ixmKey &key,
                             const recordID &rid,
                             const DPS_TRANS_ID &transID)
         {
            SDB_ASSERT(key.isValid(), "can not be invalid");
            SDB_ASSERT(rid.isValid(), "cana not be invalid");
            _key.assign(key);
            _rid = rid;
            _transID = transID;
         }

         OSS_INLINE UINT32 size()const
         {
            return _key.dataSize() + sizeof(recordID) + sizeof(DPS_TRANS_ID);
         }
      private:
         ixmKey _key;
         recordID _rid;
         DPS_TRANS_ID _transID;
   };//class btreeRebuildingSortElement

   typedef bufferOwnedSortor<btreeRebuildingSortElement, btreeRebuildingSortElement::comparer> BTREE_SORTOR;
 
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_REBUILDING_SORT_ELEMENT_H_