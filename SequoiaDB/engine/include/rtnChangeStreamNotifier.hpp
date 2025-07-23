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

   Source File Name = rtnChangeStreamNotifier.hpp

   Descriptive Name = Change Stream Notifier Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CHANGE_STREAM_NOTIFIER_HPP__
#define RTN_CHANGE_STREAM_NOTIFIER_HPP__

#include "dpsDef.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "ossRWMutex.hpp"
#include "ossTypes.h"
#include "rtnBackgroundJobBase.hpp"
#include "rtnChangeStreamDispatcher.hpp"
#include "clsDef.hpp"
#include "ossQueue.hpp"
#include "rtnLogFetcher.hpp"
#include "utilCircularQueue.hpp"

namespace engine
{

   /*
      _rtnChangeStreamNotifier define
    */
   // change stream notifier is responsible for notifying change stream watchers
   class _rtnChangeStreamNotifier : public _rtnChangeStreamNotifierBase,
                                    public _dpsEventHandler,
                                    public _clsReplayEventHandler
   {
   public:
      _rtnChangeStreamNotifier() ;
      virtual ~_rtnChangeStreamNotifier() = default ;

      // initialize notifier
      INT32 init( INT32 resumableWindowMB,
                  UINT32 logBufferNum,
                  UINT32 logFileSize ) ;
      // active notifier
      INT32 active() ;
      // deactive notifier
      void  deactive() ;
      // finish notifier
      void  fini() ;
      // on config change event
      void onConfigChange( INT32 resumableWindowMB,
                           UINT32 logBufferNum,
                           UINT32 logFileSize ) ;

      // override functions of _dpsEventHandler
      virtual INT32 canAssignLogPage( UINT32 reqLen,
                                      pmdEDUCB *cb,
                                      BOOLEAN &needCache ) ;

      virtual INT32 canAssignLogPageOnSecondary( UINT32 reqLen, pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual void onPrepareLog( const utilLogExInfo &info,
                                 DPS_LSN_OFFSET offset,
                                 DPS_LSN_VER version,
                                 UINT32 length,
                                 DPS_LOG_TYPE logType ) ;

      virtual void onWriteLog( DPS_LSN_OFFSET offset ) ;

      virtual INT32 onCompleteOpr( pmdEDUCB *cb, INT32 w )
      {
         return SDB_OK ;
      }

      virtual void onSwitchLogFile( UINT32 preLogicalFileId,
                                    UINT32 preFileId,
                                    UINT32 curLogicalFileId,
                                    UINT32 curFileId )
      {
      }

      virtual void onMoveLog( DPS_LSN_OFFSET moveToOffset,
                              DPS_LSN_VER moveToVersion,
                              DPS_LSN_OFFSET expectOffset,
                              DPS_LSN_VER expectVersion,
                              DPS_MOMENT moment,
                              INT32 errcode ) ;

      // override functions of _clsReplayEventHandler
      virtual void onPrepareReplayLog( DPS_LSN_OFFSET offset )
      {
      }

      virtual void onReplayLog( const utilLogExInfo &info,
                                DPS_LSN_OFFSET offset,
                                DPS_LSN_VER version,
                                UINT32 length,
                                DPS_LOG_TYPE logType ) ;

      virtual BOOLEAN needCacheLog( const utilLogExInfo &info,
                                    DPS_LSN_OFFSET offset,
                                    DPS_LSN_VER version,
                                    UINT32 length,
                                    DPS_LOG_TYPE logType )
      {
         return LOG_TYPE_DUMMY != logType && _watcherNum.fetch() > 0 ;
      }

      // try dispatch log records
      void tryDispatch() ;

      // register watcher
      INT32 registerWatcher( rtnChangeStreamWatcher &watcher ) ;
      // unregister watcher
      void unregisterWatcher( rtnChangeStreamWatcher &watcher ) ;

      // override functions of _rtnChangeStreamNotifierBase
      virtual DPS_LSN_OFFSET getLastDispatchedOffset() const
      {
         return _lastDispatchedOffset ;
      }

      virtual DPS_LSN_OFFSET getExpectOffset() const
      {
         return _expectOffset ;
      }

      virtual BOOLEAN waitForWrite()
      {
         return ( SDB_OK == _writeEvent.wait( s_waitWriteTimeout ) ) ;
      }

      virtual void onRegisterWatcher()
      {
         _watcherNum.inc() ;
      }

      virtual void onUnregisterWatcher()
      {
         _watcherNum.dec() ;
      }

      UINT64 getResumableWindow() const
      {
         return _resumableWindow ;
      }

   protected:
      // save log in notify queue
      void _notifyLog( const utilLogExInfo &info,
                       DPS_LSN_OFFSET offset,
                       DPS_LSN_VER version,
                       UINT32 length,
                       DPS_LOG_TYPE logType ) ;

      // stop all watchers by error
      void _stopAllWatchers( INT32 errorCode, const DPS_LSN &stopLSN ) ;

      // calculate resumable window
      void _calculateResumableWindow( INT32 resumableWindowMB,
                                      UINT32 logBufferNum,
                                      UINT32 logFileSize ) ;

   protected:
      // timeout to wait for log record write event
      static const INT64 s_waitWriteTimeout = 100 ;
      // timeout to wait for produce event
      static const INT64 s_waitProduceTimeout = OSS_ONE_SEC ;
      // timeout to wait for consume event
      static const INT64 s_waitConsumeTimeout = 100 ;
      // size for notify queue
      static const UINT64 s_queueSize = 4096 ;
      // size of batch of dispatch log records
      static const UINT64 s_batchSize = 64 ;

      // size of resumable window
      UINT64         _resumableWindow = OSS_UINT64_MAX ;
      // last notified offset
      DPS_LSN_OFFSET _lastNotifiedOffset = DPS_INVALID_LSN_OFFSET ;
      // last dispatched offset
      DPS_LSN_OFFSET _lastDispatchedOffset = DPS_INVALID_LSN_OFFSET ;
      // offset expected to be notified next
      DPS_LSN_OFFSET _expectOffset = DPS_INVALID_LSN_OFFSET ;

      // notify queue
      typedef _utilStackSPSCQueue< utilChangeStreamLogInfo, s_queueSize > _rtnLogInfoQueue ;
      _rtnLogInfoQueue _notifyQueue ;

      // event for consuming log record
      ossSPSCEvent _consumeEvent ;
      // event for producing log record
      ossSPSCEvent _produceEvent ;
      // event for log record write done
      ossAutoEvent _writeEvent ;

      // number of watchers
      ossAtomic32 _watcherNum ;
      // dispatcher for watchers watching collections
      rtnChangeStreamDispatcher _clDispatcher ;
      // dispatcher for watchers watching collection spaces
      rtnChangeStreamDispatcher _csDispatcher ;
      // dispatcher for watchers watching all collection spaces and collections
      rtnChangeStreamDispatcher _allDispatcher ;
   } ;

   typedef class _rtnChangeStreamNotifier rtnChangeStreamNotifier ;

}

#endif // RTN_CHANGE_STREAM_NOTIFIER_HPP__
