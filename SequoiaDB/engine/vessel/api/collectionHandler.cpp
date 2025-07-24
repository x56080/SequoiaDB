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

   Source File Name = collectionHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/api/collectionHandler.h"
#include "vessel/vesselImpl.h"
#include "interface/IRecordFilter.h"
#include "vessel/scanCLCursor.h"
#include "vessel/indexScanCursor.h"
#include "utilSharedPtrMaker.hpp"
#include "vessel/listLobChunkCursor.h"

namespace engine
{
namespace vessel
{
   INT32 collectionHandler::createIndex(IExecutor *executor,
                                        const dmsBuildIndexOptions &o,
                                        const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _db->createIndex(executor, _gcid, o, adjunct);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::getMetaData(IExecutor *executor,
                                        CONST_CL_META_INFO_PTR &meta)
   {
      SDB_ASSERT(FALSE, "todo");
      return SDB_OK;
   }

   DMS_STORAGE_TYPE collectionHandler::getCSStorageType()
   {
      return DMS_STORAGE_NORMAL;
   }

   INT32 collectionHandler::listIndex(IExecutor *executor,
                                      ossPoolVector<bson::BSONObj> &indexes)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _db->listIndexes(executor, _gcid, indexes);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::removeIndex(IExecutor *executor,
                                        const CHAR *indexName)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor ||
                            nullptr == indexName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->removeIndex(executor, _gcid, indexName);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::removeIndex(IExecutor *executor,
                                        const CHAR *indexName,
                                        const dmsRemoveIndexOptions &o)
   {
      return SDB_NOT_SUPPORTED;
   }

   INT32 collectionHandler::removeIndex(IExecutor *executor,
                                        const OID &indexOID,
                                        const dmsRemoveIndexOptions &o)
   {
      return SDB_NOT_SUPPORTED;
   }

   INT32 collectionHandler::insertRecord(IExecutor *executor,
                                         const bson::BSONObj &record,
                                         const dmsInsertRecordOptions &o,
                                         utilInsertResult *result)
   {
      INT32 rc = SDB_OK;
      dmlInsertRequest request;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor ||
                            !record.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      request.record.reset(record.objsize(), record.objdata());
      request.o = o;

      rc = _db->insert(executor, _gcid, request, result);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::insertBatch(IExecutor *executor,
                                        const ossPoolVector<bson::BSONObj> &batch,
                                        const dmsInsertRecordOptions &o,
                                        utilInsertResult *result)
   {
      INT32 rc = SDB_OK;

      dmlBatchInsertRequest request;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor ||
                            batch.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      request.batch.reserve(batch.size());
      for (ossPoolVector<bson::BSONObj>::const_iterator itr = batch.cbegin();
           itr != batch.cend(); ++itr)
      {
         if (!itr->isValid())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         request.batch.push_back(slice(itr->objsize(), itr->objdata()));
      }

      request.o = o;

      rc = _db->insertBatch(executor, _gcid, request, result);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::updateRecord(IExecutor *executor,
                                         const dmsRecordID &rid,
                                         IRecordUpdater *updater,
                                         const dmsUpdateRecordOptions &o,
                                         utilUpdateResult *result)
   {
      INT32 rc = SDB_OK;
      dmlUpdateRequest request;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor ||
                            !rid.isValid() ||
                            nullptr == updater))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      request.rid.resetByDmsRid(rid);
      request.o = o;

      rc = _db->update(executor, _gcid, request, updater, result);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::deleteRecord(IExecutor *executor,
                                          const dmsRecordID &rid,
                                          const dmsDeleteRecordOptions &o,
                                          utilDeleteResult *result)
   {
      INT32 rc = SDB_OK;
      dmlRemoveRequest request;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor ||
                            !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      request.rid.resetByDmsRid(rid);
      request.o = o;
      rc = _db->remove(executor, _gcid, request, result);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::scan(IExecutor *executor,
                                 const dmsScanOptions &o,
                                 DATA_CURSOR_PTR &cursor)
   {
      INT32 rc = SDB_OK;

      cursorOptions co;
      scanCLCursor *impl = nullptr;

      cursor.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      co.rowCountLimit = o.rowCountLimit;
      co.stepSize = 1024;
      co.defaultBufferSize = (UINT32)128 << 10;

      cursor = makeSharedPtrFromPool<scanCLCursor>();
      if (!cursor)
      {
         rc = SDB_OOM;
         goto error;
      }

      impl = static_cast<scanCLCursor *>(cursor.get());
      rc = impl->open(_db, &co);
      if (SDB_OK != rc)
      {
         goto error;
      }

      impl->resetToScan(_gcid, o);
   done:
      return rc;
   error:
      cursor.reset();
      goto done;
   }

   INT32 collectionHandler::scanIndex(IExecutor *executor,
                                       const CHAR *indexName,
                                       const rtnPredicateList &predicate,
                                       const dmsIndexScanOptions &o,
                                       DATA_CURSOR_PTR &cursor)
   {
      INT32 rc = SDB_OK;

      cursorOptions co;
      indexScanCursor *impl = nullptr;
      strSlice indexNameSlice(indexName);
      indexIdentifier indexId;
      //constexpr UINT32 _MAX_SCAN_STEP = 64;

      cursor.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor ||
                            indexNameSlice.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->testIndex(executor, _gcid, indexNameSlice, indexId);
      if (SDB_OK != rc)
      {
         goto error;
      }

      co.rowCountLimit = o.rowCountLimit;
      co.stepSize = o.stepSize;
      co.defaultBufferSize = (INT32)64 << 10;

      cursor = makeSharedPtrFromPool<indexScanCursor>(o, predicate,
                                                      _gcid, indexId);
      if (!cursor)
      {
         rc = SDB_OOM;
         goto error;
      }

      impl = static_cast<indexScanCursor *>(cursor.get());
      rc = impl->open(_db, &co);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      cursor.reset();
      goto done;
   }

   INT32 collectionHandler::getRecordCount(IExecutor *executor,
                                           UINT64 &count)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->count(executor, _gcid, count);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      count = 0;
      goto done;
   }

   INT32 collectionHandler::truncate(IExecutor *executor,
                                     const dmsTruncateCLOptions &o)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->truncate(executor, _gcid, o);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::insertLobChunk(IExecutor *executor,
                                           const bson::OID &oid,
                                           UINT32 chunkId,
                                           UINT32 offset,
                                           UINT32 size,
                                           const CHAR *data)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->insertLobChunk(executor, _gcid, oid,
                               chunkId, offset, size, data);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert lob chunk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::readLobChunk(IExecutor *executor,
                                         const bson::OID &oid,
                                         UINT32 chunkId,
                                         UINT32 offset,
                                         UINT32 size,
                                         CHAR *data,
                                         UINT32 &readSize)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->readLobChunk(executor, _gcid, oid,
                             chunkId, offset, size,
                             data, readSize);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::removeLobChunk(IExecutor *executor,
                                           const bson::OID &oid,
                                           UINT32 chunkId)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->removeLobChunk(executor, _gcid, oid, chunkId);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::updateLobChunk(IExecutor *executor,
                                           const bson::OID &oid,
                                           UINT32 chunkId,
                                           UINT32 offset,
                                           UINT32 size,
                                           const CHAR *data,
                                           BOOLEAN createIfNotExists) 
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->updateLobChunk(executor, _gcid, oid, chunkId,
                               offset, size, data, createIfNotExists);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::truncateLobChunk(IExecutor *executor,
                                             const bson::OID &oid,
                                             UINT32 chunkId,
                                             UINT32 size,
                                             UINT32 &tsize)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->truncateLobChunk(executor, _gcid, oid, chunkId, size, tsize);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::listLobChunks(IExecutor *executor,
                                          const dmsListLobChunkOptions &o,
                                          DATA_CURSOR_PTR &cursor)
   {
      INT32 rc = SDB_OK;
      listLobChunkCursor *impl = nullptr;
      cursorOptions co;
      cursor.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      co.defaultBufferSize = 128 << 10;
      co.bufferSizeLimit = OSS_UINT32_MAX;
      co.stepSize = OSS_UINT32_MAX;

      cursor = makeSharedPtrFromPool<listLobChunkCursor>(_gcid, o);
      if (!cursor)
      {
         rc = SDB_OOM;
         goto error;
      }

      impl = static_cast<listLobChunkCursor *>(cursor.get());
      rc = impl->open(_db, &co);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::testLobChunk(IExecutor *executor,
                                         const bson::OID &oid,
                                         UINT32 chunkId,
                                         dmsLobChunkProfile *profile)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->testLobChunk(executor, _gcid, oid, chunkId, profile);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine