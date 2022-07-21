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

   INT32 indexScanner::_pauseUntilRidReady(requestContext *context,
                                           const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(_iterator->isReadyToRead(), "can not be invalid");
      IDX_ENTRY_LOCATION_UPTR location;

      rc = _iterator->pause(location);
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
   
      rc = _iterator->resume(location.get());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to resume iterator:%d", rc);
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

      if (DMS_SCAN_FOR_NONE == o.scanFor)
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
         rc = context->tryLockRid(rid, mode, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock rid:%d", rc);
            goto error;
         }
      }
      else if (DMS_SCAN_FOR_UPDATE == o.scanFor)
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

      if (DMS_SCAN_FOR_NONE == o.scanFor)
      {
         context->waitRid(rid, ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED));
      }
      else
      {
         DPS_TRANSLOCK_TYPE mode = DMS_SCAN_FOR_UPDATE == o.scanFor ?
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

      if (_cursor->getLocation())
      {
         const indexEntryLocation *location = _cursor->getLocation().get();
         if (location->hitTheEnd())
         {
            rc = SDB_IXM_EOC;
            goto error;
         }

         rc = _iterator->locate(location);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate entry:%d", rc);
            goto error;
         }
      }
      else
      {
         const rtnPredicateListIterator *predicate = _cursor->getPredicate();
         rc = _iterator->seek(predicate->cmp(), predicate->inc());
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
      rtnPredicateListIterator *predicate = _cursor->getPredicate();
      SDB_ASSERT(nullptr != predicate, "can not be invalid");
      IDX_ENTRY_LOCATION_UPTR &location = _cursor->getLocation();
      BOOLEAN fetched = FALSE;

      while (_iterator->isReadyToRead())
      {
         recordID rid = _iterator->getRid();
         if (_cursor->testRidScanned(rid))
         {
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
                  rc = _iterator->initOrUpdateLocation(location);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to update entry location:%d", rc);
                     goto error;
                  }

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
