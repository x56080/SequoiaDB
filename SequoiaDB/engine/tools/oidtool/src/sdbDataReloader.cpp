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

   Source File Name = sdbDataReloader.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/29  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossUtil.hpp"
#include "pd.hpp"
#include "msgDef.hpp"
#include "sdbDataReloader.hpp"
#include <iostream>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace engine ;
using namespace bson ;
using namespace std ;

void threadEntry( ITask *pTask )
{
   INT32 rc = pTask->run() ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to run task[%s], rc = %d", pTask->name(), rc ) ;
   }
} ;

/**
 * RecordPool
 */
RecordPool::RecordPool( IController *pController )
{
   _totalRecordNum = 0 ;
   _currReadNum = 0 ;
   _beginLsnSum = 0 ;
   _hasReadAll = FALSE ;

   _pOptions = getInfoOptions() ;
   _pController = pController ;
   _pConnect = NULL ;
}

RecordPool::~RecordPool()
{
   fini() ;
}

INT32 RecordPool::init()
{
   INT32 rc = SDB_OK ;
   INT32 taskNum = _pOptions->taskNum ;

   if ( NULL == _pController )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Invalid controller" ) ;
      goto error ;
   }
   // init record queues, make two recordSets for each importer
   _queriedQueue.initBuffer( taskNum * 2 ) ;
   _insertedQueue.initBuffer( taskNum * 2 ) ;
   for( INT32 i = 0 ; i < ( taskNum * 2 ) ; ++i )
   {
      RecordSet *pSet = new(nothrow) RecordSet() ;
      if ( NULL == pSet )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc record set" ) ;
         goto error ;
      }
      _insertedQueue.pushBack( pSet ) ;
   }
   // get connection
   _pConnect = new(nothrow) sdb() ;
   if ( NULL == _pConnect )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed to alloc connection handle" ) ;
      goto error ;
   }
   rc = _pConnect->connect( _pOptions->hostName.c_str(), _pOptions->svcName.c_str(),
                           _pOptions->user.c_str(), _pOptions->passwd.c_str() ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to connect to host[%s:%s], rc = %d",
              _pOptions->hostName.c_str(), _pOptions->svcName.c_str(), rc ) ;
      goto error ;
   }
   // get collection
   rc = _pConnect->getCollection( _pOptions->srcCLFullName.c_str(), _cl ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get collection[%s] handle, rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }
   // get count
   rc = _cl.getCount( _totalRecordNum ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get count in cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }
   // get lsn
   rc = _getLsnSum( _beginLsnSum ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get lsn info in cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   fini() ;
   goto done ;
}

void RecordPool::fini()
{
   // release record sets
   while( !_queriedQueue.isEmpty() )
   {
      SDB_ASSERT( NULL != _queriedQueue.getFront(), "Invalid element in _queriedQueue" ) ;
      RecordSet *pSet = *_queriedQueue.getFront() ;
      delete pSet ;
      _queriedQueue.popFront() ;
   }
   while( !_insertedQueue.isEmpty() )
   {
      SDB_ASSERT( NULL != _insertedQueue.getFront(), "Invalid element in _insertedQueue" ) ;
      RecordSet *pSet = *_insertedQueue.getFront() ;
      delete pSet ;
      _insertedQueue.popFront() ;
   }
   // release connection handle
   if ( _pConnect )
   {
      _pConnect->disconnect() ;
      delete _pConnect ;
      _pConnect = NULL ;
   }
}

