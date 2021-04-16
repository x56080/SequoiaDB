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

   Source File Name = test_def.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_TEST_TEST_DEF_H_
#define VESSEL_TEST_TEST_DEF_H_

#include "ossTypes.hpp"
#include "vessel/ISession.h"
#include "vessel/IRedoLogger.h"
#include "dpsLogRecord.hpp"
#include "vessel/logRecordContext.h"

using namespace engine::vessel;
using namespace engine;

static const CHAR *DATA_PATH = "/tmp/vessel_test";

class test_session : public ::engine::vessel::ISession
{
   public:
      test_session():
      _id(0)
      {}
      virtual ~test_session(){}

   public:
      virtual UINT64 getSessionID()const
      {
         return 0;
      }

      virtual void setLastError(INT32 rc, const CHAR *fmt, ...)
      {
         return ;
      }

      virtual void clearLastError()
      {
         return;
      }

      virtual BOOLEAN quit()const
      {
         return FALSE;
      }

      virtual BOOLEAN nowait()const
      {
         return FALSE;
      }
   private:
      UINT32 _id;
};

class test_logger : public ::engine::vessel::IRedoLogger
{
   public:
      test_logger(){
         _lsn = 0;
      }
      virtual ~test_logger(){}

      virtual INT32 log(::engine::vessel::ISession *session,
                           const _dpsLogRecord *record,
                           DPS_LSN_OFFSET *lsn)
      {
         if (NULL != lsn)
         {
            *lsn = _lsn;
         }
         _lsn += record->alignedLen();
         return SDB_OK;
      }

         /// allocate lsn and log buffer.
         virtual INT32 prepare(::engine::vessel::ISession *session,
                               logRecordContext *context)
         {
            if (context->prepared())
            {
               return SDB_INVALIDARG;
            }
            context->getHead()._lsn = _lsn;
            if (OSS_BIT_TEST(context->getHead()._flags, DPS_VESSEL_LOG_FLAG_OP_HEAD))
            {
               context->getHead()._opListLSN = _lsn;
            }
            _lsn += context->getHead()._length;
            return SDB_OK;
         }

         virtual INT32 pushLogRecordElement(::engine::vessel::ISession *session,
                                            logRecordContext *context,
                                            DPS_TAG tag,
                                            UINT32 len,
                                            const void *value)
         {
            if (!context->prepared())
            {
              return SDB_INVALIDARG;
            }
            return SDB_OK;
         }

         virtual INT32 commit(::engine::vessel::ISession *session,
                              logRecordContext *context)
         {
            if (!context->prepared())
            {
               return SDB_INVALIDARG;
            }
            return SDB_OK;
         }

         /// do not commit log after commit.
         virtual INT32 abort(::engine::vessel::ISession *session,
                             logRecordContext *context)
         {
            if (!context->prepared())
            {
               return SDB_INVALIDARG;
            }
            return SDB_OK;
            
         }

         virtual INT32 pushMaxFileLSN(::engine::vessel::ISession *session,
                                      DPS_LSN_OFFSET lsn)
         {
            return SDB_OK;
         }

         virtual INT32 abortOplist(::engine::vessel::ISession *session,
                                      DPS_LSN_OFFSET lsn){return SDB_OK;}

   private:
      UINT64 _lsn;
};

#endif//VESSEL_TEST_TEST_DEF_H_