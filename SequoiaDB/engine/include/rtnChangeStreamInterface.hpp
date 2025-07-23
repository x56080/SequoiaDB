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
