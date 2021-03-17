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

   Source File Name = dmsIdxTaskStatus.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who      Description
   ====== ========= =======  ==============================================
          2019/8/23 Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIdxTaskStatus.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"
#include "msgDef.hpp"

namespace engine
{
   const CHAR* dmsTaskTypeStr( DMS_TASK_TYPE taskType )
   {
      switch( taskType )
      {
         case DMS_TASK_CREATE_IDX :
            return VALUE_NAME_CREATEIDX ;
         case DMS_TASK_DROP_IDX :
            return VALUE_NAME_DROPIDX ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   const CHAR* dmsTaskStatusStr( DMS_TASK_STATUS taskStatus )
   {
      switch( taskStatus )
      {
         case DMS_TASK_STATUS_RUN :
            return VALUE_NAME_RUNNING ;
         case DMS_TASK_STATUS_PAUSE :
            return VALUE_NAME_PAUSE ;
         case DMS_TASK_STATUS_CANCELED :
            return VALUE_NAME_CANCELED ;
         case DMS_TASK_STATUS_META :
            return VALUE_NAME_CHGMETA ;
         case DMS_TASK_STATUS_CLEANUP :
            return VALUE_NAME_CLEANUP ;
         case DMS_TASK_STATUS_ROLLBACK :
            return VALUE_NAME_ROLLBACK ;
         case DMS_TASK_STATUS_FINISH :
            return VALUE_NAME_FINISH ;
         case DMS_TASK_STATUS_END :
            return VALUE_NAME_END ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   #define DMS_BYTE_TO_MBYTE     ( 1048576.0 )
   #define DMS_TASK_PROGRESS_100 ( 100 ) // 100%

   _dmsIdxTaskStatus::_dmsIdxTaskStatus( DMS_TASK_TYPE taskType,
                                         UINT64 taskID,
                                         UINT32 locationID,
                                         UINT64 mainTaskID )
   :_taskType( taskType ),
    _taskID( taskID ),
    _locationID( locationID ),
    _mainTaskID( mainTaskID ),
    _cataTaskStatus( DMS_TASK_STATUS_RUN ),
    _dataTaskStatus( DMS_TASK_STATUS_RUN ),
    _sortBufSize( SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ),
    _isStandaloneIdx( FALSE ),
    _isGlobalIdx( FALSE ),
    _totalPageNum( 0 ),
    _processedPageNum( 0 ),
    _pageSizeSquareRoot( 0 ),
    _pcsPageNumLastTime( 0 ),
    _retryCnt( 0 ),
    _resultCode( SDB_OK ),
    _progress( 0 ),
    _speed( 0.0 ),
    _timeSpent( 0.0 ),
    _timeLeft( 0.0 ),
    _isInitialized( FALSE )
   {
      ossMemset( _clFullName, 0, DMS_COLLECTION_FULL_NAME_SZ + 1 ) ;
      ossMemset( _indexName,  0, IXM_INDEX_NAME_SIZE + 1 ) ;
      ossGetCurrentTime( _beginTimestamp ) ;
      _calculateTimestamp = _beginTimestamp ;
   }

   _dmsIdxTaskStatus& _dmsIdxTaskStatus::operator=( const _dmsIdxTaskStatus &rhs )
   {
      _taskType = rhs._taskType ;
      _taskID = rhs._taskID ;
      _locationID = rhs._locationID ;
      _mainTaskID = rhs._mainTaskID ;
      _cataTaskStatus = rhs._cataTaskStatus ;
      _dataTaskStatus = rhs._dataTaskStatus ;
      ossStrncpy( _clFullName, rhs._clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _indexDef = rhs._indexDef.getOwned() ;
      ossStrncpy( _indexName, rhs._indexName, IXM_INDEX_NAME_SIZE ) ;
      _sortBufSize = rhs._sortBufSize ;
      _isStandaloneIdx = rhs._isStandaloneIdx ;
      _isGlobalIdx = rhs._isGlobalIdx ;
      _totalPageNum = rhs._totalPageNum ;
      _processedPageNum = rhs._processedPageNum ;
      _pageSizeSquareRoot = rhs._pageSizeSquareRoot ;
      _pcsPageNumLastTime = rhs._pcsPageNumLastTime ;
      _calculateTimestamp = rhs._calculateTimestamp ;
      _beginTimestamp = rhs._beginTimestamp ;
      _endTimestamp = rhs._endTimestamp ;
      _retryCnt = rhs._retryCnt ;
      _resultCode = rhs._resultCode ;
      _progress = rhs._progress ;
      _speed = rhs._speed ;
      _timeSpent = rhs._timeSpent ;
      _timeLeft = rhs._timeLeft ;
      _isInitialized = rhs._isInitialized ;
      return *this ;
   }

   void _dmsIdxTaskStatus::setStatus( DMS_TASK_STATUS status )
   {
      _dataTaskStatus = status ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKSTAT_INIT, "_dmsIdxTaskStatus::init" )
   INT32 _dmsIdxTaskStatus::init( const CHAR* collectionName,
                                  const BSONObj& index,
                                  INT32 sortBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSIDXTASKSTAT_INIT ) ;

      if ( _isInitialized )
      {
         goto done ; ;
      }

      // colection
      ossStrncpy( _clFullName, collectionName, DMS_COLLECTION_FULL_NAME_SZ ) ;

      // index definition
      try
      {
         if ( index.hasField( IXM_NAME_FIELD ) )
         {
            _indexDef = index.getOwned() ;

            // create index, idxDef: { "name": "aIdx", "key": { "a": 1 }, ... }
            BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_NAME ) ;
            PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in index def[%s]",
                       IXM_FIELD_NAME_NAME, _indexDef.toString().c_str() ) ;
            ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;

            _isStandaloneIdx = _indexDef[ IXM_STANDALONE_FIELD ].trueValue() ;
            _isGlobalIdx = _indexDef[ IXM_GLOBAL_FIELD ].trueValue() ;
         }
         else
         {
            // drop index: { "": "aIdx" }
            BSONElement ele = index.firstElement() ;
            PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                       "Field[\"\"] invalid in index obj[%s]",
                       index.toString().c_str() ) ;
            ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      // sort buffer size
      if ( sortBufSize >= 0 )
      {
         _sortBufSize = sortBufSize ;
      }

      _isInitialized = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXTASKSTAT_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKSTAT_TOBSON, "_dmsIdxTaskStatus::toBSON" )
   BSONObj _dmsIdxTaskStatus::toBSON( UINT32 mask ) const
   {
      PD_TRACE_ENTRY( SDB_DMSIDXTASKSTAT_TOBSON ) ;

      BSONObjBuilder builder ;

      if ( DMS_TASK_MASK_GROUPNAME & mask )
      {
         builder.append( FIELD_NAME_GROUPNAME, pmdGetKRCB()->getGroupName() ) ;
      }
      if ( DMS_TASK_MASK_MAINTASKID & mask )
      {
         if ( DMS_INVALID_TASKID != _mainTaskID )
         {
            builder.append( FIELD_NAME_MAIN_TASKID, (INT64)_mainTaskID ) ;
         }
      }
      if ( DMS_TASK_MASK_TASKID & mask )
      {
         builder.append( FIELD_NAME_TASKID, (INT64)_taskID ) ;
      }
      if ( DMS_TASK_MASK_STATUS & mask )
      {
         builder.append( FIELD_NAME_STATUS, _dataTaskStatus ) ;
         builder.append( FIELD_NAME_STATUSDESC,
                         dmsTaskStatusStr( _dataTaskStatus ) ) ;
      }
      if ( DMS_TASK_MASK_TASKTYPE & mask )
      {
         builder.append( FIELD_NAME_TASKTYPE, _taskType ) ;
         builder.append( FIELD_NAME_TASKTYPEDESC,
                         dmsTaskTypeStr( _taskType ) ) ;
      }
      if ( DMS_TASK_MASK_CLNAME & mask )
      {
         builder.append( FIELD_NAME_NAME, _clFullName ) ;
      }
      if ( DMS_TASK_MASK_IDXNAME & mask )
      {
         builder.append( FIELD_NAME_INDEXNAME, _indexName ) ;
      }
      if ( DMS_TASK_MASK_IDXDEF & mask )
      {
         if ( !_indexDef.isEmpty() )
         {
            builder.append( IXM_FIELD_NAME_INDEX_DEF, _indexDef ) ;
         }
      }
      if ( DMS_TASK_MASK_SORTBUFSZ & mask )
      {
         if ( DMS_TASK_CREATE_IDX == _taskType )
         {
            builder.append( IXM_FIELD_NAME_SORT_BUFFER_SIZE, _sortBufSize ) ;
         }
      }
      if ( DMS_TASK_MASK_RESULTCODE & mask )
      {
         builder.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
         builder.append( FIELD_NAME_RESULTCODEDESC,
                         DMS_TASK_STATUS_FINISH == _dataTaskStatus ?
                         getErrDesp( _resultCode ) : "" ) ;
      }
      if ( DMS_TASK_MASK_RESULTINFO & mask )
      {
         builder.append( FIELD_NAME_RESULTINFO, _resultInfo ) ;
      }
      if ( DMS_TASK_MASK_OPINFO & mask )
      {
         builder.append( FIELD_NAME_OPINFO, _opInfo.c_str() ) ;
      }
      if ( DMS_TASK_MASK_RETRYCNT & mask )
      {
         builder.append( FIELD_NAME_RETRY_COUNT, _retryCnt ) ;
      }
      if ( DMS_TASK_MASK_PROGRESS & mask )
      {
         builder.append( FIELD_NAME_PROGRESS, _progress ) ;
      }
      if ( DMS_TASK_MASK_SPEED & mask )
      {
         builder.append( FIELD_NAME_SPEED, _speed ) ;
      }
      if ( DMS_TASK_MASK_TIMESPENT & mask )
      {
         builder.append( FIELD_NAME_TIMESPENT, _timeSpent ) ;
      }
      if ( DMS_TASK_MASK_TIMELEFT & mask )
      {
         builder.append( FIELD_NAME_TIMELEFT, _timeLeft ) ;
      }

      if ( DMS_TASK_MASK_TOTALSZ & mask )
      {
         INT64 totalSize = _totalPageNum << _pageSizeSquareRoot ;
         builder.append( FIELD_NAME_TOTAL_SIZE, totalSize ) ;
      }
      if ( DMS_TASK_MASK_PROCESSSZ & mask )
      {
         INT64 pcsedSize = _processedPageNum << _pageSizeSquareRoot ;
         builder.append( FIELD_NAME_PROCESSED_SIZE, pcsedSize ) ;
      }

      PD_TRACE_EXIT( SDB_DMSIDXTASKSTAT_TOBSON ) ;
      return builder.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKSTAT_CAL, "_dmsIdxTaskStatus::calculate" )
   void _dmsIdxTaskStatus::calculate()
   {
      PD_TRACE_ENTRY( SDB_DMSIDXTASKSTAT_CAL ) ;

      BOOLEAN isFirstCalculate = _pcsPageNumLastTime == 0 ? TRUE : FALSE ;
      ossTimestamp currentTimestamp ;
      ossGetCurrentTime( currentTimestamp ) ;

      UINT64 curTime = currentTimestamp.time * 1000000 +
                         currentTimestamp.microtm  ;
      UINT64 calTime = _calculateTimestamp.time * 1000000 +
                       _calculateTimestamp.microtm  ;
      FLOAT64 calTimeInterval = ( curTime - calTime ) / 1000000.0 ;

      // If it is first time, we should calculate. If task has finished, we
      // only calculate ONE time. If calculate frequently, we just skip.
      if ( DMS_TASK_STATUS_FINISH == status() )
      {
         if ( DMS_TASK_PROGRESS_100 == _progress )
         {
            goto done ;
         }
      }
      else if ( !isFirstCalculate )
      {
         if ( currentTimestamp.time - _calculateTimestamp.time < 5 )
         {
            goto done ;
         }
      }

      if ( DMS_TASK_STATUS_FINISH == status() )
      {
         _progress = DMS_TASK_PROGRESS_100 ;
         _timeLeft = 0.0 ;

         if ( calTimeInterval != 0.0 )
         {
            UINT32 pcsNumThisTime = _processedPageNum - _pcsPageNumLastTime ;
            UINT64 pcsSizeThisTime = (UINT64)pcsNumThisTime <<
                                     _pageSizeSquareRoot ;
            _speed = pcsSizeThisTime / DMS_BYTE_TO_MBYTE / calTimeInterval ;
         }
         else
         {
            _speed = 0.0 ;
         }

         UINT64 beginTime = _beginTimestamp.time * 1000000 +
                            _beginTimestamp.microtm  ;
         UINT64 endTime = _endTimestamp.time * 1000000 +
                          _endTimestamp.microtm  ;
         _timeSpent = ( endTime - beginTime ) / 1000000.0 ;
      }
      else
      {
         // progress
         if ( _totalPageNum != 0 )
         {
            FLOAT32 percentage = (FLOAT32)_processedPageNum / _totalPageNum ;
            _progress = percentage * 100 ;
         }
         else
         {
            _progress = 0 ;
         }
         if ( DMS_TASK_PROGRESS_100 == _progress )
         {
            // the task is not finished yet.
            _progress = 99 ;
         }

         // time spent
         UINT64 beginMs = _beginTimestamp.time * 1000000 +
                            _beginTimestamp.microtm  ;
         UINT64 currentMs = currentTimestamp.time * 1000000 +
                              currentTimestamp.microtm  ;
         _timeSpent = ( currentMs - beginMs ) / 1000000.0 ;

         // speed
         if ( calTimeInterval != 0.0 )
         {
            UINT32 pcsNumThisTime = _processedPageNum - _pcsPageNumLastTime ;
            UINT64 pcsSizeThisTime = (UINT64)pcsNumThisTime << _pageSizeSquareRoot ;
            _speed = pcsSizeThisTime / DMS_BYTE_TO_MBYTE / calTimeInterval ;
         }
         else
         {
            _speed = 0.0 ;
         }

         // time left
         if ( _speed != 0.0 )
         {
            _timeLeft = ( _totalPageNum - _processedPageNum ) / _speed ;
         }
         else if ( _progress != 0 )
         {
            // timeSpent   timeLeft
            // --------- = --------------
            // progress    100 - progress
            _timeLeft = _timeSpent / _progress *
                        ( DMS_TASK_PROGRESS_100 - _progress ) ;
         }
         else
         {
            _timeLeft = 0.0 ;
         }
      }

      _calculateTimestamp = currentTimestamp ;
      _pcsPageNumLastTime = _processedPageNum ;

   done :
      PD_TRACE_EXIT( SDB_DMSIDXTASKSTAT_CAL ) ;
      return ;
   }

   void _dmsIdxTaskStatus::setTotalPageNum( UINT32 pages )
   {
      _totalPageNum = pages ;
   }

   void _dmsIdxTaskStatus::incProcessedPageNum( INT32 delta )
   {
      _processedPageNum += delta ;
   }

   void _dmsIdxTaskStatus::resetProcessedPageNum()
   {
      _processedPageNum = 0 ;
   }

   void _dmsIdxTaskStatus::setPageSizeSquareRoot( UINT32 num )
   {
      _pageSizeSquareRoot = num ;
   }

   void _dmsIdxTaskStatus::incRetryCnt()
   {
      _retryCnt++ ;
   }

   void _dmsIdxTaskStatus::setStatus2Finish( INT32 resultCode,
                                             const BSONObj &resultInfo )
   {
      if ( DMS_TASK_STATUS_CANCELED == _dataTaskStatus )
      {
         _resultCode = SDB_TASK_HAS_CANCELED ;
      }
      else if ( DMS_TASK_STATUS_ROLLBACK == _dataTaskStatus )
      {
         _resultCode = SDB_TASK_ROLLBACK ;
      }
      else
      {
         _resultCode = resultCode ;
         _resultInfo = resultInfo.getOwned() ;
      }

      _dataTaskStatus = DMS_TASK_STATUS_FINISH ;

      ossGetCurrentTime( _endTimestamp ) ;

      resetOpInfo() ;

      calculate() ;
   }

   void _dmsIdxTaskStatus::setCataStatus2Finish()
   {
      _cataTaskStatus = DMS_TASK_STATUS_FINISH ;
   }

   void _dmsIdxTaskStatus::setOpInfo( const CHAR* opInfo )
   {
      if ( opInfo )
      {
         _opInfo = opInfo ;
      }
   }

   void _dmsIdxTaskStatus::resetOpInfo()
   {
      _opInfo.clear() ;
   }

   BOOLEAN _dmsIdxTaskStatus::isStandaloneIdx() const
   {
      return _isStandaloneIdx ;
   }

   BOOLEAN _dmsIdxTaskStatus::isGlobalIdx() const
   {
      return _isGlobalIdx ;
   }

   INT32 _dmsIdxTaskStatus::globalIdxCL( const CHAR *&clName,
                                         UINT64 &clUniqID ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_GLOBAL_OPTION ) ;
         if ( Object == ele.type() )
         {
            BSONObj option = ele.embeddedObject() ;

            ele = option.getField( FIELD_NAME_COLLECTION ) ;
            if ( String == ele.type() )
            {
               clName = ele.valuestr() ;
            }

            ele = option.getField( FIELD_NAME_CL_UNIQUEID ) ;
            if ( ele.isNumber() )
            {
               clUniqID = ele.numberLong() ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _dmsIdxTaskStatus::setIndexDef( const BSONObj &indexDef )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _indexDef = indexDef.getOwned() ;
         _isStandaloneIdx = _indexDef[ IXM_STANDALONE_FIELD ].trueValue() ;
         _isGlobalIdx = _indexDef[ IXM_GLOBAL_FIELD ].trueValue() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKMGR_CRT, "_dmsIdxTaskStatusMgr::createItem" )
   INT32 _dmsIdxTaskStatusMgr::createItem( DMS_TASK_TYPE type,
                                           dmsIdxTaskStatusPtr& statusPtr,
                                           UINT64 taskID,
                                           UINT32 locationID,
                                           UINT64 mainTaskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSIDXTASKMGR_CRT ) ;
      SDB_ASSERT( type == DMS_TASK_CREATE_IDX || type == DMS_TASK_DROP_IDX,
                  "Invalid task type" ) ;

      // When it is a standalone node, or data.createIndex(xxx), there is no
      // catalog task id on standalone node. When taskID is invalid, we should
      // generate a dummy task id.
      if ( DMS_INVALID_TASKID == taskID )
      {
         ossScopedLock lock ( &_hwmLatch, EXCLUSIVE ) ;

         // Dummy task id is in range of ( min, max ]
         if ( _dummyTaskHWM < DMS_DUMMY_CATTASKID_MIN ||
              _dummyTaskHWM >= DMS_DUMMY_CATTASKID_MAX )
         {
            _dummyTaskHWM = DMS_DUMMY_CATTASKID_MIN ;
         }
         _dummyTaskHWM++ ;
         taskID = _dummyTaskHWM ;
      }

      // new status, and add to map
      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      _dmsIdxTaskStatus* pItem = SDB_OSS_NEW _dmsIdxTaskStatus( type,
                                                                taskID,
                                                                locationID,
                                                                mainTaskID ) ;
      if ( NULL == pItem )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Fail to allocate memory" ) ;
         goto error ;
      }

      statusPtr = dmsIdxTaskStatusPtr( pItem ) ;

      try
      {
         _mapIdxStatus[taskID] = statusPtr ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSIDXTASKMGR_CRT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsIdxTaskStatusMgr::findItem( UINT64 taskID,
                                           dmsIdxTaskStatusPtr& statusPtr )
   {
      ossScopedLock lock ( &_mapLatch, SHARED ) ;

      MAP_IDSTATUS_IT it = _mapIdxStatus.find( taskID ) ;
      if ( it == _mapIdxStatus.end() )
      {
         return FALSE ;
      }
      else
      {
         statusPtr = it->second ;
         return TRUE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKMGR_DUMP, "_dmsIdxTaskStatusMgr::dumpInfo" )
   INT32 _dmsIdxTaskStatusMgr::dumpInfo( ossPoolMap<UINT64, _dmsIdxTaskStatus>& statusMap,
                                         UINT32 idxType,
                                         BOOLEAN excludeDummy,
                                         BOOLEAN excludeCataFinished )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSIDXTASKMGR_DUMP ) ;

      ossScopedLock lock ( &_mapLatch, SHARED ) ;

      MAP_IDSTATUS_IT it ;
      for ( it = _mapIdxStatus.begin() ; it != _mapIdxStatus.end() ; ++it )
      {
         dmsIdxTaskStatusPtr pIdxTask = it->second ;
         if ( excludeDummy &&
              DMS_IS_DUMMY_CATTASKID( pIdxTask->taskID() ) )
         {
            continue ;
         }
         if ( excludeCataFinished &&
              DMS_TASK_STATUS_FINISH == pIdxTask->cataStatus() )
         {
            continue ;
         }
         BOOLEAN isStandIdx = pIdxTask->isStandaloneIdx() ;
         if ( ( isStandIdx  && ( idxType & DMS_IDX_STANDALONE ) ) ||
              ( !isStandIdx && ( idxType & DMS_IDX_NORMAL ) ) )
         {
            pIdxTask->calculate() ;
            try
            {
               statusMap.insert( std::pair<UINT64, _dmsIdxTaskStatus>
                                 ( it->first, *pIdxTask ) ) ;
            }
            catch( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
            }
         }
      }

   done:
      PD_TRACE_EXIT( SDB_DMSIDXTASKMGR_DUMP ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKMGR_CNT, "_dmsIdxTaskStatusMgr::count" )
   INT32 _dmsIdxTaskStatusMgr::count( UINT32 idxType,
                                      BOOLEAN excludeDummy,
                                      BOOLEAN excludeCataFinished )
   {
      INT32 cnt = 0 ;
      PD_TRACE_ENTRY( SDB_DMSIDXTASKMGR_CNT ) ;

      ossScopedLock lock ( &_mapLatch, SHARED ) ;

      if ( ( idxType & DMS_IDX_ALL ) && !excludeDummy && !excludeCataFinished )
      {
         cnt = _mapIdxStatus.size() ;
      }
      else
      {
         for ( MAP_IDSTATUS_IT it = _mapIdxStatus.begin() ;
               it != _mapIdxStatus.end() ; ++it )
         {
            dmsIdxTaskStatusPtr pIdxTask = it->second ;
            if ( excludeDummy &&
                 DMS_IS_DUMMY_CATTASKID( pIdxTask->taskID() ) )
            {
               continue ;
            }
            if ( excludeCataFinished &&
                 DMS_TASK_STATUS_FINISH == pIdxTask->cataStatus() )
            {
               continue ;
            }
            BOOLEAN isStandIdx = pIdxTask->isStandaloneIdx() ;
            if ( ( isStandIdx  && ( idxType & DMS_IDX_STANDALONE ) ) ||
                 ( !isStandIdx && ( idxType & DMS_IDX_NORMAL ) ) )
            {
               cnt++ ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_DMSIDXTASKMGR_CNT ) ;
      return cnt ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKMGR_CLEAN, "_dmsIdxTaskStatusMgr::cleanOutOfDate" )
   void _dmsIdxTaskStatusMgr::cleanOutOfDate( BOOLEAN isPrimary )
   {
      PD_TRACE_ENTRY( SDB_DMSIDXTASKMGR_CLEAN ) ;

      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      MAP_IDSTATUS_IT it = _mapIdxStatus.begin() ;
      while ( it != _mapIdxStatus.end() )
      {
         BOOLEAN needClean = FALSE ;
         dmsIdxTaskStatusPtr pIdxTask = it->second ;

         if ( DMS_IS_DUMMY_CATTASKID( pIdxTask->taskID() ) )
         {
            // task was generated by stanalone.createIndex or data.createIndex()
            if ( DMS_TASK_STATUS_FINISH == pIdxTask->status() )
            {
               needClean = TRUE ;
            }
         }
         else
         {
            // task was generated by coord.createIndex() or slave data node
            // replay index log
            if ( isPrimary )
            {
               if ( DMS_TASK_STATUS_FINISH == pIdxTask->cataStatus() )
               {
                  needClean = TRUE ;
               }
            }
            else
            {
               if ( DMS_TASK_STATUS_FINISH == pIdxTask->status() )
               {
                  needClean = TRUE ;
               }
            }
         }

         if ( needClean )
         {
            PD_LOG( PDINFO, "Clean up expired task[%s]",
                    pIdxTask->toBSON().toString().c_str() ) ;
            _mapIdxStatus.erase( it++ ) ;
         }
         else
         {
            it++ ;
         }
      }

      PD_TRACE_EXIT( SDB_DMSIDXTASKMGR_CLEAN ) ;
   }
}

