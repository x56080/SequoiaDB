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

   Source File Name = lobChunkHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobChunkHandler.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "dmsLobDef.hpp"
#include "vessel/requestContext.h"
#include "vessel/lobChunkKey.h"
#include "vessel/listLobChunkCursor.h"

namespace engine
{
namespace vessel
{
   INT32 lobChunkHandler::insert(const globalCollectionId &gcid,
                                 const bson::OID &oid,
                                 UINT32 chunkId,
                                 UINT32 offset,
                                 UINT32 size,
                                 const CHAR *data)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;
      slice ds;
      lobChunkKey key;

      if (OSS_UNLIKELY(!gcid.isValid() ||
                       !oid.isSet() ||
                       (MAX_LOB_CHUNK_SIZE < (offset + size)) ||
                       0 == size ||
                       nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      key.set(oid, chunkId);
      ds.reset(size, data);

      rc = cl->insertLobChunk(&context, key, offset, ds);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert lob chunk:%d", rc);
         goto error;
      }

   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkHandler::read(const globalCollectionId &gcid,
                               const bson::OID &oid,
                               UINT32 chunkId,
                               UINT32 offset,
                               UINT32 size,
                               CHAR *data,
                               UINT32 &readSize)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;
      lobChunkKey key;

      if (OSS_UNLIKELY(!gcid.isValid() ||
                       !oid.isSet() ||
                       (MAX_LOB_CHUNK_SIZE < (offset + size)) ||
                       nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      key.set(oid, chunkId);
      rc = cl->readLobChunk(&context, key, offset, size, data, readSize);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkHandler::remove(const globalCollectionId &gcid,
                                 const bson::OID &oid,
                                 UINT32 chunkId)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;
      lobChunkKey key;

      if (OSS_UNLIKELY(!gcid.isValid() ||
                       !oid.isSet()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      key.set(oid, chunkId);
      rc = cl->removeLobChunk(&context, key);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkHandler::update(const globalCollectionId &gcid,
                                 const bson::OID &oid,
                                 UINT32 chunkId,
                                 UINT32 offset,
                                 UINT32 size,
                                 const CHAR *data,
                                 BOOLEAN createIfNotExists)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;
      lobChunkKey key;
      slice ds(size, data);

      if (OSS_UNLIKELY(!gcid.isValid() ||
                       !oid.isSet() ||
                       !ds.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      key.set(oid, chunkId);
      rc = cl->updateLobChunk(&context, key, offset, ds, createIfNotExists);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update lob chunk[%s], rc:%d",
                key.toString().c_str(), rc);
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkHandler::truncate(const globalCollectionId &gcid,
                                   const bson::OID &oid,
                                   UINT32 chunkId,
                                   UINT32 size,
                                   UINT32 &tsize)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;
      lobChunkKey key;

      if (OSS_UNLIKELY(!gcid.isValid() ||
                       !oid.isSet()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      key.set(oid, chunkId);
      rc = cl->truncateLobChunk(&context, key, size, tsize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update lob chunk[%s], rc:%d",
                key.toString().c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkHandler::list(listLobChunkCursor *cursor)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;

      if (OSS_UNLIKELY(nullptr == cursor ||
                       !cursor->getCollectionId().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, cursor->getCollectionId(), SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->listLobChunks(&context, cursor);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkHandler::test(const globalCollectionId &gcid,
                               const bson::OID &oid,
                               UINT32 chunkId,
                               dmsLobChunkProfile *profile)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;
      lobChunkKey key;

      if (OSS_UNLIKELY(!gcid.isValid() ||
                       !oid.isSet()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      key.set(oid, chunkId);
      rc = cl->testLobChunk(&context, key, profile);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine

