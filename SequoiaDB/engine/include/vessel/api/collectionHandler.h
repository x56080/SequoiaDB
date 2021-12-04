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

   Source File Name = collectionHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_HANDLER_H_
#define VESSEL_COLLECTION_HANDLER_H_

#include "vessel/vesselIdDef.h"
#include "vessel/objectIdentifier.h"
#include "utilInsertResult.hpp"
#include "dpsTransID.hpp"
#include "vessel/api/cursorHandler.h"
#include "vessel/indexOptions.h"
#include "vessel/indexParameters.h"
#include "vessel/cursorOptions.h"
#include "../bson/bson.hpp"
#include "vessel/collectionOptions.h"
#include "rtnPredicate.hpp"
#include "sdbInterface.hpp"
#include "vessel/recordID.h"
#include "interface/IRecordUpdater.h"

namespace engine
{
namespace vessel
{
   class vesselImpl;
   class insertOptions;
   class IQueryFilter;

   class collectionHandler : public SDBObject
   {
      public:
         OSS_INLINE collectionHandler(){}

         OSS_INLINE explicit collectionHandler(const globalCollectionId &gcid,
                                               vesselImpl *db):
                    _gcid(gcid),
                    _db(db){}

         OSS_INLINE collectionHandler(const collectionHandler &o):
                    _gcid(o._gcid),
                    _db(o._db){}

         OSS_INLINE ~collectionHandler()
         {
            _db = NULL;
         }

         collectionHandler &operator=(const collectionHandler &o)
         {
            _gcid = o._gcid;
            _db = o._db;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _gcid.isValid() && NULL != _db;
         }
         OSS_INLINE void close()
         {
            _gcid = globalCollectionId();
            _db = NULL;
            return;
         }

      public:
         INT32 createIndex(IExecutor *executor,
                           const strSlice &indexName,
                           const bson::BSONObj &keyPattern,
                           const indexParameters &params,
                           const createIndexOptions &options);

         INT32 listIndexes(IExecutor *executor,
                           ossPoolVector<bson::BSONObj> &indexes);

      public:
         INT32 insert(IExecutor *executor,
                      const slice &record,
                      STRIPING_ID striping,
                      const insertOptions &options,
                      utilInsertResult *res);

         INT32 insertBatch(IExecutor *executor,
                           const ossPoolVector<slice> &batch,
                           const insertOptions &options,
                           utilInsertResult *res);

         INT32 deleteRecord(IExecutor *executor,
                            const recordID &rid,
                            utilDeleteResult *res);

         INT32 updateRecord(IExecutor *executor,
                            const recordID &rid,
                            IRecordUpdater *updater,
                            utilUpdateResult *res);

      public:

         INT32 getTotalRecordCountInPageHead(IExecutor *executor,
                                             UINT64 &count);

         /// The releasing of cursor is not related to handler.
         /// You can call their "close" functions in any order.
         INT32 openScanCursor(IExecutor *executor,
                              IQueryFilter *filter,
                              const collectionScanOptions &o,
                              cursorHandler &cursor);

         INT32 openIndexScanCursor(IExecutor *executor,
                                   const strSlice &indexName,
                                   const rtnPredicateList &predicate,
                                   const indexScanOptions &o,
                                   cursorHandler &cursor);
          
      private:
         globalCollectionId _gcid;
         vesselImpl *_db = NULL;
   };//class collectionHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_HANDLER_H_