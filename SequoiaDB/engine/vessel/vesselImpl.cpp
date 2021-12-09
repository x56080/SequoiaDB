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

   Source File Name = vesselImpl.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/vesselImpl.h"
#include "pdTrace.hpp"
#include "vessel/globalControlFile.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/handlers.h"
#include "vessel/api/IQueryFilter.h"
#include "vessel/listCSCursor.h"
#include "vessel/listCLCursor.h"
#include "vessel/diskIOJob.h"
#include "vessel/cursorKernal.h"
#include "vessel/scanCLCursor.h"
#include "vessel/collectionSpace.h"
#include "vessel/spaceIDLockHelper.h"
#include "vessel/indexScanCursor.h"
#include "vessel/lsm/lsmDB.hpp"

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
      requestContext context;

      if (NULL == executor ||
          NULL == resource ||
          !resource->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(executor, &_env, &_outerResource);
      _open = TRUE;
      _outerResource = *resource;
      _env.options = options;
      VESSEL_FILE_GLOBAL_OPTIONS::setSparseExtending(options.sparseExtendingFile);

      rc = _env.spaceLocker.init();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _env.lpidLatchMap.init(options.lpidLatchMapBucketCount,
                                   options.lpidLatchMapLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid latch map:%d", rc);
         goto error;
      }

      rc = _env.ridLatchMap.init(options.ridLatchMapBucketCount,
                                  options.ridLatchMapLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rid latch map:%d", rc);
         goto error;
      }

      rc = _env.uniqueIndexLathMap.init(options.indexLatchMapBucketCount,
                                        options.indexLatchMapLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rid latch map:%d", rc);
         goto error;
      }

      rc = _env.cacheConsole.init32KBCache(options.cacheOptions);
      if (SDB_OK != rc)
      {
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
         PD_LOG(PDERROR, "failed to open stoarge units:%d", rc);
         goto error;
      }

      {
         backgroundWorkers::options o;
         o.cacheCleaner = options.cacheCleanerCount;
         o.commonWorker = options.commonBackgroundWorkers;
         rc = _env.workers.init(&_outerResource, &_env, o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init background workers:%d", rc);
            goto error;
         }
      }

      rc = _env.cacheWatcher.init(&_env, &_outerResource);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cache watcher:%d", rc);
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
         requestContext context;
         context.open(executor, &_env, &_outerResource);

         _env.cacheWatcher.fini();
         if (NULL != _env.lsm)
         {
            _env.lsm->closeLsmDB(TRUE, FALSE);
         }

         ///TODO: flush db by workers.
         flushWholeDirtyList(&context);
         _env.dms.createCheckpointBeforeClosing(&context);
         _env.workers.fini();
      }
   done:
      fini();
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listCollectionSpace(IExecutor *executor,
                                         IQueryFilter *filter,
                                         cursorHandler &cursor)
   {
      INT32 rc = SDB_OK;
      listCSCursor *listCursor = NULL;
      if (OSS_UNLIKELY(NULL == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      listCursor = SDB_OSS_NEW listCSCursor();
      if (NULL == listCursor)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = listCursor->open(this, filter);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      cursor = cursorHandler(listCursor);
      listCursor = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(listCursor);
      goto done;
   }

   INT32 vesselImpl::getCollectionSpaceCount(IExecutor *executor,
                                             UINT32 &count)
   {
      INT32 rc = SDB_OK;
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

   INT32 vesselImpl::createCollectionSpace(IExecutor *executor,
                                             const CHAR *name,
                                             utilCSUniqueID uniqueId,
                                             const createCSOptions &options)
   {
      collectionSpaceId identifier;
      return createCollectionSpace(executor, name, uniqueId, options, identifier);
   }

   INT32 vesselImpl::createCollectionSpace(IExecutor *executor,
                                           const CHAR *name,
                                           utilCSUniqueID uniqueId,
                                           const createCSOptions &options,
                                           collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;
      createCSHandler handler;

      if (OSS_UNLIKELY(NULL == executor ||
                       NULL == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      handler.init(&_env, executor, &_outerResource);

      rc = handler.doit(name, uniqueId, options, identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::testCollectionSpace(IExecutor *executor,
                                         const CHAR *name,
                                         collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;

      requestContext context;

      if (OSS_UNLIKELY(NULL == executor ||
                       NULL == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(executor, &_env, &_outerResource);
      rc = _env.dms.testCS(&context, strSlice(name), identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::dropCollectionSpace(IExecutor *executor,
                                         const CHAR *name,
                                         UINT32 logicalID,
                                         const dropCSOptions &options)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == executor ||
          NULL == name ||
          DMS_COLLECTION_SPACE_NAME_SZ < ossStrlen(name) ||
          DMS_INVALID_LOGICCSID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::createCollection(IExecutor *executor,
                                        const CHAR *csName,
                                        const CHAR* clName,
                                        utilCLInnerID innerID,
                                        const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      createCLHandler handler;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      handler.init(&_env, executor, &_outerResource);

      rc = handler.doit(strSlice(csName), strSlice(clName), innerID, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listCollections(IExecutor *executor,
                                     const CHAR *csName,
                                     IQueryFilter *filter,
                                     cursorHandler &cursor)
   {
      INT32 rc = SDB_OK;
      listCLCursor *listCursor = NULL;
      requestContext context;
      collectionSpaceId identifier;

      if (OSS_UNLIKELY(NULL == executor || NULL == csName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(executor, &_env, &_outerResource);
      rc = _env.dms.testCS(&context, strSlice(csName), identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }

      listCursor = SDB_OSS_NEW listCLCursor();
      if (NULL == listCursor)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = listCursor->open(this, filter);
      if (SDB_OK != rc)
      {
         goto error;
      }

      listCursor->setCollectionSpace(identifier.getLid(), identifier.getSpaceId());

      cursor = cursorHandler(listCursor);

      listCursor = NULL;
   done:
      context.close();
      return rc;
   error:
      SAFE_OSS_DELETE(listCursor);
      goto done;
   }

   INT32 vesselImpl::getCollectionCount(IExecutor *executor,
                                        const CHAR *csName,
                                        UINT32 &count)
   {
      INT32 rc = SDB_OK;
      requestContext context;
      collectionSpace *obj = NULL;

      if (OSS_UNLIKELY(NULL == executor || NULL == csName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(executor, &_env, &_outerResource);

      rc = _env.dms.getCSByName(&context, strSlice(csName), SHARED, &obj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      count = obj->getCollectionCount();
      context.unlockSpaceID();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::openCollection(IExecutor *executor,
                                    const CHAR *csName,
                                    const CHAR *clName,
                                    const openCLOptions &options,
                                    collectionHandler &handler)
   {
      INT32 rc = SDB_OK;
      openCLHandler h;
      
      if (OSS_UNLIKELY(NULL == executor ||
                       NULL == csName ||
                       NULL == clName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      h.init(&_env, executor, &_outerResource);

      rc = h.doit(this, strSlice(csName), strSlice(clName), options, handler);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 vesselImpl::pushMoreToCursor(IExecutor *executor,
                                      cursorKernal *cursor)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == executor ||
                       NULL == cursor ||
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
         handler.init(&_env, executor, &_outerResource);

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
         handler.init(&_env, executor, &_outerResource);

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
         handler.init(&_env, executor, &_outerResource);
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
         handler.init(&_env, executor, &_outerResource);

         rc = handler.doit(static_cast<indexScanCursor*>(cursor));
         if (SDB_OK != rc)
         {
            goto error;
         }
         break;
      }
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::createIndex(IExecutor *executor,
                                 const globalCollectionId &gcid,
                                 const strSlice &indexName,
                                 const bson::BSONObj &keyPattern,
                                 const indexParameters &params,
                                 const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      createIndexHandler handler;
      if (OSS_UNLIKELY(NULL == executor ||
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

      handler.init(&_env, executor, &_outerResource);

      rc = handler.doit(gcid, indexName, keyPattern, params, options);
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
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      requestContext context;
      spaceIDLockHelper lh(&context);

      if (OSS_UNLIKELY(NULL == executor ||
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

      context.open(executor, &_env, &_outerResource);
      rc = lh.lock(gcid.getSpaceId(), SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space id[%d], rc:%d", gcid.getSpaceId(), rc);
         goto error;
      }
      rc = _env.dms.getCSByLockedSpaceID(&context,
                                          gcid.getCSLid(),
                                          &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByMBID(&context, gcid.getMbId(),
                                   gcid.getCLLid(), SHARED, &cl);
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
      if (NULL != cl)
      {
         context.unlockMB();
      }
      lh.unlock();
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
      dmlHandler handler;
      if (OSS_UNLIKELY(NULL == executor ||
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

      handler.init(&_env, executor, &_outerResource);

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
      dmlHandler handler;
      if (OSS_UNLIKELY(NULL == executor ||
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

      handler.init(&_env, executor, &_outerResource);
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
      dmlHandler handler;

      if (OSS_UNLIKELY(NULL == executor ||
                       !gcid.isValid() ||
                       !request.isValid() ||
                       NULL == updater))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      handler.init(&_env, executor, &_outerResource);
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
      dmlHandler handler;

      if (OSS_UNLIKELY(NULL == executor ||
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

      handler.init(&_env, executor, &_outerResource);
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

   INT32 vesselImpl::getTotalRecordCountInPageHead(IExecutor *executor,
                                                   const globalCollectionId &gcid,
                                                   UINT64 &count)
   {
      INT32 rc = SDB_OK;
      countCLHandler handler;

      if (OSS_UNLIKELY(NULL == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      handler.init(&_env, executor, &_outerResource);

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

   INT32 vesselImpl::flushWholeDirtyList(requestContext *context)
   {
      INT32 rc = SDB_OK;
      
      UINT32 scanDepth = 128;
      diskIOJob job;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      do
      {
         /// we'd better dispath tasks to workers.
         rc = _env.cacheConsole.get32KBCache().createDirtyListIOJob(context, scanDepth,
                                                                    DPS_INVALID_LSN_OFFSET,
                                                                    &job);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create io job of dirty list:%d", rc);
            goto error;
         }

         if (0 == job.getTagCount())
         {
            break;
         }

         job.prepareForDispatching();

         do
         {
            diskIOTask task;
            BOOLEAN hitTheEnd = FALSE;
            rc = job.getNextTask(context, hitTheEnd, task);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get io task:%d", rc);
               goto error;
            }

            if (hitTheEnd)
            {
               break;
            }

            rc = _env.cacheConsole.get32KBCache().executeIOTask(context, &task);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to execute io task:%d", rc);
               goto error;
            }

         } while (TRUE);

         job.reset();
      }while(TRUE);
   done:
      return rc;
   error:
      job.abortUndispatchedTasks();
      goto done;
   }

   void vesselImpl::fini()
   {
      if (_open)
      {
         _open = FALSE;
         _env.cacheWatcher.fini();
         _env.workers.fini();  
         _env.checkpointer.fini();
         _env.cacheConsole.fini();
         _env.dms.close();
         _env.lpidLatchMap.fini();
         _env.ridLatchMap.fini();
         _env.uniqueIndexLathMap.fini();
         _env.spaceLocker.fini();
         _env.options = openDBOptions();
         if (NULL != _env.lsm)
         {
            if (_env.lsm->isDBOpened())
            {
               _env.lsm->closeLsmDB(FALSE, TRUE);
            }
            SDB_OSS_DEL _env.lsm;
            _env.lsm = NULL;
         }
         _outerResource.logger = NULL;
      }
      return;
   }

   INT32 vesselImpl::initLsmDB(const openDBOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _env.lsm, "do not reinit");
      rocksdb::Status status;
      LSMConfig conf;
      conf.createDBIfMissing = TRUE;
      if (options.path.lsmPath.empty())
      {
         PD_LOG(PDERROR, "lsm db path is empty");
         rc = SDB_INVALIDARG;
         goto error;
      }

      conf.dbPath = options.path.lsmPath;
      _env.lsm = SDB_OSS_NEW lsmDB();
      if (OSS_UNLIKELY(NULL == _env.lsm))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      _env.lsm->initLsmDB(conf);

      status = _env.lsm->openLsmDB();
      if (!status.ok())
      {
         PD_LOG(PDERROR, "failed to open lsm db under path[%s], info:[%s]",
                conf.dbPath.c_str(), status.ToString().c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != _env.lsm)
      {
         SDB_OSS_DEL _env.lsm;
         _env.lsm = NULL;
      }
      goto done;
   }
} // namespace vessel
} // namespace engine