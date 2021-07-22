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
   static const UINT16 TUPLE_FLAG_WRITING_PREPARED = 0x01;

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
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }

      _tag = tag;
      _pool = pool;
      if (isWritingPrepared)
      {
         OSS_BIT_SET(_flags, TUPLE_FLAG_WRITING_PREPARED);
      }
      _lockingMode = mode;

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN liteCacheTuple::isWritingPrepared()const
   {
      return 0 != OSS_BIT_TEST(_flags, TUPLE_FLAG_WRITING_PREPARED);
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
      else if (isWritingPrepared())
      {
         goto done;
      }
      else if (OSS_SHARED_LATCH_MODE_NONE == _lockingMode ||
               OSS_SHARED_LATCH_MODE_SHARED == _lockingMode)
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_SHARED_LATCH_MODE_UPGRADE == _lockingMode)
      {
         _tag->getAccessingLatch().unlockUpgradeAndLock();
         _lockingMode = OSS_SHARED_LATCH_MODE_EXCLUSIVE;
      }

      if (!_tag->isInLruList())
      {
         lcPageTagHolder holder(_tag, (OSS_SHARED_LATCH_MODE)_lockingMode);
         rc = _pool->allocateMemPageAndInsertIntoLRU(context, FALSE, holder);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         lcPageTagHolder holder(_tag, (OSS_SHARED_LATCH_MODE)_lockingMode);
         _pool->tryToUpdateLRU(holder);
      }

      OSS_BIT_SET(_flags, TUPLE_FLAG_WRITING_PREPARED);

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
                       !isWritingPrepared()))
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
         lcPageTagHolder holder(_tag, (OSS_SHARED_LATCH_MODE)_lockingMode);
         holder.autoUnlock();
         _tag->decUsageCnt();
         _tag = NULL;
         _pool = NULL;
         _lockingMode = OSS_SHARED_LATCH_MODE_NONE;
         _flags = 0;
      }
      return;
   }

   void liteCacheTuple::commit(UINT64 lsn)
   {
      if (isValid() && isWritingPrepared())
      {
         _pool->commit(lsn, *this);
      }
   }

   void liteCacheTuple::moveTo(liteCacheTuple &tuple)
   {
      tuple.release();
      if (isValid())
      {
         tuple._tag = _tag;
         tuple._pool = _pool;
         tuple._lockingMode = _lockingMode;
         tuple._flags = _flags;

         _tag = NULL;
         _pool = NULL;
         _lockingMode = OSS_SHARED_LATCH_MODE_NONE;
         _flags = 0;
      }
      return;
   }
} /// end of namesapce vessel
} /// end of namespace engine