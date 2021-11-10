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

#include "vessel/IRedoLogger.h"
#include "ossAtomic.hpp"
#include "vessel/logRecordContext.h"

namespace engine
{
namespace vessel
{
   class dummyJournal : public IRedoLogger
   {
      public:
         dummyJournal():
         _lsn(0)
         {

         }
         virtual ~dummyJournal(){}

      public:
         virtual INT32 log(IExecutor *executor,
                           const _dpsLogRecord *record,
                           DPS_LSN_OFFSET *lsn)
         {
            UINT64 t = _lsn.add(record->alignedLen());
            if (NULL != lsn)
            {
               *lsn = t;
            }
            return SDB_OK;
         }

         /// allocate lsn and log buffer.
         virtual INT32 prepare(IExecutor *executor,
                               logRecordContext *context)
         {
            SDB_ASSERT(!context->prepared(), "impossible");
            UINT64 t = _lsn.fetch();
            context->getHead()._lsn = t;
            if (0 != OSS_BIT_TEST(context->getHead()._flags,
                                  DPS_VESSEL_LOG_FLAG_OPL_HEAD))
            {
               context->getHead()._opListLSN = t;
            }
            
            return SDB_OK;
         }

         virtual INT32 pushLogRecordElement(IExecutor *executor,
                                            logRecordContext *context,
                                            DPS_TAG tag,
                                            UINT32 len,
                                            const void *value)
         {
            SDB_ASSERT(context->prepared(), "impossible");
            return SDB_OK;
         }

         virtual INT32 commit(IExecutor *executor,
                              logRecordContext *context)
         {
            SDB_ASSERT(context->prepared(), "impossible");
            return SDB_OK;
         }

         /// do not abort log after committing.
         virtual INT32 abort(IExecutor *executor,
                             logRecordContext *context)
         {
            SDB_ASSERT(context->prepared(), "impossible");
            return SDB_OK;
         }

         virtual INT32 pushMaxFileLSN(IExecutor *executor,
                                      DPS_LSN_OFFSET lsn) {return SDB_OK;}

         /// 
         virtual INT32 abortOplist(IExecutor *executor,
                                   DPS_LSN_OFFSET lsn) {}

         virtual DPS_LSN_OFFSET getMinFileLsn() {return _lsn.fetch();}

         static dummyJournal *instance()
         {
            static dummyJournal journal;
            return &journal;
         }

      private:
         ossAtomic64 _lsn;
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_DUMMY_JOURNAL_H_