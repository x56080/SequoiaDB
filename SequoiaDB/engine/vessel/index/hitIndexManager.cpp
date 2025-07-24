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

   Source File Name = hitIndexManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/hitIndexManager.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/lsm/lsmColumnFamily.h"
#include "rocksdb/sst_file_reader.h"
#include "rocksdb/iterator.h"
#include "vessel/lsm/lsmCollector.h"
#include "vessel/keyStringCoder.h"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/lsm/lsmIteratorBound.h"
#include "vessel/lsmKeyStringEntry.h"
#include "vessel/lsm/lsmDB.h"
#include "vessel/hitTransferHandler.h"
#include "vessel/spaceIDLocker.h"
#include "vessel/lsm/lsmTableProperties.h"
#include "rocksdb/comparator.h"
#include "rocksdb/slice_transform.h"

namespace engine
{
namespace vessel
{
   extern rocksdb::Comparator* getHitComparator();
   extern const rocksdb::SliceTransform *getLsmIndexPrefixTransform();

   hitIndexManager::~hitIndexManager()
   {
      SDB_ASSERT(!_isAttached(), "detaching missed");
   }

   INT32 hitIndexManager::init()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      lsmDB *lsm = tc->getEnv()->lsm;
      SDB_ASSERT(nullptr != lsm && lsm->isOpen(), "init lsm first");
      return SDB_OK;
   }

   void hitIndexManager::fini()
   {
      if (_isAttached())
      {
         backgroundEvent event = backgroundEvent::createQuitEvent();
         _el.push(event);
         _waitForDetaching();
      }

      SDB_ASSERT(!_isAttached(), "impossible");
      _workers.fini();
      _el.clear();
      _filesToTransfer.clear();
      _job.reset();
      _status = _STATUS::DEACTIVED;
      _limiter.reset();
   }

   void hitIndexManager::attach()
   {
      SDB_ASSERT(!_isAttached(), "do not reattach");
      SDB_ASSERT(_isDeactived(), "must be deactived");
      _attached.store(TRUE);
      constexpr UINT32 millis = 1000;
      BOOLEAN quiting = FALSE;
      backgroundEvent event;
      
      PD_LOG(PDINFO, "hit manager attached");
      _status = _STATUS::STANDBY;

      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();
      UINT32 timeout = 1000;
      std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

      do
      {
         event.reset();
         BOOLEAN r = _el.popOrWaitFor(millis, event);
         if (r)
         {
            if (event.isQuitEvent())
            {
               PD_LOG(PDINFO, "quit event received");
               quiting = TRUE;
               if (_isStandby())
               {
                  _status = _STATUS::DEACTIVED;
                  break;
               }
            }
            else if (event.isResponseOf(BACKGROUND_EVENT_TYPE::HIT_ENTRY_TRANSFER))
            {
               if (OSS_LIKELY(_isWaitingResponse()))
               {
                  _status = _handleResponse(quiting, event);
                  _launchOnStatus();
               }
               else
               {
                  PD_LOG(PDERROR, "invalid launching status[%d]", _status);
                  SDB_ASSERT(FALSE, "invalid launching status");
               }
            }
            else
            {
               PD_LOG(PDERROR, "unknown event type:%d", event.getType());
            }
         }
         else if (_isStandby() && !quiting)
         {
            /// time out and not quiting, try to active transfer.
            _launchOnStatus();
         }
         else
         {
            /// not standby or quiting, do nothing.
         }

         // get sst count and adjust rate limiter after 1000ms timeout.
         std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
         if ((end - start).count() >= timeout)
         {
            UINT32 sstCount = 0;
            cf.getSSTCount(sstCount);
            _limiter.adjustSleepTime(sstCount);
            start = end;
         }

      } while (!_isDeactived());
      
      _job.reset();
      _filesToTransfer.clear();
      _attached.store(FALSE);
      PD_LOG(PDINFO, "hit manager detached");
      return;
   }


