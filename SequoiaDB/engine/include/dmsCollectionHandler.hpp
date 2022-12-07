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

   Source File Name = dmsCollectionHandler.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/28/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_COLLECTION_HANDLER_HPP_
#define SDB_DMS_COLLECTION_HANDLER_HPP_

#include "dms.hpp"
#include "dmsEventHandler.hpp"
#include "interface/IDataCollection.h"
#include "dmsCB.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "utilUniqueID.hpp"

namespace engine
{
class dmsCollectionHandler : public IDataCollection
{
public:
   dmsCollectionHandler( dmsStorageUnit *_su,
                         dmsStorageUnitID _suID,
                         dmsMBContext *_mbContext );
   virtual ~dmsCollectionHandler();

public:
   virtual BOOLEAN isClosed() const override;
   virtual void close() override;

public:
   virtual INT32 getMetaData( IExecutor *executor,
                              CONST_CL_META_INFO_PTR &meta ) override;

   virtual INT32 createIndex( IExecutor *executor,
                              const dmsBuildIndexOptions &o,
                              const bson::BSONObj &indexDef ) override;

   virtual INT32 listIndex( IExecutor *executor,
                            ossPoolVector< bson::BSONObj > &indexes ) override;

   virtual INT32 removeIndex( IExecutor *executor,
                              const CHAR *indexName ) override;

public:
   virtual INT32 truncate( IExecutor *executor,
                           const dmsTruncateCLOptions &o ) override;

public:
   virtual INT32 insertRecord( IExecutor *executor,
                               const bson::BSONObj &record,
                               const dmsInsertRecordOptions &o,
                               utilInsertResult *result ) override;
   virtual INT32 insertBatch( IExecutor *executor,
                              const ossPoolVector< bson::BSONObj > &batch,
                              const dmsInsertRecordOptions &o,
                              utilInsertResult *result ) override;

   virtual INT32 updateRecord( IExecutor *executor,
                               const dmsRecordID &rid,
                               IRecordUpdater *updater,
                               const dmsUpdateRecordOptions &o,
                               utilUpdateResult *result ) override;

   virtual INT32 deleteRecord( IExecutor *executor,
                               const dmsRecordID &rid,
                               const dmsDeleteRecordOptions &o,
                               utilDeleteResult *result ) override;

public:
   virtual INT32 scan( IExecutor *executor,
                       const dmsScanOptions &o,
                       DATA_CURSOR_PTR &cursor ) override;

   virtual INT32 scanIndex( IExecutor *executor,
                            const CHAR *indexName,
                            const rtnPredicateList &predicate,
                            const dmsIndexScanOptions &o,
                            DATA_CURSOR_PTR &cursor ) override;

   virtual INT32 getRecordCount( IExecutor *executor, UINT64 &count ) override;

public: /// lob
   virtual INT32 insertLobChunk( IExecutor *executor,
                                 const bson::OID &oid,
                                 UINT32 chunkId,
                                 UINT32 offset,
                                 UINT32 size,
                                 const CHAR *data ) override;

   virtual INT32 readLobChunk( IExecutor *executor,
                               const bson::OID &oid,
                               UINT32 chunkId,
                               UINT32 offset,
                               UINT32 size,
                               CHAR *data,
                               UINT32 &readSize ) override;

   virtual INT32 removeLobChunk( IExecutor *executor,
                                 const bson::OID &oid,
                                 UINT32 chunkId ) override;

   virtual INT32 updateLobChunk( IExecutor *executor,
                                 const bson::OID &oid,
                                 UINT32 chunkId,
                                 UINT32 offset,
                                 UINT32 size,
                                 const CHAR *data,
                                 BOOLEAN createIfNotExists ) override;

   virtual INT32 truncateLobChunk( IExecutor *executor,
                                   const bson::OID &oid,
                                   UINT32 chunkId,
                                   UINT32 size,
                                   UINT32 &tsize ) override;

   virtual INT32 listLobChunks( IExecutor *executor,
                                const dmsListLobChunkOptions &o,
                                DATA_CURSOR_PTR &cursor ) override;

   virtual INT32 testLobChunk( IExecutor *executor,
                               const bson::OID &oid,
                               UINT32 chunkId,
                               dmsLobChunkProfile *profile ) override;

private:
   OSS_INLINE void _reset()
   {
      _su = nullptr;
      _suID = DMS_INVALID_CS;
      _mbContext = nullptr;
   }

private:
   dmsStorageUnit *_su = nullptr;
   dmsStorageUnitID _suID = DMS_INVALID_CS;
   dmsMBContext *_mbContext = nullptr;
};
} // namespace engine

#endif