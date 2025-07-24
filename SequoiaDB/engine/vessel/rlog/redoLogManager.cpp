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

   Source File Name = redoLogManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/redoLogManager.h"
#include "dpsWriteContext.hpp"
#include "ossLikely.hpp"
#include "vessel/redoLogDef.h"
#include "vessel/redoLogFileReader.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 _DEFAULT_LSN_VERSION = 1;

   redoLogManager::~redoLogManager()
   {
      
   }

   INT32 redoLogManager::init(const CHAR *dirPath,
                              const redoLogOptions &o,
                              UINT64 expectedLSN)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_inited, "do not reinit");
      redoLogFilesSummary summary;

      if (OSS_UNLIKELY(nullptr == dirPath ||
                       DPS_INVALID_LSN_OFFSET == expectedLSN))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _inited = TRUE;

      rc = _bufMgr.init(o.totalBufSize, expectedLSN);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log buffers:%d", rc);
         goto error;
      }

      rc = _fileMgr.init(dirPath, expectedLSN, summary);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log files:%d", rc);
         goto error;
      }

      _expectedLSN = expectedLSN;
      if (0 < expectedLSN)
      {
         _currentLSN = summary.currentLSN;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void redoLogManager::fini()
   {
      _wctx.quitWriter();
      _wctx.reset();
      _expectedLSN = 0;
      _currentLSN = DPS_INVALID_LSN_OFFSET;
      _bufMgr.reset();
      _fileMgr.fini();
      _inited = FALSE;

      return;
   }

   INT32 redoLogManager::write(IExecutor *executor,
                               const dpsWriteRequest &request,
                               const dpsWriteOptions &o,
                               dpsLogRecordHeader *result)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      dpsWriteContext ctx(executor, &request, &o);
      if (o.compress)
      {
         rc = _compressRecord(ctx);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compress log record:%d", rc);
            goto error;
         }
      }

      rc = _write(ctx);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write log record:%d", rc);
         goto error;
      }

      if (nullptr != result)
      {
         *result = ctx.getRecord();
      }
   done:
      return rc;
   error:
      if (nullptr != result)
      {
         result->clear();
      }
      goto done;
   }

   INT32 redoLogManager::exportFilesToScan(ossPoolVector<const redoLogFile *> &files)
   {
      INT32 rc = SDB_OK;
      if (!isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _fileMgr.exportFiles(files);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to export files:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 redoLogManager::_write(dpsWriteContext &ctx)
   {
      INT32 rc = SDB_OK;
      const UINT32 alignedRecordSize = _getAlignedRecordSize(ctx.getRecordBodySizeAuto());
      UINT32 fillingSize = 0;
      std::unique_lock<std::mutex> guard(_mutex);
      DPS_LSN_OFFSET lsn = _getExpectedLSN();

      if (OSS_UNLIKELY(_bufMgr.getMaxRequestBufSize() < alignedRecordSize))
      {
         PD_LOG(PDERROR, "record size[%d] out of buffer size limit", alignedRecordSize);
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      if (_fileMgr.isTheFirstLSNInFile(lsn))
      {
         rc = _fileMgr.prepareFile(lsn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare log file:%d", rc);
            goto error;
         }
      }
      else if (_fileMgr.isLogFileToBeSwitched(lsn, alignedRecordSize, fillingSize))
      {
         rc = _fileMgr.prepareFile(lsn + fillingSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare to switch log file:%d", rc);
            goto error;
         }

         _prepareFillingRecord(fillingSize, ctx);
      }

      _prepareFormalRecord(ctx);

      _notityWriterIfBufSwitched(ctx);

      guard.unlock();

      _writeLogBuffer(ctx);
   done:
      return rc;
   error:
      goto done;
   }

   void redoLogManager::_prepareFillingRecord(UINT32 fillingSize, dpsWriteContext &ctx)
   {
      SDB_ASSERT(DPS_LOG_HEAD_SIZE <= fillingSize, "invalid filling size");
      dpsLogRecordHeader &head = ctx.getDummmyRecord();
      _bufMgr.reserveAndLock(fillingSize, ctx.getDummyPageMeta());

      DPS_LSN_OFFSET lsn = _reserveLSN(fillingSize);
      head._length = fillingSize;
      head._type = LOG_TYPE_DUMMY;
      head._lsn = lsn;
      head._version = _DEFAULT_LSN_VERSION;
      head._preLsn = _getCurrentLSN();
      
      _setCurrentLSN(lsn);

      return;
   }

   void redoLogManager::_prepareFormalRecord(dpsWriteContext &ctx)
   {
      dpsLogRecordHeader &head = ctx.getRecord();
      UINT32 totalRecordSize = _getAlignedRecordSize(ctx.getRecordBodySizeAuto());
      DPS_LSN_OFFSET lsn = _getExpectedLSN();
      UINT32 fileLastFreeSize = _fileMgr.getFileLastFreeSize(lsn, totalRecordSize);
      /// extend record size if no more record can be saved in current file
      if (fileLastFreeSize < DPS_LOG_HEAD_SIZE)
      {
         totalRecordSize += fileLastFreeSize;
      }
      
      /// always reserve buffer before modifying lsns, 
      /// which to ensure buffers locked first.
      _bufMgr.reserveAndLock(totalRecordSize, ctx.getPageMeta());

      head._type = ctx.getReq()->getType();
      head._length = totalRecordSize;
      head._lsn = lsn;
      head._version = _DEFAULT_LSN_VERSION;
      head._preLsn = _getCurrentLSN();
      head._flags = ctx.getReq()->getFlags();
      head._type = ctx.getReq()->getType();

      _setExpectedLSN(lsn + totalRecordSize);
      _setCurrentLSN(lsn);

      return;
   }

   void redoLogManager::_writeLogBuffer(dpsWriteContext &ctx)
   {
      if (ctx.isDummyRecordFilled())
      {
         static DPS_TAG _STOP = DPS_INVALID_TAG;
         utilSlice ending;
         if (DPS_LOG_HEAD_SIZE < ctx.getDummmyRecord()._length)
         {
            ending.reset(sizeof(_STOP), &_STOP);
         }
         _bufMgr.writeAndUnlock(ctx.getDummmyRecord(),
                                ending,
                                ctx.getDummyPageMeta());
      }

      _bufMgr.writeAndUnlock(ctx.getRecord(),
                             ctx.getRecordBodyData(),
                             ctx.getPageMeta());

      return;
   }

   INT32 redoLogManager::_compressRecord(dpsWriteContext &ctx)
   {
      return SDB_OK;
   }

   void redoLogManager::writerRun(IExecutor *executor)
   {
      constexpr UINT32 millis = 1000;
      constexpr UINT32 timeoutSyncInterval = 30;
      UINT32 timeoutTimes = 0;
      SDB_ASSERT(isInitialized(), "must be inited first");

      _wctx.attach();
      PD_LOG(PDINFO, "redo log writer attached");

      while (TRUE)
      {
         rlogWriterContext::event e;
         BOOLEAN timeout = FALSE;
         if (_wctx.wait(millis, timeout, e))
         {
            if (timeout)
            {
               ++timeoutTimes;
               if (timeoutSyncInterval == timeoutTimes)
               {
                  _flushAll();
                  timeoutTimes = 0;
               }
            }
            else
            {
               _flushBuffers(e);
               timeoutTimes = 0;
            }
         }
         else
         {
            break;
         }
      }

      _wctx.detach();
      PD_LOG(PDINFO, "redo log writer detached");
      return;
   }

   void redoLogManager::_flushBuffers(const rlogWriterContext::event &e)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(e.isActive(), "can not be invalid");
      UINT64 elsn = _getExpectedLSN();
      SDB_ASSERT(e.writeOffset <= elsn, "out of bound");
      while (_fileMgr.getWritingOffset() < e.writeOffset)
      {
         BOOLEAN fileIsFull = FALSE;
         redoLogBuffer *buffer = _bufMgr.getBufferByOffset(_fileMgr.getWritingOffset());
         SDB_ASSERT(0 < buffer->getUnsyncedBufSize(), "impossible");
         rc = _fileMgr.write(*buffer, fileIsFull);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to write log file at offset[%lld], rc:%d",
                   _fileMgr.getWritingOffset(), rc);
            ossPanic();
            goto done;
         }

         if (0 == buffer->getFreeSize() && 0 == buffer->getUnsyncedBufSize())
         {
            _bufMgr.freeBuffer(buffer->getBufferId());
         }
         
         if (fileIsFull)
         {
            rc = _fileMgr.switchWorkingFile();
            if (SDB_OK != rc)
            {
               PD_LOG(PDSEVERE, "failed to switch redo log file:%d", rc);
               ossPanic();
               goto done;
            }
         }
      }

      if (_fileMgr.getDirtyOffset() < e.flushOffset)
      {
         rc = _fileMgr.flush();
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to flush redo log file:%d", rc);
            ossPanic();
            goto done;
         }
      }

      _notifyWaitingReqs();

   done:
      return;
   }

   void redoLogManager::_flushAll()
   {
      rlogWriterContext::event e;
      e.writeOffset = _getExpectedLSN();
      e.flushOffset = e.writeOffset;
      _flushBuffers(e);
      return;
   }

   INT32 redoLogManager::flush(DPS_LSN_OFFSET offset, BOOLEAN async)
   {
      INT32 rc = SDB_OK;
      UINT64 flushOffset = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (DPS_INVALID_LSN_OFFSET != offset)
      {
         flushOffset = offset + 1;
         UINT64 expectedLSN = _getExpectedLSN();
         if (expectedLSN < flushOffset)
         {
            PD_LOG(PDWARNING, "flush offset[%lld] out of max lsn bound[%lld]",
                   offset, expectedLSN);
            flushOffset = expectedLSN;
         }
      }
      else
      {
         flushOffset = _getExpectedLSN();
      }

      _requestFlush(flushOffset, async);
   done:
      return rc;
   error:
      goto done;
   }

   void redoLogManager::_requestFlush(UINT64 offset, BOOLEAN async)
   {
      if (offset > _fileMgr.getDirtyOffset())
      {
         _wctx.request(offset, TRUE);
         if (!async)
         {
            std::unique_lock<std::mutex> lk(_flushReqMutex);
            _flushReqCV.wait(lk, [&, this]{ return offset <= _fileMgr.getDirtyOffset(); });
         }
      }

      return;
   }

   void redoLogManager::_notifyWaitingReqs()
   {
      _flushReqCV.notify_all();
   }

   void redoLogManager::waitUntilWriterAttached()
   {
      while (!_wctx.isWriterAttached())
      {
         ossSleepmillis(1);
      }
      return;
   }

   void redoLogManager::_notityWriterIfBufSwitched(const dpsWriteContext &ctx)
   {
      SDB_ASSERT(0 < ctx.getRecord()._length, "can not be invalid");
      UINT64 offset = ctx.getRecord()._lsn + ctx.getRecord()._length;
      if (ctx.isDummyRecordFilled() ||
          1 < ctx.getPageMeta().pageNum ||
          0 == offset % _bufMgr.getBufferPageSize())
      {
         _wctx.request(ossRoundDownToMultipleX(offset, _bufMgr.getBufferPageSize()), FALSE);
      }

      return;
   }

   DPS_LSN_OFFSET redoLogManager::getMinFileLsnOffset()
   {
      return _fileMgr.getMinLSN();
   }

   DPS_LSN_OFFSET redoLogManager::getMinBufLsnOffset()
   {
      SDB_ASSERT(FALSE, "not supported");
      return DPS_INVALID_LSN_OFFSET;
   }

   DPS_LSN_OFFSET redoLogManager::getCurrentLsnOffset()
   {
      return _getCurrentLSN();
   }

   DPS_LSN_OFFSET redoLogManager::getExpectedLsnOffset()
   {
      return _getExpectedLSN();
   }

   DPS_LSN_OFFSET redoLogManager::getCommittedLsnOffset()
   {
      SDB_ASSERT(FALSE, "not supported");
      return DPS_INVALID_LSN_OFFSET;
   }

   DPS_LSN redoLogManager::getMinFileLSN()
   {
      SDB_ASSERT(FALSE, "not supported");
      return DPS_LSN(getMinFileLsnOffset(), _DEFAULT_LSN_VERSION);
   }

   DPS_LSN redoLogManager::getMinBufLSN()
   {
      SDB_ASSERT(FALSE, "not supported");
      return DPS_LSN();
   }

   DPS_LSN redoLogManager::getCurrentLSN()
   {
      SDB_ASSERT(FALSE, "not supported");
      return DPS_LSN(getCurrentLsnOffset(), _DEFAULT_LSN_VERSION);
   }

   DPS_LSN redoLogManager::getExpectedLSN()
   {
      SDB_ASSERT(FALSE, "not supported");
      return DPS_LSN(getExpectedLsnOffset(), _DEFAULT_LSN_VERSION);
   }

   DPS_LSN redoLogManager::getCommittedLSN()
   {
      SDB_ASSERT(FALSE, "not supported");
      return DPS_LSN();
   }

   void redoLogManager::getLsnWindow(DPS_LSN &minFileLSN,
                                     DPS_LSN &minBufLSN,
                                     DPS_LSN &currentLSN,
                                     DPS_LSN *expectedLSN,
                                     DPS_LSN *committedLSN)
   {
      SDB_ASSERT(FALSE, "not supported");
   }

   INT32 redoLogManager::search(const DPS_LSN &lsn,
                                const dpsSearchOptions &o,
                                dpsMessageBlock &block)
   {
      return SDB_NOT_SUPPORTED;
   }

   INT32 redoLogManager::replicate(const CHAR *rawdata, UINT32 size)
   {
      return SDB_NOT_SUPPORTED;
   }

   INT32 redoLogManager::move(const DPS_LSN_OFFSET &lsn,
                              const DPS_LSN_VER &version)
   {
      return SDB_NOT_SUPPORTED;
   }
} // namespace vessel

} // namespace engine
