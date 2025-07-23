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

   Source File Name = rtnChangeStreamSource.hpp

   Descriptive Name = Change Stream Source

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CHANGE_STREAM_SOURCE_HPP__
#define RTN_CHANGE_STREAM_SOURCE_HPP__

#include "dpsDef.hpp"
#include "dpsLogDef.hpp"
#include "dpsMessageBlock.hpp"
#include "ossTypes.h"
#include "rtnLogFetcher.hpp"
#include "rtnLogRecordFilter.hpp"
#include "rtnStreamRecordBuilder.hpp"
#include "rtnStreamSource.hpp"
#include "rtnChangeStreamInterface.hpp"
#include "rtnChangeStreamCache.hpp"
#include "utilChangeStreamOptions.hpp"
#include "utilPooledAutoPtr.hpp"
#include "monStreamMonitorManager.hpp"
#include "utilStreamToken.hpp"

namespace engine
{

   /*
      _rtnChangeStreamSourceStatus define
    */
   typedef enum _rtnChangeStreamSourceStatus
   {
      // initial status
      RTN_CHANGE_STREAM_SOURCE_INIT = 0,
      // catching up with logs
      RTN_CHANGE_STREAM_SOURCE_FILE,
      // keeping up to date with logs
      RTN_CHANGE_STREAM_SOURCE_CACHE,
      // watching is stopped
      RTN_CHANGE_STREAM_SOURCE_STOP,
   } rtnChangeStreamSourceStatus ;

