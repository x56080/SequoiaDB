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
#include "vessel/indexObject.h"
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

   INT32 indexScanner::open(indexScanContext *context,
                            indexObject *obj)
   {
      INT32 rc = SDB_OK;
      indexIterator::options o(FALSE, 
                               context->getCursor()->getOptions().forward);   
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       !context->isCursorAttached() ||
                       NULL == obj ||
                       !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      _context = context;
      _iterator = createIndexIterator(obj->getProperties().getType());
      if (NULL == _iterator)
      {
         PD_LOG(PDERROR, "failed to create new itr obj");
         rc = SDB_OOM;
         goto error;
      }

      rc = _iterator->open(context, obj, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open iterator of index[%s], rc:%d",
                obj->getProperties().getName().c_str(), rc);
         goto error;
      }

      _obj = obj;

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void indexScanner::close()
   {
      _context = NULL;
      if (NULL != _iterator)
      {
         _iterator->close();
         SDB_OSS_DEL _iterator;
         _iterator = NULL;
      }
      _obj = NULL;
      _keyBuilder.reset();
      return;
   }

   INT32 indexScanner::batchNext(rowBatch &entryBatch)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = beginToScan();
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

      rc = fillBatch(entryBatch);
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

   INT32 indexScanner::fillBatch(rowBatch &entryBatch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != _context, "can not be invalid");
      SDB_ASSERT(entryBatch.isEmpty(), "must be empty");
      SDB_ASSERT(entryBatch.isFreeToPush(0), "should be free to push");

      rtnPredicateListIterator *predicate = _context->getCursor()->getPredicate();
      SDB_ASSERT(NULL != predicate, "can not be null");
      indexScanCursor *cursor = _context->getCursor();
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
            indexIterator::seekOptions o(predicate->after());
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
               rc = tryLockRecord(rid, locked);
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
                  rc = pauseAndRescan();
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to pause and rescan:%d", rc);
                     goto error;
                  }

                  if (!_iterator->isReadyToRead())
                  {
                     break;
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
         _context->unlockRids();
      }
      goto done;
   }

   INT32 indexScanner::pauseAndRescan()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != _context, "can not be null");
      SDB_ASSERT(_iterator->isReadyToRead(), "can not be invalid");

      recordID rid = _iterator->getRid();

      _iterator->pause();
      rc = waitRecord(rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to wait record:%d", rc);
         goto error;
      }
   
      rc = beginToScan();
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

   INT32 indexScanner::tryLockRecord(const recordID &rid, BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      const dmsIndexScanOptions &o = _context->getCursor()->getOptions();
      locked = FALSE;

      if (DMS_SCAN_FOR_NONE == o.scanFor)
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
         rc = _context->tryLockRid(rid, mode, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock rid:%d", rc);
            goto error;
         }
      }
      else if (DMS_SCAN_FOR_UPDATE == o.scanFor)
      {
         rc = _context->tryAcquireTransLock(rid, DPS_TRANSLOCK_U, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to try get trans lock:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _context->tryAcquireTransLock(rid, DPS_TRANSLOCK_S, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to try get trans lock:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::waitRecord(const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      
      const dmsIndexScanOptions &o = _context->getCursor()->getOptions();

      if (DMS_SCAN_FOR_NONE == o.scanFor)
      {
         _context->waitRid(rid, ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED));
      }
      else if (DMS_SCAN_FOR_UPDATE == o.scanFor)
      {
         rc = _context->acquireTransLock(rid, DPS_TRANSLOCK_U);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to acquire trans lock:%d", rc);
            goto error;
         }

         _context->releaseTransLock(rid);
      }
      else
      {
         rc = _context->acquireTransLock(rid, DPS_TRANSLOCK_S);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to acquire trans lock:%d", rc);
            goto error;
         }

         _context->releaseTransLock(rid);
      }
      
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 indexScanner::beginToScan()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _context, "can not be null");

      if (_context->getCursor()->hasEntry())
      {
         slice entry = _context->getCursor()->getEntryData();
         rc = _iterator->moveToTheNextOfEntry(entry);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek entry:%d", rc);
            goto error;
         }
      }
      else
      {
         const rtnPredicateListIterator *predicate = _context->getCursor()->getPredicate();
         indexIterator::seekOptions o(TRUE);

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
      rc = _iterator->next();
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
