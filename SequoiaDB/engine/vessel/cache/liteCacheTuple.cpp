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

   Source File Name = liteCacheTuple.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/liteCacheTuple.h"
#include "vessel/liteCache.h"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/liteCachePageTag.h"
#include "vessel/lcPageTagHolder.h"

namespace engine
{
namespace vessel
{
   liteCacheTuple::liteCacheTuple(const liteCacheTuple &t):
   _tag(NULL),
   _pool(NULL),
   _status(NULL)
   {
      if (t.isValid())
      {
         _tag = t._tag;
         _pool = t._pool;
         _status = t._status;
         _status->incSharedCnt();
      }
   }

   liteCacheTuple &liteCacheTuple::operator=(const liteCacheTuple &t)
   {
      release();
      if (t.isValid())
      {
         _tag = t._tag;
         _pool = t._pool;
         _status = t._status;
         _status->incSharedCnt();
      }
      return *this;
   }

   INT32 liteCacheTuple::init(liteCachePageTag *tag,
                              OSS_SHARED_LATCH_MODE mode,
                              liteCache *pool,
                              BOOLEAN isWritingPrepared)
   {
      INT32 rc = SDB_OK;
      release();

      if (NULL == tag ||
          OSS_SHARED_LATCH_MODE_NONE == mode ||
          NULL == pool)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isWritingPrepared &&
               (!tag->hasMemPage() ||
                OSS_SHARED_LATCH_MODE_EXCLUSIVE != mode))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _status = SDB_OSS_NEW liteCacheTuple::_sharedStatus();
      if (NULL == _status)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _tag = tag;
      _pool = pool;
      _status->setLockingMode(mode);
      if (isWritingPrepared)
      {
         _status->setWritingPrepared();
      }
      _status->incSharedCnt();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCacheTuple::prepareToWrite(requestContext *context)
   {
      INT32 rc = SDB_OK;      
      if (OSS_UNLIKELY(!isValid()))
      {
         SDB_ASSERT(FALSE, "must be valid");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_status->isWritingPrepared())
      {
         goto done;
      }
      else if (OSS_SHARED_LATCH_MODE_NONE == _status->getLockingMode() ||
               OSS_SHARED_LATCH_MODE_SHARED == _status->getLockingMode())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_SHARED_LATCH_MODE_UPGRADE == _status->getLockingMode())
      {
         _tag->getAccessingLatch().unlockUpgradeAndLock();
         _status->setLockingMode(OSS_SHARED_LATCH_MODE_EXCLUSIVE);
      }

      if (!_tag->isInLruList())
      {
         lcPageTagHolder holder(_tag, _status->getLockingMode());
         rc = _pool->allocateMemPageAndInsertIntoLRU(context, FALSE, holder);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         lcPageTagHolder holder(_tag, _status->getLockingMode());
         _pool->tryToUpdateLRU(holder);
      }

      _status->setWritingPrepared();

   done:
      return rc;
   error:
      goto done;
   }

   ossValuePtr liteCacheTuple::getReadableBuffer()const
   {
      ossValuePtr ptr = 0;
      if (OSS_UNLIKELY(!isValid()))
      {
         goto done;
      }

      if (_tag->hasMemPage())
      {
         ptr = _tag->getMemPage().buf();
      }
      else
      {
         ptr = _tag->getDiskPagePtr();
      }

   done:
      return ptr;
   }

   ossValuePtr liteCacheTuple::getWritableBuffer()const
   {
      ossValuePtr ptr = 0;
      if (OSS_UNLIKELY(!isValid() ||
                       !_status->isWritingPrepared()))
      {
         SDB_ASSERT(FALSE, "not valid or writing prepared");
         goto done;
      }

      SDB_ASSERT(_tag->hasMemPage(), "impossible");
      ptr = _tag->getMemPage().buf();
   done:
      return ptr;
   }

   void liteCacheTuple::release()
   {
      if (isValid())
      {
         if (0 == _status->decSharedCnt())
         {
            lcPageTagHolder holder(_tag, _status->getLockingMode());
            holder.autoUnlock();
            _tag->decUsageCnt();
            _tag = NULL;
            _pool = NULL;
            SDB_OSS_DEL _status;
            _status = NULL;

         }
         else
         {
            _tag = NULL;
            _pool = NULL;
            _status = NULL;
         }
      }
      return;
   }

   void liteCacheTuple::commit(UINT64 lsn)
   {
      if (isValid() && _status->isWritingPrepared())
      {
         _pool->commit(lsn, *this);
      }
   }


} /// end of namesapce vessel
} /// end of namespace engine