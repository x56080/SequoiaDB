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

   Source File Name = rtnChangeStreamInterface.hpp

   Descriptive Name = Change Stream Interface

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_CHANGE_STREAM_INTERFACE_HPP__
#define RTN_CHANGE_STREAM_INTERFACE_HPP__

#include "dpsLogDef.hpp"
#include "utilChangeStreamOptions.hpp"
#include "utilChangeStreamWatchInfo.hpp"

namespace engine
{

   /*
      _rtnChangeStreamNotifierBase define
    */
   // base class of change stream notifier
   class _rtnChangeStreamNotifierBase
   {
   public:
      _rtnChangeStreamNotifierBase() = default ;
      virtual ~_rtnChangeStreamNotifierBase() = default ;

      // get last dispatced offset
      virtual DPS_LSN_OFFSET getLastDispatchedOffset() const = 0 ;
      // get expect offset
      virtual DPS_LSN_OFFSET getExpectOffset() const = 0 ;
      // wait for write event
      virtual BOOLEAN waitForWrite() = 0 ;
      // on register watcher event
      virtual void onRegisterWatcher() = 0 ;
      // on unregister watcher event
      virtual void onUnregisterWatcher() = 0 ;
   } ;

   typedef class _rtnChangeStreamNotifierBase rtnChangeStreamNotifierBase ;

   /*
      _rtnChangeStreamWatcherBase define
    */
   // base class of change stream watcher
   class _rtnChangeStreamWatcherBase
   {
   public:
      _rtnChangeStreamWatcherBase() = default ;
      virtual ~_rtnChangeStreamWatcherBase() = default ;

      // check if watcher is watching
      virtual BOOLEAN isWatching() const = 0 ;

      virtual const utilChangeStreamWatchInfo &getWatchInfo() const = 0 ;

      // attach and start watch
      virtual void attachAndStartWatch( rtnChangeStreamNotifierBase *notifier,
                                        DPS_LSN_OFFSET startOffset ) = 0 ;

      // detach watch
      virtual void detach() = 0 ;

      // check if watcher is attached
      virtual BOOLEAN isAttached() const = 0 ;

      // stop watching with error code
      virtual void stopWatch( INT32 errorCode, const DPS_LSN &stopLSN ) = 0 ;

      // push log to watcher
      virtual INT32 pushLog( const utilChangeStreamLogInfo &logInfo ) = 0 ;

      // get start watch offset
      virtual DPS_LSN_OFFSET getStartWatchOffset() const = 0 ;
   } ;

   typedef class _rtnChangeStreamWatcherBase rtnChangeStreamWatcherBase ;
   typedef class _rtnChangeStreamWatcherBase rtnChangeStreamWatcher ;

}

#endif // RTN_CHANGE_STREAM_INTERFACE_HPP__
