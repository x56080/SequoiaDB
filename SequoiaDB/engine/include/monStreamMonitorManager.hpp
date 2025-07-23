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

   Source File Name = monStreamMonitorManager.hpp

   Descriptive Name = Stream Monitor Manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MON_STREAM_MONITOR_MANAGER_HPP__
#define MON_STREAM_MONITOR_MANAGER_HPP__

#include "oss.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "ossTypes.h"
#include "utilChangeStreamOptions.hpp"
#include "utilChangeStreamWatchInfo.hpp"
#include "utilStreamToken.hpp"

namespace engine
{

   /*
      _monStreamInfo define
    */
   // stream information monitor
   class _monStreamInfo
   {
   public:
      _monStreamInfo() = default ;
      _monStreamInfo( const _monStreamInfo &other ) = default ;
      ~_monStreamInfo() = default ;

      _monStreamInfo &operator =( const _monStreamInfo &other ) = default ;

      _monStreamInfo( utilStreamType type,
                      UINT64 sessionID,
                      INT64 contextID ) ;

   public:
      // type of stream
      utilStreamType _type = UTIL_UNKNOWN_STREAM ;
      // session who executes stream
      UINT64 _sessionID = 0 ;
      // context who executes stream
      INT64  _contextID = -1 ;
      // options of stream
      bson::BSONObj _options ;
   } ;

   typedef class _monStreamInfo monStreamInfo ;

   /*
      _monStreamStats define
    */
   // stream statistics monitor
   class _monStreamStats
   {
   public:
      _monStreamStats() = default ;
      _monStreamStats( const _monStreamStats &other ) = default ;
      ~_monStreamStats() = default ;

      _monStreamStats &operator =( const _monStreamStats &other ) = default ;

   public:
      void onControlRecord()
      {
         ++ _controlNum ;
         _lastProcessedType = UTIL_STREAM_CONTROL_RECORD ;
      }

      void onChangeRecord()
      {
         ++ _changeNum ;
         _lastProcessedType = UTIL_STREAM_CHANGE_RECORD ;
      }

      void onDataRecord()
      {
         ++ _dataNum ;
         _lastProcessedType = UTIL_STREAM_DATA_RECORD ;
      }

      void onBatch( UINT64 returnNum, UINT64 returnSize )
      {
         ++ _batchNum ;
         _returnNum += returnNum ;
         _returnSize += returnSize ;
      }

   public:
      // timestamp when the stream started
      UINT64 _startTimestamp = 0 ;
      // number of control records returned by stream
      UINT64 _controlNum = 0 ;
      // number of change records returned by stream
      UINT64 _changeNum = 0 ;
      // number of data records returned by stream
      UINT64 _dataNum = 0 ;
      // number of batch of records returned by stream
      UINT64 _batchNum = 0 ;
      // number of records returned by stream
      UINT64 _returnNum = 0 ;
      // size in bytes of records returned by stream
      UINT64 _returnSize = 0 ;
      // last processed record type
      utilStreamRecordType _lastProcessedType = UTIL_STREAM_INVALID_RECORD ;
   } ;

   typedef class _monStreamStats monStreamStats ;

   /*
      _monStreamSourceUsage define
    */
   class _monStreamSourceUsage
   {
   public:
      _monStreamSourceUsage() = default ;
      _monStreamSourceUsage( const _monStreamSourceUsage &other ) = default ;
      ~_monStreamSourceUsage() = default ;

      _monStreamSourceUsage &operator =( const _monStreamSourceUsage &other ) = default ;

   public:
      // queue capacity
      UINT64 _queueCapacity = 0 ;
      // current number of logs in queue
      UINT64 _inQueueNum = 0 ;
      // current size of logs in queue
      UINT64 _inQueueSize = 0 ;
      // cache capacity
      UINT64 _cacheCapacity = 0 ;
      // current number of logs in cache
      UINT64 _inCacheNum = 0 ;
      // current size of logs in cache
      UINT64 _inCacheSize = 0 ;
   } ;

   typedef class _monStreamSourceUsage monStreamSourceUsage ;

   /*
      _monStreamSourceStats define
    */
   // stream source statistics
   class _monStreamSourceStats
   {
   public:
      _monStreamSourceStats() = default ;
      _monStreamSourceStats( const _monStreamSourceStats &other ) = default ;
      virtual ~_monStreamSourceStats() = default ;

      _monStreamSourceStats &operator =( const _monStreamSourceStats &other ) = default ;

   public:
      // counts of logs received from notifier/dispatcher
      UINT64 _receivedNum = 0 ;
      // time to wait logs from notifier/dispatcher
      UINT32 _waitTime = 0 ;
      // counts of hit log cache
      UINT64 _hitCacheNum = 0 ;
      // counts of missing log cache
      UINT64 _missCacheNum = 0 ;
      // number of logs scanned file
      UINT64 _scannedNum = 0 ;
      // size of logs searched from file
      UINT64 _scannedSize = 0 ;
   } ;

