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

   Source File Name = test_def.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
      virtual void      writingDB( BOOLEAN writing, const CHAR* name ) {}

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
      virtual UINT64    getEndLsn() const {return dummyDataJournal::instance()->getCurrentLsnOffset();}
      virtual UINT32    getLsnCount () const {return 0;}
      virtual BOOLEAN   isDoRollback () const {return FALSE;}

      virtual const DPS_TRANS_ID &getTransID () const {return transID;}
      virtual UINT64    getCurTransLsn () const {return -1;}
   #if defined( SDB_ENGINE )
      virtual INT32 getTransIsolation() const
      {
         return _transIsolation;
      }
   #endif
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

      void setTransIsolation(TRANS_ISOLATION_LEVEL transIsolation)
      {
         _transIsolation = transIsolation;
      }

   public:
      EDUID _id = 0;
      DPS_TRANS_ID transID;
      TRANS_ISOLATION_LEVEL _transIsolation;

   public:
      ossPoolMap<_dpsTransLockId, ossSharedLatchMode> _locked;
};//

static void lite_buffer_pool_watcher_entry(test_executor *executor, void *obj)
{
   ::engine::vessel::vesselImpl *impl = (::engine::vessel::vesselImpl *)obj;
   impl->attachLiteBufferPoolWatcher(executor);
}

static void worker_entry(test_executor *executor, void *obj)
{
   ::engine::vessel::backgroundWorker *worker = (::engine::vessel::backgroundWorker *)obj;
   worker->attach(executor);
}

static void lob_pool_watcher_entry(test_executor *executor, void *obj)
{
   ::engine::vessel::vesselImpl *impl = (::engine::vessel::vesselImpl *)obj;
   impl->attachLobcWatcher(executor);
}

static void hit_mgr_entry(test_executor *executor, void *obj)
{
   ::engine::vessel::vesselImpl *impl = (::engine::vessel::vesselImpl *)obj;
   impl->attachHitManager(executor);
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

         switch (type)
         {
         case EDU_TYPE_VESSEL_LITE_BUFFER_POOL_WATCHER:
            _threads.push_back(std::move(std::thread(lite_buffer_pool_watcher_entry, executor, args)));
            break;
         case EDU_TYPE_VESSEL_WORKER:
            _threads.push_back(std::move(std::thread(worker_entry, executor, args)));
            break;
         case EDU_TYPE_VESSEL_LOBC_BUFFER_POOL_WATCHER:
            _threads.push_back(std::move(std::thread(lob_pool_watcher_entry, executor, args)));
            break;
         case EDU_TYPE_VESSEL_HIT_MANAGER:
            _threads.push_back(std::move(std::thread(hit_mgr_entry, executor, args)));
            break;
         default:
            SDB_ASSERT(FALSE, "invalid type");
            break;
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
                                       const bson::BSONObj &pattern,
                                       BOOLEAN isCompression = FALSE)
   {
      bson::BSONObjBuilder builder;
      builder.append(IXM_NAME_FIELD, name);
      const CHAR *typeStr = nullptr;
      if (INDEX_TYPE_BTREE == type)
      {
         typeStr = IXM_BTREE;
      }
      else if (INDEX_TYPE_LSM == type)
      {
         typeStr = IXM_LSM_TREE;
      }
      else if (INDEX_TYPE_HYBRID_TREE == type)
      {
         typeStr = IXM_HYBRID_TREE;
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid index type");
      }
      builder.append(IXM_TYPE_FIELD, typeStr);
      builder.append(IXM_KEY_FIELD, pattern);
      builder.appendBool(IXM_UNIQUE_FIELD, unique);
      if(isCompression)
      {
         builder.appendBool(IXM_COMPRESSION, TRUE);
      }
      return builder.obj();
   }
};

#endif//VESSEL_TEST_TEST_DEF_H_