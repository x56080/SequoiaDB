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
#include "dpsLogRecord.hpp"
#include "vessel/indexKeyGenerator.h"
#include "vessel/outerResource.h"
#include "pd.hpp"
#include "sdbInterface.hpp"
#include <atomic>
#include "ossThread.h"
#include "pmdDef.hpp"
#include "vessel/liteCacheWatcher.h"
#include "vessel/backgroundWorker.h"
#include "vessel/dummyJournal.h"
#include "dummyTransLockConsole.h"
#include "ixm_common.hpp"
#include "vessel/indexDef.h"
#include "vessel/vesselImpl.h"

using namespace engine::vessel;
using namespace engine;

static const CHAR *DATA_PATH = "/opt/unit_test/vessel/";
static const CHAR *LSM_PATH = "/opt/unit_test/lsm";


class test_executor : public IExecutor
{
   public:
      test_executor()
      {
         _id = ossRand();
      }
      virtual ~test_executor(){}

   public:
      virtual EDUID     getID() const
      {
         return _id;
      }
      virtual UINT32    getTID() const
      {
         return ossGetCurrentThreadID();
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

      virtual void      releaseBuff( CHAR *pBuff ) {}

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
      virtual UINT64    getEndLsn() const {return dummyDataJournal::instance()->getCurrentLSN();}
      virtual UINT32    getLsnCount () const {return 0;}
      virtual BOOLEAN   isDoRollback () const {return FALSE;}

      virtual const DPS_TRANS_ID &getTransID () const {return transID;}
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
      virtual BOOLEAN      contextInsert( INT64 contextID ) {return FALSE;}
      virtual void      contextDelete( INT64 contextID ) {}
      virtual INT64     contextPeek() {return -1;}
      virtual BOOLEAN   contextFind( INT64 contextID ) {return FALSE;}
      virtual UINT32    contextNum() {return 0;}

      virtual BOOLEAN   isLogTimeOn() const {return FALSE;}
      virtual UINT32    getLogWriteMod() const {return DPS_LOG_WRITE_MOD_INCREMENT;}

   public:
      EDUID _id = 0;
      DPS_TRANS_ID transID;

   public:
      ossPoolMap<_dpsTransLockId, ossSharedLatchMode> _locked;
};//

static void cache_watcher_entry(test_executor *executor, void *obj)
{
   ::engine::vessel::liteCacheWatcher *watcher = (::engine::vessel::liteCacheWatcher *)obj;
   watcher->attach(executor);
}

static void worker_entry(test_executor *executor, void *obj)
{
   ::engine::vessel::backgroundWorker *worker = (::engine::vessel::backgroundWorker *)obj;
   worker->activeEntry(executor);
}

static void lob_pool_watcher_entry(test_executor *executor, void *obj)
{
   ::engine::vessel::vesselImpl *impl = (::engine::vessel::vesselImpl *)obj;
   impl->attachLobcWatcher(executor);
}

class test_session_mgr : public ::engine::IExecutorMgr
{
   public:
      test_session_mgr(){}
      virtual ~test_session_mgr()
      {
         clear();
      }

   public:
      void clear()
      {
         for (UINT32 i = 0; i < _executors.size(); ++i)
         {
            SDB_OSS_DEL _executors[i];
         }
         _executors.clear();
         for (UINT32 i = 0; i < _threads.size(); ++i)
         {
            _threads[i].join();
         }
         _threads.clear();
      }
      virtual INT32 startEDU( INT32 type,
                              void *args,
                              EDUID *pEDUID,
                              const CHAR *pInitName)
      {
         INT32 rc = SDB_OK;
         test_executor *executor = SDB_OSS_NEW test_executor();
         if (NULL == executor)
         {
            rc = SDB_OOM;
            goto error;
         }

         executor->_id = _executors.size();

         if (type == EDU_TYPE_VESSEL_CACHE_WATCHER)
         {
            _threads.push_back(std::move(std::thread(cache_watcher_entry, executor, args)));
         }
         else if (type == EDU_TYPE_VESSEL_WORKER)
         {
            _threads.push_back(std::move(std::thread(worker_entry, executor, args)));
         }
         else if (type == EDU_TYPE_VESSEL_LOBC_BUFFER_POOL_WATCHER)
         {
            _threads.push_back(std::move(std::thread(lob_pool_watcher_entry, executor, args)));
         }
         else
         {
            SDB_ASSERT(FALSE, "invalid type");
         }

         _executors.push_back(executor);
      done:
         return rc;
      error:
         goto done;
      }

      virtual void      addIOService( IIOService *pIOService ){}
      virtual void      delIOSerivce( IIOService *pIOService ){}

      virtual UINT64 getMinRunningLSN(){return -1;}

      static test_session_mgr *instance()
      {
         static test_session_mgr mgr;
         return &mgr;
      }

   private:
      ossPoolVector<test_executor *> _executors;
      ossPoolVector<std::thread> _threads;
      
};//class test_session_mgr

class test_outer_resource
{
   public:
      test_outer_resource(){}
      ~test_outer_resource(){}

   public:
      static ::engine::vessel::outerResource getResource()
      {
         sdbEnablePD("/opt/diaglog/sdb.log", 1, 1000);
         setPDLevel(PDDEBUG);
         ::engine::vessel::outerResource r;
         r.indexKeyGen = ::engine::vessel::indexKeyGenForBsonRecord;
         r.journal = ::engine::vessel::dummyDataJournal::instance();
         r.executorPool = test_session_mgr::instance();
         r.transLockConsole = dummyTransLockConsole::instance();
         test_session_mgr::instance()->clear();
         return r;
      }
};//class test_outer_resource

class indexTestUtil
{
   public:
   static bson::BSONObj createIndexObj(INDEX_TYPE type,
                                       const CHAR *name,
                                       BOOLEAN unique,
                                       const bson::BSONObj &pattern)
   {
      bson::BSONObjBuilder builder;
      builder.append(IXM_NAME_FIELD, name);
      const CHAR *typeStr = INDEX_TYPE_BTREE == type ?
                            IXM_BTREE_FIELD : IXM_LSM_FIELD;
      builder.append(IXM_TYPE_FIELD, typeStr);
      builder.append(IXM_KEY_FIELD, pattern);
      builder.appendBool(IXM_UNIQUE_FIELD, unique);
      return builder.obj();
   }
};

#endif//VESSEL_TEST_TEST_DEF_H_