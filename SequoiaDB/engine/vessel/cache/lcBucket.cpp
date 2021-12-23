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
      LC_BUCKET_INNER_INDEX_ITERATOR itr = _tagIndex.begin();
      for (; itr != _tagIndex.end(); ++itr)
      {
         if (NULL != itr->second)
         {
            SDB_OSS_DEL itr->second;
         }
      }
      _tagIndex.clear();
      _head = NULL;
      _tail = NULL;
   }

   INT32 lcBucket::ensureTagAndIncUsage(const GLOBAL_PAGE_ID &gpid,
                                        const mmapPagePointer &ptr,
                                        lcPageTagHolder &holder,
                                        BOOLEAN &isNewTag)
   {
      INT32 rc = SDB_OK;
      isNewTag = FALSE;

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

      rc = insertTag(gpid, ptr, holder);
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

   BOOLEAN lcBucket::getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                       lcPageTagHolder &holder)
   {  
      BOOLEAN r = FALSE;
      SDB_ASSERT(id.isValid(), "can not be invalid");
      holder.reset(NULL);
      LC_BUCKET_INNER_INDEX_CONST_ITERATOR lower = _tagIndex.lower_bound(id);
      for (; lower != _tagIndex.end(); ++lower)
      {
         if (lower->first != id)
         {
            break;
         }
         else if (lower->second->incUsageCnt())
         {
            holder.reset(lower->second);
            r = TRUE;
            break;
         }
         else
         {
            /// tag was removed, just continue
            continue;
         }
      }
      return r;
   }

   void lcBucket::discardAndPinTags(SPACE_ID sid,
                                    ossPoolList<liteCachePageTag *> &tags)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      GLOBAL_PAGE_ID gpid;
      gpid.reset(sid, 0, 0, 0);
      LC_BUCKET_INNER_INDEX_CONST_ITERATOR lower = _tagIndex.lower_bound(gpid);
      while (lower != _tagIndex.end())
      {
         if (sid != lower->first.space())
         {
            break;
         }

         liteCachePageTag *tag = lower->second;
         if (tag->fastTestIfCanBeRecycled(FALSE) &&
             tag->getAccessingLatch().tryLock())
         {
            if (tag->fastTestIfCanBeRecycled(FALSE))
            {
               tag->getAccessingLatch().unlock();
               removeFromList(tag);
               lower = _tagIndex.erase(tag->getBucketIterator());

               /// do not access lower in this loop any more.
               tag->reset();
               SDB_OSS_DEL tag;
               continue;
            }
            else
            {
               tag->getAccessingLatch().unlock();
               ///failed to recycle tag, continue to discard it.
            }
         }

         if (tag->discardAndIncUsageCnt())
         {
            tags.push_back(tag);
         }
         
         ++lower;
         continue;
      }

      return;
   }

   INT32 lcBucket::insertTag(const GLOBAL_PAGE_ID &id,
                             const mmapPagePointer &ptr,
                             lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *tag = NULL;
      SDB_ASSERT(id.isValid(), "can not be ivnalid");
      SDB_ASSERT(ptr.isValid(), "can not be invalid");
      LC_BUCKET_INNER_INDEX_ITERATOR itr;

      tag = recycleTag();
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
      pushFront(tag);
      itr = _tagIndex.insert(std::make_pair(id, tag));
      tag->setBucketIndex(itr);
      tag->setInBucket();
      holder.reset(tag);
      tag = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_FREE(tag);
      goto done;
   }

   void lcBucket::pushFront(liteCachePageTag *tag)
   {
      SDB_ASSERT(NULL != tag, "can not be null");
      SDB_ASSERT(NULL == tag->getPreInBucket(), "must be null");
      SDB_ASSERT(NULL == tag->getNextInBucket(), "must be null");
      liteCachePageTag *oldHead = _head;
      _head = tag;
      _head->setNextInBucket(oldHead);
      _head->setPreInBucket(NULL);

      if (NULL != oldHead)
      {
         oldHead->setPreInBucket(_head);
      }
      else
      {
         _tail = tag;
      }
      return;
   }

   void lcBucket::pushBack(liteCachePageTag *tag)
   {
      SDB_ASSERT(NULL != tag, "can not be null");
      SDB_ASSERT(NULL == tag->getPreInBucket(), "must be null");
      SDB_ASSERT(NULL == tag->getNextInBucket(), "must be null");
      liteCachePageTag *oldTail = _tail;
      _tail = tag;
      tag->setPreInBucket(oldTail);
      tag->setNextInBucket(NULL);
      if (NULL != oldTail)
      {
         oldTail->setNextInBucket(tag);
      }
      else
      {
         _head = tag;
      }
      return;
   }

   void lcBucket::removeFromList(liteCachePageTag *tag)
   {
      SDB_ASSERT(NULL != tag, "can not be null");

      liteCachePageTag *pre = tag->getPreInBucket();
      liteCachePageTag *next = tag->getNextInBucket();

      if (NULL != pre)
      {
         pre->setNextInBucket(next);
      }
      else
      {
         _head = next;
      }

      if (NULL != next)
      {
         next->setPreInBucket(pre);
      }
      else
      {
         _tail = pre;
      }

      tag->setPreInBucket(NULL);
      tag->setNextInBucket(NULL);
      return;
   }

   liteCachePageTag *lcBucket::popBack()
   {
      liteCachePageTag *back = _tail;

      if (NULL == back)
      {
         goto done;
      }

      SDB_ASSERT(NULL == back->getNextInBucket(), "must be null");
      if (NULL != back->getPreInBucket())
      {
         back->getPreInBucket()->setNextInBucket(NULL);
         _tail = back->getPreInBucket();
      }
      else
      {
         _head = NULL;
         _tail = NULL;
      }

      back->setPreInBucket(NULL);
   done:
      return back;
   }

   liteCachePageTag *lcBucket::recycleTag()
   {
      static const UINT32 _MAX_LOOP = 16;
      static const UINT32 _RECYCLE_THRESHOLD = 8;
      liteCachePageTag *out = NULL;
      UINT32 maxLoop = 0;

      if (_tagIndex.size() < _RECYCLE_THRESHOLD)
      {
         goto done;
      }

      maxLoop = _tagIndex.size() < _MAX_LOOP?
                _tagIndex.size() : _MAX_LOOP;
      
      for (UINT32 i = 0; i < maxLoop; ++i)
      {      
         liteCachePageTag *tag = popBack();
         /// no latch holding, just for fast skip.
         if (tag->fastTestIfCanBeRecycled(FALSE) &&
             tag->getAccessingLatch().tryLock())
         {
            /// Now, we are hoding bucket latch and tag accessing latch.
            /// We do not need to hold pin latch to check if tag can
            /// be recycled.
            if (tag->fastTestIfCanBeRecycled(FALSE))
            {
               tag->getAccessingLatch().unlock();
               _tagIndex.erase(tag->getBucketIterator());
               tag->reset();
               out = tag;
               break;
            }
            else
            {
               tag->getAccessingLatch().unlock();
               pushFront(tag);
            }
         }
         else
         {
            pushFront(tag);
         }
      }
      
   done:
      return out;
   }
} /// end of namespace vessel
} /// end of namespace engine