   typedef class _monStreamSourceStats monStreamSourceStats ;

   /*
      _monStreamItem define
    */
   // stream monitor with brief information
   class _monStreamItem : public _utilPooledObject, public _monStreamInfo
   {
   public:
      _monStreamItem() = default ;
      _monStreamItem( const _monStreamItem &item ) = default ;
      ~_monStreamItem() = default ;

      _monStreamItem &operator =( const _monStreamItem &item ) = default ;

      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;

   public:
      // current token returned by stream
      utilStreamToken _currentToken ;
   } ;

   typedef class _monStreamItem monStreamItem ;
   typedef ossPoolList< monStreamItem > monStreamItemList ;

   /*
      _monStreamDetailedItem define
    */
   // stream monitor with detailed information and statistics
   class _monStreamDetailedItem : public _monStreamItem, public _monStreamStats
   {
   public:
      _monStreamDetailedItem() = default ;
      _monStreamDetailedItem( const _monStreamDetailedItem &item ) = default ;
      ~_monStreamDetailedItem() = default ;

      _monStreamDetailedItem &operator =( const _monStreamDetailedItem &item ) = default ;

      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;

   public:
      // source statistics
      monStreamSourceStats _sourceStats ;
      // current source usage
      monStreamSourceUsage _sourceUsage ;
   } ;

   typedef class _monStreamDetailedItem monStreamDetailedItem ;
   typedef ossPoolList< monStreamDetailedItem > monStreamDetailedItemList ;

   /*
      _monStreamSourceMonitor define
    */
   // stream source monitor
   class _monStreamSourceMonitor : public _monStreamSourceStats
   {
   public:
      _monStreamSourceMonitor() = default ;
      virtual ~_monStreamSourceMonitor() = default ;

      // get current token
      virtual void getCurrentToken( utilStreamToken &token ) const = 0 ;

      // get current usage
      virtual monStreamSourceUsage getUsage() const = 0 ;

      void onReceived()
      {
         ++ _receivedNum ;
      }

      void onWait( UINT64 waitTime )
      {
         _waitTime += waitTime ;
      }

      void onHitCache()
      {
         ++ _hitCacheNum ;
      }

      void onMissCache()
      {
         ++ _missCacheNum ;
      }

      void onSearched( UINT64 size )
      {
         ++ _scannedNum ;
         _scannedSize += size ;
      }

      UINT64 getReceivedNum() const
      {
         return _receivedNum ;
      }

      UINT64 getWaitTime() const
      {
         return _waitTime ;
      }

      UINT64 getHitCacheNum() const
      {
         return _hitCacheNum ;
      }

      UINT64 getMissCacheNum() const
      {
         return _missCacheNum ;
      }

      UINT64 getScannedNum() const
      {
         return _scannedNum ;
      }
   } ;

   typedef class _monStreamSourceMonitor monStreamSourceMonitor ;

   /*
      _monStreamMonitor define
    */
   // monitor of stream
   class _monStreamMonitor : public _monStreamInfo, public _monStreamStats
   {
   public:
      _monStreamMonitor( utilStreamType type,
                         UINT64 sessionID,
                         INT64 contextID,
                         monStreamSourceMonitor &sourceMonitor ) ;
      ~_monStreamMonitor() = default ;

      // initialize the monitor with options in BSON format
      INT32 init( const bson::BSONObj &options ) ;

      // dump brief monitor item
      INT32 dumpItem( monStreamItem &item ) const ;
      // dump detailed monitor item
      INT32 dumpDetailedItem( monStreamDetailedItem &item ) const ;

   protected:
      // monitor of stream source
      monStreamSourceMonitor &_sourceMonitor ;
   } ;

   typedef class _monStreamMonitor monStreamMonitor ;

   /*
      _monStreamMonitorManager define
    */
   // manager of stream monitors
   class _monStreamMonitorManager : public SDBObject
   {
   public:
      _monStreamMonitorManager() = default ;
      ~_monStreamMonitorManager() = default ;

      // register monitor to manager
      INT32 registerMonitor( const monStreamMonitor &monitor ) ;
      // unregister monitor from manager
      void unregisterMonitor( const monStreamMonitor &monitor ) ;

      // dump brief monitor items
      INT32 dumpItems( monStreamItemList &items ) ;
      // dump detailed monitor items
      INT32 dumpDetailedItems( monStreamDetailedItemList &items ) ;

   protected:
      // mutex to protect the monitors
      ossRWMutex _mutex ;

      // running stream monitors
      typedef ossPoolSet< const monStreamMonitor * > _monStreamMonitorSet ;
      typedef _monStreamMonitorSet::iterator _monStreamMonitorSetIter ;
      typedef _monStreamMonitorSet::const_iterator _monStreamMonitorSetCIter ;
      _monStreamMonitorSet _streamMonitors ;
   } ;

   typedef class _monStreamMonitorManager monStreamMonitorManager ;

}

#endif // MON_STREAM_MONITOR_MANAGER_HPP__