INT32 RecordPool::run()
{
   INT32 rc = SDB_OK ;
   INT64 records = 0 ;
   sdbCursor cursor ;
   BSONObj obj ;
   static INT64 lastTotalRead = 0 ;
   static time_t beginTime = 0 ;
   static pt::time_duration diffMicro = pt::microseconds(0) ;
   time_t currentTime = 0 ;
   UINT32 diffTime = 0 ;
   UINT32 needTime = 0 ;
   FLOAT64 speed = 0 ;

   if ( 0 == beginTime )
   {
      beginTime = time( NULL ) ;
   }

   // query records
   rc = _cl.query( cursor ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to query cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }

   while( !_pController->isStop() )
   {
      pt::ptime startTime = pt::microsec_clock::universal_time() ;
      _queryEvent.wait( OSS_ONE_SEC ) ;
      pt::ptime endTime = pt::microsec_clock::universal_time() ;
      diffMicro += endTime - startTime ;

      while ( !_isQueueEmpty( _insertedQueue ) )
      {
         // get record set. Only one thread read _insertedQueue,
         // so pSet is impossible to be NULL
         _queueMutex.get() ;
         RecordSet *pSet = *_insertedQueue.getFront() ;
         _insertedQueue.popFront() ;
         _queueMutex.release() ;

         // read record to RecordSet
         pSet->vecObjs.clear() ;
         records = 0 ;
         while( SDB_OK == ( rc = cursor.next( obj ) ) )
         {
            pSet->vecObjs.push_back( obj.getOwned() ) ;
            ++records ;
            if ( records == _pOptions->limit )
            {
               break ;
            }
         }
         // put to queried queue, and then notify importer to get record
         if ( records > 0 )
         {
            _queueMutex.get() ;
            _queriedQueue.pushBack( pSet ) ;
            _queueMutex.release() ;
            _insertEvent.signal() ;
         }
         else
         {
            _queueMutex.get() ;
            _insertedQueue.pushBack( pSet ) ;
            _queueMutex.release() ;
         }
         _currReadNum += records ;
         // check has read all the records or not
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            if ( _currReadNum != _totalRecordNum )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid cl[%s] status, total read[%d] is not equal to "
                       "total record num[%d]",
                       _pOptions->srcCLFullName.c_str(),
                       _currReadNum, _totalRecordNum ) ;
               goto error ;
            }
            else
            {
               if ( !_checkSrcCLStatus() )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Invalid cl[%s] status", _pOptions->srcCLFullName.c_str() ) ;
                  goto error ;
               }
               PD_LOG( PDEVENT, "Success to get all the record from cl[%s]",
                       _pOptions->srcCLFullName.c_str() ) ;
               _hasReadAll = TRUE ;
               goto done ;
            }
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get record from cl[%s], rc = %d",
                    _pOptions->srcCLFullName.c_str(), rc ) ;
            goto error ;
         }

         // check and display speed
         currentTime = time( NULL ) ;
         diffTime = currentTime - beginTime ;
         if ( diffTime > (UINT32)_pOptions->displayPeriod )
         {
            beginTime = currentTime ;
            // check count and LSN
            if ( !_checkSrcCLStatus() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid cl[%s] status", _pOptions->srcCLFullName.c_str() ) ;
               goto error ;
            }
            // add speed and need time to log
            speed = ( _currReadNum - lastTotalRead ) / diffTime ;
            needTime = speed > 1 ? ( ( _totalRecordNum - _currReadNum ) / speed ) : 0 ;
            lastTotalRead = _currReadNum ;
            PD_LOG( PDEVENT, "Query Thread: has read %ld records, the speed is: %f rec/s, "
                    "still need %d second(s), query wait time is: %ld usec in current period",
                    _currReadNum, speed, needTime, diffMicro.total_microseconds() ) ;
            diffMicro = pt::microseconds(0) ;
         }
      }
   }

done:
   cursor.close() ;
   setErrNo( rc ) ;
   return rc ;
error:
   _pController->stop() ;
   goto done ;
}

BOOLEAN RecordPool::_isQueueEmpty( RECORD_QUEUE &queue )
{
   ossScopedLock lock( &_queueMutex, SHARED ) ;
   return queue.isEmpty() ;
}

BOOLEAN RecordPool::_checkSrcCLStatus()
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj obj ;
   SINT64 recordNum = 0 ;
   UINT64 lsnSum = 0 ;

   // check record count
   rc = _cl.getCount( recordNum ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get count in cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      return FALSE ;
   }
   if ( recordNum != _totalRecordNum )
   {
      PD_LOG( PDERROR, "The record num in cl[%s] has changed from %ld to %ld",
              _pOptions->srcCLFullName.c_str(), _totalRecordNum, recordNum ) ;
      return FALSE ;
   }

   // check lob count
   rc = _cl.listLobs( cursor ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to list lob in cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      return FALSE ;
   }
   rc = cursor.next( obj ) ;
   if ( SDB_DMS_EOC != rc )
   {
      cursor.close() ;
      PD_LOG( PDERROR, "Collection[%s] has lob", _pOptions->srcCLFullName.c_str() ) ;
      return FALSE ;
   }

   // check lsn
   rc = _getLsnSum( lsnSum ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get lsn info in cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      return FALSE ;
   }
   if ( lsnSum != _beginLsnSum )
   {
      PD_LOG( PDERROR, "The lsn sum in cl[%s] has changed from %ld to %ld",
              _pOptions->srcCLFullName.c_str(), _beginLsnSum, lsnSum ) ;
      return FALSE ;
   }

   return TRUE ;
}

