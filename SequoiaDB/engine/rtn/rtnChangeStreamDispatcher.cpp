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

   Source File Name = rtnChangeStreamDispatcher.cpp

   Descriptive Name = Change Stream Dispatcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnChangeStreamDispatcher.hpp"
#include "dpsDef.hpp"
#include "dpsLogWrapper.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "utilChangeStreamWatchInfo.hpp"

namespace engine
{

   /*
      rtnChangeStreamDispatcher implement
    */
   _rtnChangeStreamDispatcher::_rtnChangeStreamDispatcher( rtnChangeStreamNotifierBase &notifier,
                                                           utilWatchType watchLevel )
   : _notifier( notifier ),
     _watchLevel( watchLevel )
   {
      SDB_ASSERT( UTIL_WATCH_COLLECTION == _watchLevel ||
                  UTIL_WATCH_COLLECTION_SPACE == _watchLevel ||
                  UTIL_WATCH_ALL == _watchLevel,
                  "invalid watch level" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMDISPATCHER_DISPATCHLOG, "_rtnChangeStreamDispatcher::dispatchLog" )
   INT32 _rtnChangeStreamDispatcher::dispatchLog( const utilChangeStreamLogInfo &logInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMDISPATCHER_DISPATCHLOG ) ;

      BOOLEAN isMatched = FALSE ;

      ossScopedRWLock lock( &_watcherMutex, SHARED ) ;

      rc = _infoFilter.filter( _watchLevel, logInfo, isMatched ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to filter info, rc: %d", rc ) ;

      if ( isMatched )
      {
         _dispatchLog( logInfo ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMDISPATCHER_DISPATCHLOG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMDISPATCHER_REGWATCHER, "_rtnChangeStreamDispatcher::registerWatcher" )
   BOOLEAN _rtnChangeStreamDispatcher::registerWatcher(
                                             rtnChangeStreamWatcher &watcher )
   {
      BOOLEAN isAdded = FALSE ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMDISPATCHER_REGWATCHER ) ;

      try
      {
         DPS_LSN_OFFSET startExpectOffset = DPS_INVALID_LSN_OFFSET ;
         while ( TRUE )
         {
            ossScopedRWLock lock( &_watcherMutex, EXCLUSIVE ) ;
            if ( _watchers.insert( &watcher ).second )
            {
               _infoFilter.updateFilter( _watchLevel, watcher.getWatchInfo() ) ;
               _notifier.onRegisterWatcher() ;
               startExpectOffset = _notifier.getExpectOffset() ;
               break ;
            }
         }
         watcher.attachAndStartWatch( &_notifier, startExpectOffset ) ;
         isAdded = TRUE ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register watcher to [%s] dispatcher, "
                 "occurred exception %s", getWatchLevelName(), e.what() ) ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMDISPATCHER_REGWATCHER ) ;

      return isAdded ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMDISPATCHER_UNREGWATCHER, "_rtnChangeStreamDispatcher::unregisterWatcher" )
   BOOLEAN _rtnChangeStreamDispatcher::unregisterWatcher( rtnChangeStreamWatcher &watcher )
   {
      BOOLEAN isRemoved = FALSE ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMDISPATCHER_UNREGWATCHER ) ;

      if ( watcher.isAttached() )
      {
         ossScopedRWLock lock( &_watcherMutex, EXCLUSIVE ) ;
         _rtnWatcherSetIter iter = _watchers.find( &watcher ) ;
         if ( iter != _watchers.end() )
         {
            _watchers.erase( iter ) ;
            isRemoved = TRUE ;
         }

         if ( isRemoved )
         {
            _notifier.onUnregisterWatcher() ;
            _infoFilter.clearFilter() ;
            for ( _rtnWatcherSetIter iter = _watchers.begin() ;
                  iter != _watchers.end() ;
                  ++ iter )
            {
               _infoFilter.updateFilter( _watchLevel, (*iter)->getWatchInfo() ) ;
            }
         }
      }

      if ( isRemoved )
      {
         watcher.detach() ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMDISPATCHER_UNREGWATCHER ) ;

      return isRemoved ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMDISPATCHER_STOPALLWATCHERS, "_rtnChangeStreamDispatcher::stopAllWatchers" )
   void _rtnChangeStreamDispatcher::stopAllWatchers( INT32 errorCode,
                                                     const DPS_LSN &stopLSN )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMDISPATCHER_STOPALLWATCHERS ) ;

      ossScopedRWLock lock( &_watcherMutex, SHARED ) ;
      for ( _rtnWatcherSetIter iter = _watchers.begin() ;
            iter != _watchers.end() ;
            ++ iter )
      {
         rtnChangeStreamWatcher *watcher = *iter ;

         if ( NULL != watcher && watcher->isWatching() )
         {
            watcher->stopWatch( errorCode, stopLSN ) ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMDISPATCHER_STOPALLWATCHERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMDISPATCHER__DISPATCHLOG, "_rtnChangeStreamDispatcher::_dispatchLog" )
   INT32 _rtnChangeStreamDispatcher::_dispatchLog( const utilChangeStreamLogInfo &logInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMDISPATCHER__DISPATCHLOG ) ;

      for ( _rtnWatcherSetIter iter = _watchers.begin() ;
            iter != _watchers.end() ;
            ++ iter )
      {
         rtnChangeStreamWatcher *watcher = *iter ;

         if ( NULL != watcher && watcher->isWatching() )
         {
            // push log to watcher
            // NOTE: we should not report error to caller
            INT32 tmpRC = watcher->pushLog( logInfo ) ;
            if ( SDB_OK != tmpRC )
            {
               const DPS_LSN &lsn = logInfo.getLSN() ;
               PD_LOG( PDERROR, "Failed to push log [version: %u, offset: %llu], "
                       "rc: %d", lsn.version, lsn.offset, tmpRC ) ;
               watcher->stopWatch( tmpRC, lsn ) ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMDISPATCHER__DISPATCHLOG, rc ) ;

      return rc ;
   }

}
