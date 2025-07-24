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

   Source File Name = dpsLogWrapper.cpp

   Descriptive Name = Data Protection Service Log Wrapper

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log wrapper,
   which is also called DPS Control Block

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/01/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsLogWrapper.hpp"
#include "dpsLogDef.hpp"
#include "dpsReplicaLogMgr.hpp"
#include "pd.hpp"
#include "dpsMergeBlock.hpp"
#include "dpsOp2Record.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "dpsLogRecordDef.hpp"
#include "pmd.hpp"
#include "dpsUtil.hpp"
#include "dpsPubElementDef.hpp"

namespace engine
{
   #define DPS_NO_WRITE_TIME                 ( 5000 )   // 5 seconds

   /*
      _dpsLogWrapper implement
   */
   _dpsLogWrapper::_dpsLogWrapper()
   {
      _initialized   = FALSE ;
      _dpslocal      = FALSE ;

      _syncInterval  = 0 ;
      _syncRecordNum = 0 ;
      _writeReordNum = 0 ;
      _lastWriteTick = 0 ;
      _lastSyncTime  = 0 ;
   }
   _dpsLogWrapper::~_dpsLogWrapper()
   {
      SDB_ASSERT( _vecEventHandler.size() == 0,
                  "Event handler size is not 0" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_INIT, "_dpsLogWrapper::init" )
   INT32 _dpsLogWrapper::init ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_INIT ) ;

      pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;

      _dpslocal = optCB->isDpsLocal() ;
      _buf.setLogFileSz( optCB->getReplLogFileSz() ) ;
      _buf.setLogFileNum( optCB->getReplLogFileNum() ) ;

      rc = _buf.init( optCB->getReplLogPath(),
                      optCB->getReplLogBuffSize(),
                      sdbGetTransCB() ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _syncInterval = optCB->getSyncInterval() ;
      _syncRecordNum = optCB->getSyncRecordNum() ;
      dpsGetGlobalLogConfig().updateConf( optCB->logTimeOn(),
                                          optCB->logWriteMod() ) ;

      pmdGetSyncMgr()->setLogAccess( this ) ;
      pmdGetSyncMgr()->setMainUnit( this ) ;

      if ( optCB->archiveOn() )
      {
         rc = _archiver.init( this, optCB->getArchivePath() ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      _initialized = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_REGEVENTHANDLER, "_dpsLogWrapper::regEventHandler" )
   void _dpsLogWrapper::regEventHandler( dpsEventHandler *pHandler )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_REGEVENTHANDLER ) ;

      SDB_ASSERT( pHandler, "Handle can't be NULL" ) ;
      SDB_ASSERT( pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must register in main thread" ) ;
      for ( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
      {
         SDB_ASSERT( pHandler != _vecEventHandler[ i ],
                     "Handle can't be same" ) ;
      }
      _vecEventHandler.push_back( pHandler ) ;
      _buf.regEventHandler( pHandler ) ;

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_REGEVENTHANDLER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_UNREGEVENTHANDLER, "_dpsLogWrapper::unregEventHandler" )
   void _dpsLogWrapper::unregEventHandler( dpsEventHandler *pHandler )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_UNREGEVENTHANDLER ) ;

      SDB_ASSERT( pHandler, "Handle can't be NULL" ) ;
      SDB_ASSERT( pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must unregister in main thread" ) ;

      vector< dpsEventHandler* >::iterator it = _vecEventHandler.begin() ;
      while ( it != _vecEventHandler.end() )
      {
         if ( *it == pHandler )
         {
            _vecEventHandler.erase( it ) ;
            break ;
         }
         ++it ;
         continue ;
      }
      _buf.unregEventHandler( pHandler ) ;

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_UNREGEVENTHANDLER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_ACTIVE, "_dpsLogWrapper::active" )
   INT32 _dpsLogWrapper::active ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_ACTIVE ) ;

      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // dps log writer
      rc = pEDUMgr->startEDU( EDU_TYPE_LOGGW, (void*)this, &eduID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start dps log writer failed, rc: %d", rc ) ;
         goto error ;
      }

