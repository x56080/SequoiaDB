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
