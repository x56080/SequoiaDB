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

   Source File Name = liteIOBufferPool.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LITE_IO_BUFFER_POOL_H_
#define VESSEL_LITE_IO_BUFFER_POOL_H_

#include "vessel/ioBufferControlBlock.h"
#include "vessel/bufferPoolOptions.h"
#include "vessel/blockBasedMemPool.h"
#include "vessel/dirtyLiteBufferList.h"
#include "vessel/liteIOBuffer.h"
#include "vessel/ioBufferFlushJob.h"
#include "vessel/backgroundEvent.h"

#include <mutex>//c++11
#include <atomic>//c++11

namespace engine
{
namespace vessel
{
   class liteIOBufferPool : public SDBObject
   {
      public:
         liteIOBufferPool() = default;
         ~liteIOBufferPool();

         liteIOBufferPool(const liteIOBufferPool &) = delete;
         liteIOBufferPool &operator=(const liteIOBufferPool &) = delete;

      private:
         typedef std::vector<SHARED_IO_BUFFER_CB_LIST> _BUCKET_VEC;
         typedef std::vector<std::mutex *> _BUCKET_MUTEX_VEC;
         typedef std::chrono::steady_clock _STEADY_CLOCK;

      public:
         struct allocateOptions : public SDBObject
         {
            ossSharedLatchMode mode{OSS_SHARED_LATCH_MODE_ENUM_SHARED};
         };

      public:
         OSS_INLINE BOOLEAN isValid()const {return 0 < _bufferSize;}
         OSS_INLINE UINT32 getBufferSize()const {return _bufferSize;}
         INT32 init(UINT32 bufferSize, const liteBufferPoolOptions &o);
         void fini();

         INT32 allocate(const globalPageID &gpid,
                        const allocateOptions &o,
                        liteIOBuffer &buffer);

         INT32 allocateToReset(const globalPageID &gpid,
                               liteIOBuffer &buffer);

         void commit(UINT64 lsn, liteIOBuffer &buffer);

         void release(liteIOBuffer &buffer);

         INT32 makeWritable(liteIOBuffer &buffer);

         ///WARNING: will not suspend new dirty buffers!
         void flushAll();

         /// get exclusive lock of space first
         void discard(SPACE_ID sid);

      public:
         void watcherAttach();

         BOOLEAN isWatcherAttached()const {return _attached.load(std::memory_order_relaxed);}

         void waitUntilWatcherAttached()const;

         OSS_INLINE BOOLEAN isFlushing()const {return !_flushList.empty();}

         INT32 executeFlushTask(const bufferFlushTaskId &taskId);
         
      private:

         INT32 _ensureBufferCB(const globalPageID &gpid,
                               SHARED_IO_BUFFER_CB &bcb);

         BOOLEAN _findBufferCB(const globalPageID &gpid,
                               SHARED_IO_BUFFER_CB_LIST &bucket,
                               SHARED_IO_BUFFER_CB &bcb);

         INT32 _getMmmapPtr(const globalPageID &gpid,
                            mmapPagePointer &ptr);

         void _discardBuffersInBucket(SPACE_ID sid,
                                      SHARED_IO_BUFFER_CB_LIST &bucket,
                                      SHARED_IO_BUFFER_CB_LIST &l);

         OSS_INLINE SHARED_IO_BUFFER_CB_LIST &_getBucket(const globalPageID &gpid,
                                                         UINT32 &bucketNo)
         {
            UINT32 hash = gpid.hash();
            bucketNo = (hash & (_buckets.size() - 1));
            return _buckets.at(bucketNo);
         }
         OSS_INLINE std::mutex &_getBucketMutex(UINT32 bucketNo)
         {
            UINT32 n = (bucketNo & (_bmutexes.size() - 1));
            return *(_bmutexes.at(n));
         }

      private:
         void _resetFlushTime();

         UINT32 _getTimeSpanFromLastFlush()const;

         /// return false if no dirty buffers to flush.
         BOOLEAN _flushDirtyList(UINT64 maxBufferSize);

         void _handleFlushTaskRes(const backgroundEvent &e);

         BOOLEAN _betterToFlush(UINT64 &size)const;

         void _finishFlush();

         void detatchWatcher();

      private:
         UINT32 _bufferSize = 0;
         liteBufferPoolOptions _o;
         blockBasedMemPool _memPool;
         _BUCKET_VEC _buckets;
         _BUCKET_MUTEX_VEC _bmutexes;
         dirtyLiteBufferList _dl;    

         /// watcher env
         std::atomic_bool _attached{FALSE};
         SHARED_IO_BUFFER_CB_LIST _flushList;
         DPS_LSN_OFFSET _maxFlushLSN = DPS_INVALID_LSN_OFFSET;
         ioBufferFlushJob _job;
         UINT32 _completedTaskNum = 0;
         _STEADY_CLOCK::time_point _lastFlushTime;
         autoEventList<backgroundEvent> _eventList;
   };//class liteIOBufferPool
} // namespace vessel

} // namespace engine


#endif//VESSEL_LITE_IO_BUFFER_POOL_H_
