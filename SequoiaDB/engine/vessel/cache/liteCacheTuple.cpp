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
      if (tag->noChunkPages())
      {
         rc = _pool->allocateChunkPagesAndInsertIntoLRU(context, _holder);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
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

      rc = validateRWPtr(offset, len, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      tag = _holder.tag();
      if (tag->noChunkPages())
      {
         *ptr = (const CHAR *)(tag->getDiskPagePtr() + offset);
      }
      else
      {
         UINT32 pageOffset = offset % tag->getCachePageSize();
         const lcChunkPage *page = tag->pages() + (offset / tag->getCachePageSize());
         *ptr = (const CHAR *)(page->buf() + pageOffset);
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

      rc = validateRW(offset, len, buf, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      tag = _holder.tag();
      if (tag->noChunkPages())
      {
         ossMemcpy(buf, (const void *)(tag->getDiskPagePtr() + offset), len);
      }
      else
      {
         readPages(tag->getCachePageSize(), tag->pages(), offset, len, buf);
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
      lcChunkPage *page = NULL;
      UINT32 pageOffset = 0;

      if (OSS_UNLIKELY(NULL == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateRWPtr(offset, len, FALSE);
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
      if (OSS_UNLIKELY(tag->noChunkPages()))
      {
         PD_LOG(PDERROR, "writing prepared but no chunk pages");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      pageOffset = offset % tag->getCachePageSize();
      page = tag->pages() + (offset / tag->getCachePageSize());
      *ptr = (CHAR *)(page->buf() + pageOffset);
   
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

      rc = validateRW(offset, len, data, FALSE);
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
      if (OSS_UNLIKELY(tag->noChunkPages()))
      {
         PD_LOG(PDERROR, "writing prepared but no chunk pages");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      writePages(tag->getCachePageSize(), tag->pages(),
                 offset, len, data);

   done:
      return rc;
   error:
      goto done;
   }

   void liteCacheTuple::writePages(UINT32 pageSize, lcChunkPage *pages,
                                   UINT32 offset, UINT32 len, const CHAR *data)
   {
      UINT32 copySize = 0;
      do
      {
         UINT32 pageNO = (offset + copySize) / pageSize;
         lcChunkPage *page = pages + pageNO;
         UINT32 pageOffset = (offset + copySize) % pageSize;
         UINT32 cpLen = (pageSize - pageOffset) < len ? (pageSize - pageOffset) : len;
         ossMemcpy((void *)(page->buf() + pageOffset), data + copySize, cpLen);
         copySize += cpLen;

      } while(copySize < len);
      return;
   }

   INT32 liteCacheTuple::validateRW(UINT32 offset, UINT32 len, const CHAR *buf, BOOLEAN readonly)const
   {
      INT32 rc = SDB_OK;
      LOCK_MODE lm = readonly ? LOCK_MODE_SHARED : LOCK_MODE_UPGRADE;

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
      else if (OSS_UNLIKELY(NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.getLockMode() < lm))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getDiskPageSize() < (offset+len)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getDiskPageSize() <= offset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == _holder.tag()->getDiskPagePtr()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else
      {
         /// do nothing.
      }
      

   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCacheTuple::validateRWPtr(UINT32 offset, UINT32 len, BOOLEAN readonly)const
   {
      INT32 rc = SDB_OK;
      LOCK_MODE lm = readonly ? LOCK_MODE_SHARED : LOCK_MODE_UPGRADE;

      if (OSS_UNLIKELY(!valid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.getLockMode() < lm))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getDiskPageSize() < offset+len))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_holder.tag()->getDiskPageSize() <= offset))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == _holder.tag()->getDiskPagePtr()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else
      {
         UINT32 pageOffset = offset % _holder.tag()->getCachePageSize();
         if (OSS_UNLIKELY(_holder.tag()->getCachePageSize() < pageOffset + len))
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void liteCacheTuple::readPages(UINT32 pageSize, const lcChunkPage *pages,
                                  UINT32 offset, UINT32 len, CHAR *buf)const
   {
      UINT32 copySize = 0;
      do
      {
         UINT32 pageNO = (offset + copySize) / pageSize;
         const lcChunkPage *page = pages + pageNO;
         UINT32 pageOffset = (offset + copySize) % pageSize;
         UINT32 cpLen = (pageSize - pageOffset) < len ? (pageSize - pageOffset) : len;
         ossMemcpy(buf + copySize, (const void *)(page->buf() + pageOffset), cpLen);
         copySize += cpLen;

      } while(copySize < len);
      return;
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