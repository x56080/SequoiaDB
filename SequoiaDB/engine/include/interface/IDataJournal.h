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

   Source File Name = IDataJournal.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_DATA_JOURNAL_H_
#define SDB_I_DATA_JOURNAL_H_

#include "sdbInterface.hpp"
#include "dpsLogRecord.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsRequest.hpp"

namespace engine
{
   class IDataJournal : public SDBObject
   {
      public:
         IDataJournal() = default;
         virtual ~IDataJournal() = default;
         IDataJournal(const IDataJournal &) = delete;
         IDataJournal &operator=(const IDataJournal &) = delete;
      public:
         virtual DPS_LSN getMinFileLSN() = 0;
         virtual DPS_LSN getMinBufLSN() = 0;
         virtual DPS_LSN getCurrentLSN() = 0;
         virtual DPS_LSN getExpectedLSN() = 0;
         virtual DPS_LSN getCommittedLSN() = 0;

         virtual void getLsnWindow(DPS_LSN &minFileLSN,
                                   DPS_LSN &minBufLSN,
                                   DPS_LSN &currentLSN,
                                   DPS_LSN *expectedLSN,
                                   DPS_LSN *committedLSN) = 0;

         virtual DPS_LSN_OFFSET getMinFileLsnOffset()
         {
            return getMinFileLSN().offset;
         }
         virtual DPS_LSN_OFFSET getMinBufLsnOffset()
         {
            return getMinBufLSN().offset;
         }
         virtual DPS_LSN_OFFSET getCurrentLsnOffset()
         {
            return getCurrentLSN().offset;
         }
         virtual DPS_LSN_OFFSET getExpectedLsnOffset()
         {
            return getExpectedLSN().offset;
         }
         virtual DPS_LSN_OFFSET getCommittedLsnOffset()
         {
            return getCommittedLSN().offset;
         }

      public:
         virtual INT32 write(IExecutor *executor,
                             const dpsWriteRequest &request,
                             const dpsWriteOptions &o,
                             dpsLogRecordHeader *result) = 0;

         virtual INT32 search(const DPS_LSN &lsn,
                              const dpsSearchOptions &o,
                              dpsMessageBlock &block) = 0;

         virtual INT32 replicate(const CHAR *rawdata, UINT32 size) = 0;

         virtual INT32 flush(DPS_LSN_OFFSET offset, BOOLEAN async) = 0;

         virtual INT32 flush(DPS_LSN_OFFSET offset) { return flush(offset, FALSE);}

         virtual INT32 move(const DPS_LSN_OFFSET &lsn,
                            const DPS_LSN_VER &version) = 0;
   };//class IDataJournal
} // namespace engine


#endif//SDB_I_DATA_JOURNAL_H_