INT32 RecordPool::_getLsnSum( UINT64 &lsnSum )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj obj ;
   BSONObj cond = BSON( "Name" << _pOptions->srcCLFullName.c_str() << "RawData" << true ) ;
   BSONObj sel = BSON( "Name" << "" << "Details.GroupName" << ""
                       << "Details.DataCommitLSN" << ""
                       << "Details.IndexCommitLSN" << "" ) ;
   BSONObj sort = BSON( "Details.GroupName" << 1 ) ;
   string preGoupName = "" ;
   UINT64 totalLsn = 0 ;

   // get cl snapshot
   rc = _pConnect->getSnapshot( cursor, SDB_SNAP_COLLECTIONS, cond, sel, sort ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get snapshot of cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }
   // get lsn sum
   while ( SDB_OK == ( rc = cursor.next( obj ) ) )
   {
      // obj is:
      // { "Name": "test33.test", "Details": [ { "GroupName": "db1", "DataCommitLSN": 1498929048, "IndexCommitLSN": 1498927400 } ] }
      BSONElement detailsEle = obj.getField( FIELD_NAME_DETAILS ) ;
      BSONObjIterator itr( detailsEle.embeddedObject() ) ;
      if ( itr.more() )
      {
         BSONObj detailObj = itr.next().embeddedObject() ;
         string groupName = detailObj.getStringField( FIELD_NAME_GROUPNAME ) ;
         // if it's the node in the same group, let's ignore current node
         if ( groupName == preGoupName )
         {
            continue ;
         }
         else
         {
         #if defined ( _DEBUG )
            PD_LOG( PDDEBUG, "snapshot obj for lsn is: %s", obj.toString( TRUE, FALSE, TRUE ).c_str() ) ;
         #endif
            preGoupName = groupName ;
            INT64 dataLsn = detailObj.getField( FIELD_NAME_DATA_COMMIT_LSN ).numberLong() ;
            INT64 idxLsn = detailObj.getField( FIELD_NAME_IDX_COMMIT_LSN ).numberLong() ;
            totalLsn += dataLsn + idxLsn ;
         }
      }
   }
   if ( SDB_DMS_EOC != rc )
   {
      PD_LOG( PDERROR, "Failed to get snapshot from cursor of cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }
   rc = SDB_OK ;
   lsnSum = totalLsn ;

done:
   return rc ;
error:
   goto done ;
}

INT32 RecordPool::getRecordSet( RecordSet **ppRecordSet, UINT32 *pWaitTimeMicro )
{
   INT32 rc = SDB_OK ;
   RecordSet *pOutSet = NULL ;
   RecordSet **ppTmpSet = NULL ;
   // value for wait time
   pt::ptime beginWaitMicro ;
   pt::ptime endWaitMicro ;
   pt::time_duration diffMicro = pt::microseconds( 0 ) ;

   while( !_pController->isStop() )
   {
      ppTmpSet = NULL ;
      pOutSet = NULL ;

      if ( _isQueueEmpty( _queriedQueue ) )
      {
         // check finish or not
         // when the pool has read all the records, it will put the record
         // to _queriedQueue and set _hasReadAll to be TRUE
         if ( _hasReadAll )
         {
            // double check
            // when _hasReadAll is true, make sure the last RecordSet will
            // be got by importer
            if ( _isQueueEmpty( _queriedQueue ) )
            {
               rc = SDB_DMS_EOC ;
               goto done ;
            }
         }
         // notify pool to query records
         _queryEvent.signal() ;
         // and then waiting
         beginWaitMicro = pt::microsec_clock::universal_time() ;
         if ( SDB_TIMEOUT == _insertEvent.wait( OSS_ONE_SEC ) )
         {
            diffMicro += pt::seconds( 1 ) ; 
            continue ;
         }
         endWaitMicro = pt::microsec_clock::universal_time() ;
         diffMicro += endWaitMicro - beginWaitMicro ;
      }

      // get data for importing
      _queueMutex.get() ;
      ppTmpSet = _queriedQueue.getFront() ;
      if ( ppTmpSet )
      {
         pOutSet = *ppTmpSet ;
         _queriedQueue.popFront() ;
      }
      _queueMutex.release() ;
      if ( NULL != pOutSet )
      {
         break ;
      }
   }
   // output
   if ( pOutSet )
   {
      *ppRecordSet = pOutSet ;
   }
   else
   {
      // when come here, means controller has stop
      rc = SDB_SYS ;
      goto error ;
   }
   if ( pWaitTimeMicro )
   {
      *pWaitTimeMicro = diffMicro.total_microseconds() ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 RecordPool::releaseRecordSet( RecordSet *pRecordSet )
{
   SDB_ASSERT( NULL != pRecordSet, "Invalid pRecordSet" ) ;
   if ( !_pController->isStop() )
   {
      _queueMutex.get() ;
      _insertedQueue.pushBack( pRecordSet ) ;
      _queueMutex.release() ;
      // notify pool to query records
      _queryEvent.signal() ;
   }
   else
   {
      delete pRecordSet ;
   }

   return SDB_OK ;
}

/**
 * RecordImporter
 */
RecordImporter::RecordImporter( IController *pController,
                                IDataProvider *pDataProvider )
: _pController( pController ),
  _pDataProvider( pDataProvider )
{
   _pOptions = getInfoOptions() ;
   _pConnect = NULL ;
}

RecordImporter::~RecordImporter()
{
   if ( _pConnect )
   {
      _pConnect->disconnect() ;
      delete _pConnect ;
      _pConnect = NULL ;
   }
}

INT32 RecordImporter::_init()
{
   INT32 rc = SDB_OK ;

   if ( NULL == _pController )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Invalid controller" ) ;
      goto error ;
   }
   if ( NULL == _pDataProvider )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Invalid data provider" ) ;
      goto error ;
   }
   // get connection
   _pConnect = new(nothrow) sdb() ;
   if ( NULL == _pConnect )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed to alloc connection handle" ) ;
      goto error ;
   }
   rc = _pConnect->connect( _pOptions->hostName.c_str(), _pOptions->svcName.c_str(),
                            _pOptions->user.c_str(), _pOptions->passwd.c_str() ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to connect to host[%s:%s], rc = %d",
              _pOptions->hostName.c_str(), _pOptions->svcName.c_str(), rc ) ;
      goto error ;
   }
   // get collection
   rc = _pConnect->getCollection( _pOptions->destCLFullName.c_str(), _cl ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get collection[%s] handle, rc = %d",
              _pOptions->destCLFullName.c_str(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 RecordImporter::run()
{
   INT32 rc = SDB_OK ;
   // value for run time
   pt::ptime beginRunSec = pt::second_clock::universal_time() ;
   pt::ptime currentRunSec ;
   pt::time_duration diffSec = pt::seconds( 0 ) ;
   UINT32 totalWaitTimeMicro = 0 ;

   // prepare for running task
   rc = _init() ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to init importer in cl[%s], rc = %d",
               _pOptions->destCLFullName.c_str(), rc ) ;
      goto error ;
   }

   // going to get record, and then insert to new cl
   while ( !_pController->isStop() )
   {
      RecordSet *pSet = NULL ;
      UINT32 waitTimeMicro = 0 ;
      // get record set, make sure it will be released back
      rc = _pDataProvider->getRecordSet( &pSet, &waitTimeMicro ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get record set for cl[%s], rc = %d",
                 _pOptions->destCLFullName.c_str(), rc ) ;
         goto error ;
      }
      totalWaitTimeMicro += waitTimeMicro ;
      // insert records
      rc = _cl.bulkInsert( FLG_INSERT_CONTONDUP, pSet->vecObjs ) ;
      pSet->vecObjs.clear() ;
      _pDataProvider->releaseRecordSet( pSet ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to insert record into cl[%s], rc = %d",
                 _pOptions->destCLFullName.c_str(), rc ) ;
         goto error ;
      }
      // display wait time
      currentRunSec = pt::second_clock::universal_time() ;
      diffSec = currentRunSec - beginRunSec ;
      if ( diffSec.total_seconds() > _pOptions->displayPeriod )
      {
         PD_LOG( PDEVENT, "Insert thread: insert wait time is: %ld usec "
                 "within the period", totalWaitTimeMicro ) ;
         beginRunSec = currentRunSec ;
         totalWaitTimeMicro = 0 ;
      }
   }