   /*
      _rtnChangeStreamSource define
    */
   // source of change stream
   class _rtnChangeStreamSource : public _rtnChangeStreamWatcherBase,
                                  public _rtnStreamSourceBase
   {
   public:
      _rtnChangeStreamSource( rtnStreamSourceProcessor &processor,
                              utilChangeStreamOptions &options ) ;
      virtual ~_rtnChangeStreamSource() = default ;

      // initialize source
      INT32 init( UINT64 resumableWindow, BOOLEAN enableControl ) ;
      // finish source
      void fini() ;

      // override functions of _rtnStreamSourceBase
      virtual INT32 getRecords( INT64 timeout ) ;

      // override functions of _rtnChangeStreamWatcherBase
      virtual BOOLEAN isWatching() const
      {
         return _isWatching ;
      }

      virtual const utilChangeStreamWatchInfo &getWatchInfo() const
      {
         return _watchInfo ;
      }

      virtual void attachAndStartWatch( rtnChangeStreamNotifierBase *notifier,
                                        DPS_LSN_OFFSET startOffset ) ;

      virtual void detach()
      {
         _notifier = NULL ;
      }

      virtual BOOLEAN isAttached() const
      {
         return NULL != _notifier ;
      }

      virtual void stopWatch( INT32 returnCode, const DPS_LSN &stopLSN )
      {
         _watchRC = returnCode ;
         _stopWatchingLSN = stopLSN ;
         _isWatching = FALSE ;
      }

      virtual INT32 pushLog( const utilChangeStreamLogInfo &logInfo ) ;

      virtual DPS_LSN_OFFSET getStartWatchOffset() const
      {
         return _startWatchOffset ;
      }

      // override functions of _monStreamTokenMonitor
      virtual void getCurrentToken( utilStreamToken &token ) const ;

      virtual monStreamSourceUsage getUsage() const
      {
         return _logCache.getUsage() ;
      }

      // check if watching is stopped
      BOOLEAN isStopped() const
      {
         return RTN_CHANGE_STREAM_SOURCE_STOP == _status ;
      }

      // get source status
      rtnChangeStreamSourceStatus getStatus() const
      {
         return _status ;
      }

      // get name of status
      const CHAR *getStatusName() const
      {
         return _getStatusName( _status ) ;
      }

      // get return code of watch
      INT32 getWatchRC() const
      {
         return _watchRC ;
      }

   protected:
      // initialize watch information
      INT32 _initWatchInfo() ;
      // initialize watching collection spaces
      INT32 _initCollectionSpaces( const utilWatchCSNameSet &collectionSpaces ) ;
      // initialize watching collections
      INT32 _initCollections( const utilWatchCLNameSet &collections ) ;
      // check if token is valid
      INT32 _checkToken() ;

      // get records from cache
      INT32 _getRecordsFromCache( INT64 timeout ) ;

      // get records from file
      INT32 _getRecordsFromFile() ;

      // check error
      INT32 _checkError( const dpsLogRecord &record,
                         INT32 &errorCode,
                         ossPoolString &errorDesc ) ;
      // build error control record by given log record
      INT32 _buildErrorControl( const rtnChangeStreamLogInfo &logInfo,
                                INT32 errorCode,
                                const ossPoolString &errorDesc,
                                BOOLEAN isResumAt,
                                BOOLEAN isResumable,
                                bson::BSONObj &result ) ;
      // build error control record by error code directly
      INT32 _buildErrorControl( INT32 errorCode,
                                const DPS_LSN &errorLSN,
                                BOOLEAN isResumAt,
                                BOOLEAN isResumable,
                                bson::BSONObj &result ) ;
      // build empty control record
      INT32 _buildEmptyControl( bson::BSONObj &result ) ;

      // process log record
      INT32 _processLogRecord( const rtnChangeStreamLogInfo &logInfo,
                               BOOLEAN &hasProcessed ) ;
      // process error log record
      INT32 _processErrorLogRecord( const rtnChangeStreamLogInfo &logInfo,
                                    INT32 errorCode,
                                    const ossPoolString &errorDesc ) ;
      // process error
      INT32 _processError( INT32 errorCode,
                           const DPS_LSN &errorLSN,
                           BOOLEAN isResumAt,
                           BOOLEAN isResumable ) ;
      // process empty
      INT32 _processEmpty() ;

      // on emtpy timeout event
      void _onEmptyTimeout( INT64 waitTime ) ;

      // process transactions
      INT32 _processTrans( const rtnStreamChangeRecord &recordData ) ;

      static const CHAR *_getStatusName( rtnChangeStreamSourceStatus status )
      {
         const CHAR *name = "unknown" ;

         switch ( status )
         {
         case RTN_CHANGE_STREAM_SOURCE_INIT :
         {
            name = "init" ;
            break ;
         }
         case RTN_CHANGE_STREAM_SOURCE_FILE :
         {
            name = "file" ;
            break ;
         }
         case RTN_CHANGE_STREAM_SOURCE_CACHE :
         {
            name = "cache" ;
            break ;
         }
         case RTN_CHANGE_STREAM_SOURCE_STOP :
         {
            name = "stopped" ;
            break ;
         }
         default :
         {
            name = "unknown" ;
            break ;
         }
         }

         return name ;
      }

   protected:
      // pointer to notifier
      rtnChangeStreamNotifierBase *_notifier = NULL ;
      // status of source
      rtnChangeStreamSourceStatus _status = RTN_CHANGE_STREAM_SOURCE_INIT ;
      // watch information
      utilChangeStreamWatchInfo _watchInfo ;
      // indicates if watching
      BOOLEAN _isWatching = FALSE ;
      // return code of watch
      INT32 _watchRC = SDB_OK ;
      // stop watching LSN
      DPS_LSN _stopWatchingLSN ;
      // change stream options
      utilChangeStreamOptions &_options ;
      // log filter
      rtnLogRecordFilter _logFilter ;
      // log cache
      rtnChangeStreamCache _logCache ;
      // log fetcher
      rtnLogFetcher _logFetcher ;
      // bufferred log message block
      dpsMessageBlock _mb ;
      // LSN to start watch
      DPS_LSN_OFFSET _startWatchOffset = DPS_INVALID_LSN_OFFSET ;
      // expect LSN ( next LSN to process )
      DPS_LSN_OFFSET _expectOffset = DPS_INVALID_LSN_OFFSET ;
      // last processed LSN
      DPS_LSN _lastProcessedLSN ;
      // bufferred builder of BSON object
      bson::BSONObjBuilder _bsonBuilder ;
      // change record builder
      rtnStreamChangeRecordBuilder _changeBuilder ;
      // enable control records
      BOOLEAN _enableControl = FALSE ;
      // empty timeout ( since last change record was generated )
      UINT64 _emptyTimeout = 0 ;
      // empty count ( since last change record was generated )
      UINT32 _emptyCount = 0 ;
      // control record builder
      rtnStreamControlRecordBuilder _controlBuilder ;
   } ;

   typedef class _rtnChangeStreamSource rtnChangeStreamSource ;

}

#endif // RTN_CHANGE_STREAM_SOURCE_HPP__
