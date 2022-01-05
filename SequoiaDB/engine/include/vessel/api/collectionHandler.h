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

#include "interface/IDataCollection.h"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
   class vesselImpl;

   class collectionHandler : public IDataCollection
   {
      public:
         collectionHandler(){}
         explicit collectionHandler(vesselImpl *db,
                                    const globalCollectionId &gcid):
                  _gcid(gcid), _db(db){}
         virtual ~collectionHandler(){}
         collectionHandler &operator=(const collectionHandler &o)
         {
            _db = o._db;
            _gcid = o._gcid;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _db && _gcid.isValid();
         }
         OSS_INLINE const globalCollectionId &getGlobalId()
         {
            return _gcid;
         }

      public:
         virtual BOOLEAN isClosed()const
         {
            return !isOpen();
         }
         virtual void close()
         {
            _gcid.reset();
            _db = NULL;
         }

      public:
         virtual INT32 createIndex(IExecutor *executor,
                                   const dmsBuildIndexOptions &o,
                                   const bson::BSONObj &indexDef);

         virtual INT32 listIndex(IExecutor *executor,
                                 ossPoolVector<bson::BSONObj> &indexes);

         virtual INT32 removeIndex(IExecutor *executor,
                                   const CHAR *indexName);

      public:
         virtual INT32 truncate(IExecutor *executor,
                                const dmsTruncateCLOptions &o);

      public:
         virtual INT32 insertRecord(IExecutor *executor,
                                    const bson::BSONObj &record,
                                    const dmsInsertRecordOptions &o,
                                    utilInsertResult *result);
         virtual INT32 insertBatch(IExecutor *executor,
                                   const ossPoolVector<bson::BSONObj> &batch,
                                   const dmsInsertRecordOptions &o,
                                   utilInsertResult *result);

         virtual INT32 updateRecord(IExecutor *executor,
                                    const dmsRecordID &rid,
                                    IRecordUpdater *updater,
                                    const dmsUpdateRecordOptions &o,
                                    utilUpdateResult *result);

         virtual INT32 deleteRecord(IExecutor *executor,
                                    const dmsRecordID &rid,
                                    const dmsDeleteRecordOptions &o,
                                    utilDeleteResult *result);

      public:
         virtual INT32 scan(IExecutor *executor,
                            const dmsScanOptions &o,
                            DATA_CURSOR_PTR &cursor);

         virtual INT32 scanIndex(IExecutor *executor,
                                 const CHAR *indexName,
                                 const rtnPredicateList &predicate,
                                 const dmsIndexScanOptions &o,
                                 DATA_CURSOR_PTR &cursor);

         virtual INT32 getRecordCount(IExecutor *executor,
                                      UINT64 &count);
          
      private:
         globalCollectionId _gcid;
         vesselImpl *_db = NULL;
   };//class collectionHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_HANDLER_H_