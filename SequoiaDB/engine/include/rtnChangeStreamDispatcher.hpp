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

   Source File Name = rtnChangeStreamDispatcher.hpp

   Descriptive Name = Change Stream Dispatcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CHANGE_STREAM_DISPATCHER_HPP__
#define RTN_CHANGE_STREAM_DISPATCHER_HPP__

#include "dpsLogDef.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "ossTypes.h"
#include "rtnBackgroundJobBase.hpp"
#include "clsDef.hpp"
#include "ossQueue.hpp"
#include "dpsMessageBlock.hpp"
#include "rtnChangeStreamInterface.hpp"
#include "rtnLogInfoFilter.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      rtnChangeStreamDispatcher define
    */
   // each dispatcher watches on a group of objects, and dispacth logs to log
   // fetchers watching on those objects
   class _rtnChangeStreamDispatcher : public _utilPooledObject
   {
   public:
      _rtnChangeStreamDispatcher( rtnChangeStreamNotifierBase &notifer,
                                  utilWatchType watchLevel ) ;
      ~_rtnChangeStreamDispatcher() = default ;

      // dispatch log record
      INT32 dispatchLog( const utilChangeStreamLogInfo &logInfo ) ;

      // register watcher
      BOOLEAN registerWatcher( rtnChangeStreamWatcher &watcher ) ;
      // unregister watcher
      BOOLEAN unregisterWatcher( rtnChangeStreamWatcher &watcher ) ;

      // stop all watchers by error
      void stopAllWatchers( INT32 errorCode,
                            const DPS_LSN &stopLSN ) ;

      // get watch level
      utilWatchType getWatchLevel() const
      {
         return _watchLevel ;
      }

      // get watch level name
      const CHAR *getWatchLevelName() const
      {
         return utilGetWatchTypeName( _watchLevel ) ;
      }

   protected:
      // dispatch log record
      INT32 _dispatchLog( const utilChangeStreamLogInfo &logInfo ) ;

   protected:
      // change stream notifier
      rtnChangeStreamNotifierBase &_notifier ;

      // watch level
      utilWatchType _watchLevel ;

      // log information filter
      rtnLogInfoFilter _infoFilter ;

      // mutex to protected watchers
      mutable ossRWMutex _watcherMutex ;

      // set of watchers
      typedef ossPoolSet< rtnChangeStreamWatcher * > _rtnWatcherSet ;
      typedef _rtnWatcherSet::iterator _rtnWatcherSetIter ;
      _rtnWatcherSet _watchers ;
   } ;

   typedef class _rtnChangeStreamDispatcher rtnChangeStreamDispatcher ;

}

#endif // RTN_CHANGE_STREAM_DISPATCHER_HPP__
