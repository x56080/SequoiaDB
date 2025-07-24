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

   Source File Name = dpsBlackHoleInst.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "interface/IDataProtectionService.h"
#include "pdTrace.hpp"
#include <atomic>
#include <mutex>
#include <memory>

namespace engine
{
   class dpsBlackHoleInst : public IDataProtectionService
   {
      public:
         dpsBlackHoleInst() = default;
         virtual ~dpsBlackHoleInst() = default;

      private:
         static constexpr UINT32 _DPS_FILE_SIZE = 64 << 20;
         static constexpr UINT32 _DPS_BUFFER_SIZE = 64 << 10;

      public:
         virtual DPS_LSN getMinFileLSN() override
         {
            DPS_LSN_OFFSET offset = _getCommittedOffset();
            if (DPS_INVALID_LSN_OFFSET == offset)
            {
               return DPS_LSN();
            }
            else if (offset < _DPS_FILE_SIZE)
            {
               return DPS_LSN(0, _version);
            }
            else
            {
               return DPS_LSN(offset - _DPS_FILE_SIZE, _version);
            }
         }

         virtual DPS_LSN getMinBufLSN() override
         {
            DPS_LSN_OFFSET offset = _getCurrentOffset();
            if (DPS_INVALID_LSN_OFFSET == offset)
            {
               return DPS_LSN();
            }
            else if (offset < _DPS_BUFFER_SIZE)
            {
               return DPS_LSN(0, _version);
            }
            else
            {
               return DPS_LSN(offset - _DPS_BUFFER_SIZE, _version);
            }
         }

         virtual DPS_LSN getCurrentLSN() override
         {
            return DPS_LSN(_getCurrentOffset(), _version);
         }

         virtual DPS_LSN getExpectedLSN() override
         {
            return DPS_LSN(_getExpectedOffset(), _version);
         }

         virtual DPS_LSN getCommittedLSN() override
         {
            return DPS_LSN(_getCommittedOffset(), _version);
         }

         virtual void getLsnWindow(DPS_LSN &minFileLSN,
                                    DPS_LSN &minBufLSN,
                                    DPS_LSN &currentLSN,
                                    DPS_LSN *expectedLSN,
                                    DPS_LSN *committedLSN) override
         {
            minFileLSN = getMinFileLSN();
            minBufLSN = getMinBufLSN();
            currentLSN = getCurrentLSN();
            if (nullptr != expectedLSN)
            {
               *expectedLSN = getExpectedLSN();
            }
            if (nullptr != committedLSN)
            {
               *committedLSN = getCommittedLSN();
            }
         }

         virtual INT32 write(IExecutor *executor,
                             const dpsWriteRequest &request,
                             const dpsWriteOptions &o,
                             dpsLogRecordHeader *result) override
         {
            UINT32 size = ossAlign4(sizeof(dpsLogRecordHeader) + request.getElementDataSize());
            std::unique_lock<std::mutex> guard(_mutex);
            DPS_LSN_OFFSET offset = _expected.fetch_add(size, std::memory_order_relaxed);
            DPS_LSN_OFFSET current = _current.exchange(offset, std::memory_order_relaxed);
            guard.unlock();

            if (nullptr != result)
            {
               result->clear();
               result->_lsn = offset;
               result->_preLsn = current;
               result->_length = size;
               result->_version = _version;
               result->_type = request.getType();
               result->_flags = request.getFlags();
            }

            return SDB_OK;
         }

         virtual INT32 search(const DPS_LSN &lsn,
                              const dpsSearchOptions &o,
                              dpsMessageBlock &block) override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 replicate(const CHAR *rawdata, UINT32 size) override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 flush( DPS_LSN_OFFSET offset, BOOLEAN async ) override
         {
            std::unique_lock<std::mutex> guard(_mutex);
            if (DPS_INVALID_LSN_OFFSET == offset)
            {
               _committed.store(_getCurrentOffset());
            }
            else
            {
               DPS_LSN_OFFSET committed = _getCommittedOffset();
               if (DPS_INVALID_LSN_OFFSET == committed)
               {
                  _committed.store(offset, std::memory_order_relaxed);
               }
               else if (committed < offset)
               {
                  _committed.store(offset, std::memory_order_relaxed);
               }
               else
               {
                  /// do nothing.
               }
            }

            return SDB_OK;
         }

         virtual INT32 move(const DPS_LSN_OFFSET &lsn,
                            const DPS_LSN_VER &version) override
         {
            return SDB_NOT_SUPPORTED;
         }

      public:
         virtual void regEventHandler(dpsEventHandler *handler) override
         {
            return;
         }

         virtual void unregEventHandler(dpsEventHandler *handler) override
         {
            return;
         }

         virtual INT32 completeOpr(IExecutor *executor, INT32 w) override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 archive() override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 process(IExecutor *executor, dpsRequestContext &ctx) override
         {
            return SDB_NOT_SUPPORTED;
         }

         virtual INT32 loadOpl( const DPS_LSN &lastNodeLSN,
                                dpsOperationList &opl ) override
         {
            return SDB_NOT_SUPPORTED;
         }

      public:
         virtual BOOLEAN isClosed() const override {return FALSE;}
         virtual BOOLEAN canSync( BOOLEAN &force ) const override {return TRUE;}

         virtual INT32 sync( BOOLEAN force,
                             BOOLEAN sync,
                             IExecutor* cb ) override {return SDB_OK;}

         virtual void lock() override {}
         virtual void unlock() override {}
      
      private:
         OSS_INLINE DPS_LSN_OFFSET _getExpectedOffset()const
         {
            return _expected.load(std::memory_order_relaxed);
         }
         OSS_INLINE DPS_LSN_OFFSET _getCurrentOffset()const
         {
            return _current.load(std::memory_order_relaxed);
         }
         OSS_INLINE DPS_LSN_OFFSET _getCommittedOffset()const
         {
            return _committed.load(std::memory_order_relaxed);
         }

      private:
         std::mutex _mutex;
         std::atomic<DPS_LSN_OFFSET> _expected{0};
         std::atomic<DPS_LSN_OFFSET> _current{DPS_INVALID_LSN_OFFSET};
         std::atomic<DPS_LSN_OFFSET> _committed{DPS_INVALID_LSN_OFFSET};
         DPS_LSN_VER _version = 1;
   };//class dpsBlackHoleInst

   std::unique_ptr<IDataProtectionService> dpsCreateBlackHoleInst()
   {
      return std::move(std::unique_ptr<IDataProtectionService>(SDB_OSS_NEW dpsBlackHoleInst()));
   }
} // namespace engine
