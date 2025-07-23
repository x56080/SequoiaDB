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

   Source File Name = rtnChangeStreamCache.hpp

   Descriptive Name = Log Fetcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CHANGE_STREAM_CACHE_HPP__
#define RTN_CHANGE_STREAM_CACHE_HPP__

#include "dpsDef.hpp"
#include "dpsLogDef.hpp"
#include "dpsLogRecord.hpp"
#include "monStreamMonitorManager.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "pmdDef.hpp"
#include "utilSPSCBlockQueue.hpp"
#include "utilChangeStreamWatchInfo.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      _rtnChangeStreamLogInfo define
    */
   // information for change stream log record
   class _rtnChangeStreamLogInfo : public _utilChangeStreamLogInfo
   {
   protected:
      // flags to control behavior of filter
      static const UINT32 _FLAG_CHECK_NOTHING = 0x00000000 ;
      // need check record for filter
      static const UINT32 _FLAG_CHECK_RECORD = 0x00000001 ;
      // need check error ( drop collection, etc. )
      static const UINT32 _FLAG_CHECK_ERROR = 0x00000002 ;
      // need check log type
      static const UINT32 _FLAG_CHECK_TYPE = 0x00000004 ;

   public:
      _rtnChangeStreamLogInfo() = default ;
      _rtnChangeStreamLogInfo( const _rtnChangeStreamLogInfo &logInfo ) ;
      ~_rtnChangeStreamLogInfo() = default ;

      _rtnChangeStreamLogInfo &operator=( const _rtnChangeStreamLogInfo &logInfo ) = default ;

      _rtnChangeStreamLogInfo( const utilChangeStreamLogInfo &logInfo,
                               BOOLEAN needFilterRecord,
                               BOOLEAN needCheckError,
                               BOOLEAN needFilterType ) ;
      _rtnChangeStreamLogInfo( const dpsLogRecordHeader *recordHeader,
                               BOOLEAN needFilterRecord = TRUE,
                               BOOLEAN needCheckError = TRUE,
                               BOOLEAN needFilterType = TRUE ) ;

      void setCheckRecord()
      {
         OSS_BIT_SET( _flags, _FLAG_CHECK_RECORD ) ;
      }

      void setCheckError()
      {
         OSS_BIT_SET( _flags, _FLAG_CHECK_ERROR ) ;
      }

      void setCheckType()
      {
         OSS_BIT_SET( _flags, _FLAG_CHECK_TYPE ) ;
      }

      BOOLEAN needCheckRecord() const
      {
         return OSS_BIT_TEST( _flags, _FLAG_CHECK_RECORD ) ? TRUE : FALSE ;
      }

      BOOLEAN needCheckError() const
      {
         return OSS_BIT_TEST( _flags, _FLAG_CHECK_ERROR ) ? TRUE : FALSE ;
      }

      BOOLEAN needCheckType() const
      {
         return OSS_BIT_TEST( _flags, _FLAG_CHECK_TYPE ) ? TRUE : FALSE ;
      }

      BOOLEAN isRecordCached() const
      {
         return _cache.isFilled() ;
      }

      const dpsLogRecordHeader *getCachedRecord() const
      {
         return (const dpsLogRecordHeader *)( _cache.getBuffer() ) ;
      }

      BOOLEAN isRecordFetched() const
      {
         return NULL != _fetchedRecord ? TRUE : FALSE ;
      }

      const dpsLogRecordHeader *getFetchedRecord() const
      {
         return _fetchedRecord ;
      }

      void resetInfo()
      {
         _resetInfo() ;
         _flags = _FLAG_CHECK_NOTHING ;
         _fetchedRecord = NULL ;
      }

      void releaseCache()
      {
         _cache.release() ;
         _fetchedRecord = NULL ;
      }

   protected:
      // check flags
      UINT32 _flags = _FLAG_CHECK_NOTHING ;
      // pointer to record
      const dpsLogRecordHeader *_fetchedRecord = NULL ;
   } ;

   typedef class _rtnChangeStreamLogInfo rtnChangeStreamLogInfo ;

   /*
      _rtnChangeStreamCache define
    */
   class _rtnChangeStreamCache : public _utilPooledObject
   {
   public:
      _rtnChangeStreamCache() = default ;
      ~_rtnChangeStreamCache() = default ;

      INT32 init( UINT32 cacheSize, UINT64 resumableWindow ) ;

      // push log record into cache
      INT32 pushLog( const utilChangeStreamLogInfo &logInfo,
                     BOOLEAN needFilterRecord,
                     BOOLEAN needCheckError,
                     BOOLEAN needFilterType ) ;
      // get log record from cache
      INT32 popLog( rtnChangeStreamLogInfo &logInfo ) ;

      BOOLEAN isEmpty() const
      {
         return _logQueue.isEmpty() ;
      }

      monStreamSourceUsage getUsage() const
      {
         INT32 readOffset = 0 ;
         INT32 writeOffset = 0 ;
         monStreamSourceUsage usage ;
         usage._queueCapacity = _queueCapacity ;
         usage._cacheCapacity = _cacheCapacity ;

         // calculate number of records in queue
         readOffset = _queueReadIndex ;
         writeOffset = _queueWriteIndex ;
         usage._inQueueNum = ( writeOffset > readOffset ) ?
                             ( writeOffset - readOffset ) : ( 0 ) ;

         // calculate size of records in queue
         readOffset = _queueReadOffset ;
         writeOffset = _queueWriteOffset ;
         usage._inQueueSize = ( usage._inQueueNum > 0 &&
                                writeOffset > readOffset ) ?
                              ( writeOffset - readOffset ) : ( 0 ) ;

         // calculate number of records in cache
         readOffset = _cacheReadIndex ;
         writeOffset = _cacheWriteIndex ;
         usage._inCacheNum = ( writeOffset > readOffset ) ?
                             ( writeOffset - readOffset ) : ( 0 ) ;

         // calculate size of records in cache
         readOffset = _cacheReadOffset ;
         writeOffset = _cacheWriteOffset ;
         usage._inCacheSize = ( usage._inCacheNum > 0 &&
                                writeOffset > readOffset ) ?
                              ( writeOffset - readOffset ) : ( 0 ) ;
         return usage ;
      }

   protected:
      UINT64 _getQueueSize() const
      {
         return _queueWriteOffset - _queueReadOffset ;
      }

      UINT64 _getQueueNum() const
      {
         return _queueWriteIndex - _queueReadIndex ;
      }

      UINT64 _getCacheSize() const
      {
         return _cacheWriteOffset - _cacheWriteOffset ;
      }

      UINT64 _getCacheNum() const
      {
         return _cacheWriteIndex - _cacheReadIndex ;
      }

      BOOLEAN _isQueueEnough( UINT32 logLength ) const
      {
         return _getQueueSize() + logLength <= _queueCapacity;
      }

      BOOLEAN _isCacheEnough( UINT32 logLength ) const
      {
         return _getCacheSize() + logLength <= _cacheCapacity ;
      }

   protected:
      // capacity of queue
      UINT64 _queueCapacity = 0 ;
      // read offset of queue
      UINT64 _queueReadOffset = 0 ;
      // write offset of queue
      UINT64 _queueWriteOffset = 0 ;
      // read index of queue
      UINT64 _queueReadIndex = 0 ;
      // write index of queue
      UINT64 _queueWriteIndex = 0 ;

      // capacity of cache
      UINT64 _cacheCapacity = 0 ;
      // read offset of cache
      UINT64 _cacheReadOffset = 0 ;
      // write offset of cache
      UINT64 _cacheWriteOffset = 0 ;
      // read index of cache
      UINT64 _cacheReadIndex = 0 ;
      // write index of cache
      UINT64 _cacheWriteIndex = 0 ;

      // log queue
      typedef _utilSPSCBlockQueue< rtnChangeStreamLogInfo, 128 > _rtnLogQueue ;
      _rtnLogQueue _logQueue ;
   } ;

   typedef class _rtnChangeStreamCache rtnChangeStreamCache ;

}

#endif // RTN_CHANGE_STREAM_CACHE_HPP__
