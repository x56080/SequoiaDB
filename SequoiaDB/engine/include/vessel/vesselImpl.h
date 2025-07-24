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

   Source File Name = vesselImpl.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_VESSEL_IMPL_H_
#define VESSEL_VESSEL_IMPL_H_

#include "vessel/api/vessel.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/dmlRequest.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class cursorKernal;
   
   class vesselImpl : public IVessel
   {
      public:
         vesselImpl(){}
         virtual ~vesselImpl();
      public:
         OSS_INLINE BOOLEAN isOpen()const {return _open;}

         virtual INT32 open(IExecutor *executor,
                            const outerResource *resource,
                            const openDBOptions &options) override;
                            
         virtual INT32 close(IExecutor *executor,
                             const closeDBOptions &options) override;

         virtual INT32 close(IExecutor *executor,
                             const dmsCloseDBOptions &options) override;

      /// IDataStorageEngine begin
      public:
         virtual DMS_ENGINE_TYPE getEngineType() const override
         {
            return DMS_ENGINE_VESSEL;
         }

         virtual INT32 createCS(IExecutor *executor,
                                const CHAR *name,
                                const utilCSUniqueID &uniqueId,
                                const dmsCreateCSOptions &o,
                                const bson::BSONObj &adjunct) override;

         virtual INT32 testCS(IExecutor *executor,
                              const CHAR *name,
                              utilCSUniqueID &uniqueId) override;

         virtual INT32 testCS(IExecutor *executor,
                              utilCSUniqueID uniqueId) override;

         virtual INT32 listCS(IExecutor *executor,
                              DATA_CURSOR_PTR &cursor) override;

         virtual INT32 getCSCount(IExecutor *executor,
                                  UINT32 &countt) override; 

         virtual INT32 removeCS(IExecutor *executor,
                                const CHAR *csName) override;

      public:

         virtual INT32 createCL(IExecutor *executor,
                                const CHAR *fullName,
                                utilCLUniqueID uniqueId,
                                const dmsCreateCLOptions &o,
                                const bson::BSONObj &adjunct) override;

         virtual INT32 removeCL(IExecutor *executor,
                                const CHAR *fullName,
                                const dmsRemoveCLOptions &o) override;

         virtual INT32 testCL(IExecutor *executor,
                              const CHAR *fullName,
                              utilCLUniqueID &uniqueId) override;
                        
         virtual INT32 testCL(IExecutor *executor,
                              utilCLUniqueID uniqueId) override;

         virtual INT32 listCL(IExecutor *executor,
                              const CHAR *csName,
                              DATA_CURSOR_PTR &cursor) override;

         virtual INT32 openCL(IExecutor *executor,
                              const CHAR *fullName,
                              const dmsOpenCLOptions &o,
                              DATA_COLLECTION_PTR &ptr) override;

         virtual INT32 openCL(IExecutor *executor,
                              utilCLUniqueID uniqueId,
                              const dmsOpenCLOptions &o,
                              DATA_COLLECTION_PTR &ptr) override;

         virtual INT32 getCLCount(IExecutor *executor,
                                  const CHAR *csName,
                                  UINT32 &count) override;
         
      public:
         /// temporary
         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 const utilCSUniqueID &uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct,
                                 DMS_SU_DESCRIPTOR &desc ) override
         {
            SDB_ASSERT( FALSE, "todo" );
            return SDB_OK;
         }

         virtual INT32 removeCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &options ) override
         {
            SDB_ASSERT( FALSE, "todo" );
            return SDB_OK;
         }

         virtual INT32 removeEmptyCS( IExecutor *executor,
                                      const CHAR *name,
                                      const dmsRemoveCSOptions &options ) override
         {
            SDB_ASSERT( FALSE, "todo" );
            return SDB_OK;
         }

         virtual INT32 renameCS( IExecutor *executor,
                                 const CHAR *name,
                                 const CHAR *newName,
                                 BOOLEAN blockWrite,
                                 DMS_SU_DESCRIPTOR &desc ) override
         {
            SDB_ASSERT( FALSE, "todo" );
            return SDB_OK;
         }

         virtual INT32 unloadCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &delOptions ) override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 restoreCS( IExecutor *executor, const CHAR *name ) override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 returnCS( IExecutor *executor,
                                 dmsReturnOptions &options,
                                 BOOLEAN blockWrite ) override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc ) override
         {
            SDB_ASSERT( FALSE, "todo" );
            return SDB_OK;
         }


      /// IDataStorageEngine end

      public:
         INT32 createIndex(IExecutor *executor,
                           const globalCollectionId &gcid,
                           const dmsBuildIndexOptions &o,
                           const bson::BSONObj &adjunct);

         INT32 listIndexes(IExecutor *executor,
                           const globalCollectionId &gcid,
                           ossPoolVector<bson::BSONObj> &indexes);

         INT32 removeIndex(IExecutor *executor,
                           const globalCollectionId &gcid,
                           const CHAR *indexName);

         INT32 testIndex(IExecutor *executor,
                         const globalCollectionId &gcid,
                         const strSlice &indexName,
                         indexIdentifier &indexId);

      public:
         INT32 truncate(IExecutor *executor,
                        const globalCollectionId &gcid,
                        const dmsTruncateCLOptions &o);
      
      public:

         INT32 insert(IExecutor *executor,
                      const globalCollectionId &gcid,
                      const dmlInsertRequest &request,
                      utilInsertResult *res);

         INT32 insertBatch(IExecutor *executor,
                           const globalCollectionId &gcid,
                           const dmlBatchInsertRequest &request,
                           utilInsertResult *res);

         INT32 update(IExecutor *executor,
                      const globalCollectionId &gcid,
                      const dmlUpdateRequest &request,
                      IRecordUpdater *updater,
                      utilUpdateResult *res);

         INT32 remove(IExecutor *executor,
                      const globalCollectionId &gcid,
                      const dmlRemoveRequest &request,
                      utilDeleteResult *res);

         INT32 count(IExecutor *executor,
                     const globalCollectionId &gcid,
                     UINT64 &count);

      public:
         INT32 insertLobChunk(IExecutor *executor,
                              const globalCollectionId &gcid,
                              const bson::OID &oid,
                              UINT32 chunkId,
                              UINT32 offset,
                              UINT32 size,
                              const CHAR *data);

         INT32 readLobChunk(IExecutor *executor,
                            const globalCollectionId &gcid,
                            const bson::OID &oid,
                            UINT32 chunkId,
                            UINT32 offset,
                            UINT32 size,
                            CHAR *data,
                            UINT32 &readSize);

         INT32 removeLobChunk(IExecutor *executor,
                              const globalCollectionId &gcid,
                              const bson::OID &oid,
                              UINT32 chunkId);

         INT32 updateLobChunk(IExecutor *executor,
                              const globalCollectionId &gcid,
                              const bson::OID &oid,
                              UINT32 chunkId,
                              UINT32 offset,
                              UINT32 size,
                              const CHAR *data,
                              BOOLEAN createIfNotExists);

         INT32 truncateLobChunk(IExecutor *executor,
                                const globalCollectionId &gcid,
                                const bson::OID &oid,
                                UINT32 chunkId,
                                UINT32 size,
                                UINT32 &tsize);

         INT32 testLobChunk(IExecutor *executor,
                            const globalCollectionId &gcid,
                            const bson::OID &oid,
                            UINT32 chunkId,
                            dmsLobChunkProfile *profile);

         INT32 flushLsmDB();
      public:
         INT32 pushMoreToCursor(IExecutor *executor,
                                 cursorKernal *cursor);   

      public:
         void attachLobcWatcher(IExecutor *executor);
         void attachLiteBufferPoolWatcher(IExecutor *executor);
         void attachHitManager(IExecutor *executor);
         
      private:

         void fini();
         INT32 initLsmDB(const openDBOptions &options);
         INT32 activeBackgroundThreads(const openDBOptions &options);

      private:
         BOOLEAN _open = FALSE;
         instanceEnv _env;
   }; /// end of class vesselImpl 


} /// end of namespace vessel 
} /// end of namespace engine
#endif // VESSEL_VESSEL_IMPL_H_