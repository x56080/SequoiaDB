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

   Source File Name = rtnChangeStreamCache.cpp

   Descriptive Name = Change Stream Cache

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnChangeStreamCache.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _rtnChangeStreamLogInfo implement
    */
   _rtnChangeStreamLogInfo::_rtnChangeStreamLogInfo( const utilChangeStreamLogInfo &logInfo,
                                                     BOOLEAN needFilterRecord,
                                                     BOOLEAN needCheckError,
                                                     BOOLEAN needFilterType )
   : _utilChangeStreamLogInfo( logInfo )
   {
      if ( needFilterRecord )
      {
         setCheckRecord() ;
      }
      if ( needCheckError )
      {
         setCheckError() ;
      }
      if ( needFilterType )
      {
         setCheckType() ;
      }
   }

   _rtnChangeStreamLogInfo::_rtnChangeStreamLogInfo( const dpsLogRecordHeader *recordHeader,
                                                     BOOLEAN needFilterRecord,
                                                     BOOLEAN needCheckError,
                                                     BOOLEAN needFilterType )
   : _utilChangeStreamLogInfo()
   {
      if ( needFilterRecord )
      {
         setCheckRecord() ;
      }
      if ( needCheckError )
      {
         setCheckError() ;
      }
      if ( needFilterType )
      {
         setCheckType() ;
      }
      if ( NULL != recordHeader )
      {
         _fetchedRecord = recordHeader ;
         _lsn.set( _fetchedRecord->_lsn, _fetchedRecord->_version ) ;
         _length = _fetchedRecord->_length ;
         _logType = (DPS_LOG_TYPE)( _fetchedRecord->_type ) ;
      }
   }

   /*
      _rtnChangeStreamCache implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAM_INIT, "_rtnChangeStreamCache::init" )
   INT32 _rtnChangeStreamCache::init( UINT32 cacheSize, UINT64 resumableWindow )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAM_INIT ) ;

      _cacheCapacity = cacheSize ;
      _cacheReadOffset = 0 ;
      _cacheWriteOffset = 0 ;
      _cacheReadIndex = 0 ;
      _cacheWriteIndex = 0 ;
      _queueCapacity = cacheSize + resumableWindow ;
      _queueReadOffset = 0 ;
      _queueWriteOffset = 0 ;
      _queueReadIndex = 0 ;
      _queueWriteIndex = 0 ;

      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAM_INIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAM_PUSHLOG, "_rtnChangeStreamCache::pushLog" )
   INT32 _rtnChangeStreamCache::pushLog( const utilChangeStreamLogInfo &logInfo,
                                         BOOLEAN needFilterRecord,
                                         BOOLEAN needCheckError,
                                         BOOLEAN needFilterType )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAM_PUSHLOG ) ;

      try
      {
         rtnChangeStreamLogInfo cachedInfo( logInfo, needFilterRecord,
                                            needCheckError, needFilterType ) ;
         BOOLEAN isCached = cachedInfo.isLogRecordCacheReady() ;
         UINT32 logLength = logInfo.getLength() ;

         PD_CHECK( _isQueueEnough( logLength ),
                   SDB_STREAM_NOT_CATCHUP, error, PDERROR,
                   "Change stream cache is full" ) ;

         // cache is full, release cache
         if ( isCached && !_isCacheEnough( logLength ) )
         {
            cachedInfo.releaseCache() ;
            isCached = FALSE ;
         }
         _logQueue.push( std::move( cachedInfo ) ) ;
         _queueWriteOffset += logLength ;
         ++ _queueWriteIndex ;
         if ( isCached )
         {
            _cacheWriteOffset += logLength ;
            ++ _cacheWriteIndex ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to push log, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAM_PUSHLOG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAM_POPLOG, "_rtnChangeStreamCache::popLog" )
   INT32 _rtnChangeStreamCache::popLog( rtnChangeStreamLogInfo &logInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAM_POPLOG ) ;

      try
      {
         if ( _logQueue.pop( logInfo ) )
         {
            _queueReadOffset += logInfo.getLength() ;
            ++ _queueReadIndex ;
            if ( logInfo.isLogRecordCacheReady() )
            {
               _cacheReadOffset += logInfo.getLength() ;
               ++ _cacheReadIndex ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to pop log, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAM_POPLOG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
