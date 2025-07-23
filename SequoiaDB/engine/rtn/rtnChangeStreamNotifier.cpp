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

   Source File Name = rtnChangeStreamNotifier.cpp

   Descriptive Name = Change Stream Notifier Source

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnChangeStreamNotifier.hpp"
#include "dpsDef.hpp"
#include "dpsLogDef.hpp"
#include "ossErr.h"
#include "ossEvent.hpp"
#include "ossLatch.hpp"
#include "ossMem.hpp"
#include "ossTypes.h"
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "rtnChangeStreamDispatcher.hpp"
#include "rtnTrace.hpp"
#include "utilChangeStreamWatchInfo.hpp"
#include "utilDataExInfo.hpp"

using namespace std ;

namespace engine
{

   static INT32 _rtnStartChangeStreamNotifierJob( rtnChangeStreamNotifier &notifier ) ;

   /*
      _rtnChangeStreamNotifier implement
    */
   _rtnChangeStreamNotifier::_rtnChangeStreamNotifier()
   : _watcherNum( 0 ),
     _clDispatcher( *this, UTIL_WATCH_COLLECTION ),
     _csDispatcher( *this, UTIL_WATCH_COLLECTION_SPACE ),
     _allDispatcher( *this, UTIL_WATCH_ALL )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_INIT, "_rtnChangeStreamNotifier::init" )
   INT32 _rtnChangeStreamNotifier::init( INT32 resumableWindowMB,
                                         UINT32 logBufferNum,
                                         UINT32 logFileSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_INIT ) ;

      // register DPS event handler
      if ( NULL != pmdGetKRCB()->getDPSCB() )
      {
         pmdGetKRCB()->getDPSCB()->regEventHandler( this ) ;
      }

      // calcuate resumable window
      _calculateResumableWindow( resumableWindowMB, logBufferNum, logFileSize ) ;

      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMNOTIFIER_INIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_ACTIVE, "_rtnChangeStreamNotifier::active" )
   INT32 _rtnChangeStreamNotifier::active()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_ACTIVE ) ;

      if ( NULL != pmdGetKRCB()->getDPSCB() )
      {
         _expectOffset = pmdGetKRCB()->getDPSCB()->expectLsn().offset ;
         PD_LOG( PDDEBUG, "initialized expected offset [%llu]", _expectOffset ) ;

         // start change stream notifier job
         rc = _rtnStartChangeStreamNotifierJob( *this ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start change stream notifier job, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMNOTIFIER_ACTIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _rtnChangeStreamNotifier::deactive()
   {
   }

   void _rtnChangeStreamNotifier::fini()
   {
      // unregister DPS event handler
      if ( NULL != pmdGetKRCB()->getDPSCB() )
      {
         pmdGetKRCB()->getDPSCB()->unregEventHandler( this ) ;
      }
      SDB_ASSERT( 0 == _watcherNum.peek(), "should be emtpy" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_ONCONFIGCHANGE, "_rtnChangeStreamNotifier::onConfigChange" )
   void _rtnChangeStreamNotifier::onConfigChange( INT32 resumableWindowMB,
                                                  UINT32 logBufferNum,
                                                  UINT32 logFileSize )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_ONCONFIGCHANGE ) ;

      // re-calcuate resumable window
      _calculateResumableWindow( resumableWindowMB, logBufferNum, logFileSize ) ;

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER_ONCONFIGCHANGE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_CANASSIGNLOGPAGE, "_rtnChangeStreamNotifier::canAssignLogPage" )
   INT32 _rtnChangeStreamNotifier::canAssignLogPage( UINT32 reqLen,
                                                     pmdEDUCB *cb,
                                                     BOOLEAN &needCache )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_CANASSIGNLOGPAGE ) ;

      if ( _watcherNum.peek() > 0 )
      {
         // need cache the log record for performance consideration
         needCache = TRUE ;
      }

      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMNOTIFIER_CANASSIGNLOGPAGE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_ONPREPARELOG, "_rtnChangeStreamNotifier::onPrepareLog" )
   void _rtnChangeStreamNotifier::onPrepareLog( const utilLogExInfo &info,
                                                DPS_LSN_OFFSET offset,
                                                DPS_LSN_VER version,
                                                UINT32 length,
                                                DPS_LOG_TYPE logType )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_ONPREPARELOG ) ;

      _notifyLog( info, offset, version, length, logType ) ;

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER_ONPREPARELOG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_ONWRITELOG, "_rtnChangeStreamNotifier::onWriteLog" )
   void _rtnChangeStreamNotifier::onWriteLog( DPS_LSN_OFFSET offset )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_ONWRITELOG ) ;

      if ( _watcherNum.peek() > 0 )
      {
         // signal all watchers, log record write done
         _writeEvent.signalAll() ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER_ONWRITELOG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_ONMOVELOG, "_rtnChangeStreamNotifier::onMoveLog" )
   void _rtnChangeStreamNotifier::onMoveLog( DPS_LSN_OFFSET moveToOffset,
                                             DPS_LSN_VER moveToVersion,
                                             DPS_LSN_OFFSET expectOffset,
                                             DPS_LSN_VER expectVersion,
                                             DPS_MOMENT moment,
                                             INT32 errcode )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_ONMOVELOG ) ;

      if ( DPS_AFTER == moment )
      {
         DPS_LSN stopLSN ;
         stopLSN.set( moveToOffset, moveToVersion ) ;

         // log is moved, the change streams will not be continous
         // stop all watchers
         _stopAllWatchers( SDB_DPS_LSN_MOVED, stopLSN ) ;

         _expectOffset = moveToOffset ;

         PD_LOG( PDDEBUG, "Move from [%llu] to [%llu]", expectOffset, moveToOffset ) ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER_ONMOVELOG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_ONREPLAYLOG, "_rtnChangeStreamNotifier::onReplayLog" )
   void _rtnChangeStreamNotifier::onReplayLog( const utilLogExInfo &info,
                                               DPS_LSN_OFFSET offset,
                                               DPS_LSN_VER version,
                                               UINT32 length,
                                               DPS_LOG_TYPE logType )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_ONREPLAYLOG ) ;

      _notifyLog( info, offset, version, length, logType ) ;

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER_ONREPLAYLOG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER_TRYDISPATCH, "_rtnChangeStreamNotifier::tryDispatch" )
   void _rtnChangeStreamNotifier::tryDispatch()
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_TRYDISPATCH ) ;

      UINT32 currentIndex = 0 ;
      utilChangeStreamLogInfo logInfoBatch[ s_batchSize ] ;

      // try pop a batch of log records
      _produceEvent.reset() ;
      if ( _notifyQueue.popBatch( logInfoBatch, s_batchSize, currentIndex ) )
      {
         _consumeEvent.signalIfWaiting() ;
      }
      else
      {
         _produceEvent.wait( s_waitProduceTimeout ) ;
      }

      for ( UINT32 i = 0 ; i < currentIndex ; ++ i )
      {
         INT32 tmpRC = SDB_OK ;

         utilChangeStreamLogInfo &logInfo = logInfoBatch[ i ] ;

         // dispatch log to collection watchers
         tmpRC = _clDispatcher.dispatchLog( logInfo ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to push log to [%s] dispatcher, "
                    "rc: %d", _clDispatcher.getWatchLevelName(), tmpRC ) ;
         }

         // dispatch log to collection space watchers
         tmpRC = _csDispatcher.dispatchLog( logInfo ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to push log to [%s] dispatcher, "
                    "rc: %d", _csDispatcher.getWatchLevelName(), tmpRC ) ;
         }

         // dispatch log to all watchers
         tmpRC = _allDispatcher.dispatchLog( logInfo ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to push log to [%s] dispatcher, "
                    "rc: %d", _allDispatcher.getWatchLevelName(), tmpRC ) ;
         }

         // update last dispatched offset
         _lastDispatchedOffset = logInfo.getOffset() ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER_TRYDISPATCH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNCHANGESTREAMNOTIFIER_REGWATCHER, "_rtnChangeStreamNotifier::registerWatcher" )
   INT32 _rtnChangeStreamNotifier::registerWatcher( rtnChangeStreamWatcher &watcher )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_REGWATCHER ) ;

      const utilChangeStreamWatchInfo &watchInfo = watcher.getWatchInfo() ;
      DPS_LSN_OFFSET startWatchOffset = watcher.getStartWatchOffset() ;
      DPS_LSN_OFFSET lastNotifiedOffset = _lastNotifiedOffset ;
      UINT64 resumableWindow = _resumableWindow ;
      utilWatchType watchLevel = UTIL_WATCH_COLLECTION ;
      BOOLEAN isAdded = FALSE ;

      // check if resumable
      if ( ( DPS_INVALID_LSN_OFFSET != watcher.getStartWatchOffset() ) &&
           ( DPS_INVALID_LSN_OFFSET != _lastNotifiedOffset ) &&
           ( _lastNotifiedOffset > watcher.getStartWatchOffset() ) )
      {
         PD_LOG_MSG_CHECK( ( lastNotifiedOffset - startWatchOffset <= resumableWindow ),
                           SDB_STREAM_NOT_RESUMABLE, error, PDERROR,
                           "Failed to register watch, the stream is not resumable, "
                           "current offset [%llu], start offset [%llu], resumable window [%llu]",
                           lastNotifiedOffset, startWatchOffset, resumableWindow ) ;
      }

      watchLevel = watchInfo.getWatchLevel() ;

      switch ( watchLevel )
      {
      case UTIL_WATCH_COLLECTION:
      {
         isAdded = _clDispatcher.registerWatcher( watcher ) ;
         break ;
      }
      case UTIL_WATCH_COLLECTION_SPACE:
      {
         isAdded = _csDispatcher.registerWatcher( watcher ) ;
         break ;
      }
      case UTIL_WATCH_ALL:
      {
         isAdded = _allDispatcher.registerWatcher( watcher ) ;
         break ;
      }
      default:
      {
         SDB_ASSERT( FALSE, "invalid watch level" ) ;
         PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to register watch "
                   "with unknown watch level" ) ;
         break ;
      }
      }

      if ( !isAdded )
      {
         // failed to register watcher
         PD_LOG( PDERROR, "Failed to register watcher to [%s] dispatcher",
                 utilGetWatchTypeName( watchLevel ) ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMNOTIFIER_REGWATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNCHANGESTREAMNOTIFIER_UNREGWATCHER, "_rtnChangeStreamNotifier::unregisterWatcher" )
   void _rtnChangeStreamNotifier::unregisterWatcher( rtnChangeStreamWatcher &watcher )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER_UNREGWATCHER ) ;

      const utilChangeStreamWatchInfo &watchInfo = watcher.getWatchInfo() ;
      utilWatchType watchLevel = UTIL_WATCH_COLLECTION ;
      BOOLEAN isRemoved = FALSE ;

      watchLevel = watchInfo.getWatchLevel() ;

      switch ( watchLevel )
      {
      case UTIL_WATCH_COLLECTION:
      {
         isRemoved = _clDispatcher.unregisterWatcher( watcher ) ;
         break ;
      }
      case UTIL_WATCH_COLLECTION_SPACE:
      {
         isRemoved = _csDispatcher.unregisterWatcher( watcher ) ;
         break ;
      }
      case UTIL_WATCH_ALL:
      {
         isRemoved = _allDispatcher.unregisterWatcher( watcher ) ;
         break ;
      }
      default:
      {
         SDB_ASSERT( FALSE, "invalid watch level" ) ;
         PD_LOG( PDWARNING, "Failed to unregister watch with unknown watch "
                 "level" ) ;
         break ;
      }
      }

      if ( !isRemoved )
      {
         PD_LOG( PDWARNING, "Failed to unregister watcher from %s dispatcher, "
                 "watcher is not found in dispatcher",
                 utilGetWatchTypeName( watchLevel ) ) ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER_UNREGWATCHER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER__NOTIFYLOG, "_rtnChangeStreamNotifier::_notifyLog" )
   void _rtnChangeStreamNotifier::_notifyLog( const utilLogExInfo &info,
                                              DPS_LSN_OFFSET offset,
                                              DPS_LSN_VER version,
                                              UINT32 length,
                                              DPS_LOG_TYPE logType )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER__NOTIFYLOG ) ;

      if ( _watcherNum.peek() > 0 )
      {
         try
         {
            utilChangeStreamLogInfo tmpInfo( info, offset, version, length, logType ) ;
            _consumeEvent.reset() ;
            while ( !_notifyQueue.push( std::move( tmpInfo ) ) )
            {
               // queue is full
               // signal produce event to wake up dispatcher
               _produceEvent.signalIfWaiting() ;
               // wait for consume event
               _consumeEvent.wait( s_waitConsumeTimeout ) ;
               _consumeEvent.reset() ;
            }
            // signal produce event to wake up dispatcher
            // if only one log in queue, which means the dispatcher is
            // not fast enough to consume logs
            if ( _notifyQueue.getSize() == 1 )
            {
               _produceEvent.signalIfWaiting() ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to push log, occurred exception %s",
                    e.what() ) ;
            INT32 stopRC = ossException2RC( &e ) ;
            DPS_LSN stopLSN ;
            stopLSN.set( offset, version ) ;
            // failed to push log, stop all watchers
            _stopAllWatchers( stopRC, stopLSN ) ;
         }
      }
      _lastNotifiedOffset = offset ;
      _expectOffset = offset + length ;

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER__NOTIFYLOG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER__STOPALLWATCHERS, "_rtnChangeStreamNotifier::_stopAllWatchers" )
   void _rtnChangeStreamNotifier::_stopAllWatchers( INT32 errorCode,
                                                    const DPS_LSN &stopLSN )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER__STOPALLWATCHERS ) ;

      _clDispatcher.stopAllWatchers( errorCode, stopLSN ) ;
      _csDispatcher.stopAllWatchers( errorCode, stopLSN ) ;
      _allDispatcher.stopAllWatchers( errorCode, stopLSN ) ;

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER__STOPALLWATCHERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMNOTIFIER__CALCRESUMEWIN, "_rtnChangeStreamNotifier::_calculateResumableWindow" )
   void _rtnChangeStreamNotifier::_calculateResumableWindow( INT32 resumableWindowMB,
                                                             UINT32 logBufferNum,
                                                             UINT32 logFileSize )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMNOTIFIER__CALCRESUMEWIN ) ;

      if ( 0 < resumableWindowMB )
      {
         // convert to bytes
         _resumableWindow = resumableWindowMB * 1024 * 1024 ;
      }
      else if ( 0 == resumableWindowMB )
      {
         // auto calculate resumable window
         // log cache size + log file size
         _resumableWindow = logBufferNum * DPS_DEFAULT_PAGE_SIZE + logFileSize ;
      }
      else
      {
         // no limit
         _resumableWindow = OSS_UINT64_MAX ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMNOTIFIER__CALCRESUMEWIN ) ;
   }

   /*
      _rtnChangeStreamNotifierJob define
    */
   class _rtnChangeStreamNotifierJob : public _rtnBaseJob
   {
   public:
      _rtnChangeStreamNotifierJob( rtnChangeStreamNotifier &notifier ) ;
      virtual ~_rtnChangeStreamNotifierJob() = default ;

   public:
      virtual RTN_JOB_TYPE type() const
      {
         return RTN_JOB_CHANGE_STREAM_NOTIFIER ;
      }

      virtual const CHAR* name() const
      {
         return "ChangeStreamNotifier" ;
      }

      virtual BOOLEAN muteXOn( const _rtnBaseJob *pOther )
      {
         return FALSE ;
      }

      virtual INT32 doit() ;

   protected:
      rtnChangeStreamNotifier &_notifier ;
   } ;

   /*
       _rtnChangeStreamNotifierJob implement
    */
   _rtnChangeStreamNotifierJob::_rtnChangeStreamNotifierJob( rtnChangeStreamNotifier &notifier )
   : _notifier( notifier )
   {
   }

   INT32 _rtnChangeStreamNotifierJob::doit()
   {
      pmdEDUCB *cb = eduCB() ;

      while ( !PMD_IS_DB_DOWN() && !cb->isForced() )
      {
         _notifier.tryDispatch() ;
      }

      return SDB_OK ;
   }

   INT32 _rtnStartChangeStreamNotifierJob( rtnChangeStreamNotifier &notifier )
   {
      INT32 rc = SDB_OK ;

      _rtnChangeStreamNotifierJob *pJob = NULL ;
      EDUID eduID = PMD_INVALID_EDUID ;

      pJob = SDB_OSS_NEW _rtnChangeStreamNotifierJob( notifier ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, &eduID ) ;
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Start change stream notifier job at EDU [%llu]", eduID ) ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