done:
   setErrNo( rc ) ;
   return rc ;
error:
   _pController->stop() ;
   goto done ;
}

/**
 * SdbDataReloader
 */
SdbDataReloader::SdbDataReloader()
: _recordPool( &_reloadCtx )
{
   _pOptions = getInfoOptions() ;
   _pQueryThd = NULL ;
}

SdbDataReloader::~SdbDataReloader()
{
}

INT32 SdbDataReloader::run( BSONObj &result )
{
   INT32 rc = SDB_OK ;
   BSONObjBuilder bob ;

   rc = _init() ;
   if ( rc )
   {
      goto error ;
   }

   rc = _run() ;
   if ( rc )
   {
      goto error ;
   }

done:
   _fini() ;
   // build result obj
   bob.append( "ErrNo", rc ) ;
   bob.append( "Action", _pOptions->action.c_str() ) ;
   bob.append( "Name", _pOptions->srcCLFullName.c_str() ) ;
   result = bob.obj() ;
   return rc ;
error:
   goto done ;
}

INT32 SdbDataReloader::_init()
{
   INT32 rc = SDB_OK ;

   rc = _recordPool.init() ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to init record pool for cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
   }

   return rc ;
}

INT32 SdbDataReloader::_run()
{
   INT32 rc = SDB_OK ;
   INT32 thdRc = SDB_OK ;
   INT32 taskNum = _pOptions->taskNum ;

   // create query thread
   try
   {
      _pQueryThd = new boost::thread( threadEntry, &_recordPool ) ;
   }
   catch ( std::exception &e )
   {
      rc = SDB_SYS ;
      PD_LOG( PDERROR, "Failed to create query thread: %s", e.what() ) ;
      goto error ;
   }

   // create import threads
   for ( INT32 i = 0 ; i < taskNum ; ++i )
   {
      boost::thread *pThread = NULL ;
      RecordImporter *pImprter = new(nothrow) RecordImporter( &_reloadCtx, &_recordPool ) ;
      if ( !pImprter )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc importer" ) ;
         goto error ;
      }
      _verImprters.push_back( pImprter ) ;
      try
      {
         pThread = new boost::thread( threadEntry, pImprter ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to create import thread: %s", e.what() ) ;
         goto error ;
      }
      _vecImprtThds.push_back( pThread ) ;
   }
   PD_LOG( PDEVENT, "Start %d import thread(s) to run, import batch is "
           "the same as query limit value: %ld", taskNum, _pOptions->limit ) ;

done:
   // join threads
   if ( _pQueryThd )
   {
      _pQueryThd->join() ;
      delete _pQueryThd ;
      _pQueryThd = NULL ;
   }
   for ( UINT32 i = 0; i < _vecImprtThds.size(); i++ )
   {
      _vecImprtThds[i]->join() ;
      delete _vecImprtThds[i] ;
   }
   // release the importers
   for ( UINT32 i = 0; i < _verImprters.size(); i++ )
   {
      RecordImporter *p = _verImprters[i] ;
      // get return rc of the importers
      if ( !thdRc && p->getErrNo() )
      {
         thdRc = p->getErrNo() ;
      }
      delete p ;
   }
   // get return rc of query task
   if ( !thdRc && _recordPool.getErrNo() )
   {
      thdRc = _recordPool.getErrNo() ;
   }
   if ( !rc && thdRc )
   {
      rc = thdRc ;
   }

   return rc ;
error:
   _fini() ;
   goto done ;
}

void SdbDataReloader::_fini()
{
   _reloadCtx.stop() ;
   _recordPool.fini() ;
}