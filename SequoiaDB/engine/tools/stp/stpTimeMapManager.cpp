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

   Source File Name = stpTimeMapManager.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "stpTimeMapManager.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"

namespace engine
{

   #define STP_TIME_MAP_MAX_CACHE_SIZE   ( 1000 )

   // table name of time map in database store
   #define STP_TIME_MAP_TABLE_NAME        "time_map"
   // column name of logical time in database store
   #define STP_TIME_MAP_LT_COLUMN_NAME    "logical_time"
   // column name of real time in database store
   #define STP_TIME_MAP_RT_COLUMN_NAME    "real_time"
   // index name of logical time in database store
   #define STP_TIME_MAP_LT_INDEX_NAME     "logical_time_index"
   // index name of real time in database store
   #define STP_TIME_MAP_RT_INDEX_NAME     "real_time_index"

   // create time map table command
   // NOTE:
   // INTEGER in SQLite is varying-length (1, 2, 4, 8 bytes) based on
   // input value, so it is safe to store UINT64 time values
   // CREATE TABLE IF NOT EXISTS time_map
   //    ( logical_time INTEGER, real_time INTEGER )
   #define STP_TIME_MAP_CRTTBL_COMMAND \
         "CREATE TABLE IF NOT EXISTS "STP_TIME_MAP_TABLE_NAME" ( "\
         STP_TIME_MAP_LT_COLUMN_NAME" INTEGER, " \
         STP_TIME_MAP_RT_COLUMN_NAME" INTEGER )"

   // create time map ( logical time ) index command
   #define STP_TIME_MAP_CRTLTIDX_COMMAND \
         "CREATE UNIQUE INDEX IF NOT EXISTS "STP_TIME_MAP_LT_INDEX_NAME \
         " ON "STP_TIME_MAP_TABLE_NAME" ( " \
         " "STP_TIME_MAP_LT_COLUMN_NAME" )"

   // create time map ( real time ) index command
   #define STP_TIME_MAP_CRTRTIDX_COMMAND \
         "CREATE UNIQUE INDEX IF NOT EXISTS "STP_TIME_MAP_RT_INDEX_NAME \
         " ON "STP_TIME_MAP_TABLE_NAME" ( " \
         " "STP_TIME_MAP_RT_COLUMN_NAME" )"

   // prepared INSERT command to save record into time map
   // INSERT INTO time_map VALUES ( ?, ? )
   #define STP_TIME_MAP_SAVEREC_COMMAND \
         "INSERT INTO "STP_TIME_MAP_TABLE_NAME" VALUES ( ?, ? )"

   // SELECT query to get records by given time
   // SELECT logical_time, real_time FROM time_map
   //    WHERE <time_name> [<|>|<=|>=|=] <real_time>
   //    ORDER BY real_time [DESC|ASC] LIMIT <N>
   #define STP_TIME_MAP_GETRECBYTIME_QUERY \
         "SELECT "STP_TIME_MAP_LT_COLUMN_NAME", "STP_TIME_MAP_RT_COLUMN_NAME \
         " FROM "STP_TIME_MAP_TABLE_NAME \
         " WHERE %s %s %llu" \
         " ORDER BY "STP_TIME_MAP_RT_COLUMN_NAME" %s LIMIT %u"

   // SELECT query to get latest or first records
   // SELECT logical_time, real_time FROM time_map
   //    ORDER BY real_time [DESC|ASC] LIMIT <N>
   #define STP_TIME_MAP_GETREC_QUERY \
         "SELECT "STP_TIME_MAP_LT_COLUMN_NAME", "STP_TIME_MAP_RT_COLUMN_NAME \
         " FROM "STP_TIME_MAP_TABLE_NAME \
         " ORDER BY "STP_TIME_MAP_LT_COLUMN_NAME" %s LIMIT %u"

   // SELECT query to get count of records in time map
   // SELECT count(*) FROM time_map
   #define STP_TIME_MAP_GETCNT_QUERY \
         "SELECT count(*) FROM "STP_TIME_MAP_TABLE_NAME

   // DELETE command to clear all records from time map
   // DELETE FROM time_map
   #define STP_TIME_MAP_CLRALL_COMMAND \
         "DELETE FROM "STP_TIME_MAP_TABLE_NAME

   // DELETE command to clear N expired records from time map
   // NOTE:
   // no support for DELETE with ORDER-BY and LIMIT in SQLite
   // so we use sub-query instead
   // DELETE FROM time_map WHERE logical_time IN
   //    ( SELECT logical_time FROM time_map
   //         ORDER BY logical_time ASC LIMIT <N> )
   #define STP_TIME_MAP_CLREXP_COMMAND \
         "DELETE FROM "STP_TIME_MAP_TABLE_NAME \
         " WHERE "STP_TIME_MAP_LT_COLUMN_NAME" IN (" \
         " SELECT "STP_TIME_MAP_LT_COLUMN_NAME \
         " FROM "STP_TIME_MAP_TABLE_NAME \
         " ORDER BY "STP_TIME_MAP_LT_COLUMN_NAME" ASC" \
         " LIMIT %u )"

   /*
      _stpTimeMapMemStore implement
    */
   _stpTimeMapMemStore::_stpTimeMapMemStore()
   : _stpTimeMapStore()
   {
   }

   _stpTimeMapMemStore::~_stpTimeMapMemStore()
   {
   }

   INT32 _stpTimeMapMemStore::initialize( const stpOptions *options )
   {
      return SDB_OK ;
   }

