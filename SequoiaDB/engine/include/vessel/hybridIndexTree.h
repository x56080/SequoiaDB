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

   Source File Name = hybridIndexTree.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
                        const indexObject *obj,
                        const bson::BSONObj &key,
                        recordID &rid);

         INT32 contains(requestContext *context,
                        const indexObject *obj,
                        const bson::BSONObjSet &keys,
                        recordID &rid);

         INT32 truncate(requestContext *context,
                        indexObject *obj);

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

      private:
         indexSpace *_is = nullptr;
   };//class indexEntryStore
} // namespace vessel

} // namespace engine


#endif//VESSEL_HYBRID_INDEX_TREE_H_