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

   Source File Name = collectionHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/api/collectionHandler.h"
#include "vessel/insertOptions.h"
#include "vessel/vesselImpl.h"
#include "vessel/api/IQueryFilter.h"
#include "vessel/scanCLCursor.h"
#include "vessel/indexScanCursor.h"

namespace engine
{
namespace vessel
{
   INT32 collectionHandler::createIndex(IExecutor *executor,
                                        const strSlice &indexName,
                                        const bson::BSONObj &keyPattern,
                                        const indexParameters &params,
                                        const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _db->createIndex(executor, _handle, indexName,
                            keyPattern, params, options);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::listIndexes(IExecutor *executor,
                                        ossPoolVector<bson::BSONObj> &indexes)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _db->listIndexes(executor, _handle, indexes);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::insert(IExecutor *executor,
                                   const slice &record,
                                   STRIPING_ID striping,
                                   const insertOptions &options,
                                   utilInsertResult *res)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == executor ||
               !record.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->insert(executor, _handle, record,
                       striping, options, res);
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
                                        const requestBatch &batch,
                                        const insertOptions &options,
                                        utilInsertResult *res)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == executor ||
               batch.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->insertBatch(executor, _handle, batch, options, res);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
 
   INT32 collectionHandler::openScanCursor(IExecutor *executor,
                                           IQueryFilter *filter,
                                           const collectionScanOptions &o,
                                           cursorHandler &cursor)
   {
      INT32 rc = SDB_OK;
      scanCLCursor *kernal = NULL;

      if (OSS_UNLIKELY(NULL == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      kernal = SDB_OSS_NEW scanCLCursor();
      if (NULL == kernal)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to allocate mem");
         goto error;
      }

      rc = kernal->open(_db, filter, &(o.cursor));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open cursor:%d", rc);
         goto error;
      }

      kernal->resetToScan(_handle, o);

      cursor = cursorHandler(kernal);
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(kernal);
      goto done;
   }
   

   INT32 collectionHandler::getTotalRecordCountInPageHead(IExecutor *executor,
                                                          UINT64 &count)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->getTotalRecordCountInPageHead(executor, _handle, count);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::openIndexScanCursor(IExecutor *executor,
                                                const strSlice &indexName,
                                                const rtnPredicateList &predicate,
                                                const indexScanOptions &o,
                                                cursorHandler &cursor)
   {
      INT32 rc = SDB_OK;
      indexScanCursor *kernal = NULL;
      if (OSS_UNLIKELY(NULL == executor ||
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

      kernal = SDB_OSS_NEW indexScanCursor(o, predicate, _handle, indexName);
      if (OSS_UNLIKELY(NULL == kernal))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = kernal->open(_db, NULL, &(o.cursor));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open cursor kernal:%d", rc);
         goto error;
      }

      cursor = cursorHandler(kernal);
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(kernal);
      goto done;
   }
}//namespace vessel
}//namespace engine