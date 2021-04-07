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

   Source File Name = lcDirtyList.cpp

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

#include "vessel/lcDirtyList.h"
#include "ossErr.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "dpsLogDef.hpp"
#include "vessel/diskIOJob.h"

namespace engine
{
namespace vessel
{
   lcDirtyList::lcDirtyList()
   :_size(0),
    _head(NULL),
    _tail(NULL),
    _cachedMinDirtyLSN(DPS_INVALID_LSN_OFFSET)
    {}

   lcDirtyList::~lcDirtyList()
   {
      
   }

   INT32 lcDirtyList::init()
   {
      INT32 rc = SDB_OK;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcDirtyList::fini()
   {
      _size = 0;
      _head = NULL;
      _tail = NULL;
      return SDB_OK;
   }

   UINT32 lcDirtyList::size()
   {
      ossScopedLock(&_latch, SHARED);
      UINT32 size = _size;
      return size;
   }

   UINT64 lcDirtyList::getMinDirtyLSN()
   {
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      ossScopedLock(&_latch, SHARED);
      lsn = _cachedMinDirtyLSN;
      if (NULL != _tail && _tail->getMinLSN() < lsn)
      {
         lsn = _tail->getMinLSN();
      }
      return lsn;
   }

   INT32 lcDirtyList::insert(lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *tag = NULL;
      SDB_ASSERT(holder.valid(), "holder should be valid");
      SDB_ASSERT(LOCK_MODE_UNIQUE == holder.getLockMode(), "tag should be under unique lock");
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      
      if (OSS_UNLIKELY(!holder.valid() ||
                        LOCK_MODE_UNIQUE != holder.getLockMode()))
      {
         PD_LOG(PDERROR, "invalid dirty list insert: %lld, %d",
                          holder.tag(), holder.getLockMode());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (OSS_UNLIKELY(holder.tag()->isInDirtyList()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      tag = holder.tag();
      lsn = tag->getMinLSN();
      if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET == lsn))
      {
         PD_LOG(PDERROR, "can not insert tag with invalid lsn");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      tag->setFastFlags(LC_TAG_FAST_FLAG_DIRTY | LC_TAG_FAST_FLAG_IN_DIRTY_LIST);
      _latch.get();
      insertIntoSortedList(tag);
      _latch.release();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcDirtyList::remove(lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      liteCachePageTag *tag = NULL;
   
      if (OSS_UNLIKELY(!holder.valid()))
      {
         PD_LOG(PDERROR, "can not remove an invalid tag from dirty list");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(LOCK_MODE_UNIQUE != holder.getLockMode()))
      {
         PD_LOG(PDERROR, "holding wrong type latch");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      tag = holder.tag();
      SDB_ASSERT(!tag->isDirty(), "can not be dirty");

      _latch.get();
      locked = TRUE;
      remove(tag);
      _latch.release();
      locked = FALSE;
      tag->clearFastFlags(LC_TAG_FAST_FLAG_IN_DIRTY_LIST);
      
   done:
      if (locked)
      {
         _latch.release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 lcDirtyList::setPendingWrite(requestContext *context,
                                      UINT32 scanDepth,
                                      UINT64 minLSN,
                                      diskIOJob *job)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *itr = NULL;
      SDB_ASSERT(NULL != context && NULL != job, "can not be null");
      SDB_ASSERT(0 == job->getTagCount(), "must be empty");

      ossScopedLock(&_latch, EXCLUSIVE);

      itr = _tail;
      for (UINT32 i = 0;i < scanDepth &&  NULL != itr; ++i)
      {
         liteCachePageTag *tag = itr;
         itr = itr->getDirtyListPre();

         if (minLSN < tag->getMinLSN())
         {
            break;
         }

         if (!tag->setPendingWrite())
         {
            continue;
         }

         rc = job->addPendingWriteTag(tag);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

   done:
      return rc;
   error:
      job->abortUndispatchedTasks();
      goto done;
   }

   INT32 lcDirtyList::cacheMinDirtyLSN()
   {
      INT32 rc = SDB_OK;
      ossScopedLock(&_latch, EXCLUSIVE);

      /// there should be only one flushing dirty list job at the same time
      if (DPS_INVALID_LSN_OFFSET != _cachedMinDirtyLSN)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      if (NULL != _tail)
      {
         _cachedMinDirtyLSN = _tail->getMinLSN();
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lcDirtyList::removeCachedMinDirtyLSN()
   {
      ossScopedLock(&_latch, EXCLUSIVE);
      _cachedMinDirtyLSN = DPS_INVALID_LSN_OFFSET;
   }

   void lcDirtyList::insertIntoSortedList(liteCachePageTag *tag)
   {
      SDB_ASSERT(NULL != tag, "should be null");
      UINT64 lsn = tag->getMinLSN();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn,
                 "should not insert tag with invalid lsn");
      if (NULL != _head)
      {
         liteCachePageTag *current = _head;
         liteCachePageTag *pre = NULL;
         do
         {
            if (lsn >= current->getMinLSN())
            {
               tag->insertIntoDirtyList(pre, current);
               current->setDirtyListPre(tag);

               if (NULL == pre)
               {
                  _head = tag;
               }
               else
               {
                  pre->setDirtyListNext(tag);
               }

               ++_size;
               goto done;
            }
            else
            {
               pre = current;
               current = current->getDirtyListNext();
            }
         } while (NULL != current);
         
         /// insert to tail
         tag->insertIntoDirtyList(_tail, NULL);
         _tail->setDirtyListNext(tag);
         _tail = tag;
         ++_size;
      }
      else /// empty list
      {
         _head = tag;
         _tail = tag;
         tag->insertIntoDirtyList(NULL, NULL);
         ++_size;
      }

   done:
      return;
   }

   void lcDirtyList::remove(liteCachePageTag *tag)
   {
      liteCachePageTag *pre = tag->getDirtyListPre();
      liteCachePageTag *next = tag->getDirtyListNext();
      
      if (NULL != pre && NULL != next)
      {
         pre->setDirtyListNext(next);
         next->setDirtyListPre(pre);
      }
      else if (NULL == pre && NULL != next)
      {
         _head = next;
         next->setDirtyListPre(NULL);
      }
      else if (NULL != pre && NULL == next)
      {
         _tail = pre;
         pre->setDirtyListNext(NULL);
      }
      else
      {
         _head = NULL;
         _tail = NULL;
      }
      
      tag->removeFromDirtyList();
      --_size;
      return;
   }
} /// end of namespace vessel
} /// end of namespace engine