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
   typedef std::multimap<PHY_EXTENT_ID, lcExtentTag*> TAG_MAP;
   typedef std::multimap<PHY_EXTENT_ID, lcExtentTag*>::iterator TAG_MAP_ITR;
   typedef std::pair<PHY_EXTENT_ID, lcExtentTag*> TAG_PAIR;

   lcBucket::lcBucket()
   {

   }

   lcBucket::~lcBucket()
   {
      TAG_MAP::iterator itr = _tags.begin();
      for (; itr != _tags.end(); ++itr)
      {
         if (NULL != itr->second)
         {
            SDB_OSS_DEL itr->second;
         }
      }
      _tags.clear();
   }

   INT32 lcBucket::ensureTagAndIncUsage(const PHY_EXTENT_ID &gpid,
                                        UINT32 diskPageSize,
                                        UINT32 cachePageSize,
                                        _ossSpinSLatch *latch,
                                        storageUnit *su,
                                        UINT32 minRecycleCount,
                                        lcExtentTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      ossValuePtr ptr = 0;
      SDB_ASSERT(!gpid.invalid(), "can not be invalid");
      SDB_ASSERT(NULL != su, "can not be null");
      SDB_ASSERT(su->isOpen(), "can not be closed");
      SDB_ASSERT(gpid.space() == su->getSpaceID(), "must be same");
      SDB_ASSERT(0 != diskPageSize && 0 != cachePageSize, "can not be zero");
      SDB_ASSERT(0 == diskPageSize % cachePageSize, "impossible");
      BOOLEAN tagLocked = FALSE;

      holder.reset(NULL);

      if (OSS_LIKELY(NULL != latch))
      {
         latch->get();
         locked = TRUE;
      }

      /// search again
      getTagAndIncUsage(gpid, NULL, holder);
      if (holder.valid())
      {
         goto done;
      }

      rc = su->getPagePtr(gpid.type(), gpid.page(), ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = insertTag(gpid, diskPageSize / cachePageSize,
                     cachePageSize, ptr,
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

      if (locked)
      {
         latch->release();
         locked = FALSE;
      }
      /// some one else may accessing tag from here.

      rc = holder.tag()->validateDiskPageAndCacheLSN();
      if (SDB_OK != rc)
      {
         holder.unlockUnique(); 
         holder.tag()->decUsageCnt();
         if (holder.tag()->tryToSetRecycled())
         {
            releaseRemovedTag(latch, holder.tag());
         }
         /// if failed to remove tag, just leave it in the bucket and wait to be recycled.
         PD_LOG(PDSEVERE, "disk page crashed, gpid:%s", gpid.toString().c_str());
         goto error;
      }

      holder.unlockUnique();
      
   done:
      if (locked) {latch->release();}
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

      const PHY_EXTENT_ID &id = tag->id();
      TAG_MAP_ITR lower = _tags.lower_bound(id);
      TAG_MAP_ITR upper = _tags.upper_bound(id);
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

   INT32 lcBucket::getTagAndIncUsage(const PHY_EXTENT_ID &id,
                                    _ossSpinSLatch *latch,
                                    lcExtentTagHolder &holder)
   {  
      if (NULL != latch)
      {
         latch->get_shared();
      }

      TAG_MAP_ITR lower = _tags.lower_bound(id);
      TAG_MAP_ITR upper = _tags.upper_bound(id);
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

   INT32 lcBucket::insertTag(const PHY_EXTENT_ID &id,
                              UINT32 pageNum,
                              UINT32 pageSize,
                              ossValuePtr diskPage,
                              UINT32 minRecycleCount,
                              lcExtentTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      lcExtentTag *tag = NULL;

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
      tag->firstInit(id, pageNum, pageSize, diskPage);
      holder.reset(tag);
      _tags.insert(TAG_PAIR(id, tag));
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

      TAG_MAP_ITR itr = _tags.begin();
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
