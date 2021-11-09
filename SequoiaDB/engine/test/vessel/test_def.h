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
#include "vessel/ISesseionManager.h"
#include "vessel/indexKeyGenerator.h"
#include "vessel/outerResource.h"
#include "pd.hpp"
#include <atomic>

using namespace engine::vessel;
using namespace engine;

static const CHAR *DATA_PATH = "/tmp/vessel_test";
//static const CHAR *DATA_PATH = "/opt/test/vessel_test";
static const CHAR *LSM_PATH = "/tmp/vessel_lsm";

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
         UINT64 t = _lsn.fetch_add(record->alignedLen());
         if (NULL != lsn)
         {
            *lsn = t;
         }
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
            if (0 != OSS_BIT_TEST(context->getHead()._flags,
                                  DPS_VESSEL_LOG_FLAG_OPL_HEAD))
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

         virtual DPS_LSN_OFFSET getMinFileLsn()
         {
            return _lsn.load();
         } 
                                      
         static test_logger *instance()
         {
            sdbEnablePD("/tmp/sdb.log", 1, 1000);
            static test_logger logger;
            return &logger;
         }
   public:
      std::atomic_ullong _lsn;
};

class test_session : public ::engine::vessel::ISession
{
   public:
      test_session(test_logger *logger):
      _id(0),_logger(logger){}
      test_session(UINT32 id, test_logger *logger):
      _id(id),
      _logger(logger)
      {}
      virtual ~test_session(){}

   public:
      virtual UINT64 getSessionID()const
      {
         return _id;
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

      virtual UINT64 getLastLSN()const
      {
         return _logger->_lsn.load();
      }

      virtual void waitForCurrentWritingId()
      {
         return ;
      }
   private:
      UINT32 _id = 0;
      test_logger *_logger = NULL;
};

class test_session_mgr : public ::engine::vessel::ISessionManager
{
   public:
      test_session_mgr(){}
      virtual ~test_session_mgr(){}

   public:
      virtual ::engine::vessel::ISession *createNewSession()
      {
         static std::atomic_int id;
         test_logger *logger = test_logger::instance();
         return SDB_OSS_NEW test_session(id++, logger);
      }
      virtual void destroySession(::engine::vessel::ISession *session)
      {
         SAFE_OSS_DELETE(session);
      }

      static test_session_mgr *instance()
      {
         static test_session_mgr mgr;
         return &mgr;
      }
      
};//class test_session_mgr

class test_outer_resource
{
   public:
      test_outer_resource(){}
      ~test_outer_resource(){}

   public:
      static ::engine::vessel::outerResource getResource()
      {
         ::engine::vessel::outerResource r;
         r.indexKeyGen = ::engine::vessel::indexKeyGenForBsonRecord;
         r.logger = test_logger::instance();
         r.sessionMgr = test_session_mgr::instance();
         return r;
      }
};//class test_outer_resource

#endif//VESSEL_TEST_TEST_DEF_H_