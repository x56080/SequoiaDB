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

namespace engine
{
namespace vessel
{
   indexScanner::~indexScanner()
   {
      close();
   }

   INT32 indexScanner::open(requestContext *context,
                            indexContext *ic,
                            const indexScanOptions &o,
                            const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == ic ||
                       !ic->isValid() ||
                       mode.isNone()))
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
      _forward = o.forward;
      _mode = mode;

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
      _forward = TRUE;
      _mode.setNone();
      _keyBuilder.reset();
      return;
   }

   INT32 indexScanner::batchNext(indexScanContext *context)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen() ||
                       !context->isCursorAttached()))
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

      rc = fillBatch(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fill batch:%d", rc);
         goto error;
      }

      if (context->getBatch().isEmpty())
      {
         rc = SDB_IXM_EOC;
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 indexScanner::fillBatch(indexScanContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(0 == context->getBatch().getEntryCount(), "must be empty");
      SDB_ASSERT(context->getRidLatchContext().isEmpty(), "must be empty");
      SDB_ASSERT(!_mode.isNone(), "can not be none");
      SDB_ASSERT(_iterator->isReadyToRead(), "must be ready to read");

      UNORDERED_RID_SET *ridSet = context->getRidSet();
      SDB_ASSERT(NULL != ridSet, "can not be null");
      UINT32 maxBatchSize = context->getCursor()->getOptions().stepLength;
      rtnPredicateListIterator *predicate = context->getCursor()->getPredicate();
      SDB_ASSERT(NULL != predicate, "can not be null");

      do
      {
         _keyBuilder.reset();
         ixmKey key(_iterator->getKey().data());
         bson::BSONObj keyObj = key.toBson(&_keyBuilder);
         INT32 res = predicate->advance(keyObj);
         if (-2 == res)
         {
            break;
         }
         else if (0 <= res)
         {
            indexIterator::options o(predicate->after(), _forward);
            rc = _iterator->advanceTo(keyObj, rc, predicate->cmp(),
                                      predicate->inc(), o);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to reseek key:%d", rc);
               goto error;
            }
         }
         else
         {
            recordID rid = _iterator->getRid();

            if (0 == ridSet->count(rid))
            {
               BOOLEAN locked = FALSE;
               rc = context->tryLockRid(rid, _mode, locked);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to try lock rid:%d", rc);
                  goto error;
               }
               else if (locked)
               {
                  rc = _iterator->pushCurrentEntryToBatch(context->getBatch());
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to add entry to batch:%d", rc);
                     goto error;
                  }
                  ridSet->insert(rid);
               }
               else if (0 < context->getBatch().getEntryCount())
               {
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

            /// rid duplicated or pushed into batch
            rc = moveIterator();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to move iterator:%d", rc);
               goto error;
            }
         }
      } while( _iterator->isReadyToRead() &&
               context->getBatch().getEntryCount() < maxBatchSize);

   done:
      return rc;
   error:
      context->clearBatchAndRidLatch();
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
      
      rc = waitRid(context, rid, _mode, timeout);
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
      SDB_ASSERT(!mode.isNone(), "can not be invalid");

      objectLatchHelper<recordIdLatchKey> lh;
      RECORD_ID_LATCH_MAP &lm = context->getEnv()->ridLatchMap;

      INT32 millis = 1000;
      INT32 totalMillis = 30000;
      recordIdLatchKey key(context->getLogicalCSID(),
                           context->getLogicalCLID(),
                           rid);
      timeout = FALSE;

      for (INT32 i = 0; i < totalMillis; i += millis)
      {
         if (lh.testNotExistsOrWait(lm, key, mode, millis))
         {
            goto done;
         }
         else if (context->getSession()->quit())
         {
            PD_LOG(PDERROR, "session[%lld] quit", context->getSession()->getSessionID());
            rc = SDB_APP_INTERRUPT;
            goto error;
         }
         else
         {
            continue;
         }
      }

      timeout = TRUE;
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
         indexIterator::options o(FALSE, _forward);
         rc = _iterator->seekEntry(context->getCursor()->getEntryData(), o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek entry:%d", rc);
            goto error;
         }
      }
      else
      {
         const rtnPredicateListIterator *predicate = context->getCursor()->getPredicate();
         indexIterator::options o(TRUE, _forward);

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
      if (_forward)
      {
         rc = _iterator->next();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _iterator->prev();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get prev:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel   
} // namespace vessel
