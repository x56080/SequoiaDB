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

   Source File Name = hybridIndexTree.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_HYBRID_INDEX_TREE_H_
#define VESSEL_HYBRID_INDEX_TREE_H_

#include "vessel/recordID.h"
#include "dpsTransID.hpp"
#include "dpsDef.hpp"
#include "../../bson/bson.hpp"
#include "ossMemPool.hpp"
#include "vessel/objectIdentifier.h"
#include "vessel/indexIterator.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class dmlContext;
   class indexObject;
   class dmlIndexRequest;
   class lsmWriteBatch;
   class indexSpace;
   class spacePteAccessCtx;

   class hybridIndexTree : public SDBObject
   {
      public:
         hybridIndexTree() = default;
         hybridIndexTree(indexSpace *is);
         ~hybridIndexTree() = default;
         hybridIndexTree(const hybridIndexTree &) = delete;
         hybridIndexTree &operator=(const hybridIndexTree &) = delete;

      public:
         void init(indexSpace *is);
         OSS_INLINE BOOLEAN isValid() const {return nullptr != _is;}

         INT32 insert(requestContext *context,
                      const indexObject *obj,
                      const bson::BSONObjSet &keys,
                      const DPS_LSN_OFFSET &lsn,
                      const recordID &rid,
                      const DPS_TRANS_ID &transID);

         INT32 write(requestContext *context,
                     const indexObject *obj,
                     const ossPoolList<bson::BSONObj> *insert,
                     const ossPoolList<bson::BSONObj> *remove,
                     const DPS_LSN_OFFSET &lsn,
                     const recordID &rid,
                     const DPS_TRANS_ID &transID);

         INT32 contains(requestContext *context,
                        indexObject *obj,
                        const bson::BSONObj &key,
                        recordID &rid);

         INT32 contains(requestContext *context,
                        indexObject *obj,
                        const bson::BSONObjSet &keys,
                        recordID &rid);

         INT32 truncate(requestContext *context,
                        indexObject *obj,
                        spacePteAccessCtx *ac,
                        BOOLEAN removeEntryPage);

         INT32 handleDmlRequests(dmlContext *context,
                                 const ossPoolVector<dmlIndexRequest *> &requests);

         INT32 createIterator(requestContext *context,
                              indexObject *obj,
                              INDEX_ITERATOR_UPTR &ptr);

      private:
         INT32 _fillBatch(const globalLogicalClId &clid,
                          const indexObject *obj,
                          const ossPoolList<bson::BSONObj> *insert,
                          const ossPoolList<bson::BSONObj> *remove,
                          const DPS_LSN_OFFSET &lsn,
                          const recordID &rid,
                          const DPS_TRANS_ID &transID,
                          lsmWriteBatch *batch);


         INT32 _truncateBtree(requestContext *context,
                              indexObject *obj,
                              spacePteAccessCtx *ac,
                              BOOLEAN removeEntryPage);
      private:
         indexSpace *_is = nullptr;
   };//class indexEntryStore
} // namespace vessel

} // namespace engine


#endif//VESSEL_HYBRID_INDEX_TREE_H_