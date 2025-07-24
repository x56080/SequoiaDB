/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = liteIOBufferPool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/liteIOBufferPool.h"
#include "pdTrace.hpp"
#include "utilSharedPtrMaker.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "pmdEnv.hpp"

namespace engine
{
namespace vessel
{
   liteIOBufferPool::~liteIOBufferPool()
   {
      fini();
   }

   INT32 liteIOBufferPool::init(UINT32 bufferSize, const liteBufferPoolOptions &o)
   {
      INT32 rc = SDB_OK;
      fini();

      if (OSS_UNLIKELY(!isValidPageSize(bufferSize)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _bufferSize = bufferSize;
      _o = o;
      _o.correctIfNecessary();

      rc = _memPool.init(o.maxMemSize, _bufferSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init mem pool:%d", rc);
         goto error;
      }

      _buckets.resize(o.buckets);
      _bmutexes.resize(o.bucketLatches, nullptr);
      for (UINT32 i = 0; i < o.bucketLatches; ++i)
      {
         std::mutex *m = new(std::nothrow) std::mutex();
         if (OSS_UNLIKELY(nullptr == m))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
         _bmutexes[i] = m;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void liteIOBufferPool::fini()
   {
      detatchWatcher();
      _eventList.clear();
      _completedTaskNum = 0;
      _job.reset();
      _attached.store(false);
      _flushList.clear();
      _maxFlushLSN = DPS_INVALID_LSN_OFFSET;

      _dl.clear();

      for (UINT32 i = 0; i < _bmutexes.size(); ++i)
      {
         if (nullptr != _bmutexes.at(i))
         {
            delete _bmutexes.at(i);
         }
      }
      _bmutexes.clear();
      _bmutexes.shrink_to_fit();

      _buckets.clear();
      _buckets.shrink_to_fit();

      _memPool.fini();

      _o = liteBufferPoolOptions();
      _bufferSize = 0;
   }

   INT32 liteIOBufferPool::allocate(const globalPageID &gpid,
                                    const liteIOBufferPool::allocateOptions &o,
                                    liteIOBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      SHARED_IO_BUFFER_CB bcb;

      buffer.reset();

      if (OSS_UNLIKELY(!gpid.isValid() ||
                       o.mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _ensureBufferCB(gpid, bcb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer cb:%d", rc);
         goto error;
      }

      bcb->getMutex().lockWith(o.mode);
      buffer.init(this, std::move(bcb), o.mode);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteIOBufferPool::allocateToReset(const globalPageID &gpid,
                                           liteIOBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      allocateOptions o;
      o.mode.setExclusive();
      buffer.reset();

      rc = allocate(gpid, o, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate buffer:%d", rc);
         goto error;
      }

      if (!buffer._bcb->hasMemoryBlock())
      {
         blockBasedMemPool::memBlock mb;
         rc = _memPool.allocate(mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate memory block:%d", rc);
            goto error;
         }

         buffer._bcb->_mb = mb;
      }

      SDB_ASSERT(buffer.isWritable(), "must be writable");
   done:
      return rc;
   error:
      if (buffer.isValid())
      {
         release(buffer);
      }
      goto done;
   }

   INT32 liteIOBufferPool::makeWritable(liteIOBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode = buffer.getMode();

      if (OSS_UNLIKELY(!buffer.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (mode.isNone() || mode.isShared())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (mode.isUpgrade())
      {
         buffer._bcb->getMutex().unlockUpgradeAndLock();
         buffer._mode.setExclusive();
      }
      
      if (!buffer._bcb->hasMemoryBlock())
      {
         strictBuffer b;
         blockBasedMemPool::memBlock mb;
         rc = _memPool.allocate(mb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate memory block:%d", rc);
            goto error;
         }

         b.makeWritable(_bufferSize, mb.getBuffer());
         b.write(0, _bufferSize, buffer._bcb->getMPtr().getBuf());
         buffer._bcb->_mb = mb;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void liteIOBufferPool::commit(UINT64 lsn, liteIOBuffer &buffer)
   {
      SDB_ASSERT(buffer.isWritable(), "must be writable");

      if (OSS_LIKELY(buffer.isWritable() && DPS_INVALID_LSN_OFFSET != lsn))
      {
         buffer._bcb->updateLSNPair(lsn);
         _dl.insert(buffer._bcb);
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
      }

      return;
   }

   void liteIOBufferPool::release(liteIOBuffer &buffer)
   {
      if (buffer.isValid())
      {
         buffer._bcb->getMutex().unlockWith(buffer._mode);
         buffer._bcb->ctl().decRefCnt();

         /// buffer can not be recycled if :
         /// 1. some one else pinned buffer;
         /// 2. buffer is dirty
         if (buffer._bcb->ctl().setRecyclingFromNormal())
         {
            if (buffer._bcb->hasMemoryBlock())
            {
               buffer._bcb->releaseMemoryBlock(_memPool);
            }
            BOOLEAN r = buffer._bcb->ctl().setDiscardedFromRecycling();
            SDB_ASSERT(r, "must be true");
         }

         buffer._mode.setNone();
         buffer._bcb.reset();
         buffer._pool = nullptr;
      }

      return;
   }

   void liteIOBufferPool::waitUntilWatcherAttached()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      
      while (!isWatcherAttached())
      {
         ossSleepmillis(1);
      }
   }

   void liteIOBufferPool::watcherAttach()
   {
      bool expected = false;
      constexpr UINT32 millis = 10;
      backgroundEvent event, quitEvent;
      backgroundEvent flushEventRecved, runningFlushEvent;

      if (!_attached.compare_exchange_strong(expected, true))
      {
         SDB_ASSERT(FALSE, "already attached");
         goto done;
      }

      PD_LOG(PDINFO, "lite buffer pool watcher attached");
      _resetFlushTime();
      do
      {
         event.reset();
         if (_eventList.popOrWaitFor(millis, event))
         {
            if (event.isQuitEvent())
            {
               SDB_ASSERT(!quitEvent.isValid(), "already been quiting");
               quitEvent = event;
            }
            else if (BACKGROUND_EVENT_TYPE::FLUSH_LITE_BUF_POOL == event.getType())
            {
               SDB_ASSERT(!flushEventRecved.isValid() &&
                          !runningFlushEvent.isValid(), "already been running");
               SDB_ASSERT(!quitEvent.isValid(), "can not be quiting");

               if (isFlushing())
               {
                  flushEventRecved = event;
               }
               else
               {
                  if (!_flushDirtyList(0))
                  {
                     if (event.hasResponser())
                     {
                        event.getResponser()->push(event.createSimpleResponse());
                     }
                  }
                  else
                  {
                     runningFlushEvent = event;
                  }
               }
            }
            else if (event.isResponseOf(BACKGROUND_EVENT_TYPE::DATA_BUF_TASK))
            {
               _handleFlushTaskRes(event);
               if (isFlushing())
               {
                  continue;
               }
               else if (runningFlushEvent.isValid())
               {
                  if (runningFlushEvent.hasResponser())
                  {
                     runningFlushEvent.getResponser()->push(
                                 runningFlushEvent.createSimpleResponse());
                  }
                  runningFlushEvent.reset();
               }
               else if (flushEventRecved.isValid())
               {
                  runningFlushEvent = flushEventRecved;
                  flushEventRecved.reset();

                  if (!_flushDirtyList(0))
                  {
                     if (runningFlushEvent.hasResponser())
                     {
                        runningFlushEvent.getResponser()->push(
                                 runningFlushEvent.createSimpleResponse());
                     }
                     runningFlushEvent.reset();
                  }
               }
               else
               {
                  UINT64 size = 0;
                  if (!quitEvent.isValid() && _betterToFlush(size))
                  {
                     _flushDirtyList(size);
                  }
               }
            }
            else
            {
               PD_LOG(PDERROR, "invalid event type found:%d", event.getType());
               SDB_ASSERT(FALSE, "unknown type");
            }
         }
         else /// if (_eventList.popOrWaitFor(millis, event))
         {
            UINT64 size = 0;
            if (!quitEvent.isValid() &&
                !isFlushing() &&
                _betterToFlush(size))
            {
               _flushDirtyList(size);
            }
         }
      } while (!quitEvent.isValid() || isFlushing());
      
      SDB_ASSERT(quitEvent.isQuitEvent(), "must be quit");
      if (quitEvent.hasResponser())
      {
         backgroundEvent res = quitEvent.createSimpleResponse();
         quitEvent.getResponser()->push(res);
      }

      _attached.store(false);
      PD_LOG(PDINFO, "lite buffer pool watcher detached");

   done:
      return;
   }

   void liteIOBufferPool::detatchWatcher()
   {
      if (isWatcherAttached())
      {
         backgroundEvent event = backgroundEvent::createQuitEvent();
         _eventList.push(event);

         while (isWatcherAttached())
         {
            ossSleepmillis(1);
         }
      }
   }

   void liteIOBufferPool::flushAll()
   {
      SDB_ASSERT(isValid() && isWatcherAttached(), "can not be invalid");
      backgroundEvent event, res;
      event.initAsRequest(BACKGROUND_EVENT_TYPE::FLUSH_LITE_BUF_POOL);
      autoEventList<backgroundEvent> list;
      event.setResponser(&list);
      _eventList.push(event);
      list.popOrWait(res);
   }

   void liteIOBufferPool::discard(SPACE_ID sid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SHARED_IO_BUFFER_CB_LIST l;
      std::array<blockBasedMemPool::memBlock, 16> batch;
      UINT32 size = 0;
      
      _dl.discard(sid, l);
      while (!l.empty())
      {
         SHARED_IO_BUFFER_CB &bcb = l.front();
         SDB_ASSERT(bcb->hasMemoryBlock(), "impossible");
         bcb->ctl().resetFlags();
         if (OSS_LIKELY(bcb->ctl().setRecyclingFromNormal()))
         {
            bcb->releaseMemoryBlock(_memPool);
            batch[size++] = bcb->getMemoryBlock();
            bcb->resetMemoryBlock();
            BOOLEAN r = bcb->ctl().setDiscardedFromRecycling();
            SDB_ASSERT(r, "impossible");
            l.pop_front();

            if (batch.max_size() == size)
            {
               _memPool.release(size, batch.data());
               size = 0;
            }
         }
         else
         {
            SDB_ASSERT(FALSE, "should not be failed");
         }
      }

      if (batch.max_size() == size)
      {
         _memPool.release(size, batch.data());
         size = 0;
      }

      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         std::unique_lock<std::mutex> guard(_getBucketMutex(i));
         SHARED_IO_BUFFER_CB_LIST &bucket = _buckets.at(i);
         _discardBuffersInBucket(sid, bucket, l);
      }

      /// some buffers may be flushing now.
      while (!l.empty())
      {
         SHARED_IO_BUFFER_CB_LIST::iterator itr = l.begin();
         while (itr != l.end())
         {
            SHARED_IO_BUFFER_CB &bcb = *itr;
            SDB_ASSERT(!bcb->hasMemoryBlock(), "impossible");
            bufferControlBlock ctlSnapshot = bcb->ctl().load();
            if (ctlSnapshot.isDiscarded())
            {
               itr = l.erase(itr);
            }
            else
            {
               ++itr;
            }
         }

         if (l.empty())
         {
            break;
         }
         else
         {
            /// wait buffers flush done
            ossSleepmillis(10);
         }
      }
      
      return;
   }

   void liteIOBufferPool::_discardBuffersInBucket(SPACE_ID sid,
                                                  SHARED_IO_BUFFER_CB_LIST &bucket,        
                                                  SHARED_IO_BUFFER_CB_LIST &l)
   {
      SHARED_IO_BUFFER_CB_LIST::iterator itr = bucket.begin();
      while (itr != bucket.end())
      {
         SHARED_IO_BUFFER_CB &cb = *itr;
         bufferControlBlock ctlSnapshot = cb->ctl().load();
         if (ctlSnapshot.isDiscarded())
         {
            itr = bucket.erase(itr);
         }
         else if (cb->getGlobalPid().getSpaceId() == sid)
         {
            /// it may be flushing.
            SHARED_IO_BUFFER_CB_LIST::iterator pos = itr++;
            l.splice(l.end(), bucket, pos, itr);
         }
         else
         {
            ++itr;
         }
      }

      return;
   }

   INT32 liteIOBufferPool::_ensureBufferCB(const globalPageID &gpid,
                                           SHARED_IO_BUFFER_CB &bcb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(gpid.isValid(), "can not be invalid");

      UINT32 bucketNo = 0;
      SHARED_IO_BUFFER_CB_LIST &bucket = _getBucket(gpid, bucketNo);
      std::mutex &m = _getBucketMutex(bucketNo);
      std::unique_lock<std::mutex> guard(m);

      if (!_findBufferCB(gpid, bucket, bcb))
      {
         bufferControlBlock ctl;
         ctl.init(BUFFER_STATUS::NORMAL, 1, 0);
         mmapPagePointer mptr;
         rc = _getMmmapPtr(gpid, mptr);
         if (SDB_OK != rc)
         {
            goto error;
         }

         bcb = makeSharedPtrFromPool<ioBufferControlBlock>(gpid, ctl, mptr);
         if (!bcb)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         
         bucket.push_front(bcb);
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN liteIOBufferPool::_findBufferCB(const globalPageID &gpid,
                                           SHARED_IO_BUFFER_CB_LIST &bucket,
                                           SHARED_IO_BUFFER_CB &bcb)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(gpid.isValid(), "can not be invalid");
      SDB_ASSERT(!bcb, "must be invalid");
      SHARED_IO_BUFFER_CB_LIST::iterator itr = bucket.begin();
      while (itr != bucket.end())
      {
         SHARED_IO_BUFFER_CB &cb = *itr;
         bufferControlBlock ctlSnapshot = cb->ctl().load();
         if (ctlSnapshot.isDiscarded())
         {
            itr = bucket.erase(itr);
         }
         else if (!ctlSnapshot.isNormal())
         {
            ++itr;
         }
         else if (gpid == cb->getGlobalPid())
         {
            if (cb->ctl().incRefCntIfNormal())
            {
               bcb = cb;
               r = TRUE;
               break;
            }
            else
            {
               ++itr;
            }
         }
         else
         {
            ++itr;
         }
      }

      return r;
   }

   INT32 liteIOBufferPool::_getMmmapPtr(const globalPageID &gpid,
                                        mmapPagePointer &ptr)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      rc = tc->getEnv()->dms.getMmapPagePtr(gpid, ptr, &_bufferSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get ptr of gpid[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void liteIOBufferPool::_resetFlushTime()
   {
      _lastFlushTime = _STEADY_CLOCK::now();
   }

   UINT32 liteIOBufferPool::_getTimeSpanFromLastFlush()const
   {
      _STEADY_CLOCK::time_point now = _STEADY_CLOCK::now();
      return std::chrono::duration_cast<std::chrono::milliseconds>
             (now - _lastFlushTime).count();
   }

   BOOLEAN liteIOBufferPool::_betterToFlush(UINT64 &size)const
   {
      BOOLEAN r = FALSE;
      UINT32 dirtyListSize = _dl.getSize();
      FLOAT32 dirtyPct = static_cast<FLOAT32>(dirtyListSize) /
                         _memPool.getTotalBlockNum();
      constexpr UINT64 _MAX_TIMEOUT_FLUSH_SIZE = (UINT64)1 << 30;

      if (_o.flushDirtyListThreshold <= dirtyPct)
      {
         r = TRUE;
         size = _o.flushBatchSize;
      }
      else if (0 < dirtyListSize &&
               _o.flushDirtyListMillis <= _getTimeSpanFromLastFlush())
      {
         r = TRUE;
         size = (static_cast<UINT64>(_memPool.getBlockSize()) * dirtyListSize) >> 2;
         if (_MAX_TIMEOUT_FLUSH_SIZE < size)
         {
            size = _MAX_TIMEOUT_FLUSH_SIZE;
         }
         else if (size < _o.flushBatchSize)
         {
            size = _o.flushBatchSize;
         }
      }

      return r;
   }

   BOOLEAN liteIOBufferPool::_flushDirtyList(UINT64 maxBufferSize)
   {
      SDB_ASSERT(!isFlushing(), "can not be flushing");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      backgroundWorkers &workers = tc->getEnv()->workers;
      IDataJournal *journal = tc->getEnv()->resource.journal;
      backgroundEvent event;
      event.initAsRequest(BACKGROUND_EVENT_TYPE::DATA_BUF_TASK);
      event.setResponser(&_eventList);
      UINT32 cnt = 0 == maxBufferSize ?
                   0 : maxBufferSize / _bufferSize;

      _dl.makeFlushList(cnt, _flushList, _maxFlushLSN);

      if (!_flushList.empty())
      {
         _job.build(_flushList);
         journal->flush(_maxFlushLSN);
         while (_job.hasMoreTasks())
         {
            bufferFlushTaskId tid = _job.getNextTask();
            event.getShortData<bufferFlushTaskId>().offset = tid.offset;
            event.getShortData<bufferFlushTaskId>().size = tid.size;
            workers.pushEvent(event);
         }

         PD_LOG(PDDEBUG, "begin to flush dirty buffers, task count[%d, %d]",
                _job.getTotalTaskNum(),
                _job.getDispatchedTaskNum());
         SDB_ASSERT(0 < _job.getDispatchedTaskNum(), "impossible");
      }

      return !_flushList.empty();
   }

   void liteIOBufferPool::_handleFlushTaskRes(const backgroundEvent &e)
   {
      bufferFlushTaskId task;
      SDB_ASSERT(e.isResponseOf(BACKGROUND_EVENT_TYPE::DATA_BUF_TASK), "can not be others");
      task = e.getShortData<bufferFlushTaskId>();
      SDB_ASSERT(task.isValid() && task.offset < _job.getTotalTaskNum(),
                 "can not be invalid");
      SDB_ASSERT(_completedTaskNum < _job.getDispatchedTaskNum(),
                 "can not be invalid");

      if (e.getRC() != SDB_OK)
      {
         PD_LOG(PDSEVERE, "failed to complete flush:%d", e.getRC());
      }

      if (++_completedTaskNum == _job.getDispatchedTaskNum())
      {
         _finishFlush();
         PD_LOG(PDDEBUG, "end to flush dirty list");
      }
   }

   void liteIOBufferPool::_finishFlush()
   {
      _maxFlushLSN = DPS_INVALID_LSN_OFFSET;
      _completedTaskNum = 0;
      _job.reset();
      _dl.resetFlushLSN();
      _flushList.clear();
      _resetFlushTime();
   }

   INT32 liteIOBufferPool::executeFlushTask(const bufferFlushTaskId &taskId)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      IDataJournal *journal = tc->getEnv()->resource.journal;
      dataManagementService &dms = tc->getEnv()->dms;
      std::array<blockBasedMemPool::memBlock, 16> batch;
      UINT32 size = 0;

      if (OSS_UNLIKELY(!taskId.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isFlushing()))
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (_job.getTotalTaskNum() < (taskId.offset + taskId.size))
      {
         PD_LOG(PDERROR, "invalid task pos[%d,%d], current task num:%d",
                taskId.offset, taskId.size, _job.getTotalTaskNum());
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else
      {
         /// all buffers in same task should belong to one file.
         ioBufferFlushTask firstTask = _job.get(taskId.offset);
         GLOBAL_PAGE_ID gpid = firstTask.bcb->getGlobalPid();
         storageFileCluster *fcluster = dms.getStorageFileClsuter(gpid.getSpaceId(),
                                                                  gpid.getSpaceType());
         SDB_ASSERT(nullptr != fcluster, "can not be null");
         UINT32 fd = fcluster->getFileSpaceId(gpid.getPageId());
         SDB_ASSERT(0 <= fd, "can not be invalid");

         for (UINT32 i = 0; i < taskId.size; ++i)
         {
            ioBufferFlushTask task = _job.get(taskId.offset + i);
            ioBufferControlBlock *bcb = task.bcb;
            bcb->getMutex().lockShared();

            if (_maxFlushLSN < bcb->getMaxDirtyLSN())
            {
               journal->flush(bcb->getMaxDirtyLSN());
            }
            strictBuffer buffer;
            buffer.makeWritable(_bufferSize, bcb->getMPtr().getBuf());
            buffer.write(0, _bufferSize, bcb->getMemoryBlock().getBuffer());
            bcb->resetLSNPair();///WARNING: not under x lock

            /// we must clear dirty flag before unlock.
            /// writer may waiting x lock now.
            /// flag must be reset before reinsert into dirty list.
            bcb->ctl().clearFlag(LITE_IO_BUFFER_CTL_FLAGS::DIRTY);
            bcb->getMutex().unlockShared();
            bcb->ctl().clearFlag(LITE_IO_BUFFER_CTL_FLAGS::PENDDING_FLUSH);
            
            /// flush all buffers asap
         }//for (UINT32 i = 0; i < taskId.size; ++i)

         for (UINT32 i = 0; i < taskId.size; ++i)
         {
            ioBufferFlushTask task = _job.get(taskId.offset + i);
            ioBufferControlBlock *bcb = task.bcb;
            if (bcb->ctl().setRecyclingFromNormal())
            {  
               batch[size++] = bcb->getMemoryBlock();
               bcb->resetMemoryBlock();/// reset, not release
               BOOLEAN r = bcb->ctl().setDiscardedFromRecycling();
               SDB_ASSERT(r, "must be true");

               if (batch.max_size() == size)
               {
                  _memPool.release(size, batch.data());
                  size = 0;
               }
            }
         }

         if (0 < size)
         {
            _memPool.release(size, batch.data());
         }

         rc = fcluster->fsyncFile(fd);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to fsync file:%d, rc:%d", fd, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine

