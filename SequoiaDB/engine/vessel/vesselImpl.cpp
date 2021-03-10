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
#include "vessel/listCSCursor.h"
#include "vessel/listCLCursor.h"
#include "vessel/diskIOJob.h"
#include "vessel/cursorKernal.h"

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

   INT32 vesselImpl::initOuterResource(const outerResource &outer)
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
      rc = _env.spaceLocker.init(MAX_SPACE_COUNT);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _env.csContainer.open(&context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open space container:%d", rc);
         goto error;
      }

      rc = _env.cache.init(options.cacheOptions, &(_env.csContainer));
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
         _env.cache.fini();
         _env.csContainer.close(&context);
         _env.spaceLocker.fini();
         _env.options = openDBOptions();
         _open = FALSE;
         context.close();
      }
   done:
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

      rc = listCursor->open(this, filter, NULL);
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

      count = _env.csContainer.getCSCount();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 vesselImpl::createCollectionSpace(ISession *session,
                                           const CHAR *name,
                                           utilCSUniqueID uniqueID,
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

      rc = handler.doit(name, uniqueID, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      handler.fini();
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
      handler.fini();
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

      rc = _env.csContainer.testCS(&context, strSlice(csName),
                                   DMS_INVALID_LOGICCLID, logicalID, sid);
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

      rc = listCursor->open(this, filter, NULL);
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

      rc = _env.csContainer.getCLCount(&context, csName, count);
      if (SDB_OK != rc)
      {
         goto error;
      }
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

      rc = h.doit(strSlice(csName), strSlice(clName), options, handler);
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

         handler.fini();

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

         handler.fini();
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

   INT32 vesselImpl::insert(ISession *session,
                            const collectionHandle &handle,
                            const recordData &record,
                            const DPS_TRANS_ID &transID,
                            STRIPING_ID striping,
                            const insertOptions *options,
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
   done:
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

} // namespace vessel
} // namespace engine