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
