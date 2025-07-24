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

   Source File Name = hitIndexManager.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_HIT_INDEX_MANAGER_H_
#define VESSEL_HIT_INDEX_MANAGER_H_

#include "vessel/hitTransferTaskCtx.h"
#include "ossMemPool.hpp"
#include "vessel/autoEventList.hpp"
#include "vessel/backgroundEvent.h"
#include "vessel/backgroundWorkers.h"
#include "vessel/lpsPteWriteBatch.h"
#include "vessel/hitRateLimiter.h"


#include <atomic>

namespace rocksdb
{
   class SstFileReader;
   class Iterator;
}

namespace engine
{
namespace vessel
{
   class hitIndexManager : public SDBObject
   {
      public:
         hitIndexManager() = default;
         ~hitIndexManager();

      public:
         INT32 init();
         void waitForAttaching() {_waitForAttaching();}
         void fini();

      public:/// callback functions!
         void attach();
         INT32 executeTask(UINT32 taskId);

      public:
         INT32 setLimiter(const hitRateLimitOptions &o);
         void limitRate() const;

      private:
         using _FILE_VEC = ossPoolVector<std::string>;
         using _TASK_CTX_VEC = ossPoolVector<HIT_TRANS_TASK_CTX>;
         using _EVENT_LIST = autoEventList<backgroundEvent>;
         using _SST_READER = std::unique_ptr<rocksdb::SstFileReader>;

         struct _csTransferJob : public SDBObject
         {
            _csTransferJob() = default;
            ~_csTransferJob() = default;
            _csTransferJob(const _csTransferJob &) = delete;
            _csTransferJob &operator=(const _csTransferJob &) = delete;
            _csTransferJob(_csTransferJob &&o) noexcept;
            _csTransferJob &operator=(_csTransferJob &&o) noexcept;

            void reset();

            OSS_INLINE BOOLEAN isValid()const
            {
               return DMS_INVALID_LOGICCSID != csid;
            }
            OSS_INLINE BOOLEAN isDone()const
            {
               return completedTaskNum == tasks.size();
            }
            OSS_INLINE BOOLEAN hasLockedSid()const
            {
               return INVALID_SPACE_ID != lockedSid;
            }
            OSS_INLINE BOOLEAN hasError()const
            {
               return 0 < errorTaskNum;
            }


            UINT32 csid = DMS_INVALID_LOGICCSID;
            SPACE_ID lockedSid = INVALID_SPACE_ID;
            LPS_PTE_WRITE_BATCH batch;
            _TASK_CTX_VEC tasks;
            UINT32 completedTaskNum = 0;
            UINT32 errorTaskNum = 0;
         };

         struct _fileTransferJob : public SDBObject
         {
            _fileTransferJob() = default;
            ~_fileTransferJob() = default;
            _fileTransferJob(const _fileTransferJob &) = delete;
            _fileTransferJob &operator=(const _fileTransferJob &) = delete;

            void reset();

            OSS_INLINE BOOLEAN isValid()const
            {
               return nullptr != reader.get();
            }
            OSS_INLINE void popBack()
            {
               cjobs.pop_back();
            }
            OSS_INLINE BOOLEAN hasNoCsJob()const
            {
               return cjobs.empty();
            }
            OSS_INLINE _csTransferJob &getCurrentCsJob()
            {
               return cjobs.front();
            }
            OSS_INLINE _csTransferJob *getCurrentCsJobPtr()
            {
               return &cjobs.front();
            }

            _SST_READER reader;
            globalIndexID maxId;
            UINT64 lsn = 0;
            ossPoolList<_csTransferJob> cjobs;
            UINT64 startTime = 0;
         };

      private:
         OSS_INLINE BOOLEAN _isAttached()const
         {
            return _attached.load(std::memory_order_relaxed);
         }
         void _waitForAttaching()const;
         void _waitForDetaching()const;

      private:
         enum class _STATUS : INT32
         {
            DEACTIVED = 0x0,
            STANDBY = 0x01,
            WAITING_RESPONSE = 0x02,
            _DRIVING_STATUS = 0x03,
            TRANSFER_CS = 0x04,
            TRANSFER_FILE = 0x05,
         };

         OSS_INLINE BOOLEAN _isDrivingStatus(_STATUS status)const
         {
            return _STATUS::_DRIVING_STATUS <= status; 
         }
         OSS_INLINE BOOLEAN _isStandby()const
         {
            return _STATUS::STANDBY == _status;
         }
         OSS_INLINE BOOLEAN _isWaitingResponse()const
         {
            return _STATUS::WAITING_RESPONSE == _status;
         }
         OSS_INLINE BOOLEAN _isDeactived()const
         {
            return _STATUS::DEACTIVED == _status;
         }
         
      private:
         void _launchOnStatus();
         _STATUS _launchOnStandby();
         _STATUS _launchOnTransferCS();
         _STATUS _launchOnTransferFile();

      private:
         //INT32 _beginToTransfer(BOOLEAN reloadFiles);
         INT32 _reloadFilesToTransfer();
         //INT32 _createJobFromFileList();
         INT32 _initFileTransferJob(const std::string &name,
                                    BOOLEAN &ignored);
         INT32 _buildFileTransferJob();

         INT32 _buildCsJob(std::unique_ptr<rocksdb::Iterator> &itr);
         INT32 _popBackSSTAndRemove();

         //INT32 _transferFirstUnremovedCS(BOOLEAN &allRemoved);
         INT32 _beginToTransferCurrentCS(BOOLEAN &csRemoved);


      private:
         _STATUS _handleResponse(BOOLEAN isQuiting, const backgroundEvent &e);

         void _completeTask(UINT32 taskId, INT32 rc, _csTransferJob &cjob);

         void _rollbackCurrentCSJob();

         INT32 _commitCsJob();

         void _finishCurrentFileJob();

         _csTransferJob *_getCurrentCSJob();

         INT32 _adjustWorkers(BOOLEAN hasJob);

         void _dispatchJob();

      private:
         std::atomic_bool _attached{FALSE};
         _STATUS _status = _STATUS::DEACTIVED;
         _EVENT_LIST _el;
         _FILE_VEC _filesToTransfer;
         _fileTransferJob _job;

         backgroundWorkers _workers;
         hitRateLimiter _limiter;
   };//hitIndexManager
} // namespace vessel

} // namespace engine


#endif//VESSEL_HIT_INDEX_MANAGER_H_