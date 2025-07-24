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

   
*******************************************************************************/
#include "dpsDef.hpp"
#include "dpsTransDef.hpp"
#include "dpsTransLockDef.hpp"
#include "ossSharedLatch.hpp"
#include "sdbInterface.hpp"
#include "vessel/dummyJournal.h"
namespace engine {
class testExecutor : public IExecutor
{
   public:
      testExecutor()
      {
         _id = ossRand();
      }
      virtual ~testExecutor(){}

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
      virtual UINT64    getEndLsn() const {return vessel::dummyDataJournal::instance()->getCurrentLsnOffset();}
      virtual UINT32    getLsnCount () const {return 0;}
      virtual BOOLEAN   isDoRollback () const {return FALSE;}
   #if defined( SDB_ENGINE )
      virtual INT32 getTransIsolation() const
      {
         return _transIsolation;
      }
   #endif

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

      void setTransIsolation(TRANS_ISOLATION_LEVEL transIsolation)
      {
         _transIsolation = transIsolation;
      }

   public:
      EDUID _id = 0;
      DPS_TRANS_ID transID;
      TRANS_ISOLATION_LEVEL _transIsolation = TRANS_ISOLATION_RU;

   public:
      ossPoolMap<_dpsTransLockId, ossSharedLatchMode> _locked;
};//
}