      // dps trans rollback task
      rc = pEDUMgr->startEDU( EDU_TYPE_DPSROLLBACK, NULL, &eduID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start dps trans rollback failed, rc: %d", rc ) ;
         goto error ;
      }

      // dps log archiving
      if ( pmdGetKRCB()->getOptionCB()->archiveOn() )
      {
         rc = pEDUMgr->startEDU( EDU_TYPE_LOGARCHIVEMGR, (void*)this, &eduID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Start dps log archiving failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_ACTIVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsLogWrapper::deactive ()
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_FINI, "_dpsLogWrapper::fini" )
   INT32 _dpsLogWrapper::fini ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_FINI ) ;

      if ( pmdGetKRCB()->getOptionCB()->archiveOn() )
      {
         rc = _archiver.fini() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      _buf.fini() ;

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_FINI, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dpsLogWrapper::onConfigChange ()
   {
      pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;
      _dpslocal = optCB->isDpsLocal() ;
      _syncInterval = optCB->getSyncInterval() ;
      _syncRecordNum = optCB->getSyncRecordNum() ;
      dpsGetGlobalLogConfig().updateConf( optCB->logTimeOn(),
                                          optCB->logWriteMod() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_SEARCH, "_dpsLogWrapper::search" )
   INT32 _dpsLogWrapper::search( const DPS_LSN &minLsn,
                                 _dpsMessageBlock *mb,
                                 UINT8 type,
                                 INT32 maxNum,
                                 INT32 maxTime,
                                 INT32 maxSize )
   {
      SDB_ASSERT ( _initialized, "shouldn't call search without init" ) ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_SEARCH ) ;

      INT32 rc = SDB_OK ;
      UINT32 length = 0 ;
      DPS_LSN searchLsn = minLsn ;
      UINT64 bTime = 0 ;

      if ( maxTime > 0 )
      {
         bTime = (UINT64)time( NULL ) ;
      }

      while( TRUE )
      {
         rc = _buf.search( searchLsn, mb, type, FALSE, &length ) ;
         if ( rc )
         {
            break ;
         }
         searchLsn.offset += length ;

         if ( maxNum > 0 )
         {
            --maxNum ;
         }
         if ( maxSize > 0 )
         {
            maxSize = (UINT32)maxSize > length ? maxSize - length : 0 ;
         }

         /// max num check
         if ( 0 == maxNum )
         {
            break ;
         }
         /// max size check
         if ( 0 == maxSize )
         {
            break ;
         }
         /// max time check
         if ( maxTime > 0 && time( NULL ) - bTime >= (UINT32)maxTime )
         {
            break ;
         }
      }

      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_SEARCH, rc ) ;
      return rc ;
   }

   INT32 _dpsLogWrapper::searchHeader( const DPS_LSN &lsn,
                                       _dpsMessageBlock *mb,
                                       UINT8 type )
   {
      SDB_ASSERT ( _initialized, "shouldn't call search without init" ) ;
      return _buf.search( lsn, mb, type, TRUE ) ;
   }

   DPS_LSN _dpsLogWrapper::getStartLsn ( BOOLEAN logBufOnly )
   {
      if ( !_initialized )
      {
         DPS_LSN lsn ;
         return lsn ;
      }
      return _buf.getStartLsn ( logBufOnly ) ;
   }

