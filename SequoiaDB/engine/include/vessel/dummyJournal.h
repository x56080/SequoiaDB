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

******************************************************************************/

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
         virtual DPS_LSN_OFFSET getMinFileLSN() {return _lsn.load(std::memory_order_relaxed);}
         virtual DPS_LSN_OFFSET getMinBufLSN() {return _lsn.load(std::memory_order_relaxed);}
         virtual DPS_LSN_OFFSET getCurrentLSN() {return _lsn.load(std::memory_order_relaxed);}
         virtual DPS_LSN_OFFSET getExpectedLSN() {return _lsn.load(std::memory_order_relaxed);}
         virtual DPS_LSN_OFFSET getMinDirtyLSN() {return _lsn.load(std::memory_order_relaxed);}

      public:
         virtual void registerEventHandler(dpsEventHandler *handler){}
         virtual void unregisterEventHandler(dpsEventHandler *handler){}

      public:
         virtual INT32 write(const dpsPackedRequest &request,
                             const dpsWriteOptions &o,
                             dpsLogRecordHeader *result)
         {
            UINT32 size = sizeof(dpsLogRecordHeader) + request.getPackedElementsSize();
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

         virtual INT32 flushAll() {return SDB_OK;}
         virtual INT32 flush(DPS_LSN_OFFSET lsn) {return SDB_OK;}
         virtual INT32 truncate(DPS_LSN_OFFSET lsn) {return SDB_OK;}
      private:
         std::atomic_ullong _lsn = {};
   };//class dummyDataJournal
} // namespace vessel

} // namespace engine


#endif//VESSEL_DUMMY_JOURNAL_H_