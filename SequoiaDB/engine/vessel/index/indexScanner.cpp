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

   Source File Name = indexScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanner.h"
#include "vessel/indexDef.h"
#include "vessel/requestContext.h"
#include "vessel/indexIterator.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/indexScanContext.h"
#include "vessel/objectLatchHelper.hpp"
#include "vessel/instanceEnv.h"
#include "rtnPredicate.hpp"
#include "vessel/indexContext.h"
#include "vessel/indexScanCursor.h"
#include "vessel/indexUtils.h"
#include "rtnPredicate.hpp"
#include "vessel/runtimeMbContext.h"

namespace engine
{
namespace vessel
{
   indexScanner::~indexScanner()
   {
      close();
   }

   INT32 indexScanner::open(indexScanContext *context,
                            indexContext *ic)
   {
      INT32 rc = SDB_OK;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       !context->isCursorAttached() ||
                       NULL == ic ||
                       !ic->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      _iterator = createIndexIterator(ic->getObj().getIndexType());
      if (NULL == _iterator)
      {
         PD_LOG(PDERROR, "failed to create new itr obj");
         rc = SDB_OOM;
         goto error;
      }

      rc = _iterator->open(context, ic);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open iterator of index[%s], rc:%d",
                ic->getObj().getIndexName().str(), rc);
         goto error;
      }

      _ic = ic;
      _o = context->getCursor()->getOptions();

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void indexScanner::close()
   {
      if (NULL != _iterator)
      {
         _iterator->close();
         SDB_OSS_DEL _iterator;
         _iterator = NULL;
      }
      _ic = NULL;
      _o = indexScanOptions();
      _keyBuilder.reset();
      return;
   }

   INT32 indexScanner::batchNext(indexScanContext *context,
                                 rowBatch &entryBatch)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       !context->isCursorAttached() ||
                       !entryBatch.isEmpty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = beginToScan(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to begin to scan:%d", rc);
         goto error;
      }

      if (!_iterator->isReadyToRead())
      {
         rc = SDB_IXM_EOC;
         goto error;
      }

      rc = fillBatch(context, entryBatch);
      if (SDB_OK != rc)
      {
         if (SDB_IXM_EOC != rc)
         {
            PD_LOG(PDERROR, "failed to fill batch:%d", rc);
         }
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 indexScanner::fillBatch(indexScanContext *context,
                                 rowBatch &entryBatch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != context && context->isMbContextAttached(), "can not be invalid");
      SDB_ASSERT(entryBatch.isEmpty(), "must be empty");
      SDB_ASSERT(context->getMbContext()->getRidLatchContext().isEmpty(), "must be empty");
      SDB_ASSERT(entryBatch.isFreeToPush(0), "should be free to push");

      rtnPredicateListIterator *predicate = context->getCursor()->getPredicate();
      SDB_ASSERT(NULL != predicate, "can not be null");
      indexScanCursor *cursor = context->getCursor();
      SDB_ASSERT(NULL != cursor, "can not be null");
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      static constexpr UINT32 _MAX_ROW_COUNT = 1024;
      UINT32 pushed = 0;

      do
      {
         SDB_ASSERT(_iterator->isReadyToRead(), "must be ready to read");
         UINT32 entrySize = _iterator->getCurrentEntrySize();
         if (!entryBatch.isFreeToPush(entrySize))
         {
            SDB_ASSERT(!entryBatch.isEmpty(), "row buffer size may be to small");
            break;
         }

         _keyBuilder.reset();
         bson::BSONObj keyObj = _iterator->getKeyObj(&_keyBuilder);
         INT32 res = predicate->advance(keyObj);
         if (-2 == res)
         {
            break;
         }
         else if (0 <= res)
         {
            indexIterator::options o(predicate->after(), _o.forward);
            rc = _iterator->fastNext(keyObj, rc, predicate->cmp(),
                                     predicate->inc(), o);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to reseek key:%d", rc);
               goto error;
            }

            continue;
         }
         else
         {
            recordID rid = _iterator->getRid();

            if (!cursor->testRidScanned(rid))
            {
               BOOLEAN locked = FALSE;
               rc = context->tryLockRid(rid, mode, locked);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to try lock rid:%d", rc);
                  goto error;
               }
               else if (locked)
               {
                  rc = _iterator->pushCurrentEntryToBatch(entryBatch);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to add entry to batch:%d", rc);
                     goto error;
                  }
                  
                  ++pushed;
                  cursor->markRidScanned(rid);
               }
               else if (!entryBatch.isEmpty())
               {
                  /// return without waiting for rid latch.
                  break;
               }
               else
               {
                  rc = pauseAndRescan(context);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to pause and rescan:%d", rc);
                     goto error;
                  }
                  continue;
               }
            }

            if (!entryBatch.isFreeToPush(0) ||
                _MAX_ROW_COUNT == pushed)
            {
               break;
            }

            /// rid duplicated or pushed into batch
            rc = moveIterator();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to move iterator:%d", rc);
               goto error;
            }
         }
      } while( _iterator->isReadyToRead());

      if (entryBatch.isEmpty())
      {
         rc = SDB_IXM_EOC;
         goto error;
      }

   done:
      return rc;
   error:
      if (!entryBatch.isEmpty())
      {
         entryBatch.clearRows();
         context->unlockRids();
      }
      goto done;
   }

   INT32 indexScanner::pauseAndRescan(indexScanContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_iterator->isReadyToRead(), "can not be invalid");

      BOOLEAN timeout = FALSE;
      recordID rid = _iterator->getRid();
      _iterator->pause();

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      
      rc = waitRid(context, rid, mode, timeout);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to wait rid latch:%d", rc);
         goto error;
      }
      else if (timeout)
      {
         PD_LOG(PDERROR, "timeout to wait latch obj[%d,%d], mb[%d,%d]",
                  rid.getPageID(), rid.getSlotID(),
                  context->getSpaceID(), context->getMBID());
         rc = SDB_VESSEL_LATCH_OBJ_BUSY;
         goto error;
      }

      rc = beginToScan(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to begin to scan:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::waitRid(indexScanContext *context,
                               const recordID &rid,
                               const ossSharedLatchMode &mode,
                               BOOLEAN &timeout)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(mode.isShared() || mode.isExclusive(), "can not be invalid");

      objectLatchHelper<recordIdLatchKey> lh;
      RECORD_ID_LATCH_MAP &lm = context->getEnv()->ridLatchMap;

      recordIdLatchKey key(context->getSpaceID(),
                           context->getMBID(),
                           rid);

      timeout = FALSE;

      lh.testNotExistsOrWait(lm, key, mode);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::beginToScan(indexScanContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->isCursorAttached(), "must be attached");
      

      if (context->getCursor()->hasEntry())
      {
         slice entry = context->getCursor()->getEntryData();
         rc = _iterator->moveToTheNextOfEntry(entry, _o.forward);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek entry:%d", rc);
            goto error;
         }
      }
      else
      {
         const rtnPredicateListIterator *predicate = context->getCursor()->getPredicate();
         indexIterator::options o(TRUE, _o.forward);

         rc = _iterator->seek(bson::BSONObj(),
                              0, predicate->cmp(),
                              predicate->inc(), o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek predicate:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::moveIterator()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(_iterator->isReadyToRead(), "must be ready to read");
      rc = _iterator->next(_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get next:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel   
} // namespace vessel
