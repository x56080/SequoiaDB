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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
      lcExtentTag *tag = NULL;
      SDB_ASSERT(valid(), "must be valid");
      SDB_ASSERT(!_writingPrepared, "multiple prepared");

      if (OSS_UNLIKELY(!valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_writingPrepared))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.getLockMode() < LOCK_MODE_UPGRADE))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (LOCK_MODE_UPGRADE == _holder.getLockMode())
      {
         _holder.lockUniqueFromUpgrade();
      }

      tag = _holder.tag();
      if (!tag->inLruList())
      {
         rc = _pool->allocateMemPageAndInsertIntoLRU(context, _holder);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(tag->hasMemPage(), "must have mem page when in lru");
         rc = _pool->tryToUpdateLRU(_holder);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      _writingPrepared = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCacheTuple::getReadPtr(UINT32 offset, UINT32 len, const CHAR **ptr)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(valid(), "can not read invalid tuple");
      SDB_ASSERT(LOCK_MODE_NONE < _holder.getLockMode(), "can not read a unlocked tuple");
      const lcExtentTag *tag = NULL;

      if (OSS_UNLIKELY(NULL == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateRead(offset, len);
      if (SDB_OK != rc)
      {
         goto error;
      }

      tag = _holder.tag();
      if (!tag->hasMemPage())
      {
         *ptr = (const CHAR *)(tag->getDiskPagePtr() + offset);
      }
      else
      {
         const freeListPage &page = tag->getMemPage();
         *ptr = (const CHAR *)(page.buf() + offset);
      }
  
   done:
      return rc;
   error:
      ptr = NULL;
      goto done;
   }

   INT32 liteCacheTuple::read(UINT32 offset, UINT32 len, CHAR *buf)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(valid(), "can not read invalid tuple");
      SDB_ASSERT(LOCK_MODE_NONE < _holder.getLockMode(), "can not read a unlocked tuple");
      const lcExtentTag *tag = NULL;

      if (OSS_UNLIKELY(NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateRead(offset, len);
      if (SDB_OK != rc)
      {
         goto error;
      }

      tag = _holder.tag();
      if (!tag->hasMemPage())
      {
         ossMemcpy(buf, (const void *)(tag->getDiskPagePtr() + offset), len);
      }
      else
      {
         const void *pageBuf = (const void *)(tag->getMemPage().buf());
         readPage(pageBuf, offset, len, buf);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCacheTuple::getWritePtr(UINT32 offset, UINT32 len, CHAR **ptr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(valid(), "can not write invalid tuple");
      SDB_ASSERT(_writingPrepared, "must be prepared");
      SDB_ASSERT(LOCK_MODE_UNIQUE == _holder.getLockMode(), "wrong type locking");
      lcExtentTag *tag = _holder.tag();

      if (OSS_UNLIKELY(NULL == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateWrite(offset, len);
      if (SDB_OK != rc)
      {
         goto done;
      }

      if (OSS_UNLIKELY(!_writingPrepared))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      tag = _holder.tag();
      *ptr = (CHAR *)(tag->getMemPage().buf()+ offset);
   
   done:
      return rc;
   error:
      ptr = NULL;
      goto done;
   }

   INT32 liteCacheTuple::write(UINT32 offset,
                               UINT32 len,
                               const CHAR *data)
   {
      INT32 rc = SDB_OK;
      lcExtentTag *tag = NULL;
      SDB_ASSERT(valid(), "can not write invalid tuple");
      SDB_ASSERT(_writingPrepared, "must be prepared");
      SDB_ASSERT(LOCK_MODE_UNIQUE == _holder.getLockMode(), "wrong type locking");

      if (OSS_UNLIKELY(NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateWrite(offset, len);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (OSS_UNLIKELY(!_writingPrepared))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tag = _holder.tag();
      writePage((void *)(tag->getMemPage().buf()), offset, len, data);

   done:
      return rc;
   error:
      goto done;
   }

   void liteCacheTuple::writePage(void *pageBuf,
                                  UINT32 offset,
                                  UINT32 len,
                                  const void *data)
   {
      SDB_ASSERT(NULL != pageBuf, "can not be null");
      SDB_ASSERT(NULL != data, "can not be null");
      CHAR *ptr = (CHAR *)pageBuf;
      ossMemcpy(ptr + offset, data, len);
      return;
   }

   void liteCacheTuple::readPage(const void *pageBuf,
                                 UINT32 offset,
                                 UINT32 len,
                                 void *buf)const
   {
      SDB_ASSERT(NULL != pageBuf, "can not be null");
      SDB_ASSERT(NULL != buf, "can not be null");
      const CHAR *ptr = (const CHAR *)pageBuf;
      ossMemcpy(buf, ptr + offset, len);
      return;
   }

   INT32 liteCacheTuple::validateRead(UINT32 offset, UINT32 len)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == len))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.getLockMode() < LOCK_MODE_SHARED))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getPageSize() < (offset+len)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getPageSize() <= offset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == _holder.tag()->getDiskPagePtr()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCacheTuple::validateWrite(UINT32 offset, UINT32 len)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == len))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.getLockMode() != LOCK_MODE_UNIQUE))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getPageSize() < (offset+len)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getPageSize() <= offset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!_holder.tag()->hasMemPage()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == _holder.tag()->getDiskPagePtr()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCacheTuple::getMinLSN(DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(valid(), "must be valid");
      SDB_ASSERT(LOCK_MODE_SHARED <= _holder.getLockMode(), "wrong type locking");
      if (OSS_UNLIKELY(!valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lsn = _holder.tag()->getMinLSN();
   done:
      return rc;
   error:
      goto done;
   }

} /// end of namesapce vessel
} /// end of namespace engine