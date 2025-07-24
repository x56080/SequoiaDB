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

   Source File Name = vesselImpl.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/vesselImpl.h"
#include "pdTrace.hpp"
#include "vessel/globalControlFile.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/handlers.h"
#include "interface/IRecordFilter.h"
#include "vessel/listCSCursor.h"
#include "vessel/listCLCursor.h"
#include "vessel/cursorKernal.h"
#include "vessel/scanCLCursor.h"
#include "vessel/collectionSpace.h"
#include "vessel/spaceIDLockHelper.h"
#include "vessel/indexScanCursor.h"
#include "vessel/lsm/lsmDB.h"
#include "utilSharedPtrMaker.hpp"
#include "vessel/api/collectionHandler.h"
#include "dmsLobDef.hpp"

namespace engine
{
namespace vessel
{
   vesselImpl::~vesselImpl()
   {
      fini();
   }

   INT32 vesselImpl::open(IExecutor *executor,
                          const outerResource *resource,
                          const openDBOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reopen");
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      requestContext context;

      if (nullptr == executor ||
          nullptr == resource ||
          !resource->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _open = TRUE;
      _env.options = options;
      _env.resource = *resource;

      rc = _env.spaceLocker.init();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _env.latchEnv.lpidLatchMap.init(options.lpidLatchMapBucketCount,
                                   options.lpidLatchMapLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid latch map:%d", rc);
         goto error;
      }

      rc = _env.latchEnv.ridLatchMap.init(options.ridLatchMapBucketCount,
                                  options.ridLatchMapLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rid latch map:%d", rc);
         goto error;
      }

      rc = _env.latchEnv.uniqueIndexLathMap.init(options.indexLatchMapBucketCount,
                                        options.indexLatchMapLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rid latch map:%d", rc);
         goto error;
      }

      rc = _env.latchEnv.lobcLatchMap.init(options.lobcLatchBucketCount,
                                        options.lobcLatchMapLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lobc latch map:%d", rc);
         goto error;
      }

      _env.latchEnv.lobRegionLatchVec.init(1024);

      rc = _env.ioBufferPool.init(DEFAULT_STORAGE_PAGE_SIZE, options.bufferPoolOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lite buffer pool:%d", rc);
         goto error;
      }

      rc = _env.lobcBufferPool.init(options.lobcPoolOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lobc buffer pool:%d", rc);
         goto error;
      }

      rc = initLsmDB(options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm db:%d", rc);
         goto error;
      }

      rc = _env.dms.open(&context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open data management service:%d", rc);
         goto error;
      }

      rc = _env.hitMgr.init();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init hit index manager :%d", rc);
         goto error;
      }

      _env.hitMgr.setLimiter(options.limitOptions);

      rc = activeBackgroundThreads(options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to active background threads:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 vesselImpl::close(IExecutor *executor, const closeDBOptions &options)
   {
      INT32 rc = SDB_OK;
      if (isOpen() && closeDBOptions::CLOSE_MODE_NORMAL == options.closeMode)
      {
         THREAD_CONTEXT_OWNER tco(executor, &_env);

         _env.hitMgr.fini();
         _env.lobcBufferPool.flushAllDirtyBuffers();
         _env.ioBufferPool.flushAll();

         if (nullptr != _env.lsm)
         {
            _env.lsm->close();
         }

         _env.workers.fini();
      }
   done:
      fini();
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::close(IExecutor *executor,
                           const dmsCloseDBOptions &options)
   {
      if (isOpen() && dmsCloseDBOptions::DMS_CLOSE_MODE_NORMAL == options.closeMode)
      {
         THREAD_CONTEXT_OWNER tco(executor, &_env);

         _env.hitMgr.fini();
         _env.lobcBufferPool.flushAllDirtyBuffers();
         _env.ioBufferPool.flushAll();

         if (nullptr != _env.lsm)
         {
            _env.lsm->close();
         }

         _env.workers.fini();
      }

      return SDB_OK;
   }

   INT32 vesselImpl::getCSCount(IExecutor *executor,
                                UINT32 &count)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      count = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      count = _env.dms.getCSCount();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::createCS(IExecutor *executor,
                              const CHAR *name,
                              const utilCSUniqueID &uniqueId,
                              const dmsCreateCSOptions &o,
                              const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      createCSHandler handler;
      collectionSpaceId identifier;
      strSlice csName;
      
      
      if (OSS_UNLIKELY(nullptr == executor ||
                       nullptr == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      csName.reset(name);

      rc = handler.doit(csName, uniqueId, o, adjunct, identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::removeCS(IExecutor *executor,
                              const CHAR *name)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      strSlice nameSlice(name);
      removeCSHandler handler;

      if (OSS_UNLIKELY(nullptr == executor ||
                       nameSlice.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      rc = handler.doit(nameSlice);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::testCS(IExecutor *executor,
                            const CHAR *name,
                            utilCSUniqueID &uniqueId)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      collectionSpaceId identifier;

      if (OSS_UNLIKELY(nullptr == executor ||
                       nullptr == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _env.dms.testCS(strSlice(name), identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }

      uniqueId = identifier.getUniqueId();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::testCS(IExecutor *executor,
                            utilCSUniqueID uniqueId)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      collectionSpaceId identifier;

      if (OSS_UNLIKELY(nullptr == executor ||
                       !UTIL_IS_VALID_CSUNIQUEID(uniqueId)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _env.dms.testCS(uniqueId, identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listCS(IExecutor *executor,
                            DATA_CURSOR_PTR &cursor)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      listCSCursor *listCursor = nullptr;
      cursor.reset();

      if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      cursor = makeSharedPtrFromPool<listCSCursor>();
      if (!cursor)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      listCursor = static_cast<listCSCursor *>(cursor.get());
      rc = listCursor->open(this);
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

   INT32 vesselImpl::createCL(IExecutor *executor,
                              const CHAR *fullName,
                              utilCLUniqueID uniqueId,
                              const dmsCreateCLOptions &o,
                              const bson::BSONObj &adjunct)
{
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      createCLHandler handler;

      if (OSS_UNLIKELY(!isOpen() ||
                       nullptr == fullName))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      

      rc = handler.doit(fullName, uniqueId, o, adjunct);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::removeCL(IExecutor *executor,
                              const CHAR *fullName,
                              const dmsRemoveCLOptions &o)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      globalCollectionId gcid;

      if (OSS_UNLIKELY(!isOpen() ||
                       nullptr == fullName))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      {
         openCLHandler handler;
         
         rc = handler.doit(fullName, gcid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      {
         removeCLHandler handler;
         
         rc = handler.doit(gcid, o);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listCL(IExecutor *executor,
                            const CHAR *csName,
                            DATA_CURSOR_PTR &cursor)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      listCLCursor *listCursor = nullptr;
      collectionSpaceId identifier;
      cursor.reset();

      if (OSS_UNLIKELY(nullptr == executor || nullptr == csName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _env.dms.testCS(strSlice(csName), identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }

      cursor = makeSharedPtrFromPool<listCLCursor>();
      if (OSS_UNLIKELY(!cursor))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      listCursor = static_cast<listCLCursor *>(cursor.get());

      rc = listCursor->open(this);
      if (SDB_OK != rc)
      {
         goto error;
      }

      listCursor->setCollectionSpace(identifier);
   done:
      return rc;
   error:
      cursor.reset();
      goto done;
   }

   INT32 vesselImpl::getCLCount(IExecutor *executor,
                                const CHAR *csName,
                                UINT32 &count)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      requestContext context;
      collectionSpace *obj = nullptr;
      count = 0;

      if (OSS_UNLIKELY(nullptr == executor || nullptr == csName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _env.dms.getCSByName(&context, strSlice(csName), SHARED, &obj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      count = obj->getCollectionCount();
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::openCL(IExecutor *executor,
                            const CHAR *fullName,
                            const dmsOpenCLOptions &o,
                            DATA_COLLECTION_PTR &ptr)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      openCLHandler h;
      globalCollectionId gcid;
      collectionHandler *clHandler = nullptr;

      ptr.reset();
      
      if (OSS_UNLIKELY(nullptr == executor ||
                       nullptr == fullName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = h.doit(fullName, gcid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ptr = makeSharedPtrFromPool<collectionHandler>();
      if (!ptr)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      clHandler = static_cast<collectionHandler *>(ptr.get());
      *clHandler = collectionHandler(this, gcid);
   done:
      return rc;
   error:
      ptr.reset();
      goto done;
   }

   INT32 vesselImpl::openCL(IExecutor *executor,
                            utilCLUniqueID uniqueId,
                            const dmsOpenCLOptions &o,
                            DATA_COLLECTION_PTR &ptr)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      openCLHandler h;
      globalCollectionId gcid;
      collectionHandler *clHandler = nullptr;

      ptr.reset();

      if (OSS_UNLIKELY(nullptr == executor ||
                       !UTIL_IS_VALID_CLUNIQUEID(uniqueId)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = h.doit(uniqueId, gcid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ptr = makeSharedPtrFromPool<collectionHandler>();
      if (!ptr)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      clHandler = static_cast<collectionHandler *>(ptr.get());
      *clHandler = collectionHandler(this, gcid);
   done:
      return rc;
   error:
      ptr.reset();
      goto done;
   }

   INT32 vesselImpl::testCL(IExecutor *executor,
                            const CHAR *fullName,
                            utilCLUniqueID &uniqueId)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      openCLHandler h;
      globalCollectionId gcid;

      uniqueId = UTIL_UNIQUEID_NULL;

      if (OSS_UNLIKELY(nullptr == executor ||
                       nullptr == fullName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = h.doit(fullName, gcid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      uniqueId = gcid.getUniqueId();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::testCL(IExecutor *executor,
                            utilCLUniqueID uniqueId)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 vesselImpl::pushMoreToCursor(IExecutor *executor,
                                      cursorKernal *cursor)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      if (OSS_UNLIKELY(nullptr == executor ||
                       nullptr == cursor ||
                       !cursor->isOpen() ||
                       CURSOR_TYPE_INVALID == cursor->getType()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      switch (cursor->getType())
      {
      case CURSOR_TYPE_LIST_COLLECTION_SPACE:
      {
         listCollectionSpaceHandler handler;
         

         rc = handler.doit(static_cast<listCSCursor*>(cursor));
         if (SDB_OK != rc)
         {
            goto error;
         }

         break;
      }
      case CURSOR_TYPE_LIST_COLLECTION:
      {
         listCollectionsHandler handler;
         

         rc = handler.doit(static_cast<listCLCursor*>(cursor));
         if (SDB_OK != rc)
         {
            goto error;
         }
         break;
      }
      case CURSOR_TYPE_SCAN_COLLECTION:
      {
         scanCLHandler handler;
         
         rc = handler.doit(static_cast<scanCLCursor*>(cursor));
         if (SDB_OK != rc)
         {
            goto error;
         }
         break;
      }
      case CURSOR_TYPE_INDEX_SCAN:
      {
         indexScanHandler handler;
         

         rc = handler.doit(static_cast<indexScanCursor*>(cursor));
         if (SDB_OK != rc)
         {
            goto error;
         }
         break;
      }
      case CURSOR_TYPE_LIST_LOBC:
      {
         lobChunkHandler handler;
         rc = handler.list(static_cast<listLobChunkCursor *>(cursor));
         if (SDB_OK != rc)
         {
            goto error;
         }
         break;
      }
      default:
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "unknown cursor type:%d", cursor->getType());
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::createIndex(IExecutor *executor,
                                 const globalCollectionId &gcid,
                                 const dmsBuildIndexOptions &o,
                                 const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      createIndexHandler handler;
      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      

      rc = handler.doit(gcid, o, adjunct);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listIndexes(IExecutor *executor,
                                 const globalCollectionId &gcid,
                                 ossPoolVector<bson::BSONObj> &indexes)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      collectionSpace *cs = nullptr;
      collection *cl = nullptr;
      requestContext context;

      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _env.dms.getCSByLogicalID(&context, gcid.getCSLid(), SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionById(&context, gcid.getCLIdentifier(),
                                 SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->listIndexes(&context, indexes);
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

   INT32 vesselImpl::removeIndex(IExecutor *executor,
                                 const globalCollectionId &gcid,
                                 const CHAR *indexName)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      removeIndexHandler handler;

      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid() ||
                       nullptr == indexName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      
      rc = handler.doit(gcid, indexName);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::testIndex(IExecutor *executor,
                               const globalCollectionId &gcid,
                               const strSlice &indexName,
                               indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      testIndexHandler handler;

      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid() ||
                       indexName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      
      rc = handler.doit(gcid, indexName, indexId);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::insert(IExecutor *executor,
                            const globalCollectionId &gcid,
                            const dmlInsertRequest &request,
                            utilInsertResult *res)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      dmlHandler handler;
      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid() ||
                       !request.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.insert(gcid, request, res);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::insertBatch(IExecutor *executor,
                                 const globalCollectionId &gcid,
                                 const dmlBatchInsertRequest &request,
                                 utilInsertResult *res)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      dmlHandler handler;
      if (OSS_UNLIKELY(nullptr == executor ||
                       !request.isValid() ||
                       !gcid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init handler:%d", rc);
         goto error;
      }

      rc = handler.insertBatch(gcid, request, res);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::update(IExecutor *executor,
                            const globalCollectionId &gcid,
                            const dmlUpdateRequest &request,
                            IRecordUpdater *updater,
                            utilUpdateResult *res)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      dmlHandler handler;

      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid() ||
                       !request.isValid() ||
                       nullptr == updater))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      rc = handler.update(gcid, request, updater, res);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::remove(IExecutor *executor,
                            const globalCollectionId &gcid,
                            const dmlRemoveRequest &request,
                            utilDeleteResult *res)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      dmlHandler handler;

      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid() ||
                       !request.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      rc = handler.remove(gcid, request, res);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::count(IExecutor *executor,
                           const globalCollectionId &gcid,
                           UINT64 &count)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      countCLHandler handler;

      count = 0;

      if (OSS_UNLIKELY(nullptr == executor ||
                       !gcid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      

      rc = handler.doit(gcid, count);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void vesselImpl::fini()
   {
      if (_open)
      {
         _open = FALSE;
         _env.hitMgr.fini();
         _env.lobcBufferPool.fini();
         _env.ioBufferPool.fini();
         _env.workers.fini();  
         _env.checkpointer.fini();
         _env.dms.close();
         _env.latchEnv.lpidLatchMap.fini();
         _env.latchEnv.ridLatchMap.fini();
         _env.latchEnv.uniqueIndexLathMap.fini();
         _env.latchEnv.lobcLatchMap.fini();
         _env.latchEnv.lobRegionLatchVec.fini();
         _env.spaceLocker.fini();
         _env.options = openDBOptions();
         if (nullptr != _env.lsm)
         {
            if (_env.lsm->isOpen())
            {
               _env.lsm->close();
            }
            SDB_OSS_DEL _env.lsm;
            _env.lsm = nullptr;
         }
         _env.resource.reset();
      }
      return;
   }

   INT32 vesselImpl::truncate(IExecutor *executor,
                              const globalCollectionId &gcid,
                              const dmsTruncateCLOptions &o)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      truncateCLHandler handler;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == executor ||
                            !gcid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      rc = handler.doit(gcid, o);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::initLsmDB(const openDBOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr == _env.lsm, "do not reinit");
      rocksdb::Status status;
      rocksdb::Options opt;
      if (options.path.lsmPath.empty())
      {
         PD_LOG(PDERROR, "lsm db path is empty");
         rc = SDB_INVALIDARG;
         goto error;
      }

      _env.lsm = SDB_OSS_NEW lsmDB();
      if (OSS_UNLIKELY(nullptr == _env.lsm))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = _env.lsm->open(options.path.lsmPath.c_str(),
                          &options.lsmOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open lsm db under path[%s]",
                options.path.lsmPath.c_str());
         goto error;
      }
   done:
      return rc;
   error:
      if (nullptr != _env.lsm)
      {
         SDB_OSS_DEL _env.lsm;
         _env.lsm = nullptr;
      }
      goto done;
   }

   INT32 vesselImpl::insertLobChunk(IExecutor *executor,
                                    const globalCollectionId &gcid,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    const CHAR *data)
   {
      INT32 rc = SDB_OK;
      lobChunkHandler handler;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = handler.insert(gcid, oid, chunkId,
                          offset, size, data);
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

   INT32 vesselImpl::readLobChunk(IExecutor *executor,
                                  const globalCollectionId &gcid,
                                  const bson::OID &oid,
                                  UINT32 chunkId,
                                  UINT32 offset,
                                  UINT32 size,
                                  CHAR *data,
                                  UINT32 &readSize)
   {
      INT32 rc = SDB_OK;
      lobChunkHandler handler;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = handler.read(gcid, oid, chunkId, offset,
                        size, data, readSize);
      if (SDB_OK != rc)
      {
         if (SDB_LOB_SEQUENCE_NOT_EXIST != rc)
         {
            PD_LOG(PDERROR, "failed to read lob chunk:%d", rc);
         }
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::removeLobChunk(IExecutor *executor,
                                    const globalCollectionId &gcid,
                                    const bson::OID &oid,
                                    UINT32 chunkId)
   {
      INT32 rc = SDB_OK;
      lobChunkHandler handler;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = handler.remove(gcid, oid, chunkId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove lob chunk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::updateLobChunk(IExecutor *executor,
                                    const globalCollectionId &gcid,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    const CHAR *data,
                                    BOOLEAN createIfNotExists)
   {
      INT32 rc = SDB_OK;
      lobChunkHandler handler;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = handler.update(gcid, oid, chunkId, offset,
                          size, data, createIfNotExists);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update lob chunk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::truncateLobChunk(IExecutor *executor,
                                      const globalCollectionId &gcid,
                                      const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 size,
                                      UINT32 &tsize)
   {
      INT32 rc = SDB_OK;
      lobChunkHandler handler;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = handler.truncate(gcid, oid, chunkId, size, tsize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate lob chunk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::testLobChunk(IExecutor *executor,
                                  const globalCollectionId &gcid,
                                  const bson::OID &oid,
                                  UINT32 chunkId,
                                  dmsLobChunkProfile *profile)
   {
      INT32 rc = SDB_OK;
      lobChunkHandler handler;
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      if (OSS_UNLIKELY(nullptr == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = handler.test(gcid, oid, chunkId, profile);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::activeBackgroundThreads(const openDBOptions &options)
   {
      INT32 rc = SDB_OK;

      backgroundWorkers::options o;
      o.maxWorkerNum = options.cacheCleanerCount;
      rc = _env.workers.init(&_env, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init background workers:%d", rc);
         goto error;
      }

      rc = _env.resource.executorPool->startEDU(EDU_TYPE_VESSEL_LITE_BUFFER_POOL_WATCHER,
                                                this);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to active lite buffer pool watcher:%d", rc);
         goto error;
      }
      _env.ioBufferPool.waitUntilWatcherAttached();

      rc = _env.resource.executorPool->startEDU(EDU_TYPE_VESSEL_LOBC_BUFFER_POOL_WATCHER,
                                                this);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to active lobc buffer pool watcher:%d", rc);
         goto error;
      }

      _env.lobcBufferPool.waitUntilWatcherAttached();

      rc = _env.resource.executorPool->startEDU(EDU_TYPE_VESSEL_HIT_MANAGER,
                                                this);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to active hit index manager :%d", rc);
         goto error;
      }
      _env.hitMgr.waitForAttaching();
   done:
      return rc;
   error:
      goto done;
   }

   void vesselImpl::attachLobcWatcher(IExecutor *executor)
   {
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(_env.lobcBufferPool.isValid(), "can not be invalid");
      THREAD_CONTEXT_OWNER tco(executor, &_env);

      _env.lobcBufferPool.attachWatcher();
   }

   void vesselImpl::attachLiteBufferPoolWatcher(IExecutor *executor)
   {
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(_env.ioBufferPool.isValid(), "can not be invalid");
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      _env.ioBufferPool.watcherAttach();
   }

   void vesselImpl::attachHitManager(IExecutor *executor)
   {
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(_env.ioBufferPool.isValid(), "can not be invalid");
      THREAD_CONTEXT_OWNER tco(executor, &_env);
      _env.hitMgr.attach();
   }

   INT32 vesselImpl::flushLsmDB()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _env.lsm->flush();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to flush lsm db:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel
} // namespace engine