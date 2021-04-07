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

#include "vessel/lcBucket.h"
#include "ossErr.h"
#include "ossMem.hpp"
#include "vessel/storageUnit.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   lcBucket::lcBucket()
   {

   }

   lcBucket::~lcBucket()
   {
      _TAG_MAP_ITERATOR itr = _tags.begin();
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
                                        _ossSpinSLatch *latch,
                                        UINT32 pageSize,
                                        UINT32 minRecycleCount,
                                        lcExtentTagHolder &holder,
                                        BOOLEAN &newTagInBucket)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      ossValuePtr ptr = 0;
      SDB_ASSERT(!gpid.invalid(), "can not be invalid");
      SDB_ASSERT(0 < pageSize, "can not be invalid");
      BOOLEAN tagLocked = FALSE;

      newTagInBucket = FALSE;
      holder.reset(NULL);
      ossScopedLock guard(latch);

      getTagAndIncUsage(gpid, NULL, holder);
      if (holder.valid())
      {
         goto done;
      }

      rc = insertTag(gpid, pageSize,
                     minRecycleCount, holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(holder.valid(), "must be valid");

      /// impossble to be failed
      holder.tag()->incUsageCnt();
      tagLocked = holder.tryLockUnique();
      SDB_ASSERT(tagLocked, "must be locked");
      newTagInBucket = TRUE;      
   done:
      return rc;
   error:
      holder.reset(NULL);
      goto done;
   }

   INT32 lcBucket::releaseRemovedTag(_ossSpinSLatch *latch,
                                     lcExtentTag *tag)
   {
      INT32 rc = SDB_OK;
      ossScopedLock(latch, EXCLUSIVE);

      const GLOBAL_PAGE_ID &id = tag->id();
      _TAG_MAP_ITERATOR lower = _tags.lower_bound(id);
      _TAG_MAP_ITERATOR upper = _tags.upper_bound(id);
      for (; lower != upper; ++lower)
      {
         if (tag != lower->second)
         {
            continue;
         }
         if (!tag->toBeRemoved())
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         _tags.erase(lower);
         SDB_OSS_DEL tag;
         break;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcBucket::getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                    _ossSpinSLatch *latch,
                                    lcExtentTagHolder &holder)
   {  
      if (NULL != latch)
      {
         latch->get_shared();
      }

      _TAG_MAP_ITERATOR lower = _tags.lower_bound(id);
      _TAG_MAP_ITERATOR upper = _tags.upper_bound(id);
      for (; lower != upper; ++lower)
      {
         lcExtentTag *tag = lower->second;
         if (tag->incUsageCnt())
         {
            holder.reset(tag);
            break;
         }
      }

      if (NULL != latch)
      {
         latch->release_shared();
      }
      return SDB_OK;
   }

   INT32 lcBucket::insertTag(const GLOBAL_PAGE_ID &id,
                             UINT32 pageSize,
                             UINT32 minRecycleCount,
                             lcExtentTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      lcExtentTag *tag = NULL;
      SDB_ASSERT(!id.invalid(), "can not be ivnalid");
      SDB_ASSERT(0 < pageSize, "can not be invalid");

      tag = recycleTag(minRecycleCount);
      if (NULL == tag)
      {
         tag = SDB_OSS_NEW lcExtentTag();
         if (OSS_UNLIKELY(NULL == tag))
         {
            rc = SDB_OOM;
            goto error;
         }
      }

      tag->setStateNormal();
      tag->firstInit(id, pageSize);
      holder.reset(tag);
      _tags.insert(std::make_pair(id, tag));
      tag = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_FREE(tag);
      goto done;
   }

   lcExtentTag *lcBucket::recycleTag(UINT32 minRecycleCount)
   {
      lcExtentTag *tag = NULL;
      if (_tags.size() < minRecycleCount)
      {
         return NULL;
      }

      _TAG_MAP_ITERATOR itr = _tags.begin();
      for (; itr != _tags.end(); ++itr)
      {
         tag = itr->second;
         /// we are sure that this tag can not be removed now. coz we are holding bucket unique latch.
         /// if do not check first, may prevent lru eviction.
         if (!tag->recyclePreCheck())
         {
            continue;
         }

         /// some one held the latch after our fast check.
         if (!tag->rwMutex().try_lock())
         {
            continue;
         }

         if (tag->isDirty())
         {
            continue;
         }

         /// some one pinned the tag. what a coincidence!
         if (!tag->tryToSetRecycled())
         {
            tag->rwMutex().unlock();
            continue;
         }

         /// now no one will access this tag.
         tag->rwMutex().unlock();
         _tags.erase(itr);
         tag->reset();
         
         break;
      }

      return tag;
   }
} /// end of namespace vessel
} /// end of namespace engine
