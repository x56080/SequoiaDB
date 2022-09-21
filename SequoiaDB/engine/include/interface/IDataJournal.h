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

   Source File Name = IDataJournal.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_I_DATA_JOURNAL_H_
#define SDB_I_DATA_JOURNAL_H_

#include "sdbInterface.hpp"
#include "dpsLogRecord.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsWriteRequest.hpp"

namespace engine
{
   struct dpsWriteOptions : public SDBObject
   {
      UINT32 csid = DMS_INVALID_LOGICCSID;
      UINT32 clid = DMS_INVALID_LOGICCLID;
      INT32 extentPos = -1;
      UINT64 transTime = DPS_INVALID_TRANS_TIME;
      BOOLEAN notify = FALSE;
      BOOLEAN transEnabled = FALSE;
      BOOLEAN irrversible = FALSE;
      BOOLEAN flushAtOnce = FALSE;
      IExecutor *executor = nullptr;
   };//struct dpsWriteOptions

   struct dpsSearchOptions : public SDBObject
   {
      BOOLEAN searchMem = TRUE;
      BOOLEAN searchFile = TRUE;
      BOOLEAN onlyHeader = FALSE;
      INT32 limits = 1;
      INT32 maxTime = -1;
      INT32 maxSize = 5242880;
   };//struct dpsSearchOptions

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

         virtual void getLsnWindow(DPS_LSN *minFileLSN,
                                   DPS_LSN *minBufLSN,
                                   DPS_LSN *currentLSN,
                                   DPS_LSN *expectedLSN,
                                   DPS_LSN *committedLSN) = 0;

         OSS_INLINE DPS_LSN_OFFSET getMinFileLsnOffset()
         {
            return getMinFileLSN().offset;
         }
         OSS_INLINE DPS_LSN_OFFSET getMinBufLsnOffset()
         {
            return getMinBufLSN().offset;
         }
         OSS_INLINE DPS_LSN_OFFSET getCurrentLsnOffset()
         {
            return getCurrentLSN().offset;
         }
         OSS_INLINE DPS_LSN_OFFSET getExpectedLsnOffset()
         {
            return getExpectedLSN().offset;
         }
         OSS_INLINE DPS_LSN_OFFSET getCommittedLsnOffset()
         {
            return getCommittedLSN().offset;
         }

      public:
         virtual INT32 write(const dpsWriteRequest &request,
                             const dpsWriteOptions &o,
                             dpsLogRecordHeader *result) = 0;

         virtual INT32 search(const DPS_LSN &lsn,
                              const dpsSearchOptions &o,
                              dpsMessageBlock &block) = 0;

         virtual INT32 replicate(const CHAR *rawdata, UINT32 size) = 0;

         virtual INT32 flush(DPS_LSN_OFFSET lsn) = 0;

         virtual INT32 move(DPS_LSN_VER version, DPS_LSN_OFFSET lsn) = 0;
   };//class IDataJournal
} // namespace engine


#endif//SDB_I_DATA_JOURNAL_H_