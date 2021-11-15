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
                              const ossSharedLatchMode &mode,
                              liteCache *pool,
                              BOOLEAN isWritingPrepared)
   {
      INT32 rc = SDB_OK;
      release();

      if (NULL == tag ||
          mode.isNone() ||
          NULL == pool)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isWritingPrepared &&
               (!tag->hasMemPage() ||
                !mode.isExclusive()))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pool = pool;
      if (isWritingPrepared)
      {
         OSS_BIT_SET(_flags, TUPLE_FLAG_WRITING_PREPARED);
      }
      _holder = lcPageTagHolder(tag, mode);

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
      else if (_holder.getLockMode().isNone()||
               _holder.getLockMode().isShared())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (_holder.getLockMode().isUpgrade())
      {
         _holder.unlockUpgradeAndLock();
      }

      if (!_holder.tag()->isInLruList())
      {
         rc = _pool->allocateMemPageAndInsertIntoLRU(context, TRUE, _holder);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         _pool->tryToUpdateLRU(_holder);
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

      if (_holder.tag()->hasMemPage())
      {
         ptr = _holder.tag()->getMemPage().getBuf();
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
                       !isWritingPrepared()))
      {
         SDB_ASSERT(FALSE, "not valid or writing prepared");
         goto done;
      }

      SDB_ASSERT(_holder.tag()->hasMemPage(), "impossible");
      ptr = _holder.tag()->getMemPage().getBuf();
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
} /// end of namesapce vessel
} /// end of namespace engine