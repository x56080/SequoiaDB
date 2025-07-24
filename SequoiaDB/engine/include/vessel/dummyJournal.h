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

   Source File Name = dummyJournal.h

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

*******************************************************************************/
#ifndef VESSEL_DUMMY_JOURNAL_H_
#define VESSEL_DUMMY_JOURNAL_H_

#include "interface/IDataJournal.h"
#include <atomic>

namespace engine
{
namespace vessel
{
   class dummyDataJournal : public IDataJournal
   {
      public:
         static dummyDataJournal *instance()
         {
            static dummyDataJournal journal;
            return &journal;
         }

      public:
         virtual DPS_LSN getMinFileLSN()
         {
            return DPS_LSN(_lsn.load(std::memory_order_relaxed), 1);
         }
         virtual DPS_LSN getMinBufLSN()
         {
            return DPS_LSN(_lsn.load(std::memory_order_relaxed), 1);
         }
         virtual DPS_LSN getCurrentLSN()
         {
            return DPS_LSN(_lsn.load(std::memory_order_relaxed), 1);
         }
         virtual DPS_LSN getExpectedLSN()
         {
            return DPS_LSN(_lsn.load(std::memory_order_relaxed), 1);
         }
         virtual DPS_LSN getCommittedLSN()
         {
            return DPS_LSN(_lsn.load(std::memory_order_relaxed), 1);
         }

         virtual void getLsnWindow(DPS_LSN &minFileLSN,
                                    DPS_LSN &minBufLSN,
                                    DPS_LSN &currentLSN,
                                    DPS_LSN *expectedLSN,
                                    DPS_LSN *committedLSN)
         {
            return;
         }

      public:
         virtual INT32 write(IExecutor *executor,
                             const dpsWriteRequest &request,
                             const dpsWriteOptions &o,
                             dpsLogRecordHeader *result)
         {
            UINT32 size = sizeof(dpsLogRecordHeader) + request.getElementDataSize();
            UINT64 lsn = _lsn.fetch_add(ossAlign4(size), std::memory_order_relaxed);
            if (nullptr != result)
            {
               result->clear();
               result->_lsn = lsn;
               result->_preLsn = 0 == lsn ? DPS_INVALID_LSN_OFFSET : lsn - size;
               result->_length = size;
               result->_version = 1;
               result->_type = request.getType();
               result->_flags = request.getFlags();
            }

            return SDB_OK;
         }

         virtual INT32 search(const DPS_LSN &lsn,
                              const dpsSearchOptions &o,
                              dpsMessageBlock &block)
         {
            return SDB_OK;
         }

         virtual INT32 replicate(const CHAR *rawdata, UINT32 size)
         {
            return SDB_OK;
         }

         virtual INT32 flush(DPS_LSN_OFFSET offset, BOOLEAN async) {return SDB_OK;}
         virtual INT32 move(const DPS_LSN_OFFSET &lsn,
                            const DPS_LSN_VER &version) {return SDB_OK;}
      private:
         std::atomic_ullong _lsn = {};
   };//class dummyDataJournal
} // namespace vessel

} // namespace engine


#endif//VESSEL_DUMMY_JOURNAL_H_