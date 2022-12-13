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

   Source File Name = redoLogManager.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_REDO_LOG_MANAGER_H_
#define VESSEL_REDO_LOG_MANAGER_H_

#include "interface/IDataJournal.h"
#include "vessel/rlogBufferManager.h"
#include "vessel/rlogFileManager.h"
#include "vessel/rlogWriterContext.h"
#include "dpsWriteContext.hpp"
#include <mutex>
#include <atomic>

namespace engine
{
namespace vessel
{
   class redoLogManager : public IDataJournal
   {
      public:
         redoLogManager() = default;
         virtual ~redoLogManager();

      public:
         virtual INT32 write(IExecutor *executor,
                             const dpsWriteRequest &request,
                             const dpsWriteOptions &o,
                             dpsLogRecordHeader *result) override;

         virtual INT32 flush(DPS_LSN_OFFSET offset, BOOLEAN async) override;

         virtual DPS_LSN_OFFSET getMinFileLsnOffset() override;
         virtual DPS_LSN_OFFSET getMinBufLsnOffset() override;
         virtual DPS_LSN_OFFSET getCurrentLsnOffset() override;
         virtual DPS_LSN_OFFSET getExpectedLsnOffset() override;
         virtual DPS_LSN_OFFSET getCommittedLsnOffset() override;

         virtual DPS_LSN getMinFileLSN() override;
         virtual DPS_LSN getMinBufLSN() override;
         virtual DPS_LSN getCurrentLSN() override;
         virtual DPS_LSN getExpectedLSN() override;
         virtual DPS_LSN getCommittedLSN() override;
         virtual void getLsnWindow(DPS_LSN &minFileLSN,
                                   DPS_LSN &minBufLSN,
                                   DPS_LSN &currentLSN,
                                   DPS_LSN *expectedLSN,
                                   DPS_LSN *committedLSN) override;

         virtual INT32 search(const DPS_LSN &lsn,
                              const dpsSearchOptions &o,
                              dpsMessageBlock &block) override;

         virtual INT32 replicate(const CHAR *rawdata, UINT32 size) override;

         virtual INT32 move(const DPS_LSN_OFFSET &lsn,
                            const DPS_LSN_VER &version) override; 
      public:
         INT32 init(const CHAR *dirPath,
                    const redoLogOptions &o,
                    UINT64 expectedLSN);

         void fini();

         void waitUntilWriterAttached();

         INT32 exportFilesToScan(ossPoolVector<const redoLogFile *> &files);

         OSS_INLINE BOOLEAN isInitialized() const { return _inited; }

      public:
         void writerRun(IExecutor *executor);

      private:
         OSS_INLINE DPS_LSN_OFFSET _getExpectedLSN() const
         {
            return _expectedLSN.load(std::memory_order_relaxed);
         }
         OSS_INLINE void _setExpectedLSN(DPS_LSN_OFFSET v)
         {
            _expectedLSN.store(v, std::memory_order_relaxed);
         }
         OSS_INLINE DPS_LSN_OFFSET _reserveLSN(UINT32 size)
         {
            return _expectedLSN.fetch_add(size, std::memory_order_relaxed);
         }
         OSS_INLINE DPS_LSN_OFFSET _getCurrentLSN()const
         {
            return _currentLSN.load(std::memory_order_relaxed);
         }
         OSS_INLINE void _setCurrentLSN(DPS_LSN_OFFSET v)
         {
            _currentLSN.store(v, std::memory_order_relaxed);
         }
         OSS_INLINE UINT32 _getAlignedRecordSize(UINT32 bodySize) const
         {
            return ossAlign4(DPS_LOG_HEAD_SIZE + bodySize);
         }

      private:
         INT32 _compressRecord(dpsWriteContext &ctx);
         INT32 _write(dpsWriteContext &ctx);
         INT32 _generateFillingRecord(dpsWriteContext &ctx);
         void _prepareFillingRecord(UINT32 fillingSize, dpsWriteContext &ctx);
         void _prepareFormalRecord(dpsWriteContext &ctx);
         void _writeLogBuffer(dpsWriteContext &ctx);
         void _requestFlush(UINT64 offset, BOOLEAN async);
         void _notifyWaitingReqs();
         void _notityWriterIfBufSwitched(const dpsWriteContext &ctx);

      private:///background writer
         void _flushBuffers(const rlogWriterContext::event &e);
         void _flushAll();

      private:
         BOOLEAN _inited = FALSE;
         std::mutex _mutex;
         std::atomic<DPS_LSN_OFFSET> _expectedLSN = {0};
         std::atomic<DPS_LSN_OFFSET> _currentLSN = {DPS_INVALID_LSN_OFFSET};
         rlogBufferManager _bufMgr;
         rlogFileManager _fileMgr;
         rlogWriterContext _wctx;
         std::condition_variable _flushReqCV;
         std::mutex _flushReqMutex;
   };//class redoLogManager
} // namespace vessel

} // namespace engine


#endif//VESSEL_REDO_LOG_MANAGER_H_