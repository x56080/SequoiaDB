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

   #define STP_TIME_MAP_MAX_MEM_RECORDS ( 1000 )

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

   INT32 _stpTimeMapMemStore::initialize()
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

      ossScopedRWLock lock( &_mapLock, EXCLUSIVE ) ;

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
         rc = SDB_SYS ;
         goto error ;
      }

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
      ossScopedRWLock lock( &_mapLock, SHARED ) ;

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
      ossScopedRWLock lock( &_mapLock, SHARED ) ;

      _getRecordFromMap( _realTimeMap, realTime, included, mapRealTime,
                         mapLogicalTime ) ;

      record.setMap( mapLogicalTime, mapRealTime ) ;

      PD_TRACE_EXIT( SDB__STPTIMEMAPMEMSTORE_GETRECBEFORERTIME ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETRECAFTERLTIME, "_stpTimeMapMemStore::getRecordAfterLTime" )
   INT32 _stpTimeMapMemStore::getRecordAfterLTime( UINT64 logicalTime,
                                                   BOOLEAN included,
                                                   UINT32 count,
                                                   STP_TIME_MAP_RECORD_LIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETRECAFTERLTIME ) ;

      ossScopedRWLock lock( &_mapLock, SHARED ) ;

      rc = _getRecordListFromMap( _logicalTimeMap, logicalTime, included,
                                  count, TRUE, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record list from logical time "
                   "map, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETRECAFTERLTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_GETRECAFTERRTIME, "_stpTimeMapMemStore::getRecordAfterRTime" )
   INT32 _stpTimeMapMemStore::getRecordAfterRTime( UINT64 realTime,
                                                   BOOLEAN included,
                                                   UINT32 count,
                                                   STP_TIME_MAP_RECORD_LIST &recordList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_GETRECAFTERRTIME ) ;

      ossScopedRWLock lock( &_mapLock, SHARED ) ;

      rc = _getRecordListFromMap( _realTimeMap, realTime, included,
                                  count, FALSE, recordList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record list from real time "
                   "map, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_GETRECAFTERRTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPTIMEMAPMEMSTORE_CLRRECS, "_stpTimeMapMemStore::clearExpiredRecords" )
   INT32 _stpTimeMapMemStore::clearExpiredRecords()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMEMSTORE_CLRRECS ) ;

      UINT32 clearedRealNum = 0, clearedLogicalNum = 0 ;

      ossScopedRWLock lock( &_mapLock, EXCLUSIVE ) ;

      while ( _logicalTimeMap.size() > STP_TIME_MAP_MAX_MEM_RECORDS )
      {
         _logicalTimeMap.erase( _logicalTimeMap.begin() ) ;
         ++ clearedLogicalNum ;
      }

      while ( _realTimeMap.size() > STP_TIME_MAP_MAX_MEM_RECORDS )
      {
         _realTimeMap.erase( _realTimeMap.begin() ) ;
         ++ clearedRealNum ;
      }

      PD_LOG( PDDEBUG, "Cleared records, logical time map: %u, "
              "real time map: %u", clearedLogicalNum, clearedRealNum ) ;

      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE_CLRRECS, rc ) ;

      return rc ;
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
                                                     STP_TIME_MAP_RECORD_LIST &recordList )
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
            iter = timeMap.lower_bound( afterTime ) ;
         }
         else
         {
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
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPTIMEMAPMEMSTORE__GETRECLISTFROMMAP, rc ) ;
      return rc ;

   error:
      goto done ;
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
   INT32 _stpTimeMapManager::initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_INITIALIZE ) ;

      _store = SDB_OSS_NEW stpTimeMapMemStore() ;
      PD_CHECK( NULL != _store, SDB_OOM, error, PDERROR,
                "Failed to allocate time map store" ) ;

      rc = _store->initialize() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize time map store, rc: %d",
                   rc ) ;

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
   INT32 _stpTimeMapManager::saveTimeMapping()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPTIMEMAPMGR_SAVETIMEMAPPING ) ;

      SDB_ASSERT( NULL != _store, "time map store is invalid" ) ;

      STPCB *stpCB = stpGetSTPCB() ;

      stpHPTime realTime ;
      stpLogicalTimeNS logicalTime ;
      stpTimeMapRecord record ;

      if ( !_needSaveTimeMapping )
      {
         _needSaveTimeMapping = TRUE ;
      }

      // get logical time
      rc = stpCB->getMetaData()->getLogicalTimeNS( logicalTime ) ;
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

}