   DPS_LSN_OFFSET _dpsLogWrapper::readOldestBeginLsnOffset() const
   {
      return _buf.readOldestBeginLsnOffset() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_GETCURRENTLSN, "_dpsLogWrapper::getCurrentLsn" )
   DPS_LSN _dpsLogWrapper::getCurrentLsn()
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_GETCURRENTLSN ) ;
      DPS_LSN lsn = _buf.currentLsn() ;
      PD_TRACE_EXIT( SDB__DPSLGWRAPP_GETCURRENTLSN ) ;
      return lsn ;
   }

   void _dpsLogWrapper::getLsnWindow( DPS_LSN &beginLsn,
                                      DPS_LSN &endLsn,
                                      DPS_LSN *pExpectLsn,
                                      DPS_LSN *committed )
   {
      if ( _initialized )
      {
         DPS_LSN memLsn ;
         _buf.getLsnWindow( beginLsn, memLsn, endLsn, pExpectLsn, committed ) ;
      }
   }

   void _dpsLogWrapper::getLsnWindow( DPS_LSN &fileBeginLsn,
                                      DPS_LSN &memBeginLsn,
                                      DPS_LSN &endLsn,
                                      DPS_LSN *pExpectLsn,
                                      DPS_LSN *committed )
   {
      if ( _initialized )
      {
         _buf.getLsnWindow( fileBeginLsn, memBeginLsn, endLsn,
                            pExpectLsn, committed ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_EXPECTLSN, "_dpsLogWrapper::expectLsn" )
   DPS_LSN _dpsLogWrapper::expectLsn()
   {
      DPS_LSN lsn ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_EXPECTLSN ) ;
      if ( _initialized )
      {
         lsn = _buf.expectLsn() ;
      }

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_EXPECTLSN ) ;
      return lsn ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_COMMITLSN, "_dpsLogWrapper::commitLsn" )
   DPS_LSN _dpsLogWrapper::commitLsn()
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_COMMITLSN ) ;
      DPS_LSN lsn = _buf.commitLsn() ;
      PD_TRACE_EXIT( SDB__DPSLGWRAPP_COMMITLSN ) ;
      return lsn ;
   }

   INT32 _dpsLogWrapper::move( const DPS_LSN_OFFSET &offset,
                               const DPS_LSN_VER &version )
   {
      /// make sure the version is correct
      if ( DPS_INVALID_LSN_OFFSET != offset &&
           DPS_INVALID_LSN_VERSION == version )
      {
         return _buf.move( offset, DPS_INVALID_LSN_VERSION + 1 ) ;
      }
      return _buf.move( offset, version ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_WRITEDATA, "_dpsLogWrapper::writeData" )
   void _dpsLogWrapper::writeData ( dpsMergeInfo & info )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_WRITEDATA ) ;

      _lastWriteTick = pmdGetDBTick() ;
      ++_writeReordNum ;

      _buf.writeData( info ) ;

      IExecutor *cb = info.getEDUCB() ;

      /// insert lsn
      if ( cb )
      {
         if ( info.hasDummy() )
         {
            cb->insertLsn( info.getDummyBlock().record().head()._lsn ) ;
         }
         cb->insertLsn( info.getMergeBlock().record().head()._lsn ) ;
      }

      /// notify
      if ( _vecEventHandler.size() > 0 && info.isNeedNotify() )
      {
         DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET ;
         if ( info.hasDummy() )
         {
            offset = info.getDummyBlock().record().head()._lsn ;
            for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
            {
               _vecEventHandler[i]->onWriteLog( offset ) ;
            }
         }
         offset = info.getMergeBlock().record().head()._lsn ;
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            _vecEventHandler[i]->onWriteLog( offset ) ;
         }
      }

      // it is transaction operations
      if ( info.isTransEnabled() && cb &&
           cb->getTransID().isValid() )
      {
         DPS_TRANS_ID transID = cb->getTransID() ;
         dpsTransCB * transCB = sdbGetTransCB() ;
         cb->setCurTransLsn( info.getMergeBlock().record().head()._lsn ) ;

         if ( transCB->isFirstOp( transID ) )
         {
            transCB->clearFirstOpTag( transID ) ;
            cb->setTransID( transID ) ;
         }
      }

      // reset
      info.resetInfoEx() ;

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_WRITEDATA ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_COMPLETEOPR, "_dpsLogWrapper::completeOpr" )
   INT32 _dpsLogWrapper::completeOpr( IExecutor *executor, INT32 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_COMPLETEOPR ) ;
      pmdEDUCB *cb = static_cast<pmdEDUCB *>(executor);

      if ( w > 1 && cb && 0 != cb->getLsnCount() &&
           _vecEventHandler.size() > 0 )
      {
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            rc = _vecEventHandler[i]->onCompleteOpr( cb, w ) ;
            if ( rc )
            {
               break ;
            }
         }
         cb->resetLsn() ;
      }

      PD_TRACE_EXITRC ( SDB__DPSLGWRAPP_COMPLETEOPR, rc ) ;
      return rc ;
   }

   // record a row
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_RECDROW, "_dpsLogWrapper::recordRow" )
   INT32 _dpsLogWrapper::recordRow( const CHAR *row, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_RECDROW );
      if ( !_initialized )
      {
         goto done;
      }
      {
         SDB_ASSERT( NULL != row, "row should not be NULL!") ;
         _dpsMergeBlock block ;
         dpsLogRecord &record = block.record();
         dpsLogRecordHeader &header = record.head() ;
         ossMemcpy( &header, row, sizeof(dpsLogRecordHeader) );
         block.setRow( TRUE ) ;
         rc = record.push( DPS_LOG_ROW_ROWDATA,
                           header._length -  sizeof(dpsLogRecordHeader),
                           row + sizeof(dpsLogRecordHeader)) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to push row to record:%d", rc ) ;
            goto error;
         }

         _lastWriteTick = pmdGetDBTick() ;
         ++_writeReordNum ;

         rc = _buf.merge( block );
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSLGWRAPP_RECDROW, rc );
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dpsLogWrapper::isInRestore()
   {
      return _buf.isInRestore() ;
   }

   INT32 _dpsLogWrapper::commit( BOOLEAN deeply, DPS_LSN *committedLsn )
   {
      ossTimestamp t ;
      ossGetCurrentTime( t ) ;
      _lastSyncTime = t.time * 1000 + t.microtm / 1000 ;
      /// clear write info
      _writeReordNum = 0 ;

      return _buf.commit( deeply, committedLsn ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_PREPARE, "prepare" )
   INT32 _dpsLogWrapper::prepare( dpsMergeInfo &info )
   {
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP_PREPARE ) ;
      INT32 rc = SDB_OK ;
      if ( !_initialized )
      {
         goto done;
      }

<<<<<<< HEAD
=======
      if ( NULL == info.getEDUCB() ||
           !info.getEDUCB()->getTransID().isGlobTrans() ||
           !info.isTransEnabled() )
      {
         // for non-global transaction operators, set irreversible
         info.setIrreversible() ;
      }

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      if( NULL != sdbGetThreadExecutor() )
      {
         ISession* session = sdbGetThreadExecutor()->getSession() ;

         if( NULL == session || !session->isBusinessSession() )
         {
            dpsLogRecordHeader& head = info.getMergeBlock().record().head() ;
            head.setFlag( DPS_FLG_NON_BS_OP ) ;
         }
      }

      rc = _buf.preparePages( info ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to prepare pages, rc = %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dpsLogWrapper::isClosed() const
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_CANSYNC, "_dpsLogWrapper::canSync" )
   BOOLEAN _dpsLogWrapper::canSync( BOOLEAN &force ) const
   {
      BOOLEAN needSync = FALSE ;
      force = FALSE ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP_CANSYNC ) ;

      if ( !_buf.hasDirty() )
      {
         /// nothing
      }
      else if ( _syncRecordNum > 0 && _writeReordNum >= _syncRecordNum )
      {
         PD_LOG( PDDEBUG, "Write record number[%u] more than threshold[%u]",
                 _writeReordNum, _syncRecordNum ) ;
         force = TRUE ;
         needSync = TRUE ;
      }
      else if ( pmdGetTickSpanTime( _lastWriteTick ) < DPS_NO_WRITE_TIME )
      {
         /// nothing
      }
      else if ( _syncInterval > 0 )
      {
         ossTimestamp tm ;
         ossGetCurrentTime( tm ) ;
         UINT64 curTime = tm.time * 1000 + tm.microtm / 1000 ;

         if ( curTime - _lastSyncTime >= _syncInterval )
         {
            PD_LOG( PDDEBUG, "Time interval threshold tiggered, "
                    "CurTime:%llu, LastSyncTime:%llu, SyncInterval:%u",
                    curTime, _lastSyncTime, _syncInterval ) ;
            needSync = TRUE ;
         }
      }

      PD_TRACE1 ( SDB__DPSLGWRAPP_CANSYNC, PD_PACK_INT( needSync ) ) ;
      PD_TRACE_EXIT( SDB__DPSLGWRAPP_CANSYNC ) ;
      return needSync ;
   }

   INT32 _dpsLogWrapper::sync( BOOLEAN force,
                               BOOLEAN sync,
                               IExecutor *cb )
   {
      return commit( sync, NULL ) ;
   }

   void _dpsLogWrapper::lock()
   {
   }

   void _dpsLogWrapper::unlock()
   {
   }

   INT32 _dpsLogWrapper::search( const DPS_LSN &lsn,
                                 const dpsSearchOptions &o,
                                 dpsMessageBlock &block )
   {
      UINT8 type = 0;
      if (o.searchMem)
      {
         type |= DPS_SEARCH_MEM;
      }
      if (o.searchFile)
      {
         type |= DPS_SEARCH_FILE;
      }
      return o.onlyHeader ?
             searchHeader(lsn, &block, type ) :
             search( lsn, &block, type,
                     o.limits, o.maxTime, o.maxSize ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_WRITE, "_dpsLogWrapper::write" )
   INT32 _dpsLogWrapper::write( IExecutor *executor,
                                const dpsWriteRequest &request,
                                const dpsWriteOptions &o,
                                dpsLogRecordHeader *result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP_WRITE ) ;
      dpsLogRecordHeader lres ;
      dpsLogRecordHeader *rptr = nullptr == result ? & lres : result ;

      rc = _buf.write( executor, request, o, rptr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write dps request:%d", rc ) ;
         goto error ;
      }

      ///TODO: atomic number
      _lastWriteTick = pmdGetDBTick() ;
      ++_writeReordNum ;

      /// the lsn in result must be the last lsn of current executor,
      /// even there was a dummy record created.
      /// Actually, we do not need to care about dummy record here.

      if ( nullptr != executor )
      {
         executor->insertLsn( rptr->_lsn ) ;

         if ( o.transEnabled && executor->getTransID().isValid() )
         {
            DPS_TRANS_ID transID = executor->getTransID() ;
            dpsTransCB * transCB = sdbGetTransCB() ;
            executor->setCurTransLsn( rptr->_lsn ) ;
            if ( transCB->isFirstOp( transID ) )
            {
               transCB->clearFirstOpTag( transID ) ;
               executor->setTransID( transID ) ;
            }
         }
      }

      if ( o.notify )
      {
         _notifyEventHandlers( rptr->_lsn ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsLogWrapper::flush( DPS_LSN_OFFSET offset, BOOLEAN async )
   {
      INT32 rc = SDB_OK ;
      if ( DPS_INVALID_LSN_OFFSET != offset )
      {
         DPS_LSN committedLSN = _buf.commitLsn() ;
         if ( offset <= committedLSN.offset )
         {
            goto done ;
         }
      }

      rc = _buf.commit( FALSE, nullptr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to commit log record with lsn[%lld], rc:%d",
                 offset, rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsLogWrapper::archive()
   {
      if ( !_initialized )
      {
         return SDB_OK ;
      }
      return _archiver.run() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_PROCESS, "_dpsLogWrapper::process" )
   INT32 _dpsLogWrapper::process( IExecutor *executor, dpsRequestContext &ctx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP_PROCESS ) ;
      if ( OSS_UNLIKELY( nullptr == executor) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _preprocess( executor, ctx ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to preprocess dps request:%d", rc ) ;
         goto error ;
      }

      rc = write( executor, ctx.getReq(), ctx.getWriteOptions(), ctx.getResultPtr() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write dps log record:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_PROCESS, rc ) ;
      return rc ;
   error:
      goto done ; 
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP__PREPROCESS, "_dpsLogWrapper::_preprocess" )
   INT32 _dpsLogWrapper::_preprocess( IExecutor *executor, dpsRequestContext &ctx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP__PREPROCESS ) ;
      SDB_ASSERT( nullptr != executor, "can not be invalid" ) ;

      if ( ctx.hasOplCtx() && ctx.getOplCtx()->hasBuildingOpl() )
      {
         rc = _prebuildOpl( executor, ctx ) ;
         if ( SDB_OK != rc ) 
         {
            PD_LOG( PDERROR, "failed to build op list:%d", rc ) ;
            goto error ;
         }
      }

      ctx.endToBuildRequest() ;
   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP__PREPROCESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP__PREBUILDOPL, "_dpsLogWrapper::_prebuildOpl" )
   INT32 _dpsLogWrapper::_prebuildOpl( IExecutor *executor, dpsRequestContext &ctx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP__PREBUILDOPL ) ;

      dpsWriteReqBuilder &builder = ctx.getBuilder() ;
      UINT16 flags = builder.getFlags() ;
      dpsOplistContext *opl = ctx.getOplCtx() ;
      SDB_ASSERT( nullptr != opl && opl->hasBuildingOpl(), "can not be invalid" ) ;
      
#if defined (_DEBUG)
      {
         dpsRecordElements __e = builder.peekElements() ;
         SDB_ASSERT( !__e.contains( DPS_LOG_PUBLIC_OPL_NODE ), "opl node already exists!" ) ;
         SDB_ASSERT( !__e.contains( DPS_LOG_PUBLIC_OPL_ROLLBACK_INFO ),
                     "opl rollback info already exists!" ) ;
      }
#endif 

      if ( opl->isFreshOpl() )
      {
         dpsSetOplNodeType( DPS_OPL_NODE_TYPE::HEAD, flags ) ;
      }
      else if ( ctx.isToCompleteOpl() )
      {
         DPS_OPL_NODE_TYPE type = ctx.isToCompleteOpl() ?
                                  DPS_OPL_NODE_TYPE::TAIL : DPS_OPL_NODE_TYPE::BODY ;
         dpsSetOplNodeType( type, flags ) ;
         rc = builder.appendObj( DPS_LOG_PUBLIC_OPL_NODE,
                                 dpsOplNodeEle( opl->getOplLSN(), opl->getPreNodeLSN() ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append opl node info:%d", rc ) ;
            goto error ;
         }
      }

      if ( ctx.hasOplRollbackTarget() )
      {
         if ( !opl->isOplRollingBack() )
         {
            opl->setOplRollingBack() ;
         }

         rc = builder.appendObj( DPS_LOG_PUBLIC_OPL_ROLLBACK_INFO,
                                 dpsOplRollbackInfoEle(ctx.getOplRollbackTarget() ) ) ;
         if ( SDB_OK != rc ) 
         {
            PD_LOG( PDERROR, "failed to append rolback info:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         SDB_ASSERT( !opl->isOplRollingBack(), "target to roll back missed!" ) ;
      }

      builder.overwriteFlags( flags ) ;

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP__PREBUILDOPL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dpsLogWrapper::_notifyEventHandlers( DPS_LSN_OFFSET lsn )
   {
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid" ) ;
      for ( auto itr = _vecEventHandler.begin(); itr != _vecEventHandler.end(); ++itr )
      {
         (*itr)->onWriteLog( lsn ) ;
      }
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_LOADOPL, "_dpsLogWrapper::loadOpl" )
   INT32 _dpsLogWrapper::loadOpl( const DPS_LSN &lastNodeLSN,
                                  dpsOperationList &opl )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP_LOADOPL ) ;
      dpsSearchOptions o ;
      dpsMessageBlock mb ;
      DPS_LSN target = lastNodeLSN ;
      ossPoolList<utilUniqueBuffer> l ;
      opl.reset() ;

      if ( OSS_UNLIKELY(lastNodeLSN.invalid()) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      do
      {
         utilUniqueBuffer buffer ;
         DPS_LSN preLSN ;
         rc = this->search( target, o, mb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to search record[%lld], rc:%d", target.offset, rc ) ;
            goto error ;
         }

         rc = _extractOplNode( mb.startPtr(), mb.length(), preLSN, buffer ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to resave record data:%d", rc ) ;
            goto error ;
         }

         try
         {
            l.push_front( std::move( buffer ) ) ;
         }
         catch( std::exception& e )
         {
            PD_LOG( PDERROR, "unexpected exception:%s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         target = preLSN ;
         
      } while ( !target.invalid() );
      
      rc = opl.initFromRecords( std::move(l) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init opl:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_LOADOPL, rc ) ;
      return rc ;
   error:
      opl.reset() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP__EXTRACEOPLN, "_dpsLogWrapper::_extractOplNode" )
   INT32 _dpsLogWrapper::_extractOplNode( const CHAR *data,
                                          UINT32 size,
                                          DPS_LSN &preNode,
                                          utilUniqueBuffer &buffer ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP__EXTRACEOPLN ) ;
      SDB_ASSERT( nullptr != data, "can not be invalid" ) ;
      SDB_ASSERT( DPS_LOG_HEAD_SIZE <= size, "can not be invalid" ) ;
      preNode.reset() ;
      buffer.reset() ;

      const dpsLogRecordHeader *header = reinterpret_cast<const dpsLogRecordHeader *>( data ) ;
      dpsRecordElements elements( data + DPS_LOG_HEAD_SIZE,
                                  size - DPS_LOG_HEAD_SIZE ) ;
      DPS_OPL_NODE_TYPE type = dpsGetOplNodeType( header->_flags ) ;

      if ( DPS_OPL_NODE_TYPE::NONE == type )
      {
         PD_LOG( PDERROR, "record[%lld] is not opl node", header->_lsn ) ;
         rc = SDB_DPS_BROKEN_OPL ;
         goto error ;
      }
      else if ( DPS_OPL_NODE_TYPE::HEAD != type )
      {
         dpsRecordElements::iterator itr = elements.seek( DPS_LOG_PUBLIC_OPL_NODE ) ;
         if ( OSS_UNLIKELY(!itr.isValid()) )
         {
            PD_LOG( PDERROR, "failed to seek opl node info in record[%lld]", header->_lsn ) ;
            rc = SDB_DPS_BROKEN_OPL ;
            goto error ;
         }

         if ( OSS_UNLIKELY(itr.getValue().size() != sizeof(dpsOplNodeEle)) )
         {
            PD_LOG( PDERROR, "invalid element size[%d] found in record[%lld]",
                    itr.getValue().size(), header->_lsn ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         preNode = itr.getValue().castTo<dpsOplNodeEle>()->preLSN ;
         SDB_ASSERT( !preNode.invalid(), "impossible" ) ; 
      }

      buffer = utilUniqueBuffer::allocate( size ) ;
      if ( OSS_UNLIKELY(!buffer) )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( buffer.get(), data, size ) ;

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP__EXTRACEOPLN, rc ) ;
      return rc ;
   error:
      preNode.reset() ;
      buffer.reset() ;
      goto done ;
   }

   /*
      get dps cb
   */
   SDB_DPSCB* sdbGetDPSCB()
   {
      static SDB_DPSCB s_dpscb ;
      return &s_dpscb ;
   }

}

