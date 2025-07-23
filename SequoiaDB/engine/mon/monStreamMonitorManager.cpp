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

   Source File Name = monStreamMonitorManager.cpp

   Descriptive Name = Stream Monitor Manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "monStreamMonitorManager.hpp"
#include "msgDef.h"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "monTrace.hpp"
#include "utilStreamToken.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _monStreamInfo implement
    */
   _monStreamInfo::_monStreamInfo( utilStreamType type,
                                   UINT64 sessionID,
                                   INT64 contextID )
   : _type( type ),
     _sessionID( sessionID ),
     _contextID( contextID )
   {
   }

   /*
      _monStreamItem implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMITEM_TOBSON, "_monStreamItem::toBSON" )
   INT32 _monStreamItem::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMITEM_TOBSON ) ;

      try
      {
         CHAR tokenStr[ MSG_STREAM_TOKEN_STING_SIZE + 1 ] ;
         tokenStr[ 0 ] = '\0' ;

         switch ( _currentToken.getTokenType() )
         {
            case MSG_STREAM_TOKEN_TYPE_CHANGE:
            {
               utilChangeStreamToken tmpToken( _currentToken ) ;
               tmpToken.toString( tokenStr ) ;
               break ;
            }
            default:
            {
               break ;
            }
         }

         builder.append( FIELD_NAME_TYPE, utilGetStreamTypeName( _type ) ) ;
         builder.append( FIELD_NAME_SESSIONID, (INT64)_sessionID ) ;
         builder.append( FIELD_NAME_CONTEXTID, _contextID ) ;
         builder.append( FIELD_NAME_OPTIONS, _options ) ;
         builder.append( FIELD_NAME_CURRENT_TOKEN, tokenStr ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON object, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMITEM_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _monStreamDetailedItem implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMDETAILITEM_TOBSON, "_monStreamDetailedItem::toBSON" )
   INT32 _monStreamDetailedItem::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMDETAILITEM_TOBSON ) ;

      try
      {
         UINT8 tokenType = _currentToken.getTokenType() ;
         CHAR tsStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( _startTimestamp, tsStr ) ;

         // convert to seconds
         FLOAT64 timeSpent =
               (FLOAT64)( ossGetCurrentMilliseconds() - _startTimestamp ) / 1000.0 ;
         // convert to MB/s
         FLOAT64 speed =
               ( timeSpent > 0.0 ) ?
                     ( (FLOAT64)_returnSize / 1024.0 / 1024.0 / timeSpent ) :
                     ( 0.0 ) ;

         rc = _monStreamItem::toBSON( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON object, rc: %d", rc ) ;

         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_CUR_TOKEN_DESC ) ) ;

         switch ( tokenType )
         {
         case MSG_STREAM_TOKEN_TYPE_CHANGE:
         {
            utilChangeStreamToken tmpToken( _currentToken ) ;
            tmpToken.toBSON( descBuilder ) ;
            break ;
         }
         default:
         {
            break ;
         }
         }

         descBuilder.doneFast() ;

         builder.append( FIELD_NAME_STARTTIMESTAMP, tsStr ) ;
         builder.append( FIELD_NAME_TIMESPENT, timeSpent ) ;
         builder.append( FIELD_NAME_CONTROL_NUM, (INT64)_controlNum ) ;
         builder.append( FIELD_NAME_CHANGE_NUM, (INT64)_changeNum ) ;
         builder.append( FIELD_NAME_DATA_NUM, (INT64)_dataNum ) ;
         builder.append( FIELD_NAME_BATCH_NUM, (INT64)_batchNum ) ;
         builder.append( FIELD_NAME_RETURN_NUM, (INT64)_returnNum ) ;
         builder.append( FIELD_NAME_RETURN_SIZE, (INT64)_returnSize ) ;
         builder.append( FIELD_NAME_SPEED, speed ) ;

         switch ( tokenType )
         {
         case MSG_STREAM_TOKEN_TYPE_CHANGE:
         {
            BSONObjBuilder sourceBuilder( builder.subobjStart( FIELD_NAME_SOURCE_STATS ) ) ;
            sourceBuilder.append( FIELD_NAME_RECEIVED_NUM,
                                  (INT64)_sourceStats._receivedNum ) ;
            sourceBuilder.append( FIELD_NAME_WAIT_TIME,
                                  (FLOAT64)_sourceStats._waitTime / OSS_ONE_SEC ) ;
            sourceBuilder.append( FIELD_NAME_HIT_CACHE_NUM,
                                  (INT64)( _sourceStats._hitCacheNum ) ) ;
            sourceBuilder.append( FIELD_NAME_MISS_CACHE_NUM,
                                  (INT64)( _sourceStats._missCacheNum ) ) ;
            sourceBuilder.append( FIELD_NAME_SCANNED_NUM,
                                  (INT64)( _sourceStats._scannedNum ) ) ;
            sourceBuilder.append( FIELD_NAME_SCANNED_SIZE,
                                  (INT64)( _sourceStats._scannedSize ) ) ;
            sourceBuilder.append( FIELD_NAME_QUEUE_CAPACITY,
                                  (INT64)( _sourceUsage._queueCapacity ) ) ;
            sourceBuilder.append( FIELD_NAME_IN_QUEUE_NUM,
                                  (INT64)( _sourceUsage._inQueueNum ) ) ;
            sourceBuilder.append( FIELD_NAME_IN_QUEUE_SIZE,
                                  (INT64)( _sourceUsage._inQueueSize ) ) ;
            sourceBuilder.append( FIELD_NAME_CACHE_CAPACITY,
                                  (INT64)( _sourceUsage._cacheCapacity ) ) ;
            sourceBuilder.append( FIELD_NAME_IN_CACHE_NUM,
                                  (INT64)( _sourceUsage._inCacheNum ) ) ;
            sourceBuilder.append( FIELD_NAME_IN_CACHE_SIZE,
                                  (INT64)( _sourceUsage._inCacheSize ) ) ;
            sourceBuilder.doneFast() ;
            break ;
         }
         default:
         {
            break ;
         }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON object, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMDETAILITEM_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _monStreamMonitor implement
    */
   _monStreamMonitor::_monStreamMonitor( utilStreamType type,
                                         UINT64 sessionID,
                                         INT64 contextID,
                                         monStreamSourceMonitor &sourceMonitor )
   : _monStreamInfo( type, sessionID, contextID ),
     _sourceMonitor( sourceMonitor )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMNON_INIT, "_monStreamMonitor::init" )
   INT32 _monStreamMonitor::init( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMNON_INIT ) ;

      try
      {
         // copy options
         _options = options.getOwned() ;
         // initial start timestamp
         _startTimestamp = ossGetCurrentMilliseconds() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize monitor, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMNON_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMNON_DUMPITEM, "_monStreamMonitor::dumpItem" )
   INT32 _monStreamMonitor::dumpItem( monStreamItem &item ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMNON_DUMPITEM ) ;

      try
      {
         item._type = _type ;
         item._sessionID = _sessionID ;
         item._contextID = _contextID ;
         item._options = _options.copy() ;
         _sourceMonitor.getCurrentToken( item._currentToken ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to dump item, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMNON_DUMPITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMNON_DUMPDETAILEDITEM, "_monStreamMonitor::dumpDetailedItem" )
   INT32 _monStreamMonitor::dumpDetailedItem( monStreamDetailedItem &item ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMNON_DUMPDETAILEDITEM ) ;

      try
      {
         rc = dumpItem( item ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to dump item, rc: %d", rc ) ;

         item._startTimestamp = _startTimestamp ;
         item._batchNum = _batchNum ;
         item._returnNum = _returnNum ;
         item._returnSize = _returnSize ;
         item._controlNum = _controlNum ;
         item._changeNum = _changeNum ;
         item._dataNum = _dataNum ;
         item._sourceStats = _sourceMonitor ;
         item._sourceUsage = _sourceMonitor.getUsage() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to dump detailed item, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMNON_DUMPDETAILEDITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _monStreamMonitorManager implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMMGR_REGMONITOR, "_monStreamMonitorManager::registerMonitor" )
   INT32 _monStreamMonitorManager::registerMonitor( const monStreamMonitor &monitor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMMGR_REGMONITOR ) ;

      try
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _streamMonitors.insert( &monitor ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register monitor, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMMGR_REGMONITOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMMGR_UNREGMONITOR, "_monStreamMonitorManager::unregisterMonitor" )
   void _monStreamMonitorManager::unregisterMonitor( const monStreamMonitor &monitor )
   {
      PD_TRACE_ENTRY( SDB_MONSTREAMMGR_UNREGMONITOR ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
      _streamMonitors.erase( &monitor ) ;

      PD_TRACE_EXIT( SDB_MONSTREAMMGR_UNREGMONITOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMMGR_DUMPITEMS, "_monStreamMonitorManager::dumpItems" )
   INT32 _monStreamMonitorManager::dumpItems( monStreamItemList &items )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMMGR_DUMPITEMS ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;
      for ( _monStreamMonitorSetCIter iter = _streamMonitors.begin() ;
            iter != _streamMonitors.end() ;
            ++ iter )
      {
         const monStreamMonitor *monitor = *iter ;
         if ( NULL != monitor )
         {
            monStreamItem item ;

            rc = monitor->dumpItem( item ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to dump item, rc: %d", rc ) ;

            try
            {
               items.push_back( item ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDWARNING, "Failed to save item, "
                       "occurred exception [%s]", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
         else
         {
            SDB_ASSERT( NULL != monitor, "monitor shoule be valid" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDWARNING, "Failed to dump items, "
                      "monitor shoule be valid" ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMMGR_DUMPITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSTREAMMGR_DUMPDETAILEDITEMS, "_monStreamMonitorManager::dumpDetailedItems" )
   INT32 _monStreamMonitorManager::dumpDetailedItems( monStreamDetailedItemList &items )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONSTREAMMGR_DUMPDETAILEDITEMS ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;
      for ( _monStreamMonitorSetCIter iter = _streamMonitors.begin() ;
            iter != _streamMonitors.end() ;
            ++ iter )
      {
         const monStreamMonitor *monitor = *iter ;
         if ( NULL != monitor )
         {
            monStreamDetailedItem item ;
            rc = monitor->dumpDetailedItem( item ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to dump detailed item, rc: %d", rc ) ;

            try
            {
               items.push_back( item ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDWARNING, "Failed to save detailed item, "
                       "occurred exception [%s]", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
         else
         {
            SDB_ASSERT( NULL != monitor, "monitor shoule be valid" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDWARNING, "Failed to dump "
                      "detailed items, monitor shoule be valid" ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_MONSTREAMMGR_DUMPDETAILEDITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
