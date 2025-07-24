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

   Source File Name = collectionHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         virtual BOOLEAN isClosed()const override
         {
            return !isOpen();
         }
         virtual void close() override
         {
            _gcid.reset();
            _db = NULL;
         }

      public:
         virtual INT32 getMetaData(IExecutor *executor,
                                   CONST_CL_META_INFO_PTR &meta) override;

         virtual DMS_STORAGE_TYPE getCSStorageType() override;

         virtual INT32 createIndex(IExecutor *executor,
                                   const dmsBuildIndexOptions &o,
                                   const bson::BSONObj &indexDef) override;

         virtual INT32 listIndex(IExecutor *executor,
                                 ossPoolVector<bson::BSONObj> &indexes) override;

         virtual INT32 removeIndex( IExecutor *executor,
                                    const CHAR *indexName ) override;

         virtual INT32 removeIndex(IExecutor *executor,
                                   const CHAR *indexName,
                                   const dmsRemoveIndexOptions &o) override;

         virtual INT32 removeIndex(IExecutor *executor,
                                   const OID &indexOID,
                                   const dmsRemoveIndexOptions &o) override;

      public:
         virtual INT32 truncate(IExecutor *executor,
                                const dmsTruncateCLOptions &o) override;

      public:
         virtual INT32 insertRecord(IExecutor *executor,
                                    const bson::BSONObj &record,
                                    const dmsInsertRecordOptions &o,
                                    utilInsertResult *result) override;
         virtual INT32 insertBatch(IExecutor *executor,
                                   const ossPoolVector<bson::BSONObj> &batch,
                                   const dmsInsertRecordOptions &o,
                                   utilInsertResult *result) override;

         virtual INT32 updateRecord(IExecutor *executor,
                                    const dmsRecordID &rid,
                                    IRecordUpdater *updater,
                                    const dmsUpdateRecordOptions &o,
                                    utilUpdateResult *result) override;

         virtual INT32 deleteRecord(IExecutor *executor,
                                    const dmsRecordID &rid,
                                    const dmsDeleteRecordOptions &o,
                                    utilDeleteResult *result) override;

      public:
         virtual INT32 scan(IExecutor *executor,
                            const dmsScanOptions &o,
                            DATA_CURSOR_PTR &cursor) override;

         virtual INT32 scanIndex(IExecutor *executor,
                                 const CHAR *indexName,
                                 const rtnPredicateList &predicate,
                                 const dmsIndexScanOptions &o,
                                 DATA_CURSOR_PTR &cursor) override;

         virtual INT32 getRecordCount(IExecutor *executor,
                                      UINT64 &count) override;

      public:
         virtual INT32 insertLobChunk(IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 offset,
                                      UINT32 size,
                                      const CHAR *data) override;

         virtual INT32 readLobChunk(IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    CHAR *data,
                                    UINT32 &readSize) override;

         virtual INT32 removeLobChunk(IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId) override;

         virtual INT32 updateLobChunk(IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 offset,
                                      UINT32 size,
                                      const CHAR *data,
                                      BOOLEAN createIfNotExists) override;

         virtual INT32 truncateLobChunk(IExecutor *executor,
                                        const bson::OID &oid,
                                        UINT32 chunkId,
                                        UINT32 size,
                                        UINT32 &tsize) override;

         virtual INT32 listLobChunks(IExecutor *executor,
                                     const dmsListLobChunkOptions &o,
                                     DATA_CURSOR_PTR &cursor) override;

         virtual INT32 testLobChunk(IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    dmsLobChunkProfile *profile) override;
          
      private:
         globalCollectionId _gcid;
         vesselImpl *_db = NULL;
   };//class collectionHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_HANDLER_H_