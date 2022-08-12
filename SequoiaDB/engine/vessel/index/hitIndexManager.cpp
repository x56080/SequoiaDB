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

   Source File Name = hitIndexManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

namespace engine
{
namespace vessel
{
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
   }

   void hitIndexManager::attach()
   {
      SDB_ASSERT(!_isAttached(), "do not reattach");
      _attached.store(TRUE);
      constexpr UINT32 millis = 1000;
      backgroundEvent event, quitEvent;
      
      PD_LOG(PDINFO, "hit manager attached");

      do
      {
         event.reset();
         BOOLEAN r = _el.popOrWaitFor(millis, event);
         if (r)
         {
            if (event.isQuitEvent())
            {
               quitEvent = event;
            }
            else if (event.isResponseOf(BACKGROUND_EVENT_TYPE::HIT_ENTRY_TRANSFER))
            {
               BOOLEAN currentJobFinished = FALSE;
               _handleResponse(event, currentJobFinished);
               if (currentJobFinished && !quitEvent.isValid())
               {
                  _beginToTransfer(_filesToTransfer.empty());
               }
            }
            else
            {
               PD_LOG(PDERROR, "unknown event type:%d", event.getType());
            }
         }
         else if (_isTransfering())
         {
            continue;
         }
         else if (!quitEvent.isValid())
         {
            INT32 rc = _beginToTransfer(_filesToTransfer.empty());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to load files:%d", rc);
            }
         }
      } while (!quitEvent.isValid() || _isTransfering());
      

