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

   Source File Name = IDataCollection.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_DATA_COLLECTION_HPP_
#define SDB_I_DATA_COLLECTION_HPP_

#include "sdbInterface.hpp"
#include "interface/IObjectInfo.h"
#include "../bson/bson.hpp"
#include "dmsEngineOptions.hpp"
#include "utilPooledObject.hpp"
#include "utilInsertResult.hpp"
#include "ossMemPool.hpp"
#include "interface/IRecordFilter.h"
#include "interface/IRecordUpdater.h"
#include "rtnPredicate.hpp"
#include "interface/IDataCursor.h"
#include "utilUniqueID.hpp"
#include "dmsLobDef.hpp"

#include <memory> // c++ 11

namespace engine
{
   class IDataCollection : public _utilPooledObject
   {
      public:
         IDataCollection(){}
         virtual ~IDataCollection(){}
         IDataCollection(const IDataCollection &) = delete;
         IDataCollection &operator=(const IDataCollection &) = delete;

      public:
         virtual BOOLEAN isClosed()const = 0;
         virtual void close() = 0;

      public:
         virtual INT32 getMetaData(IExecutor *executor, CONST_CL_META_INFO_PTR &meta) = 0;

         virtual DMS_STORAGE_TYPE getCSStorageType() = 0;

         virtual INT32 createIndex(IExecutor *executor,
                                   const dmsBuildIndexOptions &o,
                                   const bson::BSONObj &indexDef) = 0;

         virtual INT32 listIndex(IExecutor *executor,
                                 ossPoolVector<bson::BSONObj> &indexes) = 0;

         virtual INT32 removeIndex(IExecutor *executor,
                                   const CHAR *indexName) = 0;

         virtual INT32 removeIndex(IExecutor *executor,
                                   const CHAR *indexName,
                                   const dmsRemoveIndexOptions &o) = 0;

         virtual INT32 removeIndex(IExecutor *executor,
                                   const OID &indexOID,
                                   const dmsRemoveIndexOptions &o) = 0;

      public:
         virtual INT32 truncate(IExecutor *executor,
                                const dmsTruncateCLOptions &o) = 0;

      public:
         virtual INT32 insertRecord(IExecutor *executor,
                                    const bson::BSONObj &record,
                                    const dmsInsertRecordOptions &o,
                                    utilInsertResult *result) = 0;
         virtual INT32 insertBatch(IExecutor *executor,
                                   const ossPoolVector<bson::BSONObj> &batch,
                                   const dmsInsertRecordOptions &o,
                                   utilInsertResult *result) = 0;

         virtual INT32 updateRecord(IExecutor *executor,
                                    const dmsRecordID &rid,
                                    IRecordUpdater *updater,
                                    const dmsUpdateRecordOptions &o,
                                    utilUpdateResult *result) = 0;

         virtual INT32 deleteRecord(IExecutor *executor,
                                    const dmsRecordID &rid,
                                    const dmsDeleteRecordOptions &o,
                                    utilDeleteResult *result) = 0;

      public:
         virtual INT32 scan(IExecutor *executor,
                            const dmsScanOptions &o,
                            DATA_CURSOR_PTR &cursor) = 0;

         virtual INT32 scanIndex(IExecutor *executor,
                                 const CHAR *indexName,
                                 const rtnPredicateList &predicate,
                                 const dmsIndexScanOptions &o,
                                 DATA_CURSOR_PTR &cursor) = 0;

         virtual INT32 getRecordCount(IExecutor *executor,
                                      UINT64 &count) = 0;

      public: /// lob
         virtual INT32 insertLobChunk(IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 offset,
                                      UINT32 size,
                                      const CHAR *data) = 0;

         virtual INT32 readLobChunk(IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    CHAR *data,
                                    UINT32 &readSize) = 0;

         virtual INT32 removeLobChunk(IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId) = 0;


         virtual INT32 updateLobChunk(IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 offset,
                                      UINT32 size,
                                      const CHAR *data,
                                      BOOLEAN createIfNotExists) = 0;

         virtual INT32 truncateLobChunk(IExecutor *executor,
                                        const bson::OID &oid,
                                        UINT32 chunkId,
                                        UINT32 size,
                                        UINT32 &tsize) = 0;

         virtual INT32 listLobChunks(IExecutor *executor,
                                     const dmsListLobChunkOptions &o,
                                     DATA_CURSOR_PTR &cursor) = 0;
                              

         
         virtual INT32 testLobChunk(IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    dmsLobChunkProfile *profile) = 0;




   };//class IDataCollection

   typedef std::shared_ptr<IDataCollection> DATA_COLLECTION_PTR;
} // namespace engine


#endif//SDB_I_DATA_COLLECTION_HPP_