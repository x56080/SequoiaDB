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

   Source File Name = indexScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexScanner.h"
#include "vessel/indexDef.h"
#include "vessel/requestContext.h"
#include "vessel/indexIterator.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/objectLatchHelper.hpp"
#include "vessel/instanceEnv.h"
#include "rtnPredicate.hpp"
#include "vessel/indexObject.h"
#include "vessel/indexScanCursor.h"
#include "vessel/indexUtils.h"
#include "rtnPredicate.hpp"
#include "vessel/indexScanCursor.h"

namespace engine
{
namespace vessel
{
   indexScanner::~indexScanner()
   {
      close();
   }

   INT32 indexScanner::open(indexScanCursor *cursor,
                            INDEX_ITERATOR_UPTR &&iterator)
   {
      INT32 rc = SDB_OK;  
      close();

      if (OSS_UNLIKELY(nullptr == cursor ||
                       !cursor->isOpen() ||
                       !iterator))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      _cursor = cursor;
      _iterator = std::move(iterator);
   done:
      return rc;
   error:
      goto done;
   }

   void indexScanner::close()
   {
      _cursor = nullptr;
      _iterator.reset();
      _scanning = FALSE;
      _keyBuilder.reset();
      return;
   }

   INT32 indexScanner::next(requestContext *context)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!_scanning)
      {
         rc = _beginToScan();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (_iterator->isReadyToRead())
      {
         rc = _iterator->next();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch next from iterator:%d", rc);
            goto error;
         }
      }

      rc = _fetchNextAndLock(context);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 indexScanner::saveLocation()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen() || !_iterator->isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _iterator->initOrUpdateLocation(_cursor->getCtx().getLocation());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update location info:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::_pauseUntilRidReady(requestContext *context,
                                           const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(_iterator->isReadyToRead(), "can not be invalid");

      rc = _iterator->pause();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to pause iterator:%d", rc);
         goto error;
      }

      rc = _waitRecord(context, rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to wait record:%d", rc);
         goto error;
      }

      rc = _iterator->resume();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to resume iterator:%d", rc);
         goto error;
      }

      rc = _beginToScan();
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::_tryLockRecord(requestContext *context,
                                      const recordID &rid,
                                      BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      const dmsIndexScanOptions &o = _cursor->getOptions();
      locked = FALSE;

      if (DMS_SCAN_FOR::NONE == o.scanFor)
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
         rc = context->tryLockRid(rid, mode, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock rid:%d", rc);
            goto error;
         }
      }
      else if (DMS_SCAN_FOR::UPDATE == o.scanFor)
      {
         rc = context->tryAcquireTransLock(rid, DPS_TRANSLOCK_U, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to try get trans lock:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = context->tryAcquireTransLock(rid, DPS_TRANSLOCK_S, locked);
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

   INT32 indexScanner::_waitRecord(requestContext *context,
                                   const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      
      const dmsIndexScanOptions &o = _cursor->getOptions();

      if (DMS_SCAN_FOR::NONE == o.scanFor)
      {
         context->waitRid(rid, ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED));
      }
      else
      {
         DPS_TRANSLOCK_TYPE mode = DMS_SCAN_FOR::UPDATE == o.scanFor ?
                                   DPS_TRANSLOCK_U : DPS_TRANSLOCK_S;
         rc = context->waitTransLock(rid, mode);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to wait trans lock:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 indexScanner::_beginToScan()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      indexIterator::options o;
      o.forward = _cursor->getCtx().isForward();
      o.pointGetOptimized = _cursor->getCtx().isPointGet();

      if (_cursor->getCtx().getLocation())
      {
         rc = _iterator->locateNext(_cursor->getCtx().getLocation().get(), o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate entry:%d", rc);
            goto error;
         }
      }
      else
      {
         const rtnPredicateListIterator *predicate = _cursor->getCtx().getPredicate();
         SDB_ASSERT(!o.pointGetOptimized || predicate->inc().allInclusive(), "impossible");
         rc = _iterator->seek(predicate->cmp(), predicate->inc(), o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek predicate:%d", rc);
            goto error;
         }
      }

      _scanning = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::_fetchNextAndLock(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_scanning, "must be scanning");
      rtnPredicateListIterator *predicate = _cursor->getCtx().getPredicate();
      SDB_ASSERT(nullptr != predicate, "can not be invalid");
      BOOLEAN fetched = FALSE;

      while (_iterator->isReadyToRead())
      {
         recordID rid = _iterator->getRid();
         if (_cursor->getCtx().testRidScanned(rid))
         {
            rc = _iterator->initOrUpdateLocation(_cursor->getCtx().getLocation());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update location info:%d", rc);
               goto error;
            }

            rc = _iterator->next();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fetch next:%d", rc);
               goto error;
            }
            continue;
         }
         else
         {
            _keyBuilder.reset();
            bson::BSONObj keyObj = _iterator->getKeyObj(FALSE, &_keyBuilder);

            INT32 res = predicate->advance(keyObj);
            if (-2 == res)
            {
               break;
            }
            else if (0 <= res)
            {
               rc = _iterator->advance(keyObj, rc,
                                       predicate->cmp(),
                                       predicate->inc());
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reseek key:%d", rc);
                  goto error;
               }

               continue;
            }
            else
            {
               BOOLEAN locked = FALSE;
               rc = _tryLockRecord(context, rid, locked);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to try lock rid:%d", rc);
                  goto error;
               }
               else if (locked)
               {
                  fetched = TRUE;
                  break;
               }
               else
               {
                  rc = _pauseUntilRidReady(context, rid);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to pause and rescan:%d", rc);
                     goto error;
                  }

                  continue;
               }
            }
         }
      } //while (_iterator->isReadyToRead());

      if (!fetched)
      {
         rc = SDB_IXM_EOC;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel   
} // namespace vessel