      _attached.store(FALSE);
      PD_LOG(PDINFO, "hit manager detached");
      return;
   }

   INT32 hitIndexManager::_beginToTransfer(BOOLEAN reloadFiles)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_job.isValid(), "can not be valid");

      if (reloadFiles)
      {
         rc = _reloadFilesToTransfer();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to load files:%d", rc);
            goto error;
         }
      }

      if (_filesToTransfer.empty())
      {
         if (_workers.isValid())
         {
            _workers.fini();
         }
         goto done;
      }
      
      rc = _createJobFromFileList();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build next job:%d", rc);
         goto error;
      }

      if (_job.isValid())
      {
         if (!_workers.isValid())
         {
            backgroundWorkers::options o;
            o.maxWorkerNum = 4;
            rc = _workers.init(GET_THREAD_CONTEXT()->getEnv(), o);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init background workers:%d", rc);
               goto error;
            }
         }

         ///guarantee all entries transfered into btree
         ///will not be out of lsn bound.
         ///we do not store lsn in btree index, once sst file
         /// removed, entries can not be rollback any more.
         GET_THREAD_CONTEXT()->getEnv()->resource.journal->flush(_job.lsn);

         _dispatchJob();
      }
   
   done:
      return rc;
   error:
      _job.reset();
      if (reloadFiles)
      {
         _filesToTransfer.clear();
      }
      goto done;
   }

   INT32 hitIndexManager::_createJobFromFileList()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_job.isValid(), "can not be running");

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
            break;
         }

         rc = _popBackSSTAndRemove();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove sst file:%d", rc);
            goto error;
         }
      }

      if (_job.isValid())
      {
         rc = _buildFileTransferJob();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build file job:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN hitIndexManager::_isTransfering() const
   {
      return _job.isValid();
   }

   INT32 hitIndexManager::_reloadFilesToTransfer()
   {
      INT32 rc = SDB_OK;
      _filesToTransfer.clear();
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();
      rc = cf.loadSSTs(0, TRUE, _filesToTransfer);
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
      rocksdb::Status s;
      std::shared_ptr<const rocksdb::TableProperties> properties;
      rocksdb::UserCollectedProperties::const_iterator pi;
      keyStringCoder coder;
      
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
      properties = _job.reader->GetTableProperties();
      pi = properties->user_collected_properties.find(LSM_COLLECTOR_FIELDNAME_MIN_GLOBAL_ID);
      if (properties->user_collected_properties.cend() == pi)
      {
         /// if we range delete entries, the sst file may be empty.
         PD_LOG(PDINFO, "min global index id not found in sst[%s]", name.c_str());
         ignored = TRUE;
         goto done;
      }

      pi = properties->user_collected_properties.find(LSM_COLLECTOR_FIELDNAME_MAX_GLOBAL_ID);
      if (properties->user_collected_properties.cend() == pi)
      {
         PD_LOG(PDERROR, "max global index id not found in sst[%s]", name.c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _job.maxId = coder.decodeToIndexId(pi->second.data());
      SDB_ASSERT(_job.maxId.isValid(), "can not be invalid");

      pi = properties->user_collected_properties.find(LSM_COLLECTOR_FIELDNAME_MAX_LSN);
      if (properties->user_collected_properties.cend() != pi &&
          sizeof(DPS_LSN_OFFSET) == pi->second.size())
      {
         _job.lsn = *(reinterpret_cast<const DPS_LSN_OFFSET *>(pi->second.data()));
         
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid raw lsn data in sst");
         PD_LOG(PDERROR, "invalid raw lsn data in sst[%s]", name.c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
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
            hitIndexTransferTask task(_job.reader.get(), indexId);
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

      if (OSS_UNLIKELY(!_isTransfering()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

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

   void hitIndexManager::_handleResponse(const backgroundEvent &e,
                                         BOOLEAN &currentJobFinished)
   {
      SDB_ASSERT(e.isResponseOf(BACKGROUND_EVENT_TYPE::HIT_ENTRY_TRANSFER),
                 "can not be invalid");
      UINT32 taskId = e.getShortData<UINT32>();
      currentJobFinished = FALSE;

      if (_job.hasNoCsJob())
      {
         PD_LOG(PDERROR, "has no running task now");
         goto done;
      }
      else
      {
         _csTransferJob &cjob = _job.getCurrentCsJob();
         _completeTask(taskId, e.getRC(), cjob);

         if (!cjob.isDone())
         {
            goto done;
         }
         
         if (cjob.hasError())
         {
            _redoCsJob(TRUE);
         }
         else
         {
            INT32 rc = _commitCsJob();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to commit cs job:%d", rc);
               _redoCsJob(FALSE);
               goto done;
            }

            if (_job.hasNoCsJob())
            {
               _finishCurrentFileJob();
               currentJobFinished = TRUE;
            }
            else
            {
               _dispatchJob();
            }
         }
      }
      
   done:
      return;
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
      const _csTransferJob &cjob = _job.getCurrentCsJob();
      PD_LOG(PDDEBUG, "cs[%d] job has been committed", cjob.csid);
      _job.cjobs.pop_front();
   done:
      return rc;
   error:
      goto done;
   }

   void hitIndexManager::_redoCsJob(BOOLEAN onlyErrorTasks)
   {
      SDB_ASSERT(FALSE, "TODO");
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

//////////////////////////_transferJob
   hitIndexManager::_csTransferJob::_csTransferJob(_csTransferJob &&o) noexcept :
   csid(o.csid),
   tasks(std::move(o.tasks)),
   completedTaskNum(o.completedTaskNum),
   errorTaskNum(o.errorTaskNum)
   {
      o.reset();
   }

   hitIndexManager::_csTransferJob &hitIndexManager::_csTransferJob::operator=(_csTransferJob &&o) noexcept
   {
      csid = o.csid;
      tasks = std::move(o.tasks);
      completedTaskNum = o.completedTaskNum;
      errorTaskNum = o.errorTaskNum;
      o.reset();
      return *this;
   }

   void hitIndexManager::_csTransferJob::reset()
   {
      csid = DMS_INVALID_LOGICCSID;
      tasks.clear();
      completedTaskNum = 0;
      errorTaskNum = 0;
      return;
   }

   void hitIndexManager::_fileTransferJob::reset()
   {
      reader.reset();
      maxId.reset();
      lsn = 0;
      cjobs.clear();
   }

} // namespace vessel

} // namespace engine
