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

   Source File Name = lcBucket.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lcBucket.h"
#include "ossErr.h"
#include "ossMem.hpp"

namespace engine
{
namespace vessel
{
   lcBucket::lcBucket()
   {

   }

   lcBucket::~lcBucket()
   {
      LC_BUCKET_INNER_INDEX_ITERATOR itr = _tags.begin();
      for (; itr != _tags.end(); ++itr)
      {
         if (NULL != itr->second)
         {
            SDB_OSS_DEL itr->second;
         }
      }
      _tags.clear();
   }

   INT32 lcBucket::ensureTagAndIncUsage(const GLOBAL_PAGE_ID &gpid,
                                        UINT32 minRecycleCount,
                                        const mmapPagePointer &ptr,
                                        lcPageTagHolder &holder,
                                        BOOLEAN &isNewTag)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!gpid.isValid() ||
                       !ptr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      holder.reset(NULL);

      if (getTagAndIncUsage(gpid, holder))
      {
         goto done;
      }

      rc = insertTag(gpid, ptr, minRecycleCount, holder);
      if (SDB_OK != rc)
      {
         goto error;
      }
      isNewTag = TRUE;

      SDB_ASSERT(holder.valid(), "must be valid");   
   done:
      return rc;
   error:
      holder.reset(NULL);
      goto done;
   }

   INT32 lcBucket::releaseRemovedTag(liteCachePageTag *tag)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != tag, "can not be null");

      if (NULL == tag)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!tag->isRemoved())
      {
         SDB_ASSERT(FALSE, "can not release the tag not removed");
         rc = SDB_INVALIDARG;
         goto error;
      }

      _tags.erase(tag->getBucketIterator());
      SDB_OSS_DEL tag;
      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lcBucket::getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                       lcPageTagHolder &holder)
   {  
      BOOLEAN r = FALSE;
      SDB_ASSERT(id.isValid(), "can not be invalid");
      holder.reset(NULL);
      LC_BUCKET_INNER_INDEX_CONST_ITERATOR lower = _tags.lower_bound(id);
      for (; lower != _tags.end(); ++lower)
      {
         if (lower->first != id)
         {
            break;
         }
         if (lower->second->incUsageCnt())
         {
            holder.reset(lower->second);
            r = TRUE;
            break;
         }
      }
      return r;
   }

   INT32 lcBucket::insertTag(const GLOBAL_PAGE_ID &id,
                             const mmapPagePointer &ptr,
                             UINT32 minRecycleCount,
                             lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *tag = NULL;
      SDB_ASSERT(id.isValid(), "can not be ivnalid");
      SDB_ASSERT(ptr.isValid(), "can not be invalid");
      LC_BUCKET_INNER_INDEX_ITERATOR itr;

      tag = recycleTag(minRecycleCount);
      if (NULL == tag)
      {
         tag = SDB_OSS_NEW liteCachePageTag();
         if (OSS_UNLIKELY(NULL == tag))
         {
            rc = SDB_OOM;
            goto error;
         }
      }

      tag->firstInit(id, ptr.get());
      tag->incUsageCnt(FALSE);
      itr = _tags.insert(std::make_pair(id, tag));
      tag->insertIntoBucket(itr);
      holder.reset(tag);
      tag = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_FREE(tag);
      goto done;
   }

   liteCachePageTag *lcBucket::recycleTag(UINT32 minRecycleCount)
   {
      liteCachePageTag *tag = NULL;
      if (_tags.size() < minRecycleCount)
      {
         return NULL;
      }

      LC_BUCKET_INNER_INDEX_ITERATOR itr = _tags.begin();
      for (; itr != _tags.end(); ++itr)
      {
         tag = itr->second;
         /// no latch holding, just for fast skip.
         if (!tag->fastTestIfCanBeRecycled(FALSE))
         {
            continue;
         }

         /// some one held the latch after our fast check.
         if (!tag->getAccessingLatch().tryLock())
         {
            continue;
         }

         /// check again under latch
         if (!tag->isNotInAnyList())
         {
            tag->getAccessingLatch().unlock();
            continue;
         }

         /// some one pinned tag but 
         /// failed to test because we did not hold latch. 
         if (!tag->tryToSetRemoved())
         {
            tag->getAccessingLatch().unlock();
            continue;
         }

         /// no one can access this tag after removed.
         tag->getAccessingLatch().unlock();
         _tags.erase(itr);
         tag->reset();
         
         break;
      }

      return tag;
   }
} /// end of namespace vessel
} /// end of namespace engine
