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

   Source File Name = rtnChangeStreamSource.cpp

   Descriptive Name = Change Stream Source

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnChangeStreamSource.hpp"
#include "dms.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dpsDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsLogRecordDef.hpp"
#include "ossErr.h"
#include "ossEvent.hpp"
#include "ossMemPool.hpp"
#include "ossRWMutex.hpp"
#include "ossTypes.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdEnv.hpp"
#include "pmdOptionsMgr.hpp"
#include "rtn.hpp"
#include "rtnChangeStreamCache.hpp"
#include "rtnChangeStreamDispatcher.hpp"
#include "rtnStreamRecordBuilder.hpp"
#include "rtnTrace.hpp"
#include "utilStreamToken.hpp"
#include "utilUniqueID.hpp"
#include <exception>

namespace engine
{

   // after timeout without any changes,
   // update the processed LSN without any changes
   static const UINT64 s_updateLSNTimeout = 30 * OSS_ONE_SEC ;
   // after continuous number of empty control records,
   // update the processed LSN without any changes
   static const UINT32 s_updateLSNCount = 30 ;
   // timeout to wait next changes
   static const INT64 s_maxWaitTimeout = 100 ;
   // each time to wait for next changes
   static const INT64 s_minWaitTimeStep = 10 ;

   /*
      _rtnChangeStreamSource implement
    */
   _rtnChangeStreamSource::_rtnChangeStreamSource( rtnStreamSourceProcessor &processor,
                                                   utilChangeStreamOptions &options )
   : _rtnStreamSourceBase( processor ),
     _watchInfo(),
     _options( options ),
     _logFilter( _watchInfo ),
     _bsonBuilder(),
     _changeBuilder( _bsonBuilder ),
     _controlBuilder( _bsonBuilder )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE_INIT, "_rtnChangeStreamSource::init" )
   INT32 _rtnChangeStreamSource::init( UINT64 resumableWindow,
                                       BOOLEAN enableControl )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE_INIT ) ;

      // initialize watch information
      rc = _initWatchInfo() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize watch information, rc: %d",
                   rc ) ;

      // initialize log cache
      rc = _logCache.init( _options.getCacheSizeB(), resumableWindow ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize log cache, rc: %d", rc ) ;

      // intialize change builder
      rc = _changeBuilder.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize change record builder, "
                   "rc: %d", rc ) ;

      // enable control builder
      if ( enableControl )
      {
         rc = _controlBuilder.init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize control record builer, "
                      "rc: %d", rc ) ;
      }
      _enableControl = enableControl ;

      // check token
      rc = _checkToken() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check token, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE_FINI, "_rtnChangeStreamSource::fini" )
   void _rtnChangeStreamSource::fini()
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE_FINI ) ;

      _status = RTN_CHANGE_STREAM_SOURCE_STOP ;
      _isWatching = FALSE ;

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMSOURCE_FINI ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE_GETRECORDS, "_rtnChangeStreamSource::getRecords" )
   INT32 _rtnChangeStreamSource::getRecords( INT64 timeout )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE_GETRECORDS ) ;

      switch ( _status )
      {
      case RTN_CHANGE_STREAM_SOURCE_FILE :
      {
         // get records from files
         rc = _getRecordsFromFile() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get records from file, rc: %d", rc ) ;
         break ;
      }
      case RTN_CHANGE_STREAM_SOURCE_CACHE :
      {
         // get records from cache
         rc = _getRecordsFromCache( timeout ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get records from cache, rc: %d", rc ) ;
         break ;
      }
      default :
      {
         PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                   "Failed to get records from wrong status [%s]",
                   getStatusName() ) ;
      }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE_GETRECORDS, rc ) ;
      return rc ;

   error:
      // error happened
      // - stop watching
      // - build error control record
      if ( _isWatching )
      {
         _isWatching = FALSE ;
      }
      if ( RTN_CHANGE_STREAM_SOURCE_STOP != _status )
      {
         INT32 tmpRC = SDB_OK ;
         BSONObj result ;
         DPS_LSN stopLSN = _stopWatchingLSN ;
         BOOLEAN isResumeAt = TRUE ;
         if ( DPS_INVALID_LSN_OFFSET == stopLSN.offset )
         {
            stopLSN = _lastProcessedLSN ;
            isResumeAt = FALSE ;
         }
         else if ( stopLSN.offset > _lastProcessedLSN.offset )
         {
            stopLSN = _lastProcessedLSN ;
         }

         tmpRC = _processError( rc, stopLSN, isResumeAt, FALSE ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to process error, rc: %d", tmpRC ) ;
         }

         rc = SDB_OK ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE_ATTACHANDSTARTWATCH, "_rtnChangeStreamSource::attachAndStartWatch" )
   void _rtnChangeStreamSource::attachAndStartWatch( rtnChangeStreamNotifierBase *notifier,
                                                     DPS_LSN_OFFSET startOffset )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE_ATTACHANDSTARTWATCH ) ;

      // attach
      _notifier = notifier ;

      // start watch
      _startWatchOffset = startOffset ;
      _watchRC = SDB_OK ;
      _isWatching = TRUE ;

      // update status
      if ( DPS_INVALID_LSN_OFFSET != _expectOffset &&
           _expectOffset < _startWatchOffset )
      {
         _status = RTN_CHANGE_STREAM_SOURCE_FILE ;
      }
      else
      {
         _status = RTN_CHANGE_STREAM_SOURCE_CACHE ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMSOURCE_ATTACHANDSTARTWATCH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE_PUSHLOG, "_rtnChangeStreamSource::pushLog" )
   INT32 _rtnChangeStreamSource::pushLog( const utilChangeStreamLogInfo &logInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE_PUSHLOG ) ;

      BOOLEAN isMatched = FALSE,
              needFilterRecord = FALSE,
              needCheckError = FALSE,
              needFilterType = FALSE ;

      // filter log record
      rc = _logFilter.filterInfo( logInfo, isMatched, needFilterRecord,
                                  needCheckError, needFilterType ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to filter log info, rc: %d", rc ) ;

      if ( isMatched )
      {
         // save log record
         rc = _logCache.pushLog( logInfo, needFilterRecord, needCheckError,
                                 needFilterType ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push log to cache, rc: %d", rc ) ;

         onReceived() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE_PUSHLOG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE_GETCURTOKEN, "_rtnChangeStreamSource::getCurrentToken" )
   void _rtnChangeStreamSource::getCurrentToken( utilStreamToken &token ) const
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE_GETCURTOKEN ) ;

      switch ( _processor.getMonitor()._lastProcessedType )
      {
      case UTIL_STREAM_INVALID_RECORD:
      {
         // no record generated yet
         break ;
      }
      case UTIL_STREAM_CONTROL_RECORD:
      {
         // get token from control builder
         token = _controlBuilder.getToken() ;
         break ;
      }
      case UTIL_STREAM_CHANGE_RECORD:
      {
         // get token from change builder
         token = _changeBuilder.getToken() ;
         break ;
      }
      default:
      {
         // may not sync, no neeed to report error
         PD_LOG( PDDEBUG, "invalid record type" ) ;
         break ;
      }
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMSOURCE_GETCURTOKEN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__INITWATCHINFO, "_rtnChangeStreamSource::_initWatchInfo" )
   INT32 _rtnChangeStreamSource::_initWatchInfo()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__INITWATCHINFO ) ;

      // watch collection spaces
      rc = _initCollectionSpaces( _options.getCollectionSpaces() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize watching collection "
                   "spaces, rc: %d", rc ) ;

      // watch collections
      rc = _initCollections( _options.getCollections() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize watching collections, "
                   "rc: %d", rc ) ;

      // watch types
      _watchInfo.watchLogTypes( _options.getChangeTypeMask() ) ;

      // complete watch information
      _watchInfo.complete() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__INITWATCHINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__INITCS, "_rtnChangeStreamSource::_initCollectionSpaces" )
   INT32 _rtnChangeStreamSource::_initCollectionSpaces( const utilWatchCSNameSet &collectionSpaces )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__INITCS ) ;

      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;

      for ( utilWatchCSNameSet::iterator iter = collectionSpaces.begin() ;
            iter != collectionSpaces.end() ;
            ++ iter )
      {
         const CHAR *csName = iter->_pString ;
         dmsStorageUnitID suID = DMS_INVALID_SUID ;
         UINT32 csLID = DMS_INVALID_LOGICCSID ;
         utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;

         // get collection space information
         rc = dmsCB->nameToCSInfo( csName, suID, csLID, csUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID for "
                      "collection space [%s], rc: %d", csName, rc ) ;

         // watch collection space
         rc = _watchInfo.watchCS( csName, csUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save watch info for "
                      "collection space [%s], rc: %d", csName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__INITCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__INITCL, "_rtnChangeStreamSource::_initCollections" )
   INT32 _rtnChangeStreamSource::_initCollections( const utilWatchCLNameSet &collections )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__INITCL ) ;

      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;

      for ( utilWatchCLNameSet::iterator iter = collections.begin() ;
            iter != collections.end() ;
            ++ iter )
      {
         const CHAR *clFullName = iter->_pString ;
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = DMS_INVALID_CS ;
         const CHAR* clShortName = NULL ;
         UINT16 mbID = DMS_INVALID_MBID ;
         UINT32 clLID = DMS_INVALID_LOGICCLID ;
         utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;

         // get collection space
         rc = rtnResolveCollectionNameAndLock( clFullName, dmsCB, &su,
                                               &clShortName, suID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name "
                      "[%s], rc: %d", clFullName, rc ) ;
         SDB_ASSERT( NULL != su, "storage unit should be valid" ) ;

         // get collection information
         rc = su->getCollectionInfo( clShortName, mbID, clLID, clUniqueID ) ;
         dmsCB->suUnlock( suID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID for "
                      "collection [%s], rc: %d", clFullName, rc ) ;

         // watch collection
         rc = _watchInfo.watchCL( clFullName, clUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save watch info for "
                      "collection space [%s], rc: %d", clFullName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__INITCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__CHECKTOKEN, "_rtnChangeStreamSource::_checkToken" )
   INT32 _rtnChangeStreamSource::_checkToken()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__CHECKTOKEN ) ;

      const utilChangeStreamToken &token = _options.getToken() ;

      PD_CHECK( !token.isNotResumable(),
                SDB_STREAM_NOT_RESUMABLE, error, PDERROR,
                "Failed to check token, it is not resumable" ) ;

      _expectOffset = DPS_INVALID_LSN_OFFSET ;

      if ( DPS_INVALID_LSN_OFFSET != token.getLSN() )
      {
         if ( token.isResumeAt() )
         {
            // resume at, check the LSN when we first fetch log record
            _expectOffset = token.getLSN() ;
         }
         else
         {
            // resume from, need check whether the given LSN is valid
            DPS_LSN lsn ;
            const dpsLogRecordHeader *header = NULL ;
            INT64 timeout = (INT64)( _options.getMaxWaitTimeMS() ) ;
            timeout = timeout < OSS_ONE_SEC ? OSS_ONE_SEC : timeout ;

            lsn.offset = token.getLSN() ;

            // fetch log
            rc = _logFetcher.fetchLog( lsn, timeout,_mb ) ;
            if ( SDB_DPS_LSN_OUTOFRANGE == rc )
            {
               // not synchronize yet, rewrite return code
               if ( !pmdIsPrimary() )
               {
                  rc = SDB_CLS_DATA_NOT_SYNC ;
               }
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to fetch log [version: %u, "
                         "offset: %llu], rc: %d", lsn.version, lsn.offset, rc ) ;
            header = (const dpsLogRecordHeader *)( _mb.startPtr() ) ;

            // check version
            if ( DPS_INVALID_LSN_VERSION != token.getCheckCode() )
            {
               PD_CHECK( header->_version == token.getCheckCode(),
                         SDB_STREAM_NOT_RESUMABLE, error, PDERROR,
                         "Failed to resume change stream from "
                         "[version: %u, offset: %llu]", token.getCheckCode(),
                         token.getLSN() ) ;
            }

            _expectOffset = header->_lsn + header->_length ;
            _lastProcessedLSN.set( header->_lsn, header->_version ) ;

            PD_LOG( PDDEBUG, "Checked record [version: %u, offset: %llu], next [offset: %llu]",
                    header->_version, header->_lsn, _expectOffset ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__CHECKTOKEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__GETRECORDSFROMFILE, "_rtnChangeStreamSource::_getRecordsFromFile" )
   INT32 _rtnChangeStreamSource::_getRecordsFromFile()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__GETRECORDSFROMFILE ) ;

      dpsMessageBlock mb ;
      INT64 timeout = (INT64)( _options.getMaxWaitTimeMS() ) ;
      UINT32 count = 0 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      while ( SDB_OK == _watchRC )
      {
         UINT32 mbOffset = 0 ;
         UINT64 onceFetchSize = 0 ;
         DPS_LSN minLSN ;

         PD_CHECK( !cb->isInterrupted(), cb->getInterruptRC(), error, PDWARNING,
                   "Failed to fetch log, session is interrupted" ) ;
         PD_CHECK( !PMD_IS_DB_DOWN(), SDB_DATABASE_DOWN, error, PDWARNING,
                   "Failed to fetch log, database is down" ) ;

         if ( _startWatchOffset <= _expectOffset )
         {
            _status = RTN_CHANGE_STREAM_SOURCE_CACHE ;
            PD_LOG( PDDEBUG, "Change to [%s] status",
                    _getStatusName( RTN_CHANGE_STREAM_SOURCE_CACHE ) ) ;
            break ;
         }

         // fetch a batch of records once
         onceFetchSize = _startWatchOffset - _expectOffset ;
         if ( onceFetchSize > _maxBatchSize )
         {
            onceFetchSize = _maxBatchSize ;
         }

         PD_LOG( PDDEBUG, "Search records from [offset: %llu]", _expectOffset ) ;

         minLSN.offset = _expectOffset ;
         rc = _logFetcher.fetchLogs( minLSN, onceFetchSize, timeout, mb ) ;
         if ( SDB_DPS_LSN_OUTOFRANGE == rc )
         {
            // not synchronize yet
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to fetch logs from [version: %u, "
                      "offset: %lld], rc: %d", rc ) ;

         // loop to process logs
         while ( mbOffset < mb.length() )
         {
            BOOLEAN isProcessed = FALSE ;

            rtnChangeStreamLogInfo logInfo( (const dpsLogRecordHeader *)( mb.readPtr() ) ) ;

            // move next position
            mbOffset += logInfo.getLength() ;
            mb.readPtr( mbOffset ) ;

            // process log records
            rc = _processLogRecord( logInfo, isProcessed ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to process log record, rc: %d", rc ) ;

            onSearched( logInfo.getLength() ) ;

            if ( isStopped() )
            {
               goto done ;
            }
            else if ( isProcessed )
            {
               ++ count ;
            }
         }

         // check if processor is full
         if ( isProcessorFull() )
         {
            break ;
         }
      }

      // check if error, or empty
      if ( SDB_OK != _watchRC )
      {
         rc = _processError( _watchRC, _stopWatchingLSN, TRUE, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process error, rc: %d", rc ) ;

         goto done ;
      }
      else if ( 0 == count && _enableControl )
      {
         rc = _processEmpty() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process empty, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__GETRECORDSFROMFILE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__GETRECORDSFROMCACHE, "_rtnChangeStreamSource::_getRecordsFromCache" )
   INT32 _rtnChangeStreamSource::_getRecordsFromCache( INT64 timeout )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__GETRECORDSFROMCACHE ) ;

      UINT32 count = 0 ;
      INT64 waitTime = 0 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      while ( TRUE )
      {
         rtnChangeStreamLogInfo logInfo ;
         INT32 watchRC = _watchRC ;
         BOOLEAN hasProcessed = FALSE ;

         PD_CHECK( !cb->isInterrupted(), cb->getInterruptRC(), error, PDWARNING,
                   "Failed to fetch log, session is interrupted" ) ;
         PD_CHECK( !PMD_IS_DB_DOWN(), SDB_DATABASE_DOWN, error, PDWARNING,
                   "Failed to fetch log, database is down" ) ;

         // try pop log from cache
         rc = _logCache.popLog( logInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pop log from cache, rc: %d", rc ) ;

         // process log record
         rc = _processLogRecord( logInfo, hasProcessed ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process log record, rc: %d", rc ) ;

         if ( isStopped() )
         {
            goto done ;
         }
         else if ( hasProcessed )
         {
            ++ count ;
         }
         else if ( SDB_OK != watchRC )
         {
            // process all cached logs, then process error
            rc = _processError( watchRC, _stopWatchingLSN, TRUE, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to process error, rc: %d", rc ) ;

            goto done ;
         }
         else if ( count > 0 )
         {
            // already has process some records, wait for a short time
            if ( waitTime < s_maxWaitTimeout && waitTime < timeout )
            {
               ossSleep( s_minWaitTimeStep ) ;
               waitTime += s_minWaitTimeStep ;
               continue ;
            }
            break ;
         }
         else
         {
            // process none records, wait for watch timeout
            if ( timeout < 0 )
            {
               // never timeout
               ossSleep( s_maxWaitTimeout ) ;
               waitTime += s_maxWaitTimeout ;
               continue ;
            }
            else if ( waitTime < timeout )
            {
               INT64 timeLeft = timeout - waitTime ;
               INT64 localTimeout =
                     timeLeft > s_maxWaitTimeout ? s_maxWaitTimeout : timeLeft ;
               ossSleep( localTimeout ) ;
               waitTime += localTimeout ;
               continue ;
            }
            break ;
         }

         // check if processor is full
         if ( isProcessorFull() )
         {
            break ;
         }
      }

      // check if empty
      if ( 0 == count && _enableControl )
      {
         BSONObj result ;

         rc = _processEmpty() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process empty, rc: %d", rc ) ;

         _onEmptyTimeout( waitTime ) ;
      }

   done:
      onWait( (UINT64)waitTime ) ;
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__GETRECORDSFROMCACHE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__CHECKERROR, "_rtnChangeStreamSource::_checkError" )
   INT32 _rtnChangeStreamSource::_checkError( const dpsLogRecord &record,
                                              INT32 &errorCode,
                                              ossPoolString &errorDesc )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__CHECKERROR ) ;

      DPS_LOG_TYPE logType = (DPS_LOG_TYPE)( record.head()._type ) ;

      errorCode = SDB_OK ;
      errorDesc.clear() ;

      switch ( logType )
      {
      case LOG_TYPE_CS_DELETE :
      {
         // if watching collection space, then report error for drop collection space
         const CHAR *csName = NULL ;
         dpsLogRecord::iterator iter = record.find( DPS_LOG_CSDEL_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( logType ) ) ;
         csName = iter.value() ;
         if ( _watchInfo.isWatchingCSExplicitly( csName ) ||
              _watchInfo.isWatchingCSImplicitly( csName ) )
         {
            errorCode = SDB_DMS_CS_NOTEXIST ;
            try
            {
               ossPoolStringStream ss ;
               ss << "Collection space [" << csName << "] has been dropped" ;
               errorDesc = ss.str() ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to build error description, "
                       "occurred exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
         break ;
      }
      case LOG_TYPE_CS_RENAME :
      {
         // if watching collection space, then report error for rename collection space
         const CHAR *csName = NULL ;
         dpsLogRecord::iterator iter = record.find( DPS_LOG_CSRENAME_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( logType ) ) ;
         csName = iter.value() ;
         if ( _watchInfo.isWatchingCSExplicitly( csName ) ||
              _watchInfo.isWatchingCSImplicitly( csName ) )
         {
            const CHAR *newCSName = NULL ;

            iter = record.find( DPS_LOG_CSRENAME_NEWNAME ) ;
            PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                      "filter log record, [%s] log record without "
                      "new collection space name", dpsGetOPName( logType ) ) ;
            newCSName = iter.value() ;

            errorCode = SDB_DMS_CS_NOTEXIST ;
            try
            {
               ossPoolStringStream ss ;
               ss << "Collection space [" << csName << "] has been renamed to ["
                  << newCSName << "]" ;
               errorDesc = ss.str() ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to build error description, "
                       "occurred exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
         break ;
      }
      case LOG_TYPE_CL_DELETE :
      {
         // if watching collection, then report error for drop collection
         const CHAR *clName = NULL ;


         dpsLogRecord::iterator iter = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without fullname",
                   dpsGetOPName( logType ) ) ;
         clName = iter.value() ;

         if ( _watchInfo.isWatchingCLExplicitly( clName ) )
         {
            errorCode = SDB_DMS_NOTEXIST ;
            try
            {
               ossPoolStringStream ss ;
               ss << "Collection [" << clName << "] has been dropped" ;
               errorDesc = ss.str() ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to build error description, "
                       "occurred exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }

         break ;
      }
      case LOG_TYPE_CL_RENAME :
      {
         // if watching collection, then report error for rename collection
         const CHAR *csName = NULL ;
         const CHAR *clName = NULL ;
         CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

         // get collection space name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_CLRENAME_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( record.head()._type ) ) ;
         csName = iter.value() ;

         // get collection name
         iter = record.find( DPS_LOG_CLRENAME_CLOLDNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection name", dpsGetOPName( record.head()._type ) ) ;
         clName = iter.value() ;

         ossSnprintf( fullName, sizeof( fullName ), "%s.%s", csName, clName ) ;

         if ( _watchInfo.isWatchingCLExplicitly( fullName ) )
         {
            const CHAR *clNewName = NULL ;
            iter = record.find( DPS_LOG_CLRENAME_CLNEWNAME ) ;
            PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                      "filter log record, [%s] log record without "
                      "collection name", dpsGetOPName( record.head()._type ) ) ;
            clNewName = iter.value() ;

            errorCode = SDB_DMS_NOTEXIST ;
            try
            {
               ossPoolStringStream ss ;
               ss << "Collection [" << csName << "." << clName << "] has been renamed to ["
                  << csName << "." << clNewName << "]" ;
               errorDesc = ss.str() ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to build error description, "
                       "occurred exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }

         break ;
      }
      default :
      {
         break ;
      }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__CHECKERROR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__BLDCTRLERR, "_rtnChangeStreamSource::_buildErrorControl" )
   INT32 _rtnChangeStreamSource::_buildErrorControl( const rtnChangeStreamLogInfo &logInfo,
                                                     INT32 errorCode,
                                                     const ossPoolString &errorDesc,
                                                     BOOLEAN isResumAt,
                                                     BOOLEAN isResumable,
                                                     BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__BLDCTRLERR ) ;

      rtnStreamControlRecord controlData( UTIL_STREAM_CONTROL_ERROR, logInfo.getLSN(),
                                          isResumAt, isResumable ) ;
      rc = controlData.setControlData( errorCode, errorDesc.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set control data, rc: %d", rc ) ;

      rc = _controlBuilder.buildRecord( controlData, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build error control record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__BLDCTRLERR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

      // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__BLDCTRLERR_RC, "_rtnChangeStreamSource::_buildErrorControl" )
   INT32 _rtnChangeStreamSource::_buildErrorControl( INT32 errorCode,
                                                     const DPS_LSN &errorLSN,
                                                     BOOLEAN isResumAt,
                                                     BOOLEAN isResumable,
                                                     BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__BLDCTRLERR_RC ) ;

      DPS_LSN lsn( errorLSN ) ;

      if ( DPS_INVALID_LSN_OFFSET == lsn.offset )
      {
         lsn = _lastProcessedLSN ;
      }

      rtnStreamControlRecord controlData( UTIL_STREAM_CONTROL_ERROR, lsn,
                                          isResumAt, isResumable ) ;
      ossPoolString errorDesc ;

      try
      {
         if ( SDB_DPS_LSN_MOVED == errorCode )
         {
            ossPoolStringStream ss ;
            ss << "DPS log has been moved to [version: " << lsn.version << ", "
               << "offset: " << lsn.offset << "]" ;
            errorDesc = ss.str() ;
         }
         else
         {
            errorDesc.assign( getErrDesp( errorCode ) ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build error message, "
                 "occurred exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = controlData.setControlData( errorCode, errorDesc.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set control data, rc: %d", rc ) ;

      rc = _controlBuilder.buildRecord( controlData, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build error control record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__BLDCTRLERR_RC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__BLDEMPTERR, "_rtnChangeStreamSource::_buildEmptyControl" )
   INT32 _rtnChangeStreamSource::_buildEmptyControl( BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__BLDEMPTERR ) ;

      static const CHAR * _emptyString = "No changes" ;

      rtnStreamControlRecord recordData( UTIL_STREAM_CONTROL_EMPTY, _lastProcessedLSN ) ;

      rc = recordData.setControlData( SDB_OK, _emptyString ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set control data, rc: %d", rc ) ;

      rc = _controlBuilder.buildRecord( recordData, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build empty control record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__BLDEMPTERR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__PROCESSLOGREC, "_rtnChangeStreamSource::_processLogRecord" )
   INT32 _rtnChangeStreamSource::_processLogRecord( const rtnChangeStreamLogInfo &logInfo,
                                                    BOOLEAN &hasProcessed )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__PROCESSLOGREC ) ;

      BSONObj result ;

      rtnStreamChangeRecord recordData ;

      hasProcessed = FALSE ;

      if ( DPS_INVALID_LSN_OFFSET == logInfo.getOffset() )
      {
         // log information is invalid
         goto done ;
      }

      // load record
      if ( logInfo.isRecordCached() )
      {
         // parse log
         rc = recordData.loadRecord( (const CHAR *)( logInfo.getCachedRecord() ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load record data, rc: %d", rc ) ;

         onHitCache() ;
      }
      else if ( logInfo.isRecordFetched() )
      {
         // parse log
         rc = recordData.loadRecord( (const CHAR *)( logInfo.getFetchedRecord() ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load record data, rc: %d", rc ) ;

         onMissCache() ;
      }
      else if ( LOG_TYPE_DUMMY == logInfo.getLogType() )
      {
         const dpsLogRecordHeader *header = NULL ;
         // fetch log from file
         rc = _logFetcher.fetchDummyLog( logInfo.getLSN(),
                                         logInfo.getLength(),
                                         header ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to fetcher dummy record, rc: %d", rc ) ;

         rc = recordData.loadRecord( (const CHAR *)header ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load record data, rc: %d", rc ) ;

         // dummy has no content, calculate as cache
         onHitCache() ;
      }
      else
      {
         // wait for log cache filled
         while ( logInfo.isLogRecordCacheReady() && !logInfo.isLogRecordCacheFilled() )
         {
            _notifier->waitForWrite() ;
         }

         if ( logInfo.isRecordCached() )
         {
            // if log is cached, fetch from cache
            rc = recordData.loadRecord( (const CHAR *)( logInfo.getCachedRecord() ) ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to load record data, rc: %d", rc ) ;

            onHitCache() ;
         }
         else
         {
            // fetch log from file
            rc = _logFetcher.fetchLog( logInfo.getLSN(), 0, _mb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to fetcher record, rc: %d", rc ) ;

            rc = recordData.loadRecord( _mb.startPtr() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to load record data, rc: %d", rc ) ;

            onMissCache() ;
         }
      }

      // check record if needed
      if ( logInfo.needCheckRecord() )
      {
         BOOLEAN isMatched = FALSE ;

         rc = _logFilter.filterRecord( recordData.getRecord(), isMatched ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to filter record, rc: %d", rc ) ;

         if ( !isMatched )
         {
            _lastProcessedLSN = logInfo.getLSN() ;
            _expectOffset += logInfo.getNextOffset() ;
            goto done ;
         }
      }

      // check if error shoule be reported
      if ( logInfo.needCheckError() &&
           _watchInfo.isErrorLogType( logInfo.getLogType() ) )
      {
         INT32 errorCode = SDB_OK ;
         ossPoolString errorDesc ;

         rc = _checkError( recordData.getRecord(), errorCode, errorDesc ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to filter error, rc: %d", rc ) ;

         if ( SDB_OK != errorCode )
         {
            rc = _processErrorLogRecord( logInfo, errorCode, errorDesc ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to process error log record, "
                         "rc: %d", rc ) ;
            goto done ;
         }
      }

      // check log type
      if ( logInfo.needCheckType() &&
           !( _watchInfo.isWatchingLogType( logInfo.getLogType() ) ) )
      {
         _lastProcessedLSN = logInfo.getLSN() ;
         _expectOffset += logInfo.getNextOffset() ;
         goto done ;
      }

      rc = _processTrans( recordData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process transactions, rc: %d", rc ) ;

      // build record
      rc = _changeBuilder.buildRecord( recordData, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build change record for "
                     "log [version: %u, offset: %llu], rc: %d",
                     logInfo.getVersion(), logInfo.getOffset(), rc ) ;

      rc = _processor.processChangeRecord( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process result, rc: %d", rc ) ;

      _lastProcessedLSN = logInfo.getLSN() ;
      _expectOffset += logInfo.getNextOffset() ;
      hasProcessed = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__PROCESSLOGREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__PROCESSERRORLOGREC, "_rtnChangeStreamSource::_processErrorLogRecord" )
   INT32 _rtnChangeStreamSource::_processErrorLogRecord( const rtnChangeStreamLogInfo &logInfo,
                                                         INT32 errorCode,
                                                         const ossPoolString &errorDesc )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__PROCESSERRORLOGREC ) ;

      BSONObj result ;

      _isWatching = FALSE ;

      rc = _buildErrorControl( logInfo, errorCode, errorDesc, FALSE, FALSE, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build error control record, rc: %d", rc ) ;

      rc = _processor.processControlRecord( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push error control record, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Stopped watching by error [%d] %s on log record "
              "[version: %u, offset: %llu]", errorCode, errorDesc.c_str(),
              logInfo.getVersion(), logInfo.getOffset() ) ;

      _status = RTN_CHANGE_STREAM_SOURCE_STOP ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__PROCESSERRORLOGREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__PROCESSERROR, "_rtnChangeStreamSource::_processError" )
   INT32 _rtnChangeStreamSource::_processError( INT32 errorCode,
                                                const DPS_LSN &errorLSN,
                                                BOOLEAN isResumAt,
                                                BOOLEAN isResumable )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__PROCESSERROR ) ;

      BSONObj result ;

      _isWatching = FALSE ;

      rc = _buildErrorControl( errorCode, _stopWatchingLSN, isResumAt, isResumable, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build error control record, rc: %d", rc ) ;

      rc = _processor.processControlRecord( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process control record, rc: %d", rc ) ;

      _status = RTN_CHANGE_STREAM_SOURCE_STOP ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__PROCESSERROR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__PROCESSEMPTY, "_rtnChangeStreamSource::_processEmpty" )
   INT32 _rtnChangeStreamSource::_processEmpty()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__PROCESSEMPTY ) ;

      BSONObj result ;

      rc = _buildEmptyControl( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build empty control record, rc: %d", rc ) ;

      rc = _processor.processControlRecord( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process control record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__PROCESSEMPTY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__ONEMPTYTIMEOUT, "_rtnChangeStreamSource::_onEmptyTimeout" )
   void _rtnChangeStreamSource::_onEmptyTimeout( INT64 waitTime )
   {
      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__ONEMPTYTIMEOUT ) ;

      // check if we need to push processed LSN
      _emptyTimeout += waitTime ;
      _emptyCount ++ ;
      if ( _emptyTimeout > s_updateLSNTimeout ||
           _emptyCount > s_updateLSNCount )
      {
         if ( _logCache.isEmpty() )
         {
            SDB_ASSERT( NULL != _notifier, "manager is invalid" ) ;
            if ( NULL != _notifier )
            {
               DPS_LSN_OFFSET lastOffset = _notifier->getLastDispatchedOffset() ;
               if ( _logCache.isEmpty() )
               {
                  _lastProcessedLSN.offset = lastOffset ;
               }
            }
         }

         _emptyTimeout = 0 ;
         _emptyCount = 0 ;
      }

      PD_TRACE_EXIT( SDB__RTNCHANGESTREAMSOURCE__ONEMPTYTIMEOUT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHANGESTREAMSOURCE__PROCESSTRANS, "_rtnChangeStreamSource::_processTrans" )
   INT32 _rtnChangeStreamSource::_processTrans( const rtnStreamChangeRecord &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHANGESTREAMSOURCE__PROCESSTRANS ) ;

      if ( OSS_BIT_TEST( _options.getChangeTypeMask(), UTIL_CHANGE_TYPE_TRANS ) )
      {
         DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
         const dpsLogRecord &record = recordData.getRecord() ;

         rc = dpsGetTransIDFromRecord( record, FALSE, transID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get transaction ID, "
                      "rc: %d", rc ) ;
         if ( DPS_INVALID_TRANS_ID != transID )
         {
            if ( DPS_TRANS_IS_ROLLBACK( transID ) ||
               LOG_TYPE_TS_ROLLBACK == record.head()._type )
            {
               rc = _logFilter.removeTransID( transID ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to remove transaction ID, "
                           "rc: %d", rc ) ;
            }
            else if ( LOG_TYPE_TS_COMMIT == record.head()._type )
            {
               dpsLogRecord::iterator iter = record.find( DPS_LOG_TSCOMMIT_ATTR ) ;
               if ( !iter.valid() ||
                  DPS_TS_COMMIT_ATTR_PRE != *(UINT8 *)( iter.value() ) )
               {
                  rc = _logFilter.removeTransID( transID ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to remove transaction ID, "
                              "rc: %d", rc ) ;
               }
            }
            else
            {
               rc = _logFilter.addTransID( transID ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add transaction ID, "
                           "rc: %d", rc ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCHANGESTREAMSOURCE__PROCESSTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