   INT32 hitIndexManager::_reloadFilesToTransfer()
   {
      INT32 rc = SDB_OK;
      _filesToTransfer.clear();
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();
      rc = cf.loadSSTs(0, TRUE, FALSE, _filesToTransfer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load sst files:%d", rc);
         goto error;
      }

      if (!_filesToTransfer.empty())
      {
         PD_LOG(PDINFO, "[%d] sst files loaded", _filesToTransfer.size());
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 hitIndexManager::_initFileTransferJob(const std::string &name,
                                               BOOLEAN &ignored)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!name.empty(), "can not be invalid");
      rocksdb::Options o;
      o.comparator = getHitComparator();
      o.prefix_extractor.reset(getLsmIndexPrefixTransform());
      rocksdb::Status s;
      lsmTableProperties properties;
      std::shared_ptr<const rocksdb::TableProperties> ptr;
      ignored = FALSE;

      _job.reader.reset(new rocksdb::SstFileReader(o));
      if (OSS_UNLIKELY(!_job.reader))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }
      
      s = _job.reader->Open(name);
      if (!s.ok())
      {
         PD_LOG(PDERROR, "failed to open sst file[%s], detail:%s",
                name.c_str(), s.getState());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// load properties
      ptr = _job.reader->GetTableProperties();
      properties.init(ptr.get());

      if (!properties.hasUserDefinedProperties())
      {
         ignored = TRUE;
         goto done;
      }

      {
         UINT64 minLSN = DPS_INVALID_LSN_OFFSET;
         UINT64 maxLSN = DPS_INVALID_LSN_OFFSET;
         rc = properties.getLSNPair(minLSN, maxLSN);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lsn pair:%d", rc);
            goto error;
         }

         _job.lsn = maxLSN;
      }

      {
         globalIndexID minIndexId;
         globalIndexID maxIndexId;
         rc = properties.getIndexIdPair(minIndexId, maxIndexId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index id pair:%d", rc);
            goto error;
         }

         _job.maxId = maxIndexId;
      }

