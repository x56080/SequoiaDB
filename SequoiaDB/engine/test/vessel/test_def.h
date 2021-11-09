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
#include "vessel/IRedoLogger.h"
#include "dpsLogRecord.hpp"
#include "vessel/logRecordContext.h"
#include "vessel/indexKeyGenerator.h"
#include "vessel/outerResource.h"
#include "pd.hpp"
#include "sdbInterface.hpp"
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

class test_executor : public IExecutor
{
   public:
      test_executor()
      virtual ~test_executor(){}

   public:
      virtual EDUID     getID() const
      {
         return 0;
      }
      virtual UINT32    getTID() const
      {
         return 0;
      }

      /*
         Session Related
      */
      virtual ISession* getSession() {return NULL;}
      virtual IRemoteSite* getRemoteSite() {return NULL;}

      /*
         Status and Control
      */
      virtual BOOLEAN   isInterrupted ( BOOLEAN onlyFlag = FALSE ) {return FALSE;}
      virtual BOOLEAN   isDisconnected () {return FALSE;}
      virtual BOOLEAN   isForced () {return FALSE;}

      virtual BOOLEAN   isWritingDB() const {return FALSE;}
      virtual UINT64    getWritingID() const {return 0;}
      virtual void      writingDB( BOOLEAN writing ) {}

      virtual UINT32    getProcessedNum() const {return 0;}
      virtual void      incEventCount( UINT32 step = 1 ) {}

      virtual UINT32    getQueSize() {return 0;}

      /*
         Resource Info
      */
      virtual sdbLockItem* getLockItem( SDB_LOCK_TYPE lockType ) {return NULL;}
      virtual INT32        appendInfo( EDU_INFO_TYPE type, const CHAR * format, ...) {return SDB_OK;}
      virtual INT32        printInfo ( EDU_INFO_TYPE type, const CHAR *format, ... ) {return SDB_OK;}
      virtual const CHAR*  getInfo ( EDU_INFO_TYPE type ) {return NULL;}
      virtual void         resetInfo ( EDU_INFO_TYPE type ) {}

      /*
         Buffer Manager
      */
      virtual INT32     allocBuff( UINT32 len,
                                    CHAR **ppBuff,
                                    UINT32 *pRealSize = NULL ) {return -1;}

      virtual INT32     reallocBuff( UINT32 len,
                                       CHAR **ppBuff,
                                       UINT32 *pRealSize = NULL ) {return -1;}

      virtual void      releaseBuff( CHAR *pBuff ) {}}

      virtual void*     getAlignedBuff( UINT32 size,
                                          UINT32 *pRealSize = NULL,
                                          UINT32 alignment =
                                          OSS_FILE_DIRECT_IO_ALIGNMENT ) {return NULL;}

      virtual void      releaseAlignedBuff() {return ;}

      virtual CHAR*     getBuffer( UINT32 len ) {return NULL;}

      virtual void      releaseBuffer() {}

      /*
         Operation Related
      */
      /// for read
      virtual UINT64    getBeginLsn () const {return 0;}
      virtual UINT64    getEndLsn() const {return test_logger::instance()->_lsn.load();}
      virtual UINT32    getLsnCount () const {return 0;}
      virtual BOOLEAN   isDoRollback () const {return FALSE;}

      virtual const DPS_TRANS_ID &getTransID () const {return DPS_TRANS_ID();}
      virtual UINT64    getCurTransLsn () const {return -1;}
      /// for write
      virtual void      resetLsn() {}
      virtual void      insertLsn( UINT64 lsn,
                                    BOOLEAN isRollback = FALSE ) {}

      virtual void      setTransID( const DPS_TRANS_ID &transID ) {}
      virtual void      setCurTransLsn( UINT64 lsn ) {}

      /*
         Context Related
      */
      virtual void      contextInsert( INT64 contextID ) {}
      virtual void      contextDelete( INT64 contextID ) {}
      virtual INT64     contextPeek() {return -1;}
      virtual BOOLEAN   contextFind( INT64 contextID ) {return FALSE;}
      virtual UINT32    contextNum() {return 0;}
};//

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