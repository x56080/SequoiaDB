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
#include "vessel/ISession.h"
#include "vessel/handlers.h"
#include "vessel/IQueryFilter.h"
#include "vessel/listCSCursor.h"
#include "vessel/listCLCursor.h"
#include "vessel/diskIOJob.h"
#include "vessel/cursorKernal.h"
#include "vessel/scanCLCursor.h"
#include "vessel/collectionSpace.h"


namespace engine
{
namespace vessel
{
   vesselImpl::~vesselImpl()
   {
      fini();
   }

   INT32 vesselImpl::open(ISession *session,
                          const outerResource *resource,
                          const openDBOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reopen");
      requestContext context;

      if (NULL == session ||
          NULL == resource ||
          !resource->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.open(session, &_env, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }


      _open = TRUE;
      _outerResource.logger = resource->logger;
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

      rc = _env.dms.open(&context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open stoarge units:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 vesselImpl::close(ISession *session, const closeDBOptions &options)
   {
      INT32 rc = SDB_OK;
      if (isOpen() && options.flushDirtyList)
      {
         requestContext context;
         rc = context.open(session, &_env, &_outerResource);
         if (SDB_OK != rc)
         {
            goto error;
         }
         flushWholeDirtyList(&context);
         context.close();
      }
   done:
      fini();
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listCollectionSpace(ISession *session,
                                         IQueryFilter *filter,
                                         cursorHandler &cursor)
   {
      INT32 rc = SDB_OK;
      listCSCursor *listCursor = NULL;
      if (OSS_UNLIKELY(NULL == session))
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

      rc = listCursor->open(this, filter, cursorOptions());
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

   INT32 vesselImpl::getCollectionSpaceCount(ISession *session,
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

   INT32 vesselImpl::createCollectionSpace(ISession *session,
                                           const CHAR *name,
                                           utilCSUniqueID uniqueId,
                                           const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      createCSHandler handler;

      if (OSS_UNLIKELY(NULL == session ||
                       NULL == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.init(&_env, session, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = handler.doit(name, uniqueId, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::dropCollectionSpace(ISession *session,
                                         const CHAR *name,
                                         UINT32 logicalID,
                                         const dropCSOptions &options)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == session ||
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

   INT32 vesselImpl::createCollection(ISession *session,
                                        const CHAR *csName,
                                        const CHAR* clName,
                                        utilCLInnerID innerID,
                                        const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      createCLHandler handler;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.init(&_env, session, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

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

   INT32 vesselImpl::listCollections(ISession *session,
                                     const CHAR *csName,
                                     IQueryFilter *filter,
                                     cursorHandler &cursor)
   {
      INT32 rc = SDB_OK;
      listCLCursor *listCursor = NULL;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      SPACE_ID sid = INVALID_SPACE_ID;
      requestContext context;

      if (OSS_UNLIKELY(NULL == session || NULL == csName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.open(session, &_env, &_outerResource);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = _env.dms.testCS(&context, strSlice(csName),
                           logicalID, sid);
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

      rc = listCursor->open(this, filter, cursorOptions());
      if (SDB_OK != rc)
      {
         goto error;
      }

      listCursor->setCollectionSpace(logicalID, sid);

      cursor = cursorHandler(listCursor);

      listCursor = NULL;
   done:
      context.close();
      return rc;
   error:
      SAFE_OSS_DELETE(listCursor);
      goto done;
   }

   INT32 vesselImpl::getCollectionCount(ISession *session,
                                        const CHAR *csName,
                                        UINT32 &count)
   {
      INT32 rc = SDB_OK;
      requestContext context;
      collectionSpace *obj = NULL;

      if (OSS_UNLIKELY(NULL == session || NULL == csName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.open(session, &_env, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

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

   INT32 vesselImpl::openCollection(ISession *session,
                                    const CHAR *csName,
                                    const CHAR *clName,
                                    const openCLOptions &options,
                                    collectionHandler &handler)
   {
      INT32 rc = SDB_OK;
      openCLHandler h;
      
      if (OSS_UNLIKELY(NULL == session ||
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

      rc = h.init(&_env, session, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

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


   INT32 vesselImpl::pushMoreToCursor(ISession *session,
                                      cursorKernal *cursor)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == session ||
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
         rc = handler.init(&_env, session, &_outerResource);
         if (SDB_OK != rc)
         {
            goto error;
         }

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
         rc = handler.init(&_env, session, &_outerResource);
         if (SDB_OK != rc)
         {
            goto error;
         }

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
         rc = handler.init(&_env, session, &_outerResource);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = handler.doit(static_cast<scanCLCursor*>(cursor));
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

   INT32 vesselImpl::createIndex(ISession *session,
                                 const collectionHandle &handle,
                                 const strSlice &indexName,
                                 const indexKeyPattern &keyPattern,
                                 const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      createIndexHandler handler;
      if (OSS_UNLIKELY(NULL == session))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.init(&_env, session, &_outerResource);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init handler:%d", rc);
         goto error;
      }

      rc = handler.doit(handle, indexName, keyPattern, options);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::insert(ISession *session,
                            const collectionHandle &handle,
                            const slice &record,
                            const DPS_TRANS_ID &transID,
                            STRIPING_ID striping,
                            const insertOptions &options,
                            utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      insertHandler handler;
      if (OSS_UNLIKELY(NULL == session))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.init(&_env, session, &_outerResource);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init handler:%d", rc);
         goto error;
      }

      rc = handler.doit(handle, record, transID, striping, options, res);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::getTotalRecordCountInPageHead(ISession *session,
                                                   const collectionHandle &handle,
                                                   UINT64 &count)
   {
      INT32 rc = SDB_OK;
      countCLHandler handler;

      if (OSS_UNLIKELY(NULL == session))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.init(&_env, session, &_outerResource);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = handler.doit(handle, count);
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
         job.prepare(0, diskIOJob::DIRTY_LIST, scanDepth);

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
               task.done();
               PD_LOG(PDERROR, "failed to execute io task:%d", rc);
               goto error;
            }

            task.done();

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
         _env.checkpointer.fini();
         _env.cacheConsole.fini();
         _env.dms.close();
         _env.lpidLatchMap.fini();
         _env.ridLatchMap.fini();
         _env.uniqueIndexLathMap.fini();
         _env.spaceLocker.fini();
         _env.options = openDBOptions();
         _outerResource.logger = NULL;
      }
      return;
   }

} // namespace vessel
} // namespace engine