   done:
      return rc;
   error:
      _job.reset();
      goto done;
   }

   INT32 hitIndexManager::_buildFileTransferJob()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_job.cjobs.empty(), "must be empty");
      SDB_ASSERT(nullptr != _job.reader.get(), "can not be invalid");
      SDB_ASSERT(_job.maxId.isValid(), "can not be invalid");

      lsmIteratorBound bound;
      rocksdb::ReadOptions o;
      std::unique_ptr<rocksdb::Iterator> itr(_job.reader->NewIterator(o));
      if (OSS_UNLIKELY(!itr))
      {
         PD_LOG(PDERROR, "failed to create file iterator");
         rc = SDB_OOM;
         goto error;
      }

      ///TODO: we should validate itr's status here
      itr->SeekToFirst();
      while (itr && itr->Valid())
      {
         rc = _buildCsJob(itr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build cs job:%d", rc);
            goto error;
         }
      }

      _job.startTime = ossGetCurrentMilliseconds();

      PD_LOG(PDDEBUG, "[%d] cs job built in sst file[%s]",
             _job.cjobs.size(), _filesToTransfer.back().c_str());
         
   done:
      return rc;
   error:
      _job.reset();
      goto done;
   }

   INT32 hitIndexManager::_buildCsJob(std::unique_ptr<rocksdb::Iterator> &itr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != itr.get() && itr->Valid(), "can not be invalid");
      _csTransferJob cjob;
      lsmIteratorBound bound;

      do
      {
         globalIndexID indexId;
         rocksdb::Slice key = itr->key();
         lsmKeyStringEntry entry(key.size(), key.data());
         if (OSS_UNLIKELY(!entry.isValid()))
         {
            PD_LOG(PDERROR, "unexpected invalid entry found");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         indexId = entry.getIndexId();
         if (DMS_INVALID_LOGICCSID == cjob.csid)
         {
            cjob.csid = indexId.getLogicalCSID();
         }
         else if (indexId.getLogicalCSID() != cjob.csid)
         {
            break;
         }

         /// no else
         {
            hitIndexTransferTask task(cjob.tasks.size(), _job.reader.get(), indexId);
            HIT_TRANS_TASK_CTX ctx(SDB_OSS_NEW hitTransferTaskCtx(task));
            if (OSS_UNLIKELY(!ctx))
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }
            cjob.tasks.push_back(std::move(ctx));

            if (_job.maxId != indexId)
            {
               rc = bound.init(indexId);
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  PD_LOG(PDERROR, "failed to init iterator bound:%d", rc);
                  goto error;
               }

               itr->Seek(*bound.getUpBound());
            }
            else
            {
               itr.reset();
               break;
            }
            
         }
      } while (itr->Valid());

      SDB_ASSERT(DMS_INVALID_LOGICCSID != cjob.csid, "can not be invalid");
      SDB_ASSERT(!cjob.tasks.empty(), "can not be empty");
      _job.cjobs.push_back(std::move(cjob));

   done:
      return rc;
   error:
      goto done;
   }

   INT32 hitIndexManager::_popBackSSTAndRemove()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_filesToTransfer.empty(), "can not be empty");
      const std::string &name = _filesToTransfer.back();
      PD_LOG(PDINFO, "will remove sst file:%s", name.c_str());
      std::size_t sep = name.find_last_of(OSS_FILE_SEP);
      rc = GET_THREAD_CONTEXT()->getEnv()->lsm->removeSST(name.substr(sep + 1));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove sst file:%s, rc:%d",
                name.c_str(), rc);
         goto error;
      }

      _filesToTransfer.pop_back();
   done:
      return rc;
   error:
      goto done;
   }

   void hitIndexManager::_dispatchJob()
   {
      SDB_ASSERT(!_job.cjobs.empty(), "can not be invalid");
      SDB_ASSERT(_workers.isValid(), "can not be invalid");

      _csTransferJob &job = _job.getCurrentCsJob();
      backgroundEvent e;
      e.initAsRequest(BACKGROUND_EVENT_TYPE::HIT_ENTRY_TRANSFER);
      e.setResponser(&_el);

      for (UINT32 i = 0; i < job.tasks.size(); ++i)
      {
         e.setShortData(i);
         _workers.pushEvent(e);
      }

      return;
   }

   INT32 hitIndexManager::executeTask(UINT32 taskId)
   {
      INT32 rc = SDB_OK;
      hitTransferHandler handler;
      _csTransferJob *cjob = nullptr;

      cjob = _job.getCurrentCsJobPtr();
      if (OSS_UNLIKELY(cjob->tasks.size() <= taskId))
      {
         PD_LOG(PDERROR, "task id[%d] out of bound[%d]",
                taskId, cjob->tasks.size());
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = handler.handle(cjob->tasks[taskId].get());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to handle task[%d], rc:%d", taskId, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   hitIndexManager::_STATUS hitIndexManager::_handleResponse(BOOLEAN isQuiting, 
                                                             const backgroundEvent &e)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(e.isResponseOf(BACKGROUND_EVENT_TYPE::HIT_ENTRY_TRANSFER),
                 "can not be invalid");
      SDB_ASSERT(_STATUS::WAITING_RESPONSE == _status, "can not be invalid");
      SDB_ASSERT(_job.isValid() && !_job.hasNoCsJob(), "can not be invalid");
      _STATUS s = _STATUS::STANDBY;

      UINT32 taskId = e.getShortData<UINT32>();
      _csTransferJob &cjob = _job.getCurrentCsJob();
      _completeTask(taskId, e.getRC(), cjob);
      if (!cjob.isDone())
      {
         s = _STATUS::WAITING_RESPONSE;
         goto done;
      }

      ///TODO: shoud we terminate all tasks at once if get error ?
      if (cjob.hasError())
      {
         _rollbackCurrentCSJob();
         /// duplidated entries to transfer if it is not the first
         /// cs in file
         s = isQuiting ? _STATUS::DEACTIVED : _STATUS::TRANSFER_CS;
         goto done;
      }

      rc = _commitCsJob();
      if (SDB_OK != rc)
      {
         _rollbackCurrentCSJob();
         /// duplidated entries to transfer if it is not the first
         /// cs in file
         s = isQuiting ? _STATUS::DEACTIVED : _STATUS::TRANSFER_CS;
         goto done;
      }

      if (_job.hasNoCsJob())
      {
         _finishCurrentFileJob();
         s = isQuiting ? _STATUS::DEACTIVED : _STATUS::TRANSFER_FILE;
      }
      else
      {
         s = _STATUS::TRANSFER_CS;
      }
   done:
      return s;
   }

   void hitIndexManager::_completeTask(UINT32 taskId,
                                       INT32 rc,
                                       _csTransferJob &cjob)
   {
      if (OSS_UNLIKELY(cjob.tasks.size() <= taskId))
      {
         PD_LOG(PDERROR, "task id[%d] is out of bound[%d]",
                taskId, cjob.tasks.size());
         goto done;
      }
      else
      {
         HIT_TRANS_TASK_CTX &ctx = cjob.tasks[taskId];
         SDB_ASSERT(!ctx->isDone(), "should not be done");
         ctx->setRC(rc);
         ctx->setDone();
         if (SDB_OK != rc)
         {
            ++cjob.errorTaskNum;
         }
         ++cjob.completedTaskNum;
      }
   done:
      return;
   }

   void hitIndexManager::_finishCurrentFileJob()
   {
      SDB_ASSERT(_job.isValid(), "can not be invalid");

      UINT64 millis = ossGetCurrentMilliseconds();
      PD_LOG(PDDEBUG, "cost time[%lld] to transfer file", millis - _job.startTime);
      _job.reset();
      INT32 rc = _popBackSSTAndRemove();
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to remove file transfered:%d", rc);
         ossPanic();
         goto done;
      }

   done:
      return;
   }

   INT32 hitIndexManager::_commitCsJob()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_job.isValid() && !_job.hasNoCsJob(), "can not be invalid");
      _csTransferJob &cjob = _job.getCurrentCsJob();
      SDB_ASSERT(cjob.hasLockedSid(), "impossible");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      storageUnit *su = tc->getEnv()->dms.getStorageUnit(cjob.lockedSid);
      if (OSS_UNLIKELY(nullptr == su))
      {
         PD_LOG(PDSEVERE, "failed to get storage unit[%d]", cjob.lockedSid);
         ossPanic();
      }

      /// duplicated flush here, but whatever.
      tc->getEnv()->resource.journal->flush(_job.lsn);

      rc = su->getIndexSpace().commit(cjob.batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit write batch to space:%d", rc);
         goto error;
      }
      
      tc->getEnv()->spaceLocker.unlock(cjob.lockedSid, SHARED);
      _job.cjobs.pop_front();
   done:
      return rc;
   error:
      goto done;
   }

   void hitIndexManager::_waitForAttaching()const
   {
      while (!_isAttached())
      {
         ossSleepmillis(1);
      }
   }

   void hitIndexManager::_waitForDetaching()const
   {
      while (_isAttached())
      {
         ossSleepmillis(1);
      } 
   }

   INT32 hitIndexManager::_beginToTransferCurrentCS(BOOLEAN &csRemoved)
   {
      INT32 rc = SDB_OK;
      _csTransferJob *cjob = _getCurrentCSJob();
      SDB_ASSERT(nullptr != cjob && cjob->isValid(), "can not be invalid");
      SDB_ASSERT(!cjob->hasLockedSid(), "impossible");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      spaceIDLocker &locker = tc->getEnv()->spaceLocker;
      collectionSpaceId id;
      storageUnit *su = nullptr;
      csRemoved = FALSE;

      rc = tc->getEnv()->dms.testCSByLid(cjob->csid, id);
      if (SDB_DMS_CS_NOTEXIST == rc)
      {
         PD_LOG(PDERROR, "cs[%d] has been removed", cjob->csid);
         csRemoved = TRUE;
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs identifier:%d", rc);
         goto error;
      }

      rc = locker.lock(id.getSpaceId(), SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d", id.getSpaceId(), rc);
         goto error;
      }
      cjob->lockedSid = id.getSpaceId();

      su = tc->getEnv()->dms.getStorageUnit(id.getSpaceId());
      if (nullptr == su || cjob->csid != su->getLogicalID())
      {
         PD_LOG(PDINFO, "cs[%d] has been removed", cjob->csid);
         csRemoved = TRUE;
         locker.unlock(id.getSpaceId(), SHARED);
         cjob->lockedSid = INVALID_SPACE_ID;
         goto done;
      }

      rc = su->getIndexSpace().initWriteBatch(cjob->batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init write batch:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < cjob->tasks.size(); ++i)
      {
         HIT_TRANS_TASK_CTX &task = cjob->tasks[i];
         task->setBatch(cjob->batch.get());
      }

      _dispatchJob();
   done:
      return rc;
   error:
      if (nullptr != cjob && cjob->hasLockedSid())
      {
         locker.unlock(cjob->lockedSid, SHARED);
      }
      goto done;
   }

   hitIndexManager::_csTransferJob *hitIndexManager::_getCurrentCSJob()
   {
      if (_job.isValid() && !_job.cjobs.empty())
      {
         return _job.getCurrentCsJobPtr();
      }
      else
      {
         return nullptr;
      }
   }

   INT32 hitIndexManager::_adjustWorkers(BOOLEAN hasJob)
   {
      INT32 rc = SDB_OK;
      if (hasJob && !_workers.isValid())
      {
         instanceEnv *env = GET_THREAD_CONTEXT()->getEnv();
         backgroundWorkers::options o;

         o.maxWorkerNum = env->options.hitTransferWorkerCount;
         SDB_ASSERT(0 < o.maxWorkerNum, "can not be invalid");
         rc = _workers.init(env, o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init background workers:%d", rc);
            goto error;
         }
      }
      else if (!hasJob && _workers.isValid())
      {
         _workers.fini();
      }
   done:
      return rc;
   error:
      goto done;
   }

   void hitIndexManager::_rollbackCurrentCSJob()
   {
      PD_LOG(PDINFO, "begin to rollback hit transfer job");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      spaceIDLocker &locker = tc->getEnv()->spaceLocker;
      _csTransferJob *cjob = _getCurrentCSJob();
      SDB_ASSERT(nullptr != cjob, "can not be invalid");
      SDB_ASSERT(cjob->hasLockedSid(), "must be locked");
      storageUnit *su = tc->getEnv()->dms.getStorageUnit(cjob->lockedSid);
      if (OSS_UNLIKELY(nullptr == su))
      {
         PD_LOG(PDSEVERE, "failed to get storage unit[%d]", cjob->lockedSid);
         ossPanic();
      }

      su->getIndexSpace().abort(cjob->batch);

      SDB_ASSERT(FALSE, "TODO: remove btree entry page");

      for (UINT32 i = 0; i < cjob->tasks.size(); ++i)
      {
         cjob->tasks[i]->resetToRedo();
      }

      locker.unlock(cjob->lockedSid, SHARED);
      cjob->lockedSid = INVALID_SPACE_ID;
      cjob->completedTaskNum = 0;
      cjob->errorTaskNum = 0;
      return;
   }

   void hitIndexManager::_launchOnStatus()
   {
      do
      {
         _STATUS s = _STATUS::DEACTIVED;
         switch (_status)
         {
         case _STATUS::DEACTIVED:
            s = _STATUS::DEACTIVED;
            break;
         case _STATUS::STANDBY:
            s = _launchOnStandby();
            break;
         case _STATUS::WAITING_RESPONSE:
            s = _STATUS::WAITING_RESPONSE;
            break;
         case _STATUS::TRANSFER_CS:
            s = _launchOnTransferCS();
            break;
         case _STATUS::TRANSFER_FILE:
            s = _launchOnTransferFile();
            break;
         default:
            SDB_ASSERT(FALSE, "invalid status");
            break;
         }

         _status = s;
      } while (_isDrivingStatus(_status));

      return;
   }

   hitIndexManager::_STATUS hitIndexManager::_launchOnStandby()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_STATUS::STANDBY == _status, "can not be invalid");
      SDB_ASSERT(_filesToTransfer.empty(), "must be empty");
      SDB_ASSERT(!_job.isValid(), "can not be running");
      _STATUS s = _STATUS::STANDBY;

      rc = _reloadFilesToTransfer();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load files:%d", rc);
         goto error;
      }

      rc = _adjustWorkers(!_filesToTransfer.empty());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to adjust background workers:%d", rc);
         goto error;
      }

      if (!_filesToTransfer.empty())
      {
         s = _STATUS::TRANSFER_FILE;
      }

   done:
      return s;
   error:
      _job.reset();
      _filesToTransfer.clear();
      s = _STATUS::STANDBY;
      goto done;
   }

   hitIndexManager::_STATUS hitIndexManager::_launchOnTransferFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_STATUS::TRANSFER_FILE == _status, "can not be invalid");
      SDB_ASSERT(!_job.isValid(), "can not be running");
      _STATUS s = _STATUS::STANDBY;

      while (!_filesToTransfer.empty())
      {
         BOOLEAN ignored = FALSE;
         const std::string &fn = _filesToTransfer.back();
         rc = _initFileTransferJob(fn, ignored);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to transfer file:%s, rc:%d", fn.c_str(), rc);
            goto error;
         }

         if (!ignored)
         {
            rc = _buildFileTransferJob();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build file job:%d", rc);
               goto error;
            }

            s = _STATUS::TRANSFER_CS;
            goto done;
         }

         rc = _popBackSSTAndRemove();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove sst file:%d", rc);
            goto error;
         }        
      }

      SDB_ASSERT(_filesToTransfer.empty(), "impossible");
      s = _STATUS::STANDBY;
   done:
      return s;
   error:
      _job.reset();
      _filesToTransfer.clear();
      s = _STATUS::STANDBY;
      goto done;
   }

   hitIndexManager::_STATUS hitIndexManager::_launchOnTransferCS()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_STATUS::TRANSFER_CS == _status, "can not be invalid");
      SDB_ASSERT(_job.isValid(), "can not be invalid");
      _STATUS s = _STATUS::STANDBY;

      while (!_job.hasNoCsJob())
      {
         BOOLEAN csRemoved = FALSE;
         rc = _beginToTransferCurrentCS(csRemoved);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to transfer current cs:%d", rc);
            goto error;
         }
         else if (csRemoved)
         {
            _job.popBack();
            continue;
         }
         else
         {
            s = _STATUS::WAITING_RESPONSE;
            goto done;
         }
      }

      SDB_ASSERT(_job.hasNoCsJob(), "impossible");
      _job.reset();
      s = _STATUS::TRANSFER_FILE;
   done:
      return s;
   error:
      _job.reset();
      _filesToTransfer.clear();
      s = _STATUS::STANDBY;
      goto done;
   }

   INT32 hitIndexManager::setLimiter(const hitRateLimitOptions &o)
   {
      return _limiter.set(o);
   }

   void hitIndexManager::limitRate() const
   {
      _limiter.limitRate();
   }