   INT32 _stpTimeMapMemStore::finalize()
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_SAVERECORD, "_stpTimeMapMemStore::saveRecord" )
   INT32 _stpTimeMapMemStore::saveRecord( const stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_SAVERECORD ) ;

      if ( _isDisabled() )
      {
         goto done ;
      }

      rc = _saveRecord( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save record "
                   "[logical: %llu, real: %llu], rc: %d",
                   record.getLogicalTime(), record.getRealTime(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_SAVERECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETRECBEFORELTIME, "_stpTimeMapMemStore::getRecordBeforeLTime" )
   INT32 _stpTimeMapMemStore::getRecordBeforeLTime( UINT64 logicalTime,
                                                    BOOLEAN included,
                                                    stpTimeMapRecord &record )
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETRECBEFORELTIME ) ;

      UINT64 mapRealTime = 0LL, mapLogicalTime = 0LL ;

      _getRecordFromMap( _logicalTimeMap, logicalTime, included,
                         mapLogicalTime, mapRealTime ) ;

      record.setMap( mapLogicalTime, mapRealTime ) ;

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE_GETRECBEFORELTIME ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETRECBEFORERTIME, "_stpTimeMapMemStore::getRecordBeforeRTime" )
   INT32 _stpTimeMapMemStore::getRecordBeforeRTime( UINT64 realTime,
                                                    BOOLEAN included,
                                                    stpTimeMapRecord &record )
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETRECBEFORERTIME ) ;

      UINT64 mapRealTime = 0LL, mapLogicalTime = 0LL ;

      _getRecordFromMap( _realTimeMap, realTime, included, mapRealTime,
                         mapLogicalTime ) ;

      record.setMap( mapLogicalTime, mapRealTime ) ;

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE_GETRECBEFORERTIME ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETRECSAFTERLTIME, "_stpTimeMapMemStore::getRecordsAfterLTime" )
   INT32 _stpTimeMapMemStore::getRecordsAfterLTime( UINT64 logicalTime,
                                                    BOOLEAN included,
                                                    UINT32 count,
                                                    STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETRECSAFTERLTIME ) ;

      rc = _getRecordListFromMap( _logicalTimeMap, logicalTime, included,
                                  count, FALSE, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record list from logical time "
                   "map, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETRECSAFTERLTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETRECSAFTERRTIME, "_stpTimeMapMemStore::getRecordsAfterRTime" )
   INT32 _stpTimeMapMemStore::getRecordsAfterRTime( UINT64 realTime,
                                                    BOOLEAN included,
                                                    UINT32 count,
                                                    STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETRECSAFTERRTIME ) ;

      rc = _getRecordListFromMap( _realTimeMap, realTime, included,
                                  count, TRUE, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record list from real time "
                   "map, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETRECSAFTERRTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETLASTREC, "_stpTimeMapMemStore::getLastRecord" )
   INT32 _stpTimeMapMemStore::getLastRecord( stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETLASTREC ) ;

      // if time map is empty, return nothing
      if ( !_logicalTimeMap.empty() )
      {
         // return latest record in reverse order
         STP_TIME_MAP::const_reverse_iterator iter = _logicalTimeMap.rbegin() ;

         record.setLogicalTime( iter->first ) ;
         record.setRealTime( iter->second ) ;
      }

      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETLASTREC, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETLASTRECS, "_stpTimeMapMemStore::getLastRecords" )
   INT32 _stpTimeMapMemStore::getLastRecords( UINT32 count,
                                              STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETLASTRECS ) ;

      // if time map is empty, return nothing
      if ( _logicalTimeMap.empty() )
      {
         goto done ;
      }

      // return latest records in reverse order
      try
      {
         for ( STP_TIME_MAP::const_reverse_iterator iter =
                                                   _logicalTimeMap.rbegin() ;
               iter != _logicalTimeMap.rend() && recordList.size() < count ;
               ++ iter )
         {
            stpTimeMapRecord record ;

            record.setLogicalTime( iter->first ) ;
            record.setRealTime( iter->second ) ;

            recordList.push_front( record ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save record in list, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }


   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETLASTRECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETFIRSTREC, "_stpTimeMapMemStore::getFirstRecord" )
   INT32 _stpTimeMapMemStore::getFirstRecord( stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETFIRSTREC ) ;

      // if time map is empty, return nothing
      if ( !_logicalTimeMap.empty() )
      {
         // return first record
         STP_TIME_MAP::const_iterator iter = _logicalTimeMap.begin() ;

         record.setLogicalTime( iter->first ) ;
         record.setRealTime( iter->second ) ;
      }

      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETFIRSTREC, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETFIRSTRECS, "_stpTimeMapMemStore::getFirstRecords" )
   INT32 _stpTimeMapMemStore::getFirstRecords( UINT32 count,
                                               STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETFIRSTRECS ) ;

      // if time map is empty, return nothing
      if ( _logicalTimeMap.empty() )
      {
         goto done ;
      }

      // return first records
      try
      {
         for ( STP_TIME_MAP::const_iterator iter = _logicalTimeMap.begin() ;
               iter != _logicalTimeMap.end() && recordList.size() < count ;
               ++ iter )
         {
            stpTimeMapRecord record ;

            record.setLogicalTime( iter->first ) ;
            record.setRealTime( iter->second ) ;

            recordList.push_back( record ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save record in list, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETFIRSTRECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_CLREXPRECS, "_stpTimeMapMemStore::clearExpiredRecords" )
   void _stpTimeMapMemStore::clearExpiredRecords()
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_CLREXPRECS ) ;

      if ( _isEnabled() && !_isUnlimited() )
      {
         _clearExpiredRecords() ;
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE_CLREXPRECS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_CLEARALLREC, "_stpTimeMapMemStore::clearAllRecords" )
   void _stpTimeMapMemStore::clearAllRecords()
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_CLEARALLREC ) ;

      _logicalTimeMap.clear() ;
      _realTimeMap.clear() ;

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE_CLEARALLREC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE__GETRECFROMMAP, "_stpTimeMapMemStore::_getRecordFromMap" )
   void _stpTimeMapMemStore::_getRecordFromMap( const STP_TIME_MAP &timeMap,
                                                UINT64 timeBefore,
                                                BOOLEAN included,
                                                UINT64 &mappingTime,
                                                UINT64 &mappedTime )
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE__GETRECFROMMAP ) ;

      if ( timeMap.empty() )
      {
         // map is empty: mapping and mapped times are the same
         // If the logical time and real time are almost the same
         // ( in +/- 0.5 second ) from the beginning of the system have
         // been created, no record will be saved, we can consider that
         // real time and logical time are the same
         mappingTime = timeBefore ;
         mappedTime = timeBefore ;
      }
      else if ( 1 == timeMap.size() )
      {
         // only one record in map: use this one directly
         mappingTime = timeMap.begin()->first ;
         mappedTime = timeMap.begin()->second ;
      }
      else
      {
         // search for lower bound
         STP_TIME_MAP::const_iterator iter =
                                 timeMap.lower_bound( timeBefore ) ;
         if ( ( iter != timeMap.end() && iter->first == timeBefore ) ||
              ( iter == timeMap.begin() ) )
         {
            // matched or the first one
            // use this one directly
            mappingTime = iter->first ;
            mappedTime = iter->second ;
         }
         else
         {
            // use the previous one
            // NOTE: we need the nearest one just before the given time
            -- iter ;
            mappingTime = iter->first ;
            mappedTime = iter->second ;
         }
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE__GETRECFROMMAP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE__GETRECLISTFROMMAP, "_stpTimeMapMemStore::_getRecordListFromMap" )
   INT32 _stpTimeMapMemStore::_getRecordListFromMap( const STP_TIME_MAP &timeMap,
                                                     UINT64 afterTime,
                                                     BOOLEAN included,
                                                     UINT32 maxNumReturn,
                                                     BOOLEAN fromRTimeToLTime,
                                                     STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE__GETRECLISTFROMMAP ) ;

      if ( timeMap.empty() )
      {
         goto done ;
      }

      try
      {
         STP_TIME_MAP::const_iterator iter ;
         if ( included )
         {
            // if included, use lower bound to include the given time
            iter = timeMap.lower_bound( afterTime ) ;
         }
         else
         {
            // if not included, use upper bound to exclude the given time
            iter = timeMap.upper_bound( afterTime ) ;
         }

         while ( iter != timeMap.end() && maxNumReturn > 0 )
         {
            stpTimeMapRecord record ;

            if ( fromRTimeToLTime )
            {
               record.setLogicalTime( iter->second ) ;
               record.setRealTime( iter->first ) ;
            }
            else
            {
               record.setLogicalTime( iter->first ) ;
               record.setRealTime( iter->second ) ;
            }

            recordList.push_back( record ) ;

            ++ iter ;
            -- maxNumReturn ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save record in list, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE__GETRECLISTFROMMAP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE__SAVERECORD, "_stpTimeMapMemStore::_saveRecord" )
   INT32 _stpTimeMapMemStore::_saveRecord( const stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE__SAVERECORD ) ;

      STP_TIME_MAP::iterator iterReal, iterLogical ;

      // check if we already have a record with the same real time
      iterReal = _realTimeMap.find( record.getRealTime() ) ;
      if ( iterReal != _realTimeMap.end() )
      {
         PD_LOG( PDWARNING, "Find a mapping with the same real time [%llu], "
                 "skip record [logical: %llu, real: %llu]",
                 record.getRealTime(), record.getLogicalTime(),
                 record.getRealTime() ) ;
         goto done ;
      }

      // check if we already have a record with the same logical time
      iterLogical = _logicalTimeMap.find( record.getLogicalTime() ) ;
      if ( iterLogical != _logicalTimeMap.end() )
      {
         PD_LOG( PDWARNING, "Find a mapping with the same logical time "
                 "[%llu], skip record [logical: %llu, real: %llu]",
                 record.getLogicalTime(), record.getLogicalTime(),
                 record.getRealTime() ) ;
         goto done ;
      }

      try
      {
         // save record
         _logicalTimeMap[ record.getLogicalTime() ] = record.getRealTime() ;
         _realTimeMap[ record.getRealTime() ] = record.getLogicalTime() ;
      }
      catch ( exception &e )
      {
         // rollback
         PD_LOG( PDERROR, "Failed to save record "
                 "[logical: %llu, real: %llu], occur exception: %s",
                 record.getLogicalTime(), record.getRealTime(), e.what() ) ;
         _logicalTimeMap.erase( record.getLogicalTime() ) ;
         _realTimeMap.erase( record.getRealTime() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE__SAVERECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE__CLREXPRECS, "_stpTimeMapMemStore::_clearExpiredRecords" )
   void _stpTimeMapMemStore::_clearExpiredRecords()
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE__CLREXPRECS ) ;

      // clear from logical time map
      _clearMaps( _logicalTimeMap,
                  _realTimeMap,
                  _maxTimeMapSize ) ;

      // clear from real time map
      _clearMaps( _realTimeMap,
                  _logicalTimeMap,
                  _maxTimeMapSize ) ;

      PD_LOG( PDDEBUG, "Cleared expired records, "
              "logical time map remains: %u, "
              "real time map remains: %u",
              _logicalTimeMap.size(),
              _realTimeMap.size() ) ;

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE__CLREXPRECS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE__CLRMAPS, "_stpTimeMapMemStore::_clearMaps" )
   void _stpTimeMapMemStore::_clearMaps( STP_TIME_MAP &mapping,
                                         STP_TIME_MAP &mapped,
                                         INT32 maxMapSize )
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE__CLRMAPS ) ;

      // erase records from mapping time map
      while ( (INT32)( mapping.size() ) > maxMapSize )
      {
         STP_TIME_MAP::iterator iter = mapping.begin() ;

         // make sure the mapped time map is also erased
         mapped.erase( iter->second ) ;

         mapping.erase( iter ) ;
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE__CLRMAPS ) ;
   }

   /*
      stpTimeMapDBStore implement
    */
   _stpTimeMapDBStore::_stpTimeMapDBStore()
   : _stpTimeMapStore(),
     _maxCacheSize( STP_TIME_MAP_MAX_CACHE_SIZE ),
     _currentMapSize( 0 ),
     _database()
   {
   }

   _stpTimeMapDBStore::~_stpTimeMapDBStore()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_INITIALIZE, "_stpTimeMapDBStore::initialize" )
   INT32 _stpTimeMapDBStore::initialize( const stpOptions *options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_INITIALIZE ) ;

      SDB_ASSERT( NULL != options, "options should be valid" ) ;

      const CHAR *configPath = options->getStpPath() ;
      CHAR databasePath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;

      // check config path
      PD_CHECK( NULL != configPath, SDB_INVALIDARG, error, PDERROR,
                "Config path is empty" ) ;

      // build file name of database file
      rc = utilBuildFullPath( configPath, STP_TIMEMAP_DB_FILE_NAME,
                              OSS_MAX_PATHSIZE, databasePath ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build database file path from config "
                   "path %s, rc: %d", configPath, rc ) ;

      // initialize database
      rc = _database.initialize( databasePath ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize database file [%s], "
                   "rc: %d", databasePath, rc ) ;

      // ensure table and indexes
      rc = _ensureTable() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to ensure table, rc: %d", rc ) ;

      // prepare statements
      rc = _prepareStatements() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare statements, rc: %d", rc ) ;

      // initialize cache
      rc = _cache.initialize( options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize cache, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_FINALIZE, "_stpTimeMapDBStore::finalize" )
   INT32 _stpTimeMapDBStore::finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_FINALIZE ) ;

      // close prepare statements
      _saveRecordStmt.closeStatement() ;

      // close cache
      _cache.finalize() ;

      // close database
      _database.finalize() ;

      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_FINALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_SAVERECORD, "_stpTimeMapDBStore::saveRecord" )
   INT32 _stpTimeMapDBStore::saveRecord( const stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_SAVERECORD ) ;

      if ( _isDisabled() )
      {
         goto done ;
      }

      // save record into database
      rc = _saveRecord( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save record "
                   "[logical: %llu, real: %llu] to database, rc: %d",
                   record.getLogicalTime(), record.getRealTime(), rc ) ;

      // save record into cache
      rc = _cache.saveRecord( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save record "
                   "[logical: %llu, real: %llu] to cache, rc: %d",
                   record.getLogicalTime(), record.getRealTime(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_SAVERECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETRECBEFORELTIME, "_stpTimeMapDBStore::getRecordBeforeLTime" )
   INT32 _stpTimeMapDBStore::getRecordBeforeLTime( UINT64 logicalTime,
                                                   BOOLEAN included,
                                                   stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETRECBEFORELTIME ) ;

      // get from cache first
      rc = _cache.getRecordBeforeLTime( logicalTime, included, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record before "
                   "logical time [%llu] from cache, rc: %d",
                   logicalTime, rc ) ;

      // if cached record is newer than given time, we need to check
      // from database
      if ( record.getLogicalTime() > logicalTime )
      {
         rc = _getRecordByTime( STP_TIME_MAP_LT_COLUMN_NAME,
                                logicalTime,
                                included,
                                TRUE,
                                TRUE,
                                record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get record before "
                      "logical time [%llu], rc: %d", logicalTime, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETRECBEFORELTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETRECBEFORERTIME, "_stpTimeMapDBStore::getRecordBeforeRTime" )
   INT32 _stpTimeMapDBStore::getRecordBeforeRTime( UINT64 realTime,
                                                   BOOLEAN included,
                                                   stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETRECBEFORERTIME ) ;

      // get from cache first
      rc = _cache.getRecordBeforeRTime( realTime, included, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record before "
                   "real time [%llu] from cache, rc: %d",
                   realTime, rc ) ;

      // if cached record is newer than given time, we need to check
      // from database
      if ( record.getRealTime() > realTime )
      {
         rc = _getRecordByTime( STP_TIME_MAP_RT_COLUMN_NAME,
                                realTime,
                                included,
                                TRUE,
                                TRUE,
                                record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get record before "
                      "real time [%llu], rc: %d", realTime, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETRECBEFORERTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETRECSAFTERLTIME, "_stpTimeMapDBStore::getRecordsAfterLTime" )
   INT32 _stpTimeMapDBStore::getRecordsAfterLTime( UINT64 logicalTime,
                                                   BOOLEAN included,
                                                   UINT32 count,
                                                   STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETRECSAFTERLTIME ) ;

      if ( _isLTimeInCache( logicalTime ) )
      {
         // get from cache
         rc = _cache.getRecordsAfterLTime( logicalTime, included, count,
                                           recordList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get records after "
                      "logical time [%llu] from cache, rc: %d",
                      logicalTime, rc ) ;

         goto done ;
      }

      // get from database
      rc = _getRecordsByTime( STP_TIME_MAP_LT_COLUMN_NAME,
                              logicalTime,
                              FALSE,
                              FALSE,
                              FALSE,
                              count,
                              recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get records after "
                   "logical time [%llu], rc: %d", logicalTime, rc ) ;

      // reverse record list
      recordList.reverse() ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETRECSAFTERLTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETRECSAFTERRTIME, "_stpTimeMapDBStore::getRecordsAfterRTime" )
   INT32 _stpTimeMapDBStore::getRecordsAfterRTime( UINT64 realTime,
                                                   BOOLEAN included,
                                                   UINT32 count,
                                                   STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETRECSAFTERRTIME ) ;

      if ( _isRTimeInCache( realTime ) )
      {
         // get from cache first
         rc = _cache.getRecordsAfterRTime( realTime, included, count,
                                          recordList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get records after "
                      "real time [%llu] from cache, rc: %d",
                      realTime, rc ) ;

         goto done ;
      }

      // get from database
      rc = _getRecordsByTime( STP_TIME_MAP_RT_COLUMN_NAME,
                              realTime,
                              FALSE,
                              FALSE,
                              FALSE,
                              count,
                              recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get records after "
                   "real time [%llu], rc: %d", realTime, rc ) ;

      // reverse record list
      recordList.reverse() ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETRECSAFTERRTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETLASTREC, "_stpTimeMapDBStore::getLastRecord" )
   INT32 _stpTimeMapDBStore::getLastRecord( stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETLASTREC ) ;

      if ( _cache.getTimeMapSize() > 0 )
      {
         // last record must in cache, get from cache
         rc = _cache.getLastRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get latest record from cache, "
                      "rc: %d", rc ) ;
      }
      else if ( _currentMapSize > 0 )
      {
         // no cache, but have records in database, get from database
         rc = _getRecord( TRUE, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get latest record from "
                      "database, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETLASTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETLASTRECS, "_stpTimeMapDBStore::getLastRecords" )
   INT32 _stpTimeMapDBStore::getLastRecords( UINT32 count,
                                             STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETLASTRECS ) ;

      UINT32 targetLimit = 0 ;
      UINT64 targetLogicalTime = 0 ;
      STP_TIMEMAP_RECLIST dbRecordList ;

      // get from cache first
      rc = _cache.getLastRecords( count, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get latest records from cache, "
                   "rc: %d", rc ) ;
      if ( recordList.size() >= count )
      {
         // already get expected number of records
         goto done ;
      }

      if ( (UINT32)( recordList.size() ) < _cache.getTimeMapSize() )
      {
         // range is covered by cache
         goto done ;
      }

      // get remain records from database
      targetLimit = count - recordList.size() ;
      targetLogicalTime = recordList.begin()->getLogicalTime() ;

      rc = _getRecordsByTime( STP_TIME_MAP_LT_COLUMN_NAME,
                              targetLogicalTime,
                              FALSE,
                              TRUE,
                              TRUE,
                              targetLimit,
                              dbRecordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get records after "
                   "logical time [%llu], rc: %d", targetLogicalTime, rc ) ;

      if ( !dbRecordList.empty() )
      {
         rc = _mergeRecordList( recordList, dbRecordList, TRUE, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to merge record lists, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETLASTRECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETFIRSTREC, "_stpTimeMapDBStore::getFirstRecord" )
   INT32 _stpTimeMapDBStore::getFirstRecord( stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETFIRSTREC ) ;

      UINT32 cacheSize = _cache.getTimeMapSize() ;

      if ( cacheSize > 0 &&
           cacheSize == _currentMapSize )
      {
         // first record must in cache, get from cache
         rc = _cache.getFirstRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get first record from cache, "
                      "rc: %d", rc ) ;
      }
      else if ( _currentMapSize > 0 )
      {
         // no cache, but have records in database, get from database
         rc = _getRecord( FALSE, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get first record from "
                      "database, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETFIRSTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETFIRSTRECS, "_stpTimeMapDBStore::getFirstRecords" )
   INT32 _stpTimeMapDBStore::getFirstRecords( UINT32 count,
                                              STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETFIRSTRECS ) ;

      if ( _currentMapSize == _cache.getTimeMapSize() )
      {
         // all records are cached, get from cache
         rc = _cache.getFirstRecords( count, recordList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get latest records from cache, "
                      "rc: %d", rc ) ;
         goto done ;
      }

      rc = _getRecords( FALSE, count, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get records, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE_GETFIRSTRECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_CLREXPRECS, "_stpTimeMapDBStore::clearExpiredRecords" )
   void _stpTimeMapDBStore::clearExpiredRecords()
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_CLREXPRECS ) ;

      if ( _isDisabled() || _isUnlimited() )
      {
         goto done ;
      }

      // clear cache first
      _cache.clearExpiredRecords() ;

      // clear database if needed
      if ( _currentMapSize > (UINT32)_maxTimeMapSize )
      {
         CHAR command[ UTIL_MAX_SQL_SIZE + 1 ] = { '\0' } ;

         UINT32 removeLimit = _currentMapSize - (UINT32)_maxTimeMapSize ;

         ossSnprintf( command, UTIL_MAX_SQL_SIZE,
                      STP_TIME_MAP_CLREXP_COMMAND, removeLimit ) ;

         INT32 rc = _database.execCommand( command ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to clear expired records from "
                    "time map table [%s], rc: %d",
                    STP_TIME_MAP_TABLE_NAME, rc ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Cleared expired records [%u] from "
                    "time map table [%s]", removeLimit,
                    STP_TIME_MAP_TABLE_NAME ) ;
            _currentMapSize -= removeLimit ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB__STPTIMEMAPDBSTORE_CLREXPRECS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_CLRALLRECS, "_stpTimeMapDBStore::clearAllRecords" )
   void _stpTimeMapDBStore::clearAllRecords()
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_CLRALLRECS ) ;

      // clear cache first
      _cache.clearAllRecords() ;

      // clear database
      INT32 rc = _database.execCommand( STP_TIME_MAP_CLRALL_COMMAND ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to clear all records from "
                 "time map table [%s], rc: %d", STP_TIME_MAP_TABLE_NAME, rc ) ;
      }
      else
      {
         _currentMapSize = 0 ;
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPDBSTORE_CLRALLRECS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_GETTIMEMAPSIZE, "_stpTimeMapDBStore::getTimeMapSize" )
   UINT32 _stpTimeMapDBStore::getTimeMapSize()
   {
      UINT32 timeMapSize = 0 ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_GETTIMEMAPSIZE ) ;

      // get from database
      INT32 rc = _getTimeMapSize( timeMapSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to get size of time map, rc: %d", rc ) ;
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPDBSTORE_GETTIMEMAPSIZE ) ;

      return timeMapSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE_SETMAXTIMESIZE, "_stpTimeMapDBStore::setMaxTimeMapSize" )
   void _stpTimeMapDBStore::setMaxTimeMapSize( INT32 maxTimeMapSize )
   {
      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE_SETMAXTIMESIZE ) ;

      _stpTimeMapStore::setMaxTimeMapSize( maxTimeMapSize ) ;

      // calculate cache size
      _maxCacheSize = STP_TIME_MAP_MAX_CACHE_SIZE ;
      if ( _isDisabled() )
      {
         // if disabled, also disable cache
         _maxCacheSize = 0 ;
      }
      else if ( !_isUnlimited() )
      {
         // if it is limited, check if specified size of time map is
         // smaller than default cache size
         _maxCacheSize = OSS_MIN( _maxCacheSize, (UINT32)_maxTimeMapSize ) ;
      }
      _cache.setMaxTimeMapSize( _maxCacheSize ) ;

      if ( _isEnabled() && _cache.isEmpty() )
      {
         // if time map is enabled and cache is empty, load cache
         INT32 rc = _loadCache() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to load cache, rc: %d", rc ) ;
         }
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPDBSTORE_SETMAXTIMESIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__ENSURETABLE, "_stpTimeMapDBStore::_ensureTable" )
   INT32 _stpTimeMapDBStore::_ensureTable()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__ENSURETABLE ) ;

      // check or create table
      rc = _database.execCommand( STP_TIME_MAP_CRTTBL_COMMAND ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to ensure table [%s], rc: %d",
                   STP_TIME_MAP_TABLE_NAME, rc ) ;

      // check or create logical time index
      rc = _database.execCommand( STP_TIME_MAP_CRTLTIDX_COMMAND ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to ensure logical time index [%s], "
                   "rc: %d", STP_TIME_MAP_LT_INDEX_NAME, rc ) ;

      // check or create real time index
      rc = _database.execCommand( STP_TIME_MAP_CRTRTIDX_COMMAND ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to ensure real time index [%s], "
                   "rc: %d", STP_TIME_MAP_RT_INDEX_NAME, rc ) ;

      // try get current size of time map
      rc = _getTimeMapSize( _currentMapSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get time map size, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Check database store, has %u records",
              _currentMapSize ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__ENSURETABLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__PREPARESTMTS, "_stpTimeMapDBStore::_prepareStatements" )
   INT32 _stpTimeMapDBStore::_prepareStatements()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__PREPARESTMTS ) ;

      // prepare command to save record
      rc = _database.prepareCommand( STP_TIME_MAP_SAVEREC_COMMAND,
                                     _saveRecordStmt ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare save record command, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__PREPARESTMTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__SAVERECORD, "_stpTimeMapDBStore::_saveRecord" )
   INT32 _stpTimeMapDBStore::_saveRecord( const stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__SAVERECORD ) ;

      // reset statement
      rc = _saveRecordStmt.reset() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reset statement, rc: %d", rc ) ;

      // bind logical time
      rc = _saveRecordStmt.bindINT64At( 1, record.getLogicalTime() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind parameter for logical time, "
                   "rc: %d", rc ) ;

      // bind real time
      rc = _saveRecordStmt.bindINT64At( 2, record.getRealTime() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind parameter for real time, "
                   "rc: %d", rc ) ;

      // execute statement
      rc = _saveRecordStmt.execute() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute statement, rc: %d", rc ) ;

      // update cached size of map
      ++ _currentMapSize ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__SAVERECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__LOADCACHE, "_stpTimeMapDBStore::_loadCache" )
   INT32 _stpTimeMapDBStore::_loadCache()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__LOADCACHE ) ;

      STP_TIMEMAP_RECLIST recordList ;

      if ( _isDisabled() )
      {
         goto done ;
      }

      rc = _getRecords( TRUE, _maxCacheSize, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get last records, rc: %d", rc ) ;

      for ( STP_TIMEMAP_RECLIST::iterator iter = recordList.begin() ;
            iter != recordList.end() ;
            ++ iter )
      {
         // save to cache
         rc = _cache.saveRecord( *iter ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save record to cache, rc: %d",
                      rc ) ;
      }

      PD_LOG( PDEVENT, "Load [%u] records into time map cache",
              _cache.getTimeMapSize() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__LOADCACHE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__GETTIMEMAPSIZE, "_stpTimeMapDBStore::_getTimeMapSize" )
   INT32 _stpTimeMapDBStore::_getTimeMapSize( UINT32 &timeMapSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__GETTIMEMAPSIZE ) ;

      INT32 countResult = 0 ;
      utilSQLiteContext context ;

      // get count from time map table
      rc = _database.execQuery( STP_TIME_MAP_GETCNT_QUERY, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute count query, rc: %d",rc ) ;

      rc = context.moveNext() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get count from time map table [%s], "
                   "rc: %d", STP_TIME_MAP_TABLE_NAME, rc ) ;

      rc = context.getINT32At( 0, countResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get result from query, "
                   "rc: %d", rc ) ;

      timeMapSize = (UINT32)countResult ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__GETTIMEMAPSIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__MERGERECLIST, "_stpTimeMapDBStore::_mergeRecordList" )
   INT32 _stpTimeMapDBStore::_mergeRecordList( STP_TIMEMAP_RECLIST &recordList,
                                               const STP_TIMEMAP_RECLIST &mergeList,
                                               BOOLEAN isMergeInDescOrder,
                                               BOOLEAN isMergeToEnd )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__MERGERECLIST ) ;

      try
      {
         // record list must be ascending order by logical time
         if ( isMergeInDescOrder )
         {
            for ( STP_TIMEMAP_RECLIST::const_reverse_iterator iter =
                                                         mergeList.rbegin() ;
                  iter != mergeList.rend() ;
                  ++ iter )
            {
               if ( isMergeToEnd )
               {
                  recordList.push_back( *iter ) ;
               }
               else
               {
                  recordList.push_front( *iter ) ;
               }
            }
         }
         else
         {
            for ( STP_TIMEMAP_RECLIST::const_iterator iter = mergeList.begin() ;
                  iter != mergeList.end() ;
                  ++ iter )
            {
               if ( isMergeToEnd )
               {
                  recordList.push_back( *iter ) ;
               }
               else
               {
                  recordList.push_front( *iter ) ;
               }
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to merge record lists "
                 "from database and from cache, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__MERGERECLIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__GETRECBYTIME, "_stpTimeMapDBStore::_getRecordByTime" )
   INT32 _stpTimeMapDBStore::_getRecordByTime( const CHAR *timeColumnName,
                                               UINT64 targetTime,
                                               BOOLEAN isIncluded,
                                               BOOLEAN isBefore,
                                               BOOLEAN isDescOrder,
                                               stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__GETRECBYTIME ) ;

      SDB_ASSERT( NULL != timeColumnName, "time column name is invalid" ) ;

      utilSQLiteContext context ;

      CHAR query[ UTIL_MAX_SQL_SIZE + 1 ] = { '\0' } ;
      ossSnprintf( query,
                   UTIL_MAX_SQL_SIZE,
                   STP_TIME_MAP_GETRECBYTIME_QUERY,
                   timeColumnName,
                   isIncluded ? ( isBefore ? "<=" : ">=" ) :
                                ( isBefore ? "<" : ">" ),
                   targetTime,
                   isDescOrder ? "DESC" : "ASC",
                   1 ) ;

      // execute query
      rc = _database.execQuery( query, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute query, rc: %d", rc ) ;

      // loop for result row, we only need the first record
      rc = context.moveNext() ;
      if ( SDB_OK == rc )
      {
         INT64 resLogicalTime = 0, resRealTime = 0 ;

         rc = context.getINT64At( 0, resLogicalTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, "
                      "rc: %d", rc ) ;

         rc = context.getINT64At( 1, resRealTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get real time, "
                      "rc: %d", rc ) ;

         record.setLogicalTime( resLogicalTime ) ;
         record.setRealTime( resRealTime ) ;
      }
      else if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to move next record, rc: %d",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__GETRECBYTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__GETRECSBYTIME, "_stpTimeMapDBStore::_getRecordsByTime" )
   INT32 _stpTimeMapDBStore::_getRecordsByTime( const CHAR *timeColumnName,
                                                UINT64 targetTime,
                                                BOOLEAN isIncluded,
                                                BOOLEAN isBefore,
                                                BOOLEAN isDescOrder,
                                                UINT32 count,
                                                STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__GETRECSBYTIME ) ;

      SDB_ASSERT( NULL != timeColumnName, "time column name is invalid" ) ;

      utilSQLiteContext context ;
      CHAR query[ UTIL_MAX_SQL_SIZE + 1 ] = { '\0' } ;

      ossSnprintf( query,
                   UTIL_MAX_SQL_SIZE,
                   STP_TIME_MAP_GETRECBYTIME_QUERY,
                   timeColumnName,
                   isIncluded ? ( isBefore ? "<=" : ">=" ) :
                                ( isBefore ? "<" : ">" ),
                   targetTime,
                   isDescOrder ? "DESC" : "ASC",
                   count ) ;

      // execute query
      rc = _database.execQuery( query, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute query, rc: %d", rc ) ;

      // loop for results
      while ( TRUE )
      {
         rc = context.moveNext() ;
         if ( SDB_OK == rc )
         {
            INT64 resLogicalTime = 0, resRealTime = 0 ;
            stpTimeMapRecord record ;

            rc = context.getINT64At( 0, resLogicalTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, "
                         "rc: %d", rc ) ;

            rc = context.getINT64At( 1, resRealTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get real time, "
                         "rc: %d", rc ) ;

            record.setLogicalTime( resLogicalTime ) ;
            record.setRealTime( resRealTime ) ;

            // save to output record list
            try
            {
               recordList.push_front( record ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save record in list, "
                       "occur exception: %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
         else if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to move next record, rc: %d",
                         rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__GETRECSBYTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__GETREC, "_stpTimeMapDBStore::_getRecord" )
   INT32 _stpTimeMapDBStore::_getRecord( BOOLEAN isDescOrder,
                                         stpTimeMapRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__GETREC ) ;

      utilSQLiteContext context ;

      CHAR query[ UTIL_MAX_SQL_SIZE + 1 ] = { '\0' } ;
      ossSnprintf( query, UTIL_MAX_SQL_SIZE, STP_TIME_MAP_GETREC_QUERY,
                   isDescOrder ? "DESC" : "ASC", 1 ) ;

      // execute query
      rc = _database.execQuery( query, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute query, rc: %d", rc ) ;

      // loop for result row, we only need the first record
      rc = context.moveNext() ;
      if ( SDB_OK == rc )
      {
         INT64 resLogicalTime = 0, resRealTime = 0 ;

         rc = context.getINT64At( 0, resLogicalTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, "
                      "rc: %d", rc ) ;

         rc = context.getINT64At( 1, resRealTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get real time, "
                      "rc: %d", rc ) ;

         record.setLogicalTime( resLogicalTime ) ;
         record.setRealTime( resRealTime ) ;
      }
      else if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to move next record, rc: %d",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__GETREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__GETRECS, "_stpTimeMapDBStore::_getRecords" )
   INT32 _stpTimeMapDBStore::_getRecords( BOOLEAN isDescOrder,
                                          UINT32 count,
                                          STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__GETRECS ) ;

      utilSQLiteContext context ;
      CHAR query[ UTIL_MAX_SQL_SIZE + 1 ] = { '\0' } ;

      ossSnprintf( query, UTIL_MAX_SQL_SIZE, STP_TIME_MAP_GETREC_QUERY,
                   isDescOrder ? "DESC" : "ASC", count ) ;

      // execute query
      rc = _database.execQuery( query, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute query, rc: %d", rc ) ;

      // loop for results
      while ( TRUE )
      {
         rc = context.moveNext() ;
         if ( SDB_OK == rc )
         {
            INT64 resLogicalTime = 0, resRealTime = 0 ;
            stpTimeMapRecord record ;

            rc = context.getINT64At( 0, resLogicalTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, "
                         "rc: %d", rc ) ;

            rc = context.getINT64At( 1, resRealTime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get real time, "
                         "rc: %d", rc ) ;

            record.setLogicalTime( resLogicalTime ) ;
            record.setRealTime( resRealTime ) ;

            // save to output record list
            try
            {
               recordList.push_front( record ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save record in list, "
                       "occur exception: %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
         else if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to move next record, rc: %d",
                         rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPDBSTORE__GETRECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__ISLTINCACHE, "_stpTimeMapDBStore::_isLTimeInCache" )
   BOOLEAN _stpTimeMapDBStore::_isLTimeInCache( UINT64 logicalTime )
   {
      BOOLEAN isInCache = FALSE ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__ISLTINCACHE ) ;

      if ( _cache.getTimeMapSize() )
      {
         stpTimeMapRecord firstCacheRecord ;
         if ( SDB_OK == _cache.getFirstRecord( firstCacheRecord ) )
         {
            isInCache = firstCacheRecord.getLogicalTime() < logicalTime ;
         }
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPDBSTORE__ISLTINCACHE ) ;

      return isInCache ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPDBSTORE__ISRTINCACHE, "_stpTimeMapDBStore::_isRTimeInCache" )
   BOOLEAN _stpTimeMapDBStore::_isRTimeInCache( UINT64 realTime )
   {
      BOOLEAN isInCache = FALSE ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPDBSTORE__ISLTINCACHE ) ;

      if ( _cache.getTimeMapSize() )
      {
         stpTimeMapRecord firstCacheRecord ;
         if ( SDB_OK == _cache.getFirstRecord( firstCacheRecord ) )
         {
            isInCache = firstCacheRecord.getRealTime() < realTime ;
         }
      }

      PD_TRACE_EXIT( SDB__STPTIMEMAPDBSTORE__ISLTINCACHE ) ;

      return isInCache ;
   }

   /*
      _stpTimeMapManager implement
    */
   _stpTimeMapManager::_stpTimeMapManager()
   : _needSaveTimeMapping( FALSE ),
     _lastRecord(),
     _store( NULL )
   {
   }

   _stpTimeMapManager::~_stpTimeMapManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_INITIALIZE, "_stpTimeMapManager::initialize" )
   INT32 _stpTimeMapManager::initialize( const stpOptions *options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_INITIALIZE ) ;

      SDB_ASSERT( NULL != options, "options should be valid" ) ;

      ossScopedRWLock lock( &_storeLock, EXCLUSIVE ) ;

      // create store
      _store = SDB_OSS_NEW stpTimeMapDBStore() ;
      PD_CHECK( NULL != _store, SDB_OOM, error, PDERROR,
                "Failed to allocate time map store" ) ;

      // initialize store
      rc = _store->initialize( options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize time map store, rc: %d",
                   rc ) ;

      // set maximum size of time map
      setMaxTimeMapSize( options->getMaxTimeMapSize(), TRUE ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_FINALIZE, "_stpTimeMapManager::finalize" )
   INT32 _stpTimeMapManager::finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_FINALIZE ) ;

      ossScopedRWLock lock( &_storeLock, EXCLUSIVE ) ;

      // release store
      if ( NULL != _store )
      {
         _store->finalize() ;
         SDB_OSS_DEL _store ;

         _store = NULL ;
      }

      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_FINALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_SAVETIMEMAPPING, "_stpTimeMapManager::saveTimeMapping" )
   INT32 _stpTimeMapManager::saveTimeMapping( stpMetaData *metaData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_SAVETIMEMAPPING ) ;

      SDB_ASSERT( NULL != metaData, "meta data should be valid" ) ;

      ossScopedRWLock lock( &_storeLock, EXCLUSIVE ) ;

      SDB_ASSERT( NULL != _store, "time map store is invalid" ) ;

      stpHPTime realTime ;
      stpLogicalTimeNS logicalTime ;
      stpTimeMapRecord record ;

      if ( !_needSaveTimeMapping )
      {
         _needSaveTimeMapping = TRUE ;
      }

      // get logical time
      rc = metaData->getLogicalTimeNS( logicalTime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

      // get real time
      realTime.sampleReal() ;

      record.setMap( logicalTime.getTime().toMicroSecond(),
                     realTime.toMicroSecond() ) ;

      // check if we need to save
      // if the time shift is tolerable by the last record, no need to save
      if ( _lastRecord.isValid() && _lastRecord.isTolerable( record ) )
      {
         PD_LOG( PDDEBUG, "Time mapping record [logical: %llu, real: %llu] "
                 "is tolerable with last record [logical: %llu, real: %llu]",
                 record.getLogicalTime(), record.getRealTime(),
                 _lastRecord.getLogicalTime(), _lastRecord.getRealTime() ) ;
         goto done ;
      }

      // save the time mapping store
      rc = _store->saveRecord( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save record, rc: %d", rc ) ;

      // update the last record
      _lastRecord = record ;

      PD_LOG( PDEVENT, "Save time mapping record [logical: %llu, real: %llu]",
              record.getLogicalTime(), record.getRealTime() ) ;

      // clear expired records
      _store->clearExpiredRecords() ;

   done:
      // reset need save flat if succeed
      if ( SDB_OK == rc )
      {
         _needSaveTimeMapping = FALSE ;
      }

      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_SAVETIMEMAPPING, rc ) ;

      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_CONVLOGICALTOREAL, "_stpTimeMapManager::convLTimeToRTime" )
   INT32 _stpTimeMapManager::convLTimeToRTime( const stpHPTime &logicalTime,
                                               stpHPTime &realTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_CONVLOGICALTOREAL ) ;

      stpTimeMapRecord record ;
      UINT64 logicalTimeUS = logicalTime.toMicroSecond() ;
      UINT64 realTimeUS = 0LL ;

      ossScopedRWLock lock( &_storeLock, SHARED ) ;

      PD_CHECK( NULL != _store, SDB_SYS, error, PDERROR,
                "Failed to get record from invalid time map store" ) ;

      // get record from store
      rc = _store->getRecordBeforeLTime( logicalTimeUS, TRUE, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record by logical time [%llu], "
                   "rc: %d", logicalTimeUS, rc ) ;

      PD_CHECK( record.isValid(), SDB_SYS, error, PDERROR,
                "Failed to get record by logical time [%llu], "
                "record is invalid", logicalTimeUS ) ;

      realTimeUS = record.calcRealTime( logicalTimeUS ) ;
      realTime.fromMicroSecond( realTimeUS ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_CONVLOGICALTOREAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_CONVRTTOLT, "_stpTimeMapManager::convRTimeToLTime" )
   INT32 _stpTimeMapManager::convRTimeToLTime( const stpHPTime &realTime,
                                               stpHPTime &logicalTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_CONVRTTOLT ) ;

      stpTimeMapRecord record ;
      UINT64 realTimeUS = realTime.toMicroSecond() ;
      UINT64 logicalTimeUS = 0LL ;

      ossScopedRWLock lock( &_storeLock, SHARED ) ;

      PD_CHECK( NULL != _store, SDB_SYS, error, PDERROR,
                "Failed to get record from invalid time map store" ) ;

      // get record from store
      rc = _store->getRecordBeforeRTime( realTimeUS, TRUE, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record by real time [%llu], "
                   "rc: %d", realTimeUS, rc ) ;

      PD_CHECK( record.isValid(), SDB_SYS, error, PDERROR,
                "Failed to get record by logical time [%llu], "
                "record is invalid", realTimeUS ) ;

      logicalTimeUS = record.calcLogicalTime( realTimeUS ) ;
      logicalTime.fromMicroSecond( logicalTimeUS ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_CONVRTTOLT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_GETRECSAFTERLT, "_stpTimeMapManager::getRecordsAfterLTime" )
   INT32 _stpTimeMapManager::getRecordsAfterLTime( const stpHPTime &logicalTime,
                                                   BOOLEAN included,
                                                   UINT32 count,
                                                   STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_GETRECSAFTERLT ) ;

      UINT64 logicalTimeUS = logicalTime.toMicroSecond() ;

      ossScopedRWLock lock( &_storeLock, SHARED ) ;

      PD_CHECK( NULL != _store, SDB_SYS, error, PDERROR,
                "Failed to get record from invalid time map store" ) ;

      // get records from store
      rc = _store->getRecordsAfterLTime( logicalTimeUS,
                                         included,
                                         count,
                                         recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record after "
                   "logical time [%llu] from time map store, rc: %d",
                   logicalTimeUS, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_GETRECSAFTERLT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_GETRECSAFTERRT, "_stpTimeMapManager::getRecordsAfterRTime" )
   INT32 _stpTimeMapManager::getRecordsAfterRTime( const stpHPTime &realTime,
                                                   BOOLEAN included,
                                                   UINT32 count,
                                                   STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_GETRECSAFTERRT ) ;

      UINT64 realTimeUS = realTime.toMicroSecond() ;

      ossScopedRWLock lock( &_storeLock, SHARED ) ;

      PD_CHECK( NULL != _store, SDB_SYS, error, PDERROR,
                "Failed to get record from invalid time map store" ) ;

      // get records from store
      rc = _store->getRecordsAfterRTime( realTimeUS,
                                         included,
                                         count,
                                         recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record after "
                   "real time [%llu] from time map store, rc: %d",
                   realTimeUS, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_GETRECSAFTERRT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMGR_GETLASTRECS, "_stpTimeMapManager::getLastRecords" )
   INT32 _stpTimeMapManager::getLastRecords( UINT32 count,
                                             STP_TIMEMAP_RECLIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_GETLASTRECS ) ;

      ossScopedRWLock lock( &_storeLock, SHARED ) ;

      PD_CHECK( NULL != _store, SDB_SYS, error, PDERROR,
                "Failed to get record from invalid time map store" ) ;

      // get records from store
      rc = _store->getLastRecords( count, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get latest records from "
                   "time map store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMGR_GETLASTRECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
