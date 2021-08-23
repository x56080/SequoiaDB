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
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   lcDirtyList::lcDirtyList()
   {}

   lcDirtyList::~lcDirtyList()
   {}

   INT32 lcDirtyList::init()
   {
      INT32 rc = SDB_OK;
   done:
      return rc;
   error:
      goto done;
   }

   void lcDirtyList::fini()
   {
      _size = 0;
      _head = NULL;
      _tail = NULL;
      return;
   }

   UINT32 lcDirtyList::getSizeUnderLock()
   {
      ossXLatchGuard guard(&_latch);
      return _size;
   }

   UINT32 lcDirtyList::getSizeFast()const
   {
      return *((volatile UINT32 *)(&_size));
   }

   UINT64 lcDirtyList::getMinDirtyLSN(BOOLEAN lock)
   {
      ossSpinXLatch *latch = lock ? &_latch : NULL;
      ossScopedLock guard(latch);
      return _minDirtyLsn;
   }

   INT32 lcDirtyList::upsert(DPS_LSN_OFFSET lsn, lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *tag = NULL;
     
      if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET == lsn ||
                       !holder.valid()))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!holder.getLockMode().isExclusive()))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (!holder.tag()->hasMemPage())
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      tag = holder.tag();

      if (tag->isInDirtyList())
      {
         if (!tag->isMemPageDirty() || tag->getMaxMemDirtyLSN() < lsn)
         {
            tag->setMaxMemDirtyLSN(lsn);
         }
         else
         {
            SDB_ASSERT(FALSE, "redo log might be truncated");
            PD_LOG(PDSEVERE, "redo log might be truncated. current tag's max lsn:%lld, commit lsn:%lld",
                   tag->getMaxMemDirtyLSN(), lsn);
         }
         goto done;
      }

      SDB_ASSERT(DPS_INVALID_LSN_OFFSET == tag->getMinDirtyLSN(), "must be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET == tag->getMaxMemDirtyLSN(), "must be invalid");
      tag->setMinAndMaxDirtyLSN(lsn);
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
      else if (OSS_UNLIKELY(!holder.getLockMode().isExclusive()))
      {
         PD_LOG(PDERROR, "holding wrong type latch");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      tag = holder.tag();
      SDB_ASSERT(!tag->isMemPageDirty(), "can not be dirty");

      tag->setMinAndMaxDirtyLSN(DPS_INVALID_LSN_OFFSET);
      _latch.get();
      locked = TRUE;
      remove(tag);
      
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
      SDB_ASSERT(!job->isRunning(), "must be empty");

      job->prepare(ossRand(), diskIOJob::DIRTY_LIST, scanDepth);
      ossScopedLock guard(&_latch);

      itr = _tail;
      for (UINT32 i = 0;i < scanDepth &&  NULL != itr; ++i)
      {
         liteCachePageTag *tag = itr;
         itr = itr->getDirtyListPre();

         if (DPS_INVALID_LSN_OFFSET != minLSN && minLSN < tag->getMinDirtyLSN())
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

   void lcDirtyList::updateMinDirtyLsn()
   {
      ossScopedLock guard(&_latch);
      if (NULL == _tail)
      {
         _minDirtyLsn = DPS_INVALID_LSN_OFFSET;
      }
      else
      {
         /// No need to get tag's latch.
         _minDirtyLsn = _tail->getMinDirtyLSN();
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != _minDirtyLsn, "impossible");
      }
      return;
   }

   void lcDirtyList::insertIntoSortedList(liteCachePageTag *tag)
   {
      SDB_ASSERT(NULL != tag, "should be null");
      UINT64 lsn = tag->getMinDirtyLSN();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn,
                 "should not insert tag with invalid lsn");
      if (NULL != _head)
      {
         liteCachePageTag *current = _head;
         liteCachePageTag *pre = NULL;
         do
         {
            ///head --> tail
            ///min dirty lsn: max --> min
            if (lsn >= current->getMinDirtyLSN())
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
         if (lsn < _minDirtyLsn)
         {
            _minDirtyLsn = lsn;
         }
      }
      else /// empty list
      {
         _head = tag;
         _tail = tag;
         tag->insertIntoDirtyList(NULL, NULL);
         ++_size;
         _minDirtyLsn = lsn;
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