//////////////////////////_csTransferJob
   hitIndexManager::_csTransferJob::_csTransferJob(_csTransferJob &&o) noexcept :
   csid(o.csid),
   lockedSid(o.lockedSid),
   batch(std::move(o.batch)),
   tasks(std::move(o.tasks)),
   completedTaskNum(o.completedTaskNum),
   errorTaskNum(o.errorTaskNum)
   {
      o.reset();
   }

   hitIndexManager::_csTransferJob &hitIndexManager::_csTransferJob::operator=(_csTransferJob &&o) noexcept
   {
      csid = o.csid;
      lockedSid = o.lockedSid;
      batch = std::move(o.batch);
      tasks = std::move(o.tasks);
      completedTaskNum = o.completedTaskNum;
      errorTaskNum = o.errorTaskNum;
      o.reset();
      return *this;
   }

   void hitIndexManager::_csTransferJob::reset()
   {
      csid = DMS_INVALID_LOGICCSID;
      lockedSid = INVALID_SPACE_ID;
      batch.reset();
      tasks.clear();
      completedTaskNum = 0;
      errorTaskNum = 0;
      return;
   }

//////////////////////////_fileTransferJob
   void hitIndexManager::_fileTransferJob::reset()
   {
      reader.reset();
      maxId.reset();
      lsn = 0;
      cjobs.clear();
      startTime = 0;
   }

} // namespace vessel

} // namespace engine
