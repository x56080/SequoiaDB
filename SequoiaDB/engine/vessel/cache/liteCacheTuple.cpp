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
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/outerResource.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   INT32 liteCacheTuple::prepareToWrite(requestContext *context)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *tag = NULL;
      
      
      if (OSS_UNLIKELY(!isValid()))
      {
         SDB_ASSERT(FALSE, "must be valid");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_writingPrepared)
      {
         goto done;
      }
      else if (ossSharedLatch::NONE == _holder.getLockMode() ||
               ossSharedLatch::SHARED == _holder.getLockMode())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (ossSharedLatch::UPGRADE == _holder.getLockMode())
      {
         _holder.unlockUpgradeAndLock();
      }

      tag = _holder.tag();
      if (!tag->isInLruList())
      {
         rc = _pool->allocateMemPageAndInsertIntoLRU(context, FALSE, _holder);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         _pool->tryToUpdateLRU(_holder);
      }

      _writingPrepared = TRUE;

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

      if (_holder.tag()->hasMemPage())
      {
         ptr = _holder.tag()->getMemPage().buf();
      }
      else
      {
         ptr = _holder.tag()->getDiskPagePtr();
      }

   done:
      return ptr;
   }

   ossValuePtr liteCacheTuple::getWritableBuffer()const
   {
      ossValuePtr ptr = 0;
      if (OSS_UNLIKELY(!isValid() ||
                       !_writingPrepared))
      {
         goto done;
      }

      SDB_ASSERT(_holder.tag()->hasMemPage(), "impossible");
      ptr = _holder.tag()->getMemPage().buf();
   done:
      return ptr;
   }

   void liteCacheTuple::release()
   {
      if (isValid())
      {
         _holder.autoUnlock();
         _holder.tag()->decUsageCnt();
         _holder.reset(NULL);
         _pool = NULL;
         _writingPrepared = FALSE;
      }
      return;
   }

   void liteCacheTuple::commit(UINT64 lsn)
   {
      if (isValid())
      {
         _pool->commit(lsn, *this);
      }
   }

   void liteCacheTuple::swap(liteCacheTuple &o)
   {
      liteCacheTuple tmp;
      tmp._holder = _holder;
      tmp._pool = _pool;
      tmp._writingPrepared = _writingPrepared;

      _holder = o._holder;
      _pool = o._pool;
      _writingPrepared = o._writingPrepared;

      o._holder = tmp._holder;
      o._pool = tmp._pool;
      o._writingPrepared = tmp._writingPrepared;
      return;
   }

} /// end of namesapce vessel
} /// end of namespace engine