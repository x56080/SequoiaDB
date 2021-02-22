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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
#include "vessel/cursorObject.h"
#include "vessel/listCSCursor.h"
#include "vessel/listCLCursor.h"
#include "vessel/ICursor.h"
#include "vessel/diskIOJob.h"
#include "vessel/collectionObject.h"

#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   vesselImpl::vesselImpl():
   _open(FALSE)
   {

   }

   vesselImpl::~vesselImpl()
   {

   }

   INT32 vesselImpl::setup(const outerResource &outer)
   {
      _outerResource = outer;
      return SDB_OK;
   }  

   INT32 vesselImpl::open(ISession *session, const openDBOptions &options)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;
      requestContext context;

      if (isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      rc = context.open(session, &_env, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _env.options = options;
      rc = _env.spaceLocker.setup(MAX_SPACE_COUNT);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _env.suContainer.open(&context, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _env.objContainer.setup();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initObjectContainer(session);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _env.cache.setup(options.cacheOptions, &(_env.suContainer));
      if (SDB_OK != rc)
      {
         goto error;
      }

      _open = TRUE;
   done:
      return rc;
   error:
      if (rollback)
      {
         close(session, closeDBOptions());
      }
      goto done;
   }

   INT32 vesselImpl::close(ISession *session, const closeDBOptions &options)
   {
      INT32 rc = SDB_OK;
      if (isOpen())
      {
         requestContext context;
         rc = context.open(session, &_env, &_outerResource);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = flushWholeDirtyList(&context);
         _env.cache.teardown();
         _env.objContainer.teardown();
         _env.suContainer.close(&context);
         _env.spaceLocker.teardown();
         _env.options = openDBOptions();
         _open = FALSE;
         context.close();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::fastGetCollectionSpaceCount(ISession *session,
                                                 UINT32 &count)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      count = _env.objContainer.getNameCountInIndex();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listCollectionSpace(ISession *session,
                                         IQueryFilter *filter,
                                         ICursor *cursor)
   {
      INT32 rc = SDB_OK;
      listCSCursor *listCursor = NULL;
      if (OSS_UNLIKELY(NULL == session || NULL == cursor))
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

      rc = listCursor->open(this, filter, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = cursor->setOpenedCursorObj(listCursor);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }
      listCursor = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(listCursor);
      goto done;
   }

   INT32 vesselImpl::createCollectionSpace(ISession *session,
                                           const CHAR *name,
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

      rc = handler.setup(&_env, session, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = handler.run(name, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      handler.teardown();
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
                                      UINT32 csLogicalID,
                                      const CHAR* clName,
                                      UINT32 clLogicalID,
                                      const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      createCLHandler handler;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.setup(&_env, session, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = handler.doit(csLogicalID, clName, clLogicalID, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      handler.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::listCollections(ISession *session,
                                     UINT32 csLogicalID,
                                     IQueryFilter *filter,
                                     ICursor *cursor)
   {
      INT32 rc = SDB_OK;
      listCLCursor *listCursor = NULL;
      if (OSS_UNLIKELY(NULL == session || NULL == cursor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      listCursor = SDB_OSS_NEW listCLCursor();
      if (NULL == listCursor)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      listCursor->setLogicalCSID(csLogicalID);
      rc = listCursor->open(this, filter, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cursor->setOpenedCursorObj(listCursor);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      listCursor = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(listCursor);
      goto done;
   }

   INT32 vesselImpl::openCollection(ISession *session,
                                    UINT32 csLogicalID,
                                    UINT32 clLogicalID,
                                    const openCLOptions &options,
                                    collectionObject *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj && !obj->isOpen(), "can not be invalid");
      requestContext context;
      SPACE_ID sid = INVALID_SPACE_ID;
      CL_MB_ID mid = INVALID_CL_MB_ID;
      SDB_ASSERT(NULL != obj && !obj->isOpen(), "impossible");

      if (OSS_UNLIKELY(NULL == session ||
                       DMS_INVALID_LOGICCSID == csLogicalID ||
                       DMS_INVALID_LOGICCLID == clLogicalID ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!options.notest)
      {
         rc = context.open(session, &_env, &_outerResource);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }

         rc = testCollection(&context,
                             csLogicalID,
                             clLogicalID,
                             sid, mid);
         if (SDB_OK != rc)
         {
            goto error;
         }

         context.close();

         rc = obj->open(this, csLogicalID, clLogicalID, sid, mid);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }
      else
      {
         rc = obj->open(this, csLogicalID, clLogicalID);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }      
   done:
      return rc;
   error:
      context.close();
      goto done;
   }

   INT32 vesselImpl::insert(ISession *session,
                            const collectionHandle *handle,
                            const slice &record,
                            const insertOptions &options)
   {
      INT32 rc = SDB_OK;
      insertHandler handler;
      if (OSS_UNLIKELY(NULL == session ||
                       NULL == handle ||
                       !handle->valid() ||
                       !record.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = handler.setup(&_env, session, &_outerResource);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = handler.doit(handle, record, options);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      handler.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::pushMoreToCursor(ISession *session,
                                      cursorObject *cursor)
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
         rc = handler.setup(&_env, session, &_outerResource);
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
         rc = handler.setup(&_env, session, &_outerResource);
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
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::initObjectContainer(ISession *session)
   {
      INT32 rc = SDB_OK;
      extentStorageUnit *su = NULL;
      requestContext context;
      SPACE_ID sid = _env.suContainer.getFirstSpaceIDWhenStartup();

      rc = context.open(session, &_env, &_outerResource);
      if (SDB_OK != rc)
      {
         goto error;
      }

      while (INVALID_SPACE_ID != sid)
      {
         collectionSpace *obj = NULL;
         /// unnecessary locking, but some code path require space id locked.
         rc = context.lockSpaceID(sid, SHARED);
         if (SDB_OK != rc)
         {
            goto error;
         }

         su = _env.suContainer.getNextSUWhenStartup(sid);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = _env.objContainer.allocateCSObjWhenStartup(&context, su, &obj);
         if (SDB_OK != rc)
         {
            goto error;
         }

         context.unlockSpaceID();
      }
   done:
      context.unlockSpaceID();
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::flushWholeDirtyList(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      diskIOJob job;
      diskIOTask task;

      rc = job.prepare(0, diskIOJob::DIRTY_LIST);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _env.cache.createWholeDirtyListIOJob(context, &job);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = job.prepareForDispatching();
      if (SDB_OK != rc)
      {
         goto error;
      }

      do
      {
         rc = job.getNextTask(task);
         if (SDB_VESSEL_END_OF_CURSOR == rc)
         {
            rc = SDB_OK;
            break;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         else
         {
            rc = _env.cache.executeIOTask(context, &task);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      job.abort();
      goto done;
   }

   INT32 vesselImpl::testCollection(requestContext *context,
                                    UINT32 cslid,
                                    UINT32 cllid,
                                    SPACE_ID &sid,
                                    CL_MB_ID &mid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel
} // namespace engine