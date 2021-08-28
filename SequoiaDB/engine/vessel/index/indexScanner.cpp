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
#include "vessel/indexEntryBuffer.h"
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
                            const indexObject &indexObj)
   {
      INT32 rc = SDB_OK;
      indexHandle handle;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isScanning() ||
                       !indexObj.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      SDB_ASSERT(context->getHandle().getIndexId() == indexObj.getIndexID(), "must be same");

      _context = context;
      _iterator = createIndexIterator(indexObj.getIndexType());
      if (NULL == _iterator)
      {
         PD_LOG(PDERROR, "failed to create new itr obj");
         rc = SDB_OOM;
         goto error;
      }

      rc = _iterator->open(context, context->getHandle(),
                           indexObj.getPattern().getOrdering(),
                           context->isForward());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open iterator of index[%s], rc:%d",
                indexObj.getIndexName().str(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void indexScanner::close()
   {
      if (NULL != _context)
      {
         if (NULL != _iterator)
         {
            _iterator->close();
            SDB_OSS_DEL _iterator;
            _iterator = NULL;
         }
         if (!_rlc.isEmpty())
         {
            objectLatchHelper<recordIdLatchKey> lh;
            RECORD_ID_LATCH_MAP &lm = _context->getEnv()->ridLatchMap;
            lh.releaseAll(lm, _rlc);
         }
         _context = NULL;
         _seeked = FALSE;
         _builder.reset();
      }
      return;
   }

   INT32 indexScanner::next(recordID &rid)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!_seeked)
      {
         if (_context->getEntryBuffer()->isValid())
         {
            rc = prepareToScan(_context->getEntryBuffer()->getEntry());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to prepare scan by entry:%d", rc);
               goto error;
            }
         }
         else
         {
            rc = prepareToScan(_context->getPredicate());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to prepare scan by predicate:%d", rc);
               goto error;
            }
         }
      }
      else
      {
         rc = _iterator->nextDiffKeyOrRid();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next key or rid:%d", rc);
            goto error;
         }
      }

      if (!_iterator->isReadyToRead())
      {
         rc = SDB_IXM_EOC;
         goto error;
      }

      rc = matchCurrentOrSeekNext(rid);
      if (SDB_OK != rc)
      {
         if (SDB_IXM_EOC != rc)
         {
            PD_LOG(PDERROR, "failed to match or seek next:%d", rc);
         }
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 indexScanner::prepareToScan(rtnPredicateListIterator *predicate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(!_seeked, "can not be seeked");
      SDB_ASSERT(NULL != _iterator, "can not be null");

      rc = _iterator->seek(bson::BSONObj(), 0, FALSE,
                           predicate->cmp(), predicate->inc());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek predicate:%d", rc);
         goto error;
      }
      _seeked = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::prepareToScan(const slice &entry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(!_seeked, "can not be seeked");
      SDB_ASSERT(entry.isValid(), "must be valid");
      SDB_ASSERT(NULL != _iterator, "can not be null");
     
      rc = _iterator->seek(entry, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek last entry:%d", rc);
         goto error;
      }
      _seeked = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::matchCurrentOrSeekNext(recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != _iterator && _iterator->isReadyToRead(), "must be ready");
      rid = recordID();
      rtnPredicateListIterator *predicate = _context->getPredicate();
      UNORDERED_RID_SET *ridSet = _context->getRidSet();
      SDB_ASSERT(NULL != predicate, "can not be null");
      SDB_ASSERT(NULL != ridSet, "can not be null");

      do
      {
         if (_iterator->isMarkedRemoved())
         {
            rc = _iterator->nextDiffKeyOrRid();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get next key and rid:%d", rc);
               goto error;
            }
         }
         else
         {
            _builder.reset();
            ixmKey key;
            _iterator->getKey(key);
            bson::BSONObj keyObj = key.toBson(&_builder);
            INT32 res = predicate->advance(keyObj);
            if (-2 == res)
            {
               rc = SDB_IXM_EOC;
               goto error;
            }
            else if (0 <= res)
            {
               rc = _iterator->nextTo(keyObj, res,
                                      predicate->after(),
                                      predicate->cmp(),
                                      predicate->inc());
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get next tuple from iterator:%d", rc);
                  goto error;
               }
            }
            else
            {
               BOOLEAN locked = FALSE;
               recordID currentRid = _iterator->getRid();
               if (0 < ridSet->count(currentRid))
               {
                  rc = _iterator->nextDiffKeyOrRid();
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to get next tuple from iterator:%d, rc");
                     goto error;
                  }
                  continue;
               }

               rc = lockCurrentRid(locked);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to lock current rid:%d", rc);
                  goto error;
               }

               if (locked)
               {
                  rc = _iterator->copyKeyEntryToBuffer(*(_context->getEntryBuffer()));
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to copy key entry:%d", rc);
                     goto error;
                  }

                  rid = currentRid;
                  goto done;
               }
               else
               {
                  UINT32 millis = 60000; /// 1 mins
                  BOOLEAN timeout = FALSE;
                  _iterator->pause();
                  rc = waitCurrentRid(millis, timeout);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to wait latch obj:%d", rc);
                     goto error;
                  }
                  else if (timeout)
                  {
                     PD_LOG(PDERROR, "timeout to wait latch obj[%d,%d], mb[%d,%d]",
                            currentRid.getPageID(), currentRid.getSlotID(),
                            _context->getSpaceID(), _context->getMBID());
                     rc = SDB_VESSEL_LATCH_OBJ_BUSY;
                     goto error;
                  }

                  rc = _iterator->resume();
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to resume iterator:%d", rc);
                     goto error;
                  }
               }
            }
         }
      } while (_iterator->isReadyToRead());

      rc = SDB_IXM_EOC;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::lockCurrentRid(BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != _iterator && _iterator->isReadyToRead(), "must be ready");
      objectLatchHelper<recordIdLatchKey> lh;
      RECORD_ID_LATCH_MAP &lm = _context->getEnv()->ridLatchMap;
      ossSharedLatchMode mode;
      mode.setShared();
      recordID rid = _iterator->getRid();
      SDB_ASSERT(rid.valid(), "can not be invalid");
      recordIdLatchKey key(_context->getSpaceID(),
                           _context->getMBID(),
                           rid);
      rc = lh.tryLock(lm, _rlc, key, mode, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock rid:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanner::waitCurrentRid(UINT32 millis, BOOLEAN &timeout)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != _iterator && _iterator->isReadyToRead(), "must be ready");
      objectLatchHelper<recordIdLatchKey> lh;
      RECORD_ID_LATCH_MAP &lm = _context->getEnv()->ridLatchMap;
      ossSharedLatchMode mode;
      mode.setShared();
      recordID rid = _iterator->getRid();
      SDB_ASSERT(rid.valid(), "can not be invalid");
      recordIdLatchKey key(_context->getSpaceID(),
                           _context->getMBID(),
                           rid);

      rc = lh.waitFor(lm, key, mode, millis, timeout);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to wait rid latch:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel   
} // namespace vessel
