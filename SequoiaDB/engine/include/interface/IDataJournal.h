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
#include "dpsRequest.hpp"
#include "dpsLogRecord.hpp"
#include "dpsMessageBlock.hpp"

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
         struct writeOptions : public SDBObject
         {
            BOOLEAN flushImmediately = FALSE;
         };//struct writeOptions

      public:
         virtual DPS_LSN_OFFSET getMinFileLSN() = 0;
         virtual DPS_LSN_OFFSET getMinBufLSN() = 0;
         virtual DPS_LSN_OFFSET getCurrentLSN() = 0;
         virtual DPS_LSN_OFFSET getExpectedLSN() = 0;
         virtual DPS_LSN_OFFSET getMinDirtyLSN() = 0;

      public:
         virtual void registerEventHandler(dpsEventHandler *handler) = 0;
         virtual void unregisterEventHandler(dpsEventHandler *handler) = 0;

      public:
         virtual INT32 write(const dpsPackedRequest &request,
                             const dpsWriteOptions &o,
                             dpsLogRecordHeader *result) = 0;

         virtual INT32 search(const DPS_LSN &lsn,
                              const dpsSearchOptions &o,
                              dpsMessageBlock &block) = 0;

         virtual INT32 replicate(const CHAR *rawdata, UINT32 size) = 0;

         virtual INT32 flushAll() = 0;

         virtual INT32 flush(DPS_LSN_OFFSET lsn) = 0;

         virtual INT32 truncate(DPS_LSN_OFFSET lsn) = 0;

         virtual INT32 abortOpl(DPS_LSN_OFFSET lsn){return SDB_OK;}

   };//class IDataJournal
} // namespace engine


#endif//SDB_I_DATA_JOURNAL_H_