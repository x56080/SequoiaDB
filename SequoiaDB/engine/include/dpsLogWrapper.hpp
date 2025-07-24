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

   Source File Name = dpsLogWrapper.hpp

   Descriptive Name = Data Protection Services Log Wrapper Header

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains declare for dpsLogWrapper.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSLOGWRAPPER_HPP__
#define DPSLOGWRAPPER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "interface/IDataProtectionService.h"
#include "dpsReplicaLogMgr.hpp"
#include "dpsArchiveMgr.hpp"
#include "../bson/bsonelement.h"
#include "../bson/bsonobj.h"
#include <vector>
using namespace bson;
using namespace std ;

namespace engine
{

   /*
      macro define
   */
   #define DPS_DFT_LOG_BUF_SZ          (1024)

   class _pmdEDUCB ;


   /*
      _dpsLogWrapper define
   */
   class _dpsLogWrapper : public IControlBlock,
                          public IDataProtectionService
   {
   private:
      _dpsReplicaLogMgr          _buf ;
      BOOLEAN                    _initialized ;
      BOOLEAN                    _dpslocal ;
      vector< dpsEventHandler* > _vecEventHandler ;
      dpsArchiveMgr              _archiver ;

      UINT32                     _syncInterval ;
      UINT32                     _syncRecordNum ;

      UINT32                     _writeReordNum ;
      UINT64                     _lastWriteTick ;
      UINT64                     _lastSyncTime ;

   public:
      _dpsLogWrapper() ;
      virtual ~_dpsLogWrapper() ;

   public:/// IControlBlock
      virtual SDB_CB_TYPE cbType() const override { return SDB_CB_DPS ; }
      virtual const CHAR* cbName() const override { return "DPSCB" ; }

      virtual INT32  init() override ;
      virtual INT32  active() override ;
      virtual INT32  deactive() override ;
      virtual INT32  fini() override ;
      virtual void   onConfigChange() override ;

   public:/// IDataSyncBase
      virtual BOOLEAN isClosed() const override ;
      virtual BOOLEAN canSync( BOOLEAN &force ) const override ;

      virtual INT32 sync( BOOLEAN force,
                          BOOLEAN sync,
                          IExecutor* cb ) override ;

      virtual void lock() override ;
      virtual void unlock() override ;

   public:/// IDataJournal
      virtual DPS_LSN getMinFileLSN() override { return getStartLsn(FALSE) ; }
      virtual DPS_LSN getMinBufLSN() override { return getStartLsn(TRUE) ; }
      virtual DPS_LSN getCurrentLSN() override { return getCurrentLsn() ; }
      virtual DPS_LSN getExpectedLSN() override { return expectLsn() ; }
      virtual DPS_LSN getCommittedLSN() override { return commitLsn() ; }

      virtual void getLsnWindow( DPS_LSN &minFileLSN,
                                 DPS_LSN &minBufLSN,
                                 DPS_LSN &currentLSN,
                                 DPS_LSN *expectedLSN,
                                 DPS_LSN *committedLSN ) override ;

      virtual INT32 write( IExecutor *executor,
                           const dpsWriteRequest &request,
                           const dpsWriteOptions &o,
                           dpsLogRecordHeader *result ) override ;

      virtual INT32 search( const DPS_LSN &lsn,
                            const dpsSearchOptions &o,
                            dpsMessageBlock &block )  override ;

      virtual INT32 replicate( const CHAR *rawdata, UINT32 size ) override
      {
         return recordRow(rawdata, size);
      }

      virtual INT32 flush( DPS_LSN_OFFSET offset, BOOLEAN async ) override ;

      virtual INT32 move( const DPS_LSN_OFFSET &lsn,
                          const DPS_LSN_VER &version ) override ;

   public:/// IDataProtectionService
      virtual void regEventHandler( dpsEventHandler *handler ) override ;
      virtual void unregEventHandler( dpsEventHandler *handler ) override ;
      virtual INT32 completeOpr( IExecutor *executor, INT32 w ) override ;
      virtual INT32 archive() override ;
      virtual INT32 process( IExecutor *executor, dpsRequestContext &ctx ) override ;
      virtual INT32 loadOpl( const DPS_LSN &lastNodeLSN,
                             dpsOperationList &opl ) override ;

   public: /// make it compatible with old interfaces.
      INT32 search( const DPS_LSN &minLsn,
                    _dpsMessageBlock *mb,
                    UINT8 type = DPS_SEARCH_ALL,
                    INT32 maxNum = 1,
                    INT32 maxTime = -1,
                    INT32 maxSize = 5242880 ) ;

      INT32 searchHeader( const DPS_LSN &lsn,
                          _dpsMessageBlock *mb,
                          UINT8 type = DPS_SEARCH_ALL ) ;

      DPS_LSN getStartLsn ( BOOLEAN logBufOnly = FALSE ) ;

      DPS_LSN expectLsn() ;
      DPS_LSN commitLsn() ;
      DPS_LSN getCurrentLsn() ;

      void getLsnWindow( DPS_LSN &beginLsn,
                         DPS_LSN &endLsn,
                         DPS_LSN *pExpectLsn,
                         DPS_LSN *committed );

   public:
      OSS_INLINE _dpsReplicaLogMgr *getLogMgr ()
      {
         return &_buf ;
      }
      OSS_INLINE BOOLEAN isLogLocal() const
      {
         return _dpslocal ;
      }
      OSS_INLINE INT32 run( _pmdEDUCB *cb )
      {
         if ( !_initialized )
         {
            return SDB_OK ;
         }
         return _buf.run( cb );
      }
      OSS_INLINE INT32 tearDown()
      {
         if ( !_initialized )
         {
            return SDB_OK ;
         }
         return _buf.tearDown();
      }
      OSS_INLINE BOOLEAN doLog () const
      {
         return _initialized ;
      }

      // note flushAll function is ONLY USED IN TESTCASE
      // engine should NEVER call flushAll in any situation.
      // The log write thread supposed to call run() in order to flush dirty
      // pages once at a time
      OSS_INLINE INT32 flushAll()
      {
         SDB_ASSERT ( _initialized, "shouldn't call flushAll without init" ) ;
         return _buf.flushAll() ;
      }

      OSS_INLINE void incVersion( UINT8 incVerVal = DPS_INC_VER_DFT )
      {
         _buf.incVersion( incVerVal ) ;
      }

      OSS_INLINE void cancelIncVersion()
      {
         _buf.cancelIncVersion() ;
      }

      OSS_INLINE INT32 checkSyncControl( UINT32 reqLen, _pmdEDUCB *cb )
      {
         return _buf.checkSyncControl( reqLen, cb ) ;
      }

      OSS_INLINE INT32 checkSeondarySyncControl( UINT32 reqLen, _pmdEDUCB *cb )
      {
         return _buf.checkSeondarySyncControl( reqLen, cb ) ;
      }

      OSS_INLINE INT32 saveRollbackLog( const DPS_LSN_OFFSET &offset )
      {
         return _archiver.saveRollbackLog( &_buf, offset ) ;
      }

      INT32 commit( BOOLEAN deeply, DPS_LSN *committedLsn ) ;

      INT32 recordRow( const CHAR *row, UINT32 len ) ;

   public:
      INT32 prepare( dpsMergeInfo &info ) ;
      void  writeData ( dpsMergeInfo &info ) ;

      void setLogFileSz ( UINT32 logFileSz )
      {
         _buf.setLogFileSz ( logFileSz ) ;
      }
      UINT32 getLogFileSz ()
      {
         return _buf.getLogFileSz () ;
      }
      void setLogFileNum ( UINT32 logFileNum )
      {
         _buf.setLogFileNum ( logFileNum ) ;
      }
      UINT32 getLogFileNum ()
      {
         return _buf.getLogFileNum () ;
      }
      UINT32 calcFileID ( DPS_LSN_OFFSET offset )
      {
         return _buf.calcFileID( offset ) ;
      }

      BOOLEAN isInRestore() ;

      DPS_LSN_OFFSET readOldestBeginLsnOffset() const ;

   private:
      INT32 _preprocess( IExecutor *executor, dpsRequestContext &ctx ) ;

      INT32 _prebuildOpl( IExecutor *executor, dpsRequestContext &ctx ) ;

      void _notifyEventHandlers( DPS_LSN_OFFSET lsn );

      INT32 _extractOplNode( const CHAR *data,
                             UINT32 size,
                             DPS_LSN &preNode,
                             utilUniqueBuffer &buffer ) const ;

   };
   typedef class _dpsLogWrapper SDB_DPSCB ;

   /*
      get dps cb
   */
   SDB_DPSCB* sdbGetDPSCB() ;

}

#endif // DPSLOGWRAPPER_HPP__
