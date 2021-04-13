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

   Source File Name = clsTask.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/03/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsTask.hpp"
#include "msgCatalogDef.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "ixm_common.hpp"
#include "rtn.hpp"

using namespace bson ;

namespace engine
{
   const CHAR* clsTaskTypeStr( CLS_TASK_TYPE taskType )
   {
      switch( taskType )
      {
         case CLS_TASK_SPLIT :
            return VALUE_NAME_SPLIT ;
         case CLS_TASK_SEQUENCE :
            return VALUE_NAME_ALTERSEQUENCE ;
         case CLS_TASK_CREATE_IDX :
            return VALUE_NAME_CREATEIDX ;
         case CLS_TASK_DROP_IDX :
            return VALUE_NAME_DROPIDX ;
         case CLS_TASK_COPY_IDX :
            return VALUE_NAME_COPYIDX ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   const CHAR* clsTaskStatusStr( CLS_TASK_STATUS taskStatus )
   {
      switch( taskStatus )
      {
         case CLS_TASK_STATUS_READY :
            return VALUE_NAME_READY ;
         case CLS_TASK_STATUS_RUN :
            return VALUE_NAME_RUNNING ;
         case CLS_TASK_STATUS_PAUSE :
            return VALUE_NAME_PAUSE ;
         case CLS_TASK_STATUS_CANCELED :
            return VALUE_NAME_CANCELED ;
         case CLS_TASK_STATUS_META :
            return VALUE_NAME_CHGMETA ;
         case CLS_TASK_STATUS_CLEANUP :
            return VALUE_NAME_CLEANUP ;
         case CLS_TASK_STATUS_ROLLBACK :
            return VALUE_NAME_ROLLBACK ;
         case CLS_TASK_STATUS_FINISH :
            return VALUE_NAME_FINISH ;
         case CLS_TASK_STATUS_END :
            return VALUE_NAME_END ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   /*
      _clsTask : implement
   */
   void _clsTask::setStatus( CLS_TASK_STATUS status )
   {
      _status = status ;
   }

   const CHAR* _clsTask::commandName() const
   {
      return NULL ;
   }

   BOOLEAN _clsTask::hasMainTask() const
   {
      return CLS_INVALID_TASKID != _mainTaskID ;
   }

   INT32 _clsTask::getSubTasks( ossPoolVector<UINT64>& list )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::updateTaskInfo( const CHAR* objdata,
                                   BSONObj& updator,
                                   BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::updateMainTaskInfo( _clsTask *pSubTask,
                                       const ossPoolVector<BSONObj>& subTaskInfoList,
                                       BSONObj& updator,
                                       BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::querySubTasks( const CHAR* objdata,
                                  BSONObj& matcher,
                                  BSONObj& selector )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::removeSubTask( UINT64 taskID,
                                  const ossPoolVector<BSONObj>& otherSubTasks,
                                  BSONObj& updator,
                                  BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 clsNewTask( const BSONObj &taskObj, clsTask*& pTask )
   {
      INT32 rc = SDB_OK ;
      CLS_TASK_TYPE taskType = CLS_TASK_UNKNOWN ;

      // get task type
      rc = rtnGetIntElement( taskObj, FIELD_NAME_TASKTYPE,
                             (INT32&)taskType ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get task type from obj[%s], rc: %d",
                   taskObj.toString().c_str(), rc ) ;

      rc = clsNewTask( taskType, taskObj, pTask ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 clsNewTask( CLS_TASK_TYPE taskType, const BSONObj &taskObj,
                     clsTask*& pTask )
   {
      INT32 rc = SDB_OK ;
      UINT64 taskID = CLS_INVALID_TASKID ;

      SDB_ASSERT( NULL == pTask, "pTask should be null" ) ;

      // get task id
      rc = rtnGetNumberLongElement( taskObj, FIELD_NAME_TASKID,
                                    (INT64&)taskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Faield to get task ID from obj[%s], rc: %d",
                   taskObj.toString().c_str(), rc ) ;

      // new task
      switch ( taskType )
      {
         case CLS_TASK_SPLIT :
            pTask = SDB_OSS_NEW _clsSplitTask( taskID ) ;
            break ;
         case CLS_TASK_CREATE_IDX :
            pTask = SDB_OSS_NEW _clsCreateIdxTask( taskID ) ;
            break ;
         case CLS_TASK_DROP_IDX :
            pTask = SDB_OSS_NEW _clsDropIdxTask( taskID ) ;
            break ;
         case CLS_TASK_COPY_IDX :
            pTask = SDB_OSS_NEW _clsCopyIdxTask( taskID ) ;
            break ;
         default :
            PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                         "Unknown task type[%d]",
                         taskType ) ;
      }
      PD_CHECK ( pTask, SDB_OOM, error, PDERROR,
                 "Failed to alloc memory for task[type: %d]",
                 taskType ) ;

      // init task
      rc = pTask->init( taskObj.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init task by obj[%s], rc: %d",
                   taskObj.toString().c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void  clsReleaseTask( clsTask*& pTask )
   {
      if ( pTask )
      {
         SDB_OSS_DEL pTask ;
         pTask = NULL ;
      }
   }

   /*
      _clsTaskMgr : implement
   */
   _clsTaskMgr::_clsTaskMgr ( UINT32 maxLocationID )
   {
      _locationID = CLS_INVALID_LOCATIONID ;
      _maxID      = maxLocationID ;
   }

   _clsTaskMgr::~_clsTaskMgr ()
   {
      std::map<UINT32, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _taskMap.clear() ;
   }

   UINT32 _clsTaskMgr::getLocationID ()
   {
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      ++_locationID ;
      if ( CLS_INVALID_LOCATIONID == _locationID ||
           ( CLS_INVALID_LOCATIONID != _maxID && _locationID > _maxID )  )
      {
         _locationID = CLS_INVALID_LOCATIONID + 1 ;
      }
      return _locationID ;
   }

   UINT32 _clsTaskMgr::taskCount ()
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      return (UINT32)_taskMap.size() ;
   }

   UINT32 _clsTaskMgr::taskCount( CLS_TASK_TYPE type )
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
         if ( type == pTask->taskType() )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   UINT32 _clsTaskMgr::taskCountByCL( const CHAR *pCLName )
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
         if ( pTask->collectionName() &&
              '\0' != pTask->collectionName()[0] &&
              0 == ossStrcmp( pCLName, pTask->collectionName() ) )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   UINT32 _clsTaskMgr::taskCountByCS( const CHAR *pCSName )
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
         if ( pTask->collectionSpaceName() &&
              0 == ossStrcmp( pCSName, pTask->collectionSpaceName() ) )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   INT32 _clsTaskMgr::waitTaskEvent( INT64 millisec )
   {
      return _taskEvent.wait( millisec ) ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTKMGR_ADDTK, "_clsTaskMgr::addTask" )
   INT32 _clsTaskMgr::addTask ( _clsTask * pTask, UINT32 locationID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSTKMGR_ADDTK ) ;
      _clsTask *indexTask = NULL ;

      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;

      std::map<UINT32, _clsTask*>::iterator it = _taskMap.begin () ;
      while ( it != _taskMap.end() )
      {
         indexTask = it->second ;

         if ( locationID == it->first ||
              ( pTask->taskID() != CLS_INVALID_TASKID &&
                pTask->taskID() == indexTask->taskID() ) ||
              pTask->muteXOn( indexTask ) || indexTask->muteXOn( pTask ) )
         {
            PD_LOG ( PDWARNING, "Exist task[%lld,%s] mutex with new task[%lld,%s]",
                     indexTask->taskID(), indexTask->taskName(),
                     pTask->taskID(), pTask->taskName() ) ;
            rc = SDB_CLS_MUTEX_TASK_EXIST ;
            goto error ;
         }
         ++it ;
      }
      // add to map
      try
      {
         _taskMap[ locationID ] = pTask ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSTKMGR_ADDTK, rc ) ;
      return rc ;
   error:
      SDB_OSS_DEL pTask ;
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTKMGR_RVTK1, "_clsTaskMgr::removeTask" )
   INT32 _clsTaskMgr::removeTask ( UINT32 locationID )
   {
      PD_TRACE_ENTRY ( SDB__CLSTKMGR_RVTK1 ) ;
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      std::map<UINT32, _clsTask*>::iterator it = _taskMap.find ( locationID ) ;
      if ( it != _taskMap.end() )
      {
         SDB_OSS_DEL it->second ;
         _taskMap.erase ( it ) ;
         _taskEvent.signal() ;
      }

      PD_TRACE_EXIT ( SDB__CLSTKMGR_RVTK1 ) ;
      return SDB_OK ;
   }

   _clsTask* _clsTaskMgr::findTask ( UINT32 locationID )
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      std::map<UINT32, _clsTask*>::iterator it = _taskMap.find ( locationID ) ;
      if ( it != _taskMap.end() )
      {
         return it->second ;
      }
      return NULL ;
   }

   void _clsTaskMgr::stopTask( UINT32 locationID )
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      std::map<UINT32, _clsTask*>::iterator it = _taskMap.find ( locationID ) ;
      if ( it != _taskMap.end() )
      {
         return it->second->setStatus( CLS_TASK_STATUS_CANCELED ) ;
      }
   }

   string _clsTaskMgr::dumpTasks( CLS_TASK_TYPE type )
   {
      string taskStr ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
         if ( CLS_TASK_UNKNOWN == type ||
              type == pTask->taskType() )
         {
            taskStr += "[ taskName: " ;
            taskStr += pTask->taskName() ? pTask->taskName() : "" ;
            taskStr += " collectionName: " ;
            taskStr += pTask->collectionName() ? pTask->collectionName() : "" ;
            taskStr += " ]" ;
         }
         ++it ;
         if ( it != _taskMap.end() )
         {
            taskStr += "\n" ;
         }
      }

      return taskStr ;
   }

   void _clsTaskMgr::regCollection( const string & clName )
   {
      ossScopedLock lock( &_regLatch, EXCLUSIVE ) ;

      std::map<string, UINT32>::iterator it = _mapRegister.find( clName ) ;
      if ( it == _mapRegister.end() )
      {
         _mapRegister[ clName ] = 1 ;
      }
      else
      {
         ++(it->second) ;
      }
   }

   void _clsTaskMgr::unregCollection( const string & clName )
   {
      ossScopedLock lock( &_regLatch, EXCLUSIVE ) ;

      std::map<string, UINT32>::iterator it = _mapRegister.find( clName ) ;
      if ( it != _mapRegister.end() )
      {
         if ( it->second > 1 )
         {
            --(it->second) ;
         }
         else
         {
            _mapRegister.erase( it ) ;
         }
      }
   }

   UINT32 _clsTaskMgr::getRegCount( const string & clName, BOOLEAN noLatch )
   {
      UINT32 count = 0 ;

      if ( !noLatch )
      {
         lockReg( SHARED ) ;
      }

      std::map<string, UINT32>::iterator it = _mapRegister.find( clName ) ;
      if ( it != _mapRegister.end() )
      {
         count = it->second ;
      }

      if ( !noLatch )
      {
         releaseReg( SHARED ) ;
      }

      return count ;
   }

   void _clsTaskMgr::lockReg( OSS_LATCH_MODE mode )
   {
      if ( SHARED == mode )
      {
         _regLatch.get_shared() ;
      }
      else
      {
         _regLatch.get() ;
      }
   }

   void _clsTaskMgr::releaseReg( OSS_LATCH_MODE mode )
   {
      if ( SHARED == mode )
      {
         _regLatch.release_shared() ;
      }
      else
      {
         _regLatch.release() ;
      }
   }

   /*
      _clsDummyTask : implement
   */
   _clsDummyTask::_clsDummyTask( UINT64 taskID )
   :_clsTask( taskID )
   {
   }

   _clsDummyTask::~_clsDummyTask()
   {
   }

   const CHAR* _clsDummyTask::taskName() const
   {
      return "DummyTask" ;
   }

   BOOLEAN _clsDummyTask::muteXOn ( const _clsTask *pOther )
   {
      return FALSE ;
   }

   const CHAR* _clsDummyTask::collectionName() const
   {
      return "" ;
   }

   const CHAR* _clsDummyTask::collectionSpaceName() const
   {
      return "" ;
   }

   INT32 _clsDummyTask::init( const CHAR* objdata )
   {
      return SDB_OK ;
   }

   BSONObj _clsDummyTask::toBson ( UINT32 mask )
   {
      return BSONObj() ;
   }

   /*
      _clsSplitTask : implement
   */
   _clsSplitTask::_clsSplitTask ( UINT64 taskID )
   : _clsTask ( taskID )
   {
      _sourceID = 0 ;
      _dstID = 0 ;
      _clUniqueID = UTIL_UNIQUEID_NULL ;
      _taskType = CLS_TASK_SPLIT ;
      _status = CLS_TASK_STATUS_READY ;
      _percent  = 0.0 ;
      _resultCode = SDB_OK ;
      //_lockEnd = FALSE ;
   }

   _clsSplitTask::~_clsSplitTask ()
   {
   }

   void _clsSplitTask::_makeName ()
   {
      _taskName = "Split-" ;
      _taskName += _clFullName ;
      _taskName += "{ Begin:" ;
      _taskName += _splitKeyObj.toString() ;
      _taskName += ", End:" ;
      _taskName += _splitEndKeyObj.toString() ;
      _taskName += " } " ;

      /// cs name make
      size_t npos = _clFullName.find( '.' ) ;
      _csName = _clFullName.substr( 0, npos ) ;
   }

   INT32 _clsSplitTask::init ( const CHAR * clFullName, INT32 sourceID,
                               const CHAR * sourceName, INT32 dstID,
                               const CHAR * dstName, const BSONObj & bKey,
                               const BSONObj & eKey, FLOAT64 percent,
                               clsCatalogSet &cataSet )
   {
      INT32 rc = SDB_OK ;
      //_lockEnd = FALSE ;

      _clFullName    = clFullName ;
      _clUniqueID    = cataSet.clUniqueID() ;
      _sourceID      = sourceID ;
      _sourceName    = sourceName ;
      _dstID         = dstID ;
      _dstName       = dstName ;
      _splitKeyObj   = bKey.getOwned() ;
      _splitEndKeyObj= eKey.getOwned() ;
      _shardingKey   = cataSet.getShardingKey().getOwned() ;
      _percent       = percent ;
      _shardingType  = CAT_SHARDING_TYPE_RANGE ;
      if ( cataSet.isHashSharding() )
      {
         _shardingType = CAT_SHARDING_TYPE_HASH ;
      }

      // calc the end key
      BSONObj groupUpBound ;
      BSONObj allUpbound ;
      rc = cataSet.getGroupUpBound( sourceID, groupUpBound ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group up bound, rc: %d", rc ) ;

      rc = cataSet.getGroupUpBound( 0, allUpbound ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all up bound, rc: %d", rc ) ;

      // bKey can't empty
      PD_CHECK( !_splitKeyObj.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                "Split begin key can't be empty" ) ;

      // check begin valid
      if ( cataSet.isHashSharding() )
      {
         PD_CHECK( bKey.firstElement().numberInt() <
                   groupUpBound.firstElement().numberInt() &&
                   bKey.firstElement().numberInt() >= 0, SDB_INVALIDARG,
                   error, PDERROR, "Init split task failed, catalog info: %s, "
                   "source group id: %d, bKey: %s",
                   cataSet.toCataInfoBson().toString().c_str(), sourceID,
                   bKey.toString().c_str() ) ;
      }
      else
      {
         PD_CHECK( bKey.woCompare( groupUpBound, _shardingKey, false ) < 0,
                   SDB_INVALIDARG, error, PDERROR, "Init split task failed, "
                   "catalog info : %s, source group id: %d, bKey: %s",
                   cataSet.toCataInfoBson().toString().c_str(), sourceID,
                   bKey.toString().c_str() ) ;
      }

      // calc eKey
      if ( _splitEndKeyObj.isEmpty() )
      {
         _splitEndKeyObj = groupUpBound.getOwned() ;
      }

      if ( 0 == _splitEndKeyObj.woCompare( allUpbound, BSONObj(), false ) )
      {
         _splitEndKeyObj = BSONObj() ;
      }

      // make sure eKey > bKey
      if ( !_splitEndKeyObj.isEmpty() )
      {
         if ( cataSet.isHashSharding() )
         {
            PD_CHECK( _splitKeyObj.firstElement().numberInt() <
                      _splitEndKeyObj.firstElement().numberInt(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Split begin key must less than end key" ) ;
         }
         else
         {
            PD_CHECK( _splitKeyObj.woCompare( _splitEndKeyObj, _shardingKey,
                      false ) < 0, SDB_INVALIDARG, error, PDERROR,
                      "Split begin key must less than end key" ) ;
         }

         /*if ( !isHashSharding() &&
              0 == _splitEndKeyObj.woCompare( groupUpBound, BSONObj(), false ) )
         {
            _lockEnd = TRUE ;
         }*/
      }

      _makeName () ;

   done:
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLITTK_INIT, "_clsSplitTask::init" )
   INT32 _clsSplitTask::init ( const CHAR * objdata )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLITTK_INIT ) ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      try
      {
         BSONObj jobObj ( objdata ) ;
         PD_LOG ( PDDEBUG, "Split job: %s", jobObj.toString().c_str() ) ;

         BSONElement ele = jobObj.getField ( CAT_TASKTYPE_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_TASKTYPE_NAME,
                    jobObj.toString().c_str() ) ;
         _taskType = ( CLS_TASK_TYPE )ele.numberInt () ;

         ele = jobObj.getField ( CAT_SPLITPERCENT_NAME ) ;
         PD_CHECK ( ele.isNumber() || ele.eoo(), SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SPLITPERCENT_NAME,
                    jobObj.toString().c_str() ) ;
         _percent = ele.numberDouble() ;

         ele = jobObj.getField ( CAT_STATUS_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_STATUS_NAME,
                    jobObj.toString().c_str() ) ;
         _status = ( CLS_TASK_STATUS )ele.numberInt () ;

         ele = jobObj.getField ( FIELD_NAME_RESULTCODE ) ;
         PD_CHECK ( ele.type() == NumberInt || ele.eoo(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in split task[%s]",
                    FIELD_NAME_RESULTCODE, jobObj.toString().c_str() ) ;
         _resultCode = ele.numberInt () ;

         ele = jobObj.getField ( FIELD_NAME_ENDTIMESTAMP ) ;
         PD_CHECK ( ele.type() == String || ele.eoo(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    FIELD_NAME_ENDTIMESTAMP, jobObj.toString().c_str() ) ;
         ossStringToTimestamp( ele.valuestr(), _endTS ) ;

         ele = jobObj.getField ( CAT_COLLECTION_NAME ) ;
         PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]",
                    CAT_COLLECTION_NAME, jobObj.toString().c_str() ) ;
         _clFullName = ele.str() ;

         ele = jobObj.getField ( CAT_TARGET_NAME ) ;
         PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_TARGET_NAME,
                    jobObj.toString().c_str() ) ;
         _dstName = ele.str() ;

         ele = jobObj.getField ( CAT_SOURCE_NAME ) ;
         PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SOURCE_NAME,
                    jobObj.toString().c_str() ) ;
         _sourceName = ele.str() ;

         ele = jobObj.getField ( CAT_SOURCEID_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SOURCEID_NAME,
                    jobObj.toString().c_str() ) ;
         _sourceID = ele.numberInt () ;

         ele = jobObj.getField ( CAT_TARGETID_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_TARGETID_NAME,
                    jobObj.toString().c_str() ) ;
         _dstID = ele.numberInt () ;

         ele = jobObj.getField ( CAT_SPLITVALUE_NAME ) ;
         PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SPLITVALUE_NAME,
                    jobObj.toString().c_str() ) ;
         _splitKeyObj = ele.embeddedObject().getOwned () ;

         ele = jobObj.getField( CAT_SPLITENDVALUE_NAME ) ;
         PD_CHECK( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "Field[%s] invalid in split task[%s]",
                   CAT_SPLITENDVALUE_NAME, jobObj.toString().c_str() ) ;
         _splitEndKeyObj = ele.embeddedObject().getOwned() ;

         ele = jobObj.getField( CAT_SHARDINGKEY_NAME ) ;
         PD_CHECK( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "Field[%s] invalid in split task[%s]", CAT_SHARDINGKEY_NAME,
                   jobObj.toString().c_str() ) ;
         _shardingKey = ele.embeddedObject().getOwned() ;

         ele = jobObj.getField( CAT_SHARDING_TYPE ) ;
         PD_CHECK( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                   "Field[%s] invalid in split task[%s]", CAT_SHARDING_TYPE,
                   jobObj.toString().c_str() ) ;
         _shardingType = ele.str() ;

         /*ele = jobObj.getField( CLS_SPLIT_TASK_LOCK_END ) ;
         if ( ele.eoo() )
         {
            _lockEnd = FALSE ;
         }
         else
         {
            PD_CHECK( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] invalid in split task[%s]",
                      CLS_SPLIT_TASK_LOCK_END, jobObj.toString().c_str() ) ;
            _lockEnd = ele.numberInt() ;
         }*/

         ele = jobObj.getField( CAT_CL_UNIQUEID ) ;
         if ( ele.eoo() )
         {
            _clUniqueID = UTIL_UNIQUEID_NULL ;
         }
         else
         {
            PD_CHECK( ele.type() == NumberLong, SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] invalid in split task[%s]",
                      CAT_CL_UNIQUEID, jobObj.toString().c_str() ) ;
            _clUniqueID = (utilCLUniqueID)ele.numberLong() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Init split task occur expection: %s", e.what () ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _makeName() ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSSPLITTK_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BSONObj _clsSplitTask::toBson( UINT32 mask )
   {
      BSONObjBuilder builder ;

      if ( mask & CLS_SPLIT_MASK_ID )
      {
         builder.append( CAT_TASKID_NAME, (INT64)_taskID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_TYPE )
      {
         builder.append( CAT_TASKTYPE_NAME, (INT32)_taskType ) ;
      }
      if ( mask & CLS_SPLIT_MASK_STATUS )
      {
         builder.append( CAT_STATUS_NAME, (INT32)_status ) ;
      }
      if ( mask & CLS_SPLIT_MASK_RESULTCODE )
      {
         builder.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
      }
      if ( mask & CLS_SPLIT_MASK_CLNAME )
      {
         builder.append( CAT_COLLECTION_NAME, _clFullName ) ;
      }
      if ( mask & CLS_SPLIT_MASK_UNIQUEID )
      {
         builder.append( CAT_CL_UNIQUEID, (INT64)_clUniqueID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SOURCEID )
      {
         builder.append( CAT_SOURCEID_NAME, _sourceID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SOURCENAME )
      {
         builder.append( CAT_SOURCE_NAME, _sourceName ) ;
      }
      if ( mask & CLS_SPLIT_MASK_DSTID )
      {
         builder.append( CAT_TARGETID_NAME, _dstID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_DSTNAME )
      {
         builder.append( CAT_TARGET_NAME, _dstName ) ;
      }
      if ( mask & CLS_SPLIT_MASK_BKEY )
      {
         builder.append( CAT_SPLITVALUE_NAME, _splitKeyObj ) ;
      }
      if ( mask & CLS_SPLIT_MASK_EKEY )
      {
         builder.append( CAT_SPLITENDVALUE_NAME, _splitEndKeyObj ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SHARDINGKEY )
      {
         builder.append( CAT_SHARDINGKEY_NAME, _shardingKey ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SHARDINGTYPE )
      {
         builder.append( CAT_SHARDING_TYPE, _shardingType ) ;
      }
      if ( mask & CLS_SPLIT_MASK_PERCENT )
      {
         builder.append( CAT_SPLITPERCENT_NAME, _percent ) ;
      }
      /*if ( mask & CLS_SPLIT_MASK_LOCKEND )
      {
         builder.append( CLS_SPLIT_TASK_LOCK_END, _lockEnd ) ;
      }*/
      CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      if ( mask & CLS_SPLIT_MASK_ENDTIMESTAMP )
      {
         if ( CLS_TASK_STATUS_FINISH == _status )
         {
            ossTimestampToString( _endTS, timeStr ) ;
         }
         builder.append( FIELD_NAME_ENDTIMESTAMP, timeStr ) ;
      }

      return builder.obj() ;
   }

   INT32 _clsSplitTask::calcHashPartition( clsCatalogSet & cataSet,
                                           INT32 groupID, FLOAT64 percent,
                                           BSONObj & bKey, BSONObj & eKey )
   {
      INT32 rc = SDB_OK ;
      INT32 totalNum = 0 ;
      INT32 rangeNum = 0 ;
      INT32 splitNum = 0 ;
      clsCatalogItem *cataItem = NULL ;
      clsCatalogSet::POSITION pos ;

      if ( !cataSet.isHashSharding() )
      {
         rc = SDB_CLS_SHARDING_NOT_HASH ;
         goto error ;
      }

      // calc all partition number
      pos = cataSet.getFirstItem() ;
      while ( NULL != ( cataItem = cataSet.getNextItem( pos ) ) )
      {
         if ( cataItem->getGroupID() != (UINT32)groupID )
         {
            continue ;
         }
         totalNum += ( cataItem->getUpBound().firstElement().numberInt() -
                       cataItem->getLowBound().firstElement().numberInt() ) ;
      }

      PD_CHECK( totalNum > 0, SDB_SYS, error, PDERROR,
                "Catalog Info[%s] error, group id: %d",
                cataSet.toCataInfoBson().toString().c_str(), groupID ) ;

      splitNum = totalNum * ( percent / 100.0 ) ;

      PD_CHECK( splitNum > 0 && splitNum <= totalNum,
                SDB_CLS_SPLIT_PERCENT_LOWER, error, PDERROR,
                "Calc hash split failed, catalog info: %s, group id: %d, "
                "split num: %d, total num: %d, percent: %f",
                cataSet.toCataInfoBson().toString().c_str(), groupID,
                splitNum, totalNum, percent ) ;

      // find the begin key
      splitNum = totalNum - splitNum ;
      pos = cataSet.getFirstItem() ;
      while ( NULL != ( cataItem = cataSet.getNextItem( pos ) ) )
      {
         if ( cataItem->getGroupID() != (UINT32)groupID )
         {
            continue ;
         }
         rangeNum =  cataItem->getUpBound().firstElement().numberInt() -
                     cataItem->getLowBound().firstElement().numberInt() ;

         if ( rangeNum <= splitNum )
         {
            splitNum -= rangeNum ;
         }
         else
         {
            INT32 rangeBegin =
               cataItem->getLowBound().firstElement().numberInt() ;
            bKey = BSON( "" << ( splitNum + rangeBegin ) ) ;
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _clsSplitTask::taskName () const
   {
      return _taskName.c_str() ;
   }

   const CHAR* _clsSplitTask::collectionName() const
   {
      return _clFullName.c_str() ;
   }

   const CHAR* _clsSplitTask::collectionSpaceName() const
   {
      return _csName.c_str() ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLITTK_MXON, "_clsSplitTask::muteXOn" )
   BOOLEAN _clsSplitTask::muteXOn ( const _clsTask * pOther )
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLITTK_MXON ) ;
      BOOLEAN ret = FALSE ;
      if ( taskType() != pOther->taskType() )
      {
         goto done ;
      }
      {
         _clsSplitTask *pOtherSplit = ( _clsSplitTask*)pOther ;

         if ( 0 != ossStrcmp ( clFullName (), pOtherSplit->clFullName () ) )
         {
            goto done ;
         }
         /*else if ( ( splitEndKeyObj().isEmpty() ||
                     pOtherSplit->splitEndKeyObj().isEmpty() ) &&
                   ( sourceID() == pOtherSplit->sourceID() ||
                     sourceID() == pOtherSplit->dstID() ||
                     dstID() == pOtherSplit->sourceID() ||
                     dstID() == pOtherSplit->dstID() ) )
         {
            ret = TRUE ;
            goto done ;
         }*/
         /*else if ( pOtherSplit->dstID() == sourceID() )
         {
            ret = TRUE ;
            goto done ;
         }*/
         else
         {
            INT32 beginResult = splitKeyObj().woCompare(
                                              pOtherSplit->splitKeyObj(),
                                              _getOrdering(), false ) ;
            if ( 0 == beginResult )
            {
               ret = TRUE ;
               goto done ;
            }
            else if ( beginResult < 0 &&
                      ( splitEndKeyObj().woCompare( pOtherSplit->splitKeyObj(),
                                                 _getOrdering(), false ) > 0 ||
                        splitEndKeyObj().isEmpty() ) )
            {
               ret = TRUE ;
               goto done ;
            }
            else if ( beginResult > 0 &&
                      ( splitKeyObj().woCompare( pOtherSplit->splitEndKeyObj(),
                                                 _getOrdering(), false ) < 0 ||
                        pOtherSplit->splitEndKeyObj().isEmpty() ) )
            {
               ret = TRUE ;
               goto done ;
            }
            // lock end
            /*else if ( _lockEnd && beginResult < 0 )
            {
               ret = TRUE ;
               goto done ;
            }*/
         }
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSSPLITTK_MXON ) ;
      return ret ;
   }

   const CHAR* _clsSplitTask::clFullName () const
   {
      return _clFullName.c_str() ;
   }

   utilCLUniqueID _clsSplitTask::clUniqueID () const
   {
      return _clUniqueID ;
   }

   const CHAR* _clsSplitTask::shardingType () const
   {
      return _shardingType.c_str() ;
   }

   const CHAR* _clsSplitTask::sourceName () const
   {
      return _sourceName.c_str() ;
   }

   const CHAR* _clsSplitTask::dstName () const
   {
      return _dstName.c_str() ;
   }

   UINT32 _clsSplitTask::sourceID () const
   {
      return _sourceID ;
   }

   UINT32 _clsSplitTask::dstID () const
   {
      return _dstID ;
   }

   BSONObj _clsSplitTask::splitKeyObj () const
   {
      return _splitKeyObj ;
   }

   BSONObj _clsSplitTask::splitEndKeyObj () const
   {
      return _splitEndKeyObj ;
   }

   BSONObj _clsSplitTask::shardingKey () const
   {
      return _shardingKey ;
   }

   BOOLEAN _clsSplitTask::isHashSharding () const
   {
      if ( 0 == ossStrcmp( shardingType(), FIELD_NAME_SHARDTYPE_HASH ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BSONObj _clsSplitTask::_getOrdering () const
   {
      if ( !isHashSharding () )
      {
         return _shardingKey ;
      }
      return BSONObj() ;
   }

   /*
      _clsSubTaskUnit : implement
   */
   INT32 _clsSubTaskUnit::init( const CHAR* objdata )
   {
      INT32 rc = SDB_OK ;
      BSONElement e ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj obj ( objdata ) ;
         BSONObjIterator i( obj ) ;
         while ( i.more() )
         {
            BSONElement e = i.next();

            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TASKID ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TASKID, obj.toString().c_str() ) ;
               taskID = (UINT64)e.numberLong() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TASKTYPE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TASKTYPE, obj.toString().c_str() ) ;
               taskType = (CLS_TASK_TYPE)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_STATUS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_STATUS, obj.toString().c_str() ) ;
               status = (CLS_TASK_STATUS)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RESULTCODE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RESULTCODE, obj.toString().c_str() ) ;
               resultCode = e.numberInt() ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _clsIdxTaskGroupUnit : implement
   */
   INT32 _clsIdxTaskGroupUnit::init( const CHAR* objdata )
   {
      INT32 rc = SDB_OK ;
      BSONElement e ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj obj( objdata ) ;
         BSONObjIterator i( obj ) ;
         while ( i.more() )
         {
            BSONElement e = i.next();

            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_GROUPNAME ) )
            {
               PD_CHECK ( e.type() == String, SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_GROUPNAME, obj.toString().c_str() ) ;
               groupName = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TASKID ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TASKID, obj.toString().c_str() ) ;
               taskID = (UINT64)e.numberLong() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_STATUS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_STATUS, obj.toString().c_str() ) ;
               status = (CLS_TASK_STATUS)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_PROGRESS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_PROGRESS, obj.toString().c_str() ) ;
               progress = (UINT32)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_SPEED ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_SPEED, obj.toString().c_str() ) ;
               speed = e.numberDouble() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TIMESPENT ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TIMESPENT, obj.toString().c_str() ) ;
               timeSpent = e.numberDouble() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TIMELEFT ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TIMELEFT, obj.toString().c_str() ) ;
               timeLeft = e.numberDouble() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RESULTCODE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RESULTCODE, obj.toString().c_str() ) ;
               resultCode = e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RESULTINFO ) )
            {
               PD_CHECK ( e.type() == Object, SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RESULTINFO, obj.toString().c_str() ) ;
               resultInfo = e.embeddedObject() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_OPINFO ) )
            {
               PD_CHECK ( e.type() == String, SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_OPINFO, obj.toString().c_str() ) ;
               opInfo = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RETRY_COUNT ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RETRY_COUNT, obj.toString().c_str() ) ;
               retryCnt = e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TOTAL_SIZE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TOTAL_SIZE, obj.toString().c_str() ) ;
               totalSize = e.numberLong() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_PROCESSED_SIZE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_PROCESSED_SIZE, obj.toString().c_str() ) ;
               processedSize = e.numberLong() ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   #define CLS_BYTE_TO_MBYTE     ( 1048576.0 )
   #define CLS_TASK_PROGRESS_100 ( 100 )

   /*
      _clsIdxTask : implement
   */
   _clsIdxTask::_clsIdxTask ( UINT64 taskID )
   : _clsTask ( taskID ),
     _progress( 0 ), _speed( 0.0 ), _timeSpent( 0.0 ), _timeLeft( 0.0 ),
     _totalGroups( 0 ), _succeededGroups( 0 ), _failedGroups( 0 ),
     _totalTasks( 0 ), _succeededTasks( 0 ), _failedTasks( 0 ),
     _changedMask( 0 ), _changedGroupMask( 0 ), _changedTaskMask( 0 )
   {
      ossMemset( _clFullName, 0, DMS_COLLECTION_FULL_NAME_SZ + 1 ) ;
      ossMemset( _csName,     0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _indexName,  0, IXM_INDEX_NAME_SIZE + 1 ) ;
      ossGetCurrentTime( _createTS ) ;
   }

   void _clsIdxTask::setStatus( CLS_TASK_STATUS status )
   {
      if ( _status == status )
      {
         return ;
      }
      _clsTask::setStatus( status ) ;
      _changedMask |= CLS_IDX_MASK_STATUS ;
   }

   const CHAR* _clsIdxTask::taskName () const
   {
      return _taskName.c_str() ;
   }

   const CHAR* _clsIdxTask::collectionName() const
   {
      return _clFullName ;
   }

   const CHAR* _clsIdxTask::collectionSpaceName() const
   {
      return _csName ;
   }

   const CHAR* _clsIdxTask::indexName() const
   {
      return _indexName ;
   }

   const BSONObj& _clsIdxTask::resultInfo() const
   {
      return _resultInfo ;
   }

   void _clsIdxTask::setBeginTimestamp()
   {
      ossGetCurrentTime( _beginTS ) ;
      _changedMask |= CLS_IDX_MASK_BEGINTIME ;
   }

   void _clsIdxTask::setEndTimestamp()
   {
      ossGetCurrentTime( _endTS ) ;
      _changedMask |= CLS_IDX_MASK_ENDTIME ;
   }

   void _clsIdxTask::setRun()
   {
      if ( CLS_TASK_STATUS_RUN == _status )
      {
         return ;
      }
      setStatus( CLS_TASK_STATUS_RUN ) ;
      setBeginTimestamp() ;
   }

   void _clsIdxTask::setFinish( INT32 resultCode, const BSONObj &resultInfo )
   {
      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         return ;
      }

      // timestamp
      if ( 0 == _beginTS.time && 0 == _beginTS.microtm )
      {
         setBeginTimestamp() ;
      }
      setEndTimestamp() ;

      // result
      _resultCode = resultCode ;
      _resultInfo = resultInfo.getOwned() ;
      _changedMask |= CLS_IDX_MASK_RESULT ;

      // status
      setStatus( CLS_TASK_STATUS_FINISH ) ;
   }

   void _clsIdxTask::_calculate()
   {
      /*
      * Update TimeSpent / TimeLeft / Speed / Progress field
      */
      INT64 allTotalSize = 0 ;           // unit: B
      INT64 allProcessSize = 0 ;         // unit: B
      UINT32 timeLeft = 0 ;

      _changedMask |= CLS_IDX_MASK_PROGRESS ;

      // timeLeft, find the largest value
      for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
           it != _mapGroupInfo.end() ;
           it++ )
      {
         const _clsIdxTaskGroupUnit& groupUnit = it->second ;
         allTotalSize += groupUnit.totalSize ;
         allProcessSize += groupUnit.processedSize ;
         if ( groupUnit.timeLeft > timeLeft )
         {
            timeLeft = groupUnit.timeLeft ;
         }
      }
      _timeLeft = timeLeft ;

      // timeSpent
      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         UINT64 beginMs = _beginTS.time * 1000000 + _beginTS.microtm  ;
         UINT64 endMs = _endTS.time * 1000000 + _endTS.microtm  ;
         _timeSpent = ( endMs - beginMs ) / 1000000.0 ;
      }
      else
      {
         ossTimestamp currentTS ;
         ossGetCurrentTime( currentTS ) ;
         UINT64 beginMs = _beginTS.time * 1000000 + _beginTS.microtm  ;
         UINT64 currentMs = currentTS.time * 1000000 + currentTS.microtm  ;
         _timeSpent = ( currentMs - beginMs ) / 1000000.0 ;
      }

      // progress
      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         _progress = CLS_TASK_PROGRESS_100 ;
      }
      else
      {
         if ( allTotalSize != 0 )
         {
            _progress = (FLOAT32)allProcessSize / allTotalSize * 100 ;
         }
         else
         {
            _progress = 0 ;
         }
         if ( CLS_TASK_PROGRESS_100 == _progress )
         {
            // the task is not finished yet.
            _progress = 99 ;
         }
      }

      // speed
      if ( _timeSpent != 0 )
      {
         _speed = allProcessSize / CLS_BYTE_TO_MBYTE / _timeSpent ;
      }
      else
      {
         _speed = 0.0 ;
      }
   }

   INT32 _clsIdxTask::init ( const CHAR *objdata )
   {
      INT32 rc = SDB_OK ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {

      BSONObj obj ( objdata ) ;

      BSONObjIterator i( obj ) ;
      while ( i.more() )
      {
         BSONElement ele = i.next();

         // base fields
         if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TASKTYPE ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_TASKTYPE,
                       obj.toString().c_str() ) ;
            _taskType = (CLS_TASK_TYPE)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_STATUS ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_STATUS,
                       obj.toString().c_str() ) ;
            _status = (CLS_TASK_STATUS)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_COLLECTION_NAME ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", CAT_COLLECTION_NAME,
                       obj.toString().c_str() ) ;
            ossStrncpy( _clFullName, ele.valuestr(),
                        DMS_COLLECTION_FULL_NAME_SZ );
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INDEXNAME ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_INDEXNAME, obj.toString().c_str() ) ;
            ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;
         }
         // result
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_RESULTCODE ) )
         {
            PD_CHECK ( ele.isNumber() , SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_RESULTCODE, obj.toString().c_str() ) ;
            _resultCode = ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_RESULTINFO ) )
         {
            PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_RESULTINFO, obj.toString().c_str() ) ;
            _resultInfo = ele.embeddedObject() ;
         }
         // main task
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MAIN_TASKID ) )
         {
            PD_CHECK ( ele.isNumber() , SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_MAIN_TASKID, obj.toString().c_str() ) ;
            _mainTaskID = (UINT64)ele.numberLong() ;
         }
         // group count
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALGROUP ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_TOTALGROUP, obj.toString().c_str() ) ;
            _totalGroups = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUCCEEDGROUP ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_SUCCEEDGROUP, obj.toString().c_str() ) ;
            _succeededGroups = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_FAILGROUP ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_FAILGROUP, obj.toString().c_str() ) ;
            _failedGroups = (UINT32)ele.numberInt() ;
         }
         // task count
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALTASKS ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_TOTALTASKS, obj.toString().c_str() ) ;
            _totalTasks = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUCCEEDTASKS ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_SUCCEEDTASKS, obj.toString().c_str() ) ;
            _succeededTasks = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_FAILTASKS ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_FAILTASKS, obj.toString().c_str() ) ;
            _failedTasks = (UINT32)ele.numberInt() ;
         }
         // progress info
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_PROGRESS ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_PROGRESS,
                       obj.toString().c_str() ) ;
            _progress = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SPEED ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_SPEED,
                       obj.toString().c_str() ) ;
            _speed = ele.numberDouble() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TIMESPENT ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_TIMESPENT,
                       obj.toString().c_str() ) ;
            _timeSpent = ele.numberDouble() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TIMELEFT ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_TIMELEFT,
                       obj.toString().c_str() ) ;
            _timeLeft = ele.numberDouble() ;
         }
         // timestamp
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FIELD_NAME_CREATETIMESTAMP ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_CREATETIMESTAMP, obj.toString().c_str() ) ;
            if ( 0 != ossStrcmp( ele.valuestrsafe(), "" ) )
            {
               ossStringToTimestamp( ele.valuestrsafe(), _createTS ) ;
            }
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FIELD_NAME_BEGINTIMESTAMP ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_BEGINTIMESTAMP, obj.toString().c_str() ) ;
            if ( 0 != ossStrcmp( ele.valuestrsafe(), "" ) )
            {
               ossStringToTimestamp( ele.valuestrsafe(), _beginTS ) ;
            }
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_ENDTIMESTAMP ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_ENDTIMESTAMP, obj.toString().c_str() ) ;
            if ( 0 != ossStrcmp( ele.valuestrsafe(), "" ) )
            {
               ossStringToTimestamp( ele.valuestrsafe(), _endTS ) ;
            }
         }
         // Groups
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_GROUPS_NAME ) )
         {
            PD_CHECK ( ele.type() == Array, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       CAT_GROUPS_NAME, obj.toString().c_str() ) ;

            // Groups: [ { GroupName: xxx, Status: xxx, ... }, ... ]
            BSONObjIterator it( ele.embeddedObject() ) ;
            while( it.more() )
            {
               BSONElement gEle = it.next() ;
               PD_CHECK( gEle.type() == Object , SDB_INVALIDARG, error,
                         PDERROR, "Field[%s] invalid in task[%s]",
                         CAT_GROUPS_NAME, obj.toString().c_str() ) ;

               _clsIdxTaskGroupUnit oneGroup ;
               rc = oneGroup.init( gEle.Obj().objdata() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to init group info, rc: %d",
                            rc ) ;

               _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
            }
         }
         // SubTasks
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUBTASKS ) )
         {
            PD_CHECK ( ele.type() == Array, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_SUBTASKS, obj.toString().c_str() ) ;
            _isMainTask = TRUE ;

            // "SubTasks" : [ { TaskID: xx, Status: xx, ... }, ... ]
            BSONObjIterator it( ele.embeddedObject() ) ;
            while( it.more() )
            {
               BSONElement sEle = it.next() ;
               PD_CHECK( sEle.type() == Object, SDB_INVALIDARG, error,
                         PDERROR, "Field invalid in task[%s]",
                         obj.toString().c_str() ) ;

               _clsSubTaskUnit oneSubTask ;
               rc = oneSubTask.init( sEle.Obj().objdata() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to init sub-task info, rc: %d",
                            rc ) ;

               _mapSubTask[ oneSubTask.taskID ] = oneSubTask ;
            }
         }
      }

      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = _init( objdata ) ;
      if ( rc )
      {
         goto error ;
      }

      _makeName() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _clsIdxTask::toBson( UINT32 mask )
   {
      BSONObjBuilder builder ;

      SDB_ASSERT( (UINT32)CLS_MASK_ALL == mask, "Invalid mask" ) ;

      try
      {

      // task base
      builder.append( FIELD_NAME_TASKID, (INT64)_taskID ) ;
      if ( hasMainTask() )
      {
         builder.append( FIELD_NAME_MAIN_TASKID, (INT64)_mainTaskID ) ;
      }

      builder.append( FIELD_NAME_TASKTYPE,     (INT32)_taskType ) ;
      builder.append( FIELD_NAME_TASKTYPEDESC, clsTaskTypeStr( _taskType ) ) ;
      builder.append( FIELD_NAME_STATUS,       (INT32)_status ) ;
      builder.append( FIELD_NAME_STATUSDESC,   clsTaskStatusStr( _status ) ) ;

      // collection index
      builder.append( CAT_COLLECTION_NAME, _clFullName ) ;
      if ( indexName() )
      {
         builder.append( FIELD_NAME_INDEXNAME, indexName() ) ;
      }

      // result
      builder.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
      builder.append( FIELD_NAME_RESULTCODEDESC,
                      CLS_TASK_STATUS_FINISH == _status ?
                      getErrDesp( _resultCode ) : "" ) ;
      builder.append( FIELD_NAME_RESULTINFO, _resultInfo ) ;

      // progress
      builder.append( FIELD_NAME_PROGRESS,  _progress ) ;
      builder.append( FIELD_NAME_SPEED,     _speed ) ;
      builder.append( FIELD_NAME_TIMESPENT, _timeSpent ) ;
      builder.append( FIELD_NAME_TIMELEFT,  _timeLeft ) ;

      // groups info
      BSONArrayBuilder arr( builder.subarrayStart( CAT_GROUPS_NAME ) ) ;
      for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
           it != _mapGroupInfo.end() ; ++it )
      {
         const _clsIdxTaskGroupUnit& group = it->second ;
         BSONObjBuilder b ;
         b.append( FIELD_NAME_GROUPNAME,  group.groupName.c_str() ) ;
         b.append( FIELD_NAME_STATUS,     group.status ) ;
         b.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(group.status) ) ;
         b.append( FIELD_NAME_RESULTCODE, group.resultCode ) ;
         b.append( FIELD_NAME_RESULTCODEDESC,
                   CLS_TASK_STATUS_FINISH == group.status ?
                   getErrDesp( group.resultCode ) : "" ) ;
         b.append( FIELD_NAME_RESULTINFO,      group.resultInfo ) ;
         b.append( FIELD_NAME_OPINFO,          group.opInfo.c_str() ) ;
         b.append( FIELD_NAME_RETRY_COUNT,     group.retryCnt ) ;
         b.append( FIELD_NAME_PROGRESS,        group.progress ) ;
         b.append( FIELD_NAME_SPEED,           group.speed ) ;
         b.append( FIELD_NAME_TIMESPENT,       group.timeSpent ) ;
         b.append( FIELD_NAME_TIMELEFT,        group.timeLeft ) ;
         b.append( FIELD_NAME_TOTAL_SIZE,      group.totalSize ) ;
         b.append( FIELD_NAME_PROCESSED_SIZE,  group.processedSize ) ;
         arr.append( b.obj() ) ;
      }
      arr.done() ;

      builder.append( FIELD_NAME_TOTALGROUP,   _totalGroups ) ;
      builder.append( FIELD_NAME_SUCCEEDGROUP, _succeededGroups ) ;
      builder.append( FIELD_NAME_FAILGROUP,    _failedGroups ) ;

      // sub-task info
      if ( _isMainTask )
      {
         builder.appendBool( FIELD_NAME_IS_MAINTASK, _isMainTask ) ;

         BSONArrayBuilder arr( builder.subarrayStart( FIELD_NAME_SUBTASKS ) ) ;
         for( MAP_SUBTASK_IT it = _mapSubTask.begin() ;
              it != _mapSubTask.end() ; it++ )
         {
            clsSubTaskUnit subTask = it->second ;
            BSONObjBuilder b ;
            b.append( FIELD_NAME_TASKID,     (INT64)subTask.taskID ) ;
            b.append( FIELD_NAME_TASKTYPE,   (INT32)subTask.taskType ) ;
            b.append( FIELD_NAME_STATUS,     (INT32)subTask.status ) ;
            b.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(subTask.status) ) ;
            b.append( FIELD_NAME_RESULTCODE, subTask.resultCode ) ;
            arr.append( b.obj() ) ;
         }
         arr.done() ;

         builder.append( FIELD_NAME_TOTALTASKS,   _totalTasks ) ;
         builder.append( FIELD_NAME_SUCCEEDTASKS, _succeededTasks ) ;
         builder.append( FIELD_NAME_FAILTASKS,    _failedGroups ) ;
      }

      // timestamp
      CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      ossTimestampToString( _createTS, timeStr ) ;
      builder.append( FIELD_NAME_CREATETIMESTAMP, timeStr ) ;

      if ( _status >= CLS_TASK_STATUS_RUN )
      {
         ossTimestampToString( _beginTS, timeStr ) ;
      }
      else
      {
         timeStr[0] = 0 ;
      }
      builder.append( FIELD_NAME_BEGINTIMESTAMP, timeStr ) ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         ossTimestampToString( _endTS, timeStr ) ;
      }
      else
      {
         timeStr[0] = 0 ;
      }
      builder.append( FIELD_NAME_ENDTIMESTAMP, timeStr ) ;

      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      _toBson( builder ) ;

      return builder.obj() ;
   }

   void _clsIdxTask::_makeName()
   {
      // set cs name by cl name
      INT32 i = 0 ;
      while ( _clFullName[ i ] && i < DMS_COLLECTION_SPACE_NAME_SZ )
      {
         if ( '.' == _clFullName[ i ] )
         {
            break ;
         }
         _csName[ i ] = _clFullName[ i ] ;
         ++i ;
      }

      // set task name
      if ( CLS_TASK_CREATE_IDX == _taskType )
      {
         _taskName = "Create index-" ;
      }
      else if ( CLS_TASK_DROP_IDX == _taskType )
      {
         _taskName = "Drop index-" ;
      }
      else if ( CLS_TASK_COPY_IDX == _taskType )
      {
         _taskName = "Copy index-" ;
      }
      else
      {
         _taskName = "Index-" ;
      }
      _taskName += _clFullName ;
      _taskName += "[" ;
      _taskName += _indexName ;
      _taskName += "]" ;
   }

   BOOLEAN _clsIdxTask::muteXOn ( const _clsTask* pOther )
   {
      BOOLEAN ret = FALSE ;
      _clsIdxTask *pOtherIdx = NULL ;

      if ( pOther->taskType() != CLS_TASK_CREATE_IDX &&
           pOther->taskType() != CLS_TASK_DROP_IDX )
      {
         goto done ;
      }

      pOtherIdx = (clsIdxTask*)pOther ;

      if ( pOtherIdx->isMainTask() )
      {
         goto done ;
      }

      if ( 0 != ossStrcmp ( collectionName(), pOtherIdx->collectionName() ) )
      {
         goto done ;
      }
      if ( 0 != ossStrcmp ( indexName(), pOtherIdx->indexName() ) )
      {
         goto done ;
      }

      ret = TRUE ;

   done :
      return ret ;
   }

   INT32 _clsIdxTask::countGroup()
   {
      return _mapGroupInfo.size() ;
   }

   INT32 _clsIdxTask::countSubTask()
   {
      return _mapSubTask.size() ;
   }

   INT32 _clsIdxTask::querySubTasks( const CHAR* objdata,
                                     BSONObj& matcher,
                                     BSONObj& selector )
   {
      INT32 rc = SDB_OK ;
      const CHAR* groupName = NULL ;

      try
      {
         if ( objdata )
         {
            BSONObj obj( objdata ) ;
            BSONElement ele = obj.getField( FIELD_NAME_GROUPNAME ) ;
            PD_CHECK( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] invalid in obj[%s]",
                      FIELD_NAME_GROUPNAME, obj.toString().c_str() ) ;
            groupName = ele.valuestr() ;

            // matcher : { MainTaskID: 2, "Groups.GroupName": 'db2' }
            matcher = BSON( FIELD_NAME_MAIN_TASKID << (INT64)_taskID <<
                            FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME <<
                            groupName ) ;

            // selector: { "Groups": { "$include": 1,
            //                         "$elemMatch": { "GroupName": "db2" } } }
            selector = BSON( FIELD_NAME_GROUPS <<
                             BSON( "$include" << 1 <<
                                   "$elemMatch" << BSON( FIELD_NAME_GROUPNAME <<
                                                         groupName ) ) ) ;
         }
         else
         {
            matcher = BSON( FIELD_NAME_MAIN_TASKID << (INT64)_taskID ) ;
            selector = BSONObj() ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIdxTask::getSubTasks( ossPoolVector<UINT64>& list )
   {
      SDB_ASSERT( _isMainTask, "should be main-task" ) ;
      INT32 rc = SDB_OK ;

      for ( MAP_SUBTASK_IT it = _mapSubTask.begin() ;
            it != _mapSubTask.end() ; ++it )
      {
         try
         {
            list.push_back( it->first ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

   done :
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIdxTask::removeSubTask( UINT64 taskID,
                                     const ossPoolVector<BSONObj>& otherSubTasks,
                                     BSONObj& updator,
                                     BSONObj& matcher )
   {
      SDB_ASSERT( _isMainTask, "should be main-task" ) ;

      INT32 rc = SDB_OK ;
      ossPoolMap< ossPoolString, ossPoolVector<BSONObj> > groupInfo ;
      ossPoolMap< ossPoolString, ossPoolVector<BSONObj> >::iterator groupIt ;

      _clearChangedMask() ;

      // erase SubTasks, decrease SucceededTasks of FailedTasks
      MAP_SUBTASK_IT it = _mapSubTask.find( taskID ) ;
      if ( _mapSubTask.end() == it )
      {
         goto done ;
      }

      if ( CLS_TASK_STATUS_FINISH == it->second.status )
      {
         if ( SDB_OK == it->second.resultCode )
         {
            _succeededTasks-- ;
         }
         else
         {
            _failedTasks-- ;
         }
      }
      _totalTasks-- ;
      _changedMask |= CLS_IDX_MASK_TASKCOUNT ;

      _mapSubTask.erase( it ) ;
      _changedMask |= CLS_IDX_MASK_SUBTASKS ;

      // build Groups
      for( ossPoolVector<BSONObj>::const_iterator it = otherSubTasks.begin() ;
           it != otherSubTasks.end() ; ++it )
      {
         BSONObj array ;
         rc = rtnGetArrayElement( *it, FIELD_NAME_GROUPS, array ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s] from obj[%s]",
                      FIELD_NAME_GROUPS, it->toString().c_str() ) ;

         BSONObjIterator i( array ) ;
         while( i.more() )
         {
            BSONElement e = i.next() ;
            PD_CHECK( e.type() == Object, SDB_INVALIDARG, error,
                      PDERROR, "Field invalid in task[%s]",
                      array.toString().c_str() ) ;

            BSONObj groupObj = e.Obj() ;
            const CHAR* groupName = NULL ;

            rc = rtnGetStringElement( groupObj, FIELD_NAME_GROUPNAME,
                                      &groupName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s] from obj[%s]",
                         FIELD_NAME_GROUPNAME, groupObj.toString().c_str() ) ;

            groupIt = groupInfo.find( groupName ) ;
            if ( groupInfo.end() == groupIt )
            {
               ossPoolVector<BSONObj> vec ;
               vec.push_back( groupObj ) ;
               groupInfo[groupName] = vec ;
            }
            else
            {
               ossPoolVector<BSONObj> &tmp = groupIt->second ;
               tmp.push_back( groupObj ) ;
            }
         }
      }

      _mapGroupInfo.clear() ;
      _changedMask |= CLS_IDX_MASK_GROUPS ;

      // count Groups
      for ( groupIt = groupInfo.begin() ; groupIt != groupInfo.end() ; groupIt++ )
      {
         const ossPoolVector<BSONObj>& groupVec = groupIt->second ;
         clsIdxTaskGroupUnit newGroup ;

         rc = _buildNewGroupInfo( groupVec, newGroup ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build new group info", rc ) ;

         _mapGroupInfo[ newGroup.groupName ] = newGroup ;
         if ( CLS_TASK_STATUS_FINISH == newGroup.status )
         {
            if ( SDB_OK == newGroup.resultCode )
            {
               _succeededGroups++ ;
            }
            else
            {
               _failedGroups++ ;
            }
         }
      }
      _totalGroups = _mapGroupInfo.size() ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;

      // other field
      _updateOtherBySubTaskInfo() ;

      rc = _toChangedObj( taskID, matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_UPDTASKINFO, "_clsIdxTask::updateTaskInfo" )
   INT32 _clsIdxTask::updateTaskInfo( const CHAR* objdata,
                                      BSONObj& updator,
                                      BSONObj& matcher )
   {
      SDB_ASSERT( !_isMainTask, "cann't be used by main-task" ) ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_UPDTASKINFO ) ;

      INT32 rc = SDB_OK ;
      clsIdxTaskGroupUnit newGroupInfo ;
      MAP_GROUP_INFO_IT it ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }

      /// init group unit
      rc = newGroupInfo.init( objdata ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init group info, rc: %d",
                   rc ) ;

      /// find out group unit from Groups List
      it = _mapGroupInfo.find( newGroupInfo.groupName ) ;
      PD_CHECK( it != _mapGroupInfo.end(), SDB_SYS, error, PDERROR,
                "Failed to find out group[%s] from task[%llu]",
                newGroupInfo.groupName.c_str(), _taskID ) ;

      if ( CLS_TASK_STATUS_FINISH == it->second.status &&
           newGroupInfo.resultCode == it->second.resultCode )
      {
         // 'finish + ok' can convert to 'finish + -243'
         PD_LOG( PDWARNING, "Group[%s] in task[%llu] has already finished",
                 newGroupInfo.groupName.c_str(), _taskID ) ;
         goto done ;
      }

      /// update info
      _clearChangedMask() ;

      _updateGroup( it->second, newGroupInfo ) ;

      _updateOtherByGroupInfo() ;

      rc = _toChangedObj( newGroupInfo, matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_UPDTASKINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_UPDMAINTASKINFO, "_clsIdxTask::updateMainTaskInfo" )
   INT32 _clsIdxTask::updateMainTaskInfo( clsTask *pSubTask,
                                          const ossPoolVector<BSONObj>& subTaskInfoList,
                                          BSONObj& updator,
                                          BSONObj& matcher )
   {
      SDB_ASSERT( pSubTask, "pSubTask cann't be null" ) ;
      SDB_ASSERT( _isMainTask, "should be used by main-task" ) ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_UPDMAINTASKINFO ) ;

      INT32 rc = SDB_OK ;
      BOOLEAN needChangeGroup = TRUE ;
      BOOLEAN needChangeSubtask = TRUE ;
      const clsIdxTask* pTask = NULL ;
      ossPoolMap<UINT64, clsSubTaskUnit>::iterator itTask ;
      MAP_GROUP_INFO_IT itGroup ;
      clsSubTaskUnit newSubTask ;
      clsIdxTaskGroupUnit newGroupInfo ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }

      pTask = dynamic_cast<const clsIdxTask*>( pSubTask ) ;
      PD_CHECK( NULL != pTask, SDB_INVALIDARG, error, PDERROR,
                "Failed to get index task" ) ;

      /// generate new sub-task info
      newSubTask.taskID     = pTask->taskID() ;
      newSubTask.taskType   = pTask->taskType() ;
      newSubTask.status     = pTask->status() ;
      newSubTask.resultCode = pTask->resultCode() ;

      /// find out sub-task in SubTasks list
      itTask = _mapSubTask.find( pTask->taskID() ) ;
      PD_CHECK( itTask != _mapSubTask.end(), SDB_SYS, error, PDERROR,
                "Failed to find out sub-task[%llu] from main task[%llu]",
                pTask->taskID(), _taskID ) ;

      if ( CLS_TASK_STATUS_FINISH == itTask->second.status )
      {
         needChangeSubtask = FALSE ;
         PD_LOG( PDWARNING, "Sub-task[%llu] of task[%llu] has already finished",
                 pTask->taskID(), _taskID ) ;
      }

      /// generate new group info by sub-tasks
      rc = _buildNewGroupInfo( subTaskInfoList, newGroupInfo ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build new group info",
                   rc ) ;

      /// find out group unit from Groups List
      itGroup = _mapGroupInfo.find( newGroupInfo.groupName ) ;
      PD_CHECK( itGroup != _mapGroupInfo.end(), SDB_SYS, error, PDERROR,
                "Failed to find out group[%s] from task[%llu]",
                newGroupInfo.groupName.c_str(), _taskID ) ;

      if ( CLS_TASK_STATUS_FINISH == itGroup->second.status &&
           newGroupInfo.resultCode == itGroup->second.resultCode )
      {
         // 'finish + ok' can convert to 'finish + -243'
         needChangeGroup = FALSE ;
         PD_LOG( PDWARNING, "Group[%s] in task[%llu] has already finished",
                 newGroupInfo.groupName.c_str(), _taskID ) ;
      }

      /// update info
      if ( needChangeGroup || needChangeSubtask )
      {
         _clearChangedMask() ;

         if ( needChangeGroup )
         {
            _updateGroup( itGroup->second, newGroupInfo ) ;
         }

         if ( needChangeSubtask )
         {
            _updateSubTask( itTask->second, newSubTask ) ;

            _updateOtherBySubTaskInfo() ;
         }

         rc = _toChangedObj( newGroupInfo, newSubTask, matcher, updator ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_UPDMAINTASKINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _clsIdxTask::_updateSubTask( clsSubTaskUnit& orgSubTask,
                                     const clsSubTaskUnit& newSubTask )
   {
      /*
      * Update SubTasks / SucceededTasks / FailedTasks field
      */

      BOOLEAN hasChangeSubtask = FALSE ;
      BOOLEAN change2Finish = FALSE ; // run -> finish

      if ( CLS_TASK_STATUS_FINISH == orgSubTask.status )
      {
         goto done ;
      }

      /// update SubTasks element
      if ( orgSubTask.status != newSubTask.status )
      {
         orgSubTask.status = newSubTask.status ;
         _changedTaskMask |= CLS_IDX_MASK_STATUS ;
         hasChangeSubtask = TRUE ;
         if ( CLS_TASK_STATUS_FINISH == newSubTask.status )
         {
            change2Finish = TRUE ;
         }
      }
      if ( change2Finish || orgSubTask.resultCode != newSubTask.resultCode )
      {
         orgSubTask.resultCode = newSubTask.resultCode ;
         _changedTaskMask |= CLS_IDX_MASK_RESULT ;
         hasChangeSubtask = TRUE ;
      }

      if ( hasChangeSubtask )
      {
         _changedSubtask = newSubTask ;
         _changedMask |= CLS_IDX_MASK_SUBTASKS ;
      }

      // increase sub-task count
      if ( CLS_TASK_STATUS_FINISH == newSubTask.status )
      {
         if ( SDB_OK == newSubTask.resultCode )
         {
            _succeededTasks++ ;
         }
         else
         {
            _failedTasks++ ;
         }
         _changedMask |= CLS_IDX_MASK_TASKCOUNT ;
      }

   done:
      return ;
   }

   void _clsIdxTask::_updateGroup( clsIdxTaskGroupUnit& orgGroupInfo,
                                   const clsIdxTaskGroupUnit& newGroupInfo )
   {
      /*
      * Update Groups / SucceededGroups / FailedGroups field
      */

      BOOLEAN hasChangedGroup = FALSE ;
      BOOLEAN change2Finish = FALSE ; // run -> finish

      /// increase group count
      if ( CLS_TASK_STATUS_FINISH == orgGroupInfo.status &&
           CLS_TASK_STATUS_FINISH == newGroupInfo.status )
      {
         // 'finish + ok' can convert to 'finish + -243',
         // we should DECREASE _succeededGroups
         if ( SDB_OK == orgGroupInfo.resultCode &&
              SDB_OK != newGroupInfo.resultCode )
         {
            _succeededGroups-- ;
            _failedGroups++ ;
         }
         else if ( SDB_OK != orgGroupInfo.resultCode &&
                   SDB_OK == newGroupInfo.resultCode )
         {
            _failedGroups-- ;
            _succeededGroups++ ;
         }
         _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
      }
      else if ( CLS_TASK_STATUS_FINISH == newGroupInfo.status )
      {
         if ( SDB_OK == newGroupInfo.resultCode )
         {
            _succeededGroups++ ;
         }
         else
         {
            _failedGroups++ ;
         }
         _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
      }

      /// update Groups element
      if ( orgGroupInfo.status != newGroupInfo.status )
      {
         orgGroupInfo.status = newGroupInfo.status ;
         _changedGroupMask |= CLS_IDX_MASK_STATUS ;
         hasChangedGroup = TRUE ;
         if ( CLS_TASK_STATUS_FINISH == newGroupInfo.status )
         {
            change2Finish = TRUE ;
         }
      }
      if ( change2Finish || orgGroupInfo.resultCode != newGroupInfo.resultCode )
      {
         orgGroupInfo.resultCode = newGroupInfo.resultCode ;
         orgGroupInfo.resultInfo= newGroupInfo.resultInfo ;
         _changedGroupMask |= CLS_IDX_MASK_RESULT ;
         hasChangedGroup = TRUE ;
      }
      if ( orgGroupInfo.opInfo != newGroupInfo.opInfo )
      {
         orgGroupInfo.opInfo = newGroupInfo.opInfo;
         _changedGroupMask |= CLS_IDX_MASK_OPINFO ;
         hasChangedGroup = TRUE ;
      }
      if ( orgGroupInfo.retryCnt != newGroupInfo.retryCnt )
      {
         orgGroupInfo.retryCnt = newGroupInfo.retryCnt ;
         _changedGroupMask |= CLS_IDX_MASK_RETRYCNT ;
         hasChangedGroup = TRUE ;
      }
      if ( orgGroupInfo.timeSpent != newGroupInfo.timeSpent )
      {
         orgGroupInfo.progress = newGroupInfo.progress ;
         orgGroupInfo.speed = newGroupInfo.speed ;
         orgGroupInfo.timeSpent = newGroupInfo.timeSpent ;
         orgGroupInfo.timeLeft = newGroupInfo.timeLeft ;
         _changedGroupMask |= CLS_IDX_MASK_PROGRESS ;
         hasChangedGroup = TRUE ;
      }
      if ( orgGroupInfo.totalSize != newGroupInfo.totalSize )
      {
         orgGroupInfo.totalSize = newGroupInfo.totalSize ;
         _changedGroupMask |= CLS_IDX_MASK_TOTALSZ ;
         hasChangedGroup = TRUE ;
      }
      if ( orgGroupInfo.processedSize != newGroupInfo.processedSize )
      {
         orgGroupInfo.processedSize = newGroupInfo.processedSize ;
         _changedGroupMask |= CLS_IDX_MASK_PROCESSSZ ;
         hasChangedGroup = TRUE ;
      }

      if ( hasChangedGroup )
      {
         _changedMask |= CLS_IDX_MASK_GROUPS ;
      }
   }

   INT32 _clsIdxTask::_buildNewGroupInfo( const ossPoolVector<BSONObj> &subTaskInfoList,
                                          clsIdxTaskGroupUnit &newGroupInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 totalCnt = subTaskInfoList.size() ;
      INT32 finishCnt = 0 ;
      INT32 readyCnt = 0 ;
      INT32 firstResultCode = SDB_OK ;
      BSONObj firstResultInfo ;

      for( ossPoolVector<BSONObj>::const_iterator it = subTaskInfoList.begin() ;
           it != subTaskInfoList.end() ; ++it )
      {
         // two format:
         // 1: { "Groups": [ { "GroupName": "db1", Status: xxx, ... } ] }
         // 2:               { "GroupName": "db1", Status: xxx, ... }
         const BSONObj &subTaskInfo = *it ;
         BSONObj groupObj ;
         BSONElement ele ;

         ele = subTaskInfo.firstElement() ;
         if ( Array == ele.type() )
         {
            BSONObj array = ele.Obj() ;
            ele = array.firstElement() ;
            PD_CHECK( ele.type() == Object,
                      SDB_INVALIDARG, error, PDERROR,
                      "Invalid first element type[%d] in obj[%s]",
                      ele.type(), array.toString().c_str() ) ;

            groupObj = ele.Obj() ;
         }
         else
         {
            groupObj = subTaskInfo ;
         }

         // init current group unit
         clsIdxTaskGroupUnit groupUnit ;
         rc = groupUnit.init( groupObj.objdata() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to init group unit, rc: %d",
                      rc ) ;

         // check GroupName
         if ( it == subTaskInfoList.begin() )
         {
            newGroupInfo.groupName = groupUnit.groupName ;
         }
         else
         {
            PD_CHECK( newGroupInfo.groupName == groupUnit.groupName,
                      SDB_INVALIDARG, error, PDERROR,
                      "Invalid group name[%s], expect[%s]",
                      groupUnit.groupName.c_str(), newGroupInfo.groupName.c_str() ) ;
         }

         // count group
         if ( CLS_TASK_STATUS_FINISH == groupUnit.status )
         {
            finishCnt++ ;
         }
         else if ( CLS_TASK_STATUS_READY == groupUnit.status )
         {
            readyCnt++ ;
         }
         if ( SDB_OK == firstResultCode &&
              SDB_OK != groupUnit.resultCode )
         {
            firstResultCode = groupUnit.resultCode ;
            firstResultInfo = groupUnit.resultInfo ;
         }

         // TotalSize ProcessedSize RetryCnt Speed
         newGroupInfo.totalSize     += groupUnit.totalSize ;
         newGroupInfo.processedSize += groupUnit.processedSize ;
         newGroupInfo.retryCnt      += groupUnit.retryCnt ;
         newGroupInfo.speed         += groupUnit.speed ;

         // TimeSpent TimeLeft
         if ( groupUnit.timeSpent > newGroupInfo.timeSpent )
         {
            newGroupInfo.timeSpent = groupUnit.timeSpent ;
         }
         if ( groupUnit.timeLeft > newGroupInfo.timeLeft )
         {
            newGroupInfo.timeLeft = groupUnit.timeLeft ;
         }
      }

      // Status
      if ( finishCnt == totalCnt )
      {
         newGroupInfo.status = CLS_TASK_STATUS_FINISH ;
      }
      else if ( readyCnt == totalCnt )
      {
         newGroupInfo.status = CLS_TASK_STATUS_READY ;
      }
      else
      {
         newGroupInfo.status = CLS_TASK_STATUS_RUN ;
      }

      // ResultCode ResultInfo
      if ( CLS_TASK_STATUS_FINISH == newGroupInfo.status )
      {
         newGroupInfo.resultCode = firstResultCode ;
         newGroupInfo.resultInfo = firstResultInfo ;
      }

      // Progress
      newGroupInfo.progress = (FLOAT32)newGroupInfo.processedSize /
                                       newGroupInfo.totalSize * 100 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _clsIdxTask::_updateOtherByGroupInfo()
   {
      /*
      * Update other fields except Groups / SucceedGroups / FailedGroups
      */

      UINT32 cntTotalGroup = _mapGroupInfo.size() ;
      UINT32 cntReadyGroup = 0 ;
      UINT32 cntSucGroup = 0 ;
      UINT32 cntFailGroup = 0 ;
      UINT32 cntRedefineIdx = 0 ;
      UINT32 cntNotExistIdx = 0 ;
      INT32 firstResultCode = SDB_OK ;
      BSONObj firstResultInfo ;

      // loop every group
      for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
           it != _mapGroupInfo.end() ;
           it++ )
      {
         clsIdxTaskGroupUnit &localGroup = it->second ;
         if ( CLS_TASK_STATUS_READY == localGroup.status )
         {
            cntReadyGroup++ ;
         }
         else if ( CLS_TASK_STATUS_FINISH == localGroup.status )
         {
            if ( SDB_OK == localGroup.resultCode )
            {
               cntSucGroup++ ;
            }
            else if ( SDB_IXM_REDEF == localGroup.resultCode &&
                      CLS_TASK_CREATE_IDX == _taskType )
            {
               // ignore -247 error when create index
               cntSucGroup++ ;
               cntRedefineIdx++ ;
            }
            else if ( SDB_IXM_NOTEXIST == localGroup.resultCode &&
                      CLS_TASK_DROP_IDX == _taskType )
            {
               // ignore -47 error when drop index
               cntSucGroup++ ;
               cntNotExistIdx++ ;
            }
            else
            {
               cntFailGroup++ ;
               // When one group failed, other groups will be rolled back( error
               // code is SDB_TASK_ROLLBACK ). So we looks up other error code
               // besides SDB_TASK_ROLLBACK.
               if ( SDB_OK == firstResultCode ||
                    SDB_TASK_ROLLBACK == firstResultCode )
               {
                  firstResultCode = localGroup.resultCode ;
                  firstResultInfo = localGroup.resultInfo ;
               }
            }
         }
      }

      // set first resultCode
      if ( cntTotalGroup != 0 )
      {
         if ( CLS_TASK_CREATE_IDX == _taskType &&
              cntRedefineIdx == cntTotalGroup )
         {
            // if all groups return -247 error, DO NOT ignore error
            firstResultCode = SDB_IXM_REDEF ;
         }
         else if ( CLS_TASK_DROP_IDX == _taskType &&
                   cntNotExistIdx == cntTotalGroup )
         {
            // if all groups return -47 error, DO NOT ignore error
            firstResultCode = SDB_IXM_NOTEXIST ;
         }
      }

      // switch status
      if ( 0 == cntTotalGroup )
      {
         setFinish() ;
      }
      else if ( CLS_TASK_STATUS_READY == _status )
      {
         setRun() ;
      }
      else if ( CLS_TASK_STATUS_RUN == _status )
      {
         if ( cntSucGroup == cntTotalGroup ||
              cntReadyGroup + cntFailGroup == cntTotalGroup )
         {
            setFinish( firstResultCode, firstResultInfo ) ;
         }
         else if ( cntFailGroup > 0 )
         {
            setStatus( CLS_TASK_STATUS_ROLLBACK ) ;
         }
      }
      else if ( CLS_TASK_STATUS_ROLLBACK == _status )
      {
         if ( cntReadyGroup + cntFailGroup == cntTotalGroup )
         {
            setFinish( firstResultCode, firstResultInfo ) ;
         }
      }
      else if ( CLS_TASK_STATUS_CANCELED == _status )
      {
         if ( cntReadyGroup + cntFailGroup == cntTotalGroup )
         {
            setFinish( SDB_TASK_HAS_CANCELED ) ;
         }
      }

      // caculate progress totally
      if ( CLS_TASK_STATUS_RUN    == _status ||
           CLS_TASK_STATUS_FINISH == _status )
      {
         _calculate() ;
      }
   }

   void _clsIdxTask::_updateOtherBySubTaskInfo()
   {
      /*
      * Update other fields except Groups / SucceedGroups / FailedGroups /
                                   SubTasks / SucceedTasks / FailedTasks
      */

      UINT32 cntFinished = 0 ;
      UINT32 cntTotal = _mapSubTask.size() ;
      UINT32 cntRedefineIdx = 0 ;
      UINT32 cntNotExistIdx = 0 ;
      INT32 firstResultCode = SDB_OK ;

      // loop every subTask
      for( ossPoolMap<UINT64, clsSubTaskUnit>::iterator it = _mapSubTask.begin() ;
           it != _mapSubTask.end() ; it++ )
      {
         clsSubTaskUnit &localSubTask = it->second ;
         if ( CLS_TASK_STATUS_FINISH == localSubTask.status )
         {
            cntFinished++ ;
            if ( SDB_IXM_REDEF == localSubTask.resultCode &&
                 CLS_TASK_CREATE_IDX == localSubTask.taskType )
            {
               cntRedefineIdx++ ;
            }
            else if ( SDB_IXM_NOTEXIST == localSubTask.resultCode &&
                      CLS_TASK_DROP_IDX == localSubTask.taskType )
            {
               cntNotExistIdx++ ;
            }
            else
            {
               if ( SDB_OK == firstResultCode )
               {
                  firstResultCode = localSubTask.resultCode ;
               }
            }
         }
      }

      // set first resultCode
      if ( cntTotal != 0 )
      {
         if ( cntRedefineIdx == cntTotal )
         {
            firstResultCode = SDB_IXM_REDEF ;
         }
         else if ( cntNotExistIdx == cntTotal )
         {
            firstResultCode = SDB_IXM_NOTEXIST ;
         }
      }

      // switch status
      if ( 0 == cntTotal )
      {
         setFinish() ;
      }
      else if ( CLS_TASK_STATUS_READY == _status )
      {
         setRun() ;
      }
      else if ( CLS_TASK_STATUS_RUN == _status )
      {
         if ( cntFinished == cntTotal )
         {
            setFinish( firstResultCode ) ;
         }
      }
      else if ( CLS_TASK_STATUS_CANCELED == _status )
      {
         if ( cntFinished == cntTotal )
         {
            setFinish( SDB_TASK_HAS_CANCELED ) ;
         }
      }

      // caculate progress totally
      if ( CLS_TASK_STATUS_RUN    == _status ||
           CLS_TASK_STATUS_FINISH == _status )
      {
         _calculate() ;
      }
   }

   void _clsIdxTask::_clearChangedMask()
   {
      _changedMask = 0 ;
      _changedGroupMask = 0 ;
      _changedTaskMask = 0 ;
      _changedGroup.clear() ;
      _changedSubtask.clear() ;
   }

   void _clsIdxTask::_toChangeOtherObj( BSONObjBuilder& matcherB,
                                        BSONObjBuilder& setB )
   {
      matcherB.append( FIELD_NAME_TASKID, (INT64)_taskID ) ;

      if ( _changedMask & CLS_IDX_MASK_STATUS )
      {
         setB.append( FIELD_NAME_STATUS, _status ) ;
         setB.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(_status) ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_RESULT )
      {
         setB.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
         setB.append( FIELD_NAME_RESULTCODEDESC, getErrDesp(_resultCode) ) ;
         setB.append( FIELD_NAME_RESULTINFO, _resultInfo ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_PROGRESS )
      {
         setB.append( FIELD_NAME_PROGRESS, _progress ) ;
         setB.append( FIELD_NAME_SPEED, _speed ) ;
         setB.append( FIELD_NAME_TIMESPENT, _timeSpent ) ;
         setB.append( FIELD_NAME_TIMELEFT, _timeLeft ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_GROUPCOUNT )
      {
         setB.append( FIELD_NAME_TOTALGROUP, _totalGroups ) ;
         setB.append( FIELD_NAME_SUCCEEDGROUP, _succeededGroups ) ;
         setB.append( FIELD_NAME_FAILGROUP, _failedGroups ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_TASKCOUNT )
      {
         setB.append( FIELD_NAME_TOTALTASKS, _totalTasks ) ;
         setB.append( FIELD_NAME_SUCCEEDTASKS, _succeededTasks ) ;
         setB.append( FIELD_NAME_FAILTASKS, _failedTasks ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_BEGINTIME )
      {
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( _beginTS, timeStr ) ;
         setB.append( FIELD_NAME_BEGINTIMESTAMP, timeStr ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_ENDTIME )
      {
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( _endTS, timeStr ) ;
         setB.append( FIELD_NAME_ENDTIMESTAMP, timeStr ) ;
      }
   }

   void _clsIdxTask::_toChangeSubtaskObj( BSONObjBuilder& matcherB,
                                          BSONObjBuilder& setB,
                                          const clsSubTaskUnit& subTask )
   {
      if ( _changedMask & CLS_IDX_MASK_SUBTASKS )
      {
         matcherB.append( FIELD_NAME_SUBTASKS ".$2." FIELD_NAME_TASKID,
                          (INT64)subTask.taskID ) ;

         if ( _changedTaskMask & CLS_IDX_MASK_STATUS )
         {
            setB.append( FIELD_NAME_SUBTASKS ".$2." FIELD_NAME_STATUS,
                         subTask.status ) ;
            setB.append( FIELD_NAME_SUBTASKS ".$2." FIELD_NAME_STATUSDESC,
                         clsTaskStatusStr( subTask.status ) ) ;
         }
         if ( _changedTaskMask & CLS_IDX_MASK_RESULT )
         {
            setB.append( FIELD_NAME_SUBTASKS ".$2." FIELD_NAME_RESULTCODE,
                         subTask.resultCode ) ;
            setB.append( FIELD_NAME_SUBTASKS ".$2." FIELD_NAME_RESULTCODEDESC,
                         getErrDesp( subTask.resultCode ) ) ;
         }
      }
   }

   void _clsIdxTask::_toPullSubtaskObj( BSONObjBuilder& matcherB,
                                        BSONObjBuilder& updatorB,
                                        UINT64 taskID )
   {
      if ( _changedMask & CLS_IDX_MASK_SUBTASKS )
      {
         updatorB.append( "$pull_by",
                          BSON( FIELD_NAME_SUBTASKS <<
                                BSON( FIELD_NAME_TASKID << (INT64)taskID ) ) ) ;
      }
   }

   void _clsIdxTask::_toChangeGroupObj( BSONObjBuilder& matcherB,
                                        BSONObjBuilder& setB,
                                        const clsIdxTaskGroupUnit& group )
   {
      if ( _changedMask & CLS_IDX_MASK_GROUPS )
      {
         matcherB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_GROUPNAME,
                          group.groupName.c_str() ) ;

         if ( _changedGroupMask & CLS_IDX_MASK_STATUS )
         {
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_STATUS,
                         group.status ) ;
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_STATUSDESC,
                         clsTaskStatusStr( group.status ) ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_RESULT )
         {
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_RESULTCODE,
                         group.resultCode ) ;
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_RESULTCODEDESC,
                         getErrDesp( group.resultCode ) ) ;
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_RESULTINFO,
                         group.resultInfo ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_OPINFO )
         {
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_OPINFO,
                         group.opInfo.c_str() ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_RETRYCNT )
         {
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_RETRY_COUNT,
                         group.retryCnt ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_PROGRESS )
         {
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_PROGRESS,
                         group.progress ) ;
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_SPEED,
                         group.speed ) ;
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_TIMESPENT,
                         group.timeSpent ) ;
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_TIMELEFT,
                         group.timeLeft ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_TOTALSZ )
         {
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_TOTAL_SIZE,
                         group.totalSize ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_PROCESSSZ )
         {
            setB.append( FIELD_NAME_GROUPS ".$1." FIELD_NAME_PROCESSED_SIZE,
                         group.processedSize ) ;
         }
      }
   }

   void _clsIdxTask::_toChangeGroupObj( BSONObjBuilder& matcherB,
                                        BSONObjBuilder& setB )
   {
      if ( _changedMask & CLS_IDX_MASK_GROUPS )
      {
         BSONArrayBuilder arr( setB.subarrayStart( CAT_GROUPS_NAME ) ) ;
         for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
              it != _mapGroupInfo.end() ; ++it )
         {
            const _clsIdxTaskGroupUnit& group = it->second ;
            BSONObjBuilder b ;
            b.append( FIELD_NAME_GROUPNAME,  group.groupName.c_str() ) ;
            b.append( FIELD_NAME_STATUS,     group.status ) ;
            b.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(group.status) ) ;
            b.append( FIELD_NAME_RESULTCODE, group.resultCode ) ;
            b.append( FIELD_NAME_RESULTCODEDESC,
                      CLS_TASK_STATUS_FINISH == group.status ?
                      getErrDesp( group.resultCode ) : "" ) ;
            b.append( FIELD_NAME_RESULTINFO,      group.resultInfo ) ;
            b.append( FIELD_NAME_OPINFO,          group.opInfo.c_str() ) ;
            b.append( FIELD_NAME_RETRY_COUNT,     group.retryCnt ) ;
            b.append( FIELD_NAME_PROGRESS,        group.progress ) ;
            b.append( FIELD_NAME_SPEED,           group.speed ) ;
            b.append( FIELD_NAME_TIMESPENT,       group.timeSpent ) ;
            b.append( FIELD_NAME_TIMELEFT,        group.timeLeft ) ;
            b.append( FIELD_NAME_TOTAL_SIZE,      group.totalSize ) ;
            b.append( FIELD_NAME_PROCESSED_SIZE,  group.processedSize ) ;
            arr.append( b.obj() ) ;
         }
         arr.done() ;
      }
   }

   INT32 _clsIdxTask::_toChangedObj( const clsIdxTaskGroupUnit& group,
                                     const clsSubTaskUnit& subTask,
                                     BSONObj& matcher,
                                     BSONObj& updator )
   {
      /* cl.update( { "$set": { Status: xxx, ...,
      *                         "Groups.$1.Status": xxx, ...,
      *                         "SubTasks.$2.Status": xxx, ... } },
      *             { TaskID: 1,
      *               "Groups.$1.GroupName": "db1",
      *               "SubTasks.$2.TaskID": 2 } )
      */
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder matcherBuilder, setBuilder ;
         _toChangeOtherObj( matcherBuilder, setBuilder ) ;
         _toChangeGroupObj( matcherBuilder, setBuilder, group ) ;
         _toChangeSubtaskObj( matcherBuilder, setBuilder, subTask ) ;
         matcher = matcherBuilder.obj() ;
         updator = BSON( "$set" << setBuilder.obj() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsIdxTask::_toChangedObj( const clsIdxTaskGroupUnit& group,
                                     BSONObj& matcher,
                                     BSONObj& updator )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder matcherBuilder, setBuilder ;
         _toChangeOtherObj( matcherBuilder, setBuilder ) ;
         _toChangeGroupObj( matcherBuilder, setBuilder, group ) ;
         matcher = matcherBuilder.obj() ;
         updator = BSON( "$set" << setBuilder.obj() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsIdxTask::_toChangedObj( UINT64 subTaskID,
                                     BSONObj& matcher,
                                     BSONObj& updator )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder matcherBuilder, updatorBuilder, setBuilder ;
         _toChangeOtherObj( matcherBuilder, setBuilder ) ;
         _toChangeGroupObj( matcherBuilder, setBuilder ) ;
         _toPullSubtaskObj( matcherBuilder, updatorBuilder, subTaskID ) ;
         matcher = matcherBuilder.obj() ;
         updatorBuilder.append( "$set", setBuilder.done() ) ;
         updator = updatorBuilder.obj() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   /*
      _clsCreateIdxTask : implement
   */
   INT32 _clsCreateIdxTask::sortBufSize() const
   {
      return _sortBufferSize ;
   }

   const BSONObj& _clsCreateIdxTask::indexDef() const
   {
      return _indexDef ;
   }

   INT32 _clsCreateIdxTask::globalIdxCL( const CHAR *&clName,
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
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
      }

      return rc ;
   }

   INT32 _clsCreateIdxTask::addGlobalOpt2Def( const CHAR* globalIdxCLName,
                                              utilCLUniqueID globalIdxCLUniqID )
   {
     /* IndexDef:
      * { name: 'a', key: {a: 1}, ... }
      * ==>
      * { name: 'a', key: {a: 1}, ... , GlobalOption: { Collection: xx,
      *                                                 CLUniqueID: xx } }
      */
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         builder.appendElements( _indexDef ) ;
         BSONObjBuilder sub( builder.subobjStart( IXM_FIELD_NAME_GLOBAL_OPTION ) ) ;
         sub.append( FIELD_NAME_COLLECTION, globalIdxCLName ) ;
         sub.append( FIELD_NAME_CL_UNIQUEID, (INT64)globalIdxCLUniqID ) ;
         sub.done() ;
         _indexDef = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCRTIDXTASK_INITTASK, "_clsCreateIdxTask::initTask" )
   INT32 _clsCreateIdxTask::initTask( const CHAR *clFullName,
                                      const BSONObj &index,
                                      const vector<string> &groupList,
                                      INT32 sortBufSize,
                                      UINT64 mainTaskID )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSCRTIDXTASK_INITTASK ) ;

      INT32 rc = SDB_OK ;

      // collection
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;

      // idxDef: { "name": "aIdx", "key": { "a": 1 }, ... }
      BSONObjBuilder builder ;
      builder.append( DMS_ID_KEY_NAME, OID::gen() ) ;
      builder.appendElements( index ) ;
      _indexDef = builder.obj() ;

      BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_NAME ) ;
      PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                 "Field[%s] invalid in index def[%s]",
                 IXM_FIELD_NAME_NAME, _indexDef.toString().c_str() ) ;
      ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;

      _sortBufferSize = sortBufSize ;

      // group list
      for(  vector<string>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            it++ )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         _mapGroupInfo[ oneGroup.groupName ]= oneGroup ;
      }

      _totalGroups = groupList.size() ;

      // other
      _mainTaskID = mainTaskID ;

      _makeName() ;

   done:
      PD_TRACE_EXITRC( SDB_CLSCRTIDXTASK_INITTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCRTIDXTASK_INITMAINTASK, "_clsCreateIdxTask::initMainTask" )
   INT32 _clsCreateIdxTask::initMainTask( const CHAR *clFullName,
                                          const BSONObj &index,
                                          const ossPoolSet<ossPoolString> &groupList,
                                          const ossPoolVector<UINT64> &subTaskList )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSCRTIDXTASK_INITMAINTASK ) ;

      INT32 rc = SDB_OK ;

      // collection
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;

      // idxDef: { "name": "aIdx", "key": { "a": 1 }, ... }
      BSONObjBuilder builder ;
      builder.append( DMS_ID_KEY_NAME, OID::gen() ) ;
      builder.appendElements( index ) ;
      _indexDef = builder.obj() ;

      BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_NAME ) ;
      PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                 "Field[%s] invalid in index def[%s]",
                 IXM_FIELD_NAME_NAME, _indexDef.toString().c_str() ) ;
      ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;

      // group list
      for(  ossPoolSet<ossPoolString>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            it++ )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         _mapGroupInfo[ oneGroup.groupName ]= oneGroup ;
      }

      _totalGroups = groupList.size() ;

      // sub-task list
      _isMainTask = TRUE ;

      for(  ossPoolVector<UINT64>::const_iterator it = subTaskList.begin() ;
            it != subTaskList.end() ;
            it++ )
      {
         UINT64 taskID = *it ;
         _clsSubTaskUnit oneSubTask( taskID, CLS_TASK_CREATE_IDX ) ;
         _mapSubTask[ taskID ] = oneSubTask ;
      }

      _totalTasks = subTaskList.size() ;

      // other
      _makeName() ;

   done:
      PD_TRACE_EXITRC( SDB_CLSCRTIDXTASK_INITMAINTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _clsCreateIdxTask::commandName() const
   {
      return CMD_NAME_CREATE_INDEX ;
   }

   INT32 _clsCreateIdxTask::_init( const CHAR *objdata )
   {
      INT32 rc = SDB_OK ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj obj( objdata ) ;
         BSONElement ele ;

         ele = obj.getField ( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
         PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    IXM_FIELD_NAME_SORT_BUFFER_SIZE, obj.toString().c_str() ) ;
         _sortBufferSize = ele.numberInt() ;

         ele = obj.getField ( IXM_FIELD_NAME_INDEX_DEF ) ;
         PD_CHECK ( ele.type() == Object , SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in task[%s]",
                    IXM_FIELD_NAME_INDEX_DEF, obj.toString().c_str() ) ;
         _indexDef = ele.Obj().getOwned() ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _clsCreateIdxTask::_toBson( BSONObjBuilder &builder )
   {
      try
      {
         builder.append( IXM_FIELD_NAME_INDEX_DEF,        _indexDef ) ;
         builder.append( IXM_FIELD_NAME_SORT_BUFFER_SIZE, _sortBufferSize ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }
   }

   /*
      _clsDropIdxTask : implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPIDXTASK_INITTASK, "_clsDropIdxTask::initTask" )
   INT32 _clsDropIdxTask::initTask( const CHAR *clFullName,
                                    const CHAR *indexName,
                                    const vector<string> &groupList,
                                    UINT64 mainTaskID )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSDROPIDXTASK_INITTASK ) ;

      INT32 rc = SDB_OK ;

      // collection
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;

      // index
      ossStrncpy( _indexName, indexName, IXM_INDEX_NAME_SIZE ) ;

      // group list
      for(  vector<string>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            ++it )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
      }

      _totalGroups = groupList.size() ;

      // other
      _mainTaskID = mainTaskID ;

      _makeName() ;

      PD_TRACE_EXITRC( SDB_CLSDROPIDXTASK_INITTASK, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPIDXTASK_INITMAINTASK, "_clsDropIdxTask::initMainTask" )
   INT32 _clsDropIdxTask::initMainTask( const CHAR *clFullName,
                                        const CHAR *indexName,
                                        const ossPoolSet<ossPoolString> &groupList,
                                        const ossPoolVector<UINT64> &subTaskList )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSDROPIDXTASK_INITMAINTASK ) ;

      INT32 rc = SDB_OK ;

      // collection
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;

      // index
      ossStrncpy( _indexName, indexName, IXM_INDEX_NAME_SIZE ) ;

      // group list
      for(  ossPoolSet<ossPoolString>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            it++ )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
      }

      _totalGroups = groupList.size() ;

      // sub-task list
      _isMainTask = TRUE ;

      for(  ossPoolVector<UINT64>::const_iterator it = subTaskList.begin();
            it != subTaskList.end() ;
            it++ )
      {
         UINT64 taskID = *it ;
         _clsSubTaskUnit oneSubTask( taskID, CLS_TASK_DROP_IDX ) ;
         _mapSubTask[ taskID ] = oneSubTask ;
      }

      _totalTasks = subTaskList.size() ;

      // other
      _makeName() ;

      PD_TRACE_EXITRC( SDB_CLSDROPIDXTASK_INITMAINTASK, rc ) ;
      return rc ;
   }

   const CHAR* _clsDropIdxTask::commandName() const
   {
      return CMD_NAME_DROP_INDEX ;
   }

   INT32 _clsDropIdxTask::_init( const CHAR *objdata )
   {
      return SDB_OK ;
   }

   void _clsDropIdxTask::_toBson( BSONObjBuilder &builder )
   {
   }

   /*
      _clsCopyIdxTask : implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCOPYIDXTASK_INITMAINTASK, "_clsCopyIdxTask::initMainTask" )
   INT32 _clsCopyIdxTask::initMainTask( const CHAR *clFullName,
                                        const ossPoolSet<ossPoolString> &subCLList,
                                        const ossPoolSet<ossPoolString> &indexList,
                                        const ossPoolSet<ossPoolString> &groupList,
                                        const ossPoolVector<UINT64> &subTaskList )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSCOPYIDXTASK_INITMAINTASK ) ;

      INT32 rc = SDB_OK ;

      // collection index
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;

      _subCLList = subCLList ;
      _indexList = indexList ;

      // group list
      for(  ossPoolSet<ossPoolString>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            it++ )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
      }

      _totalGroups = groupList.size() ;

      // sub-task list
      _isMainTask = TRUE ;

      for(  ossPoolVector<UINT64>::const_iterator it = subTaskList.begin();
            it != subTaskList.end() ;
            it++ )
      {
         UINT64 taskID = *it ;
         _clsSubTaskUnit oneSubTask( taskID, CLS_TASK_CREATE_IDX ) ;
         _mapSubTask[ taskID ] = oneSubTask ;
      }

      _totalTasks = subTaskList.size() ;

      // other
      _makeName() ;

      PD_TRACE_EXITRC( SDB_CLSCOPYIDXTASK_INITMAINTASK, rc ) ;
      return rc ;
   }

   INT32 _clsCopyIdxTask::_init( const CHAR *objdata )
   {
      INT32 rc = SDB_OK ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj jobObj ( objdata ) ;

         BSONElement ele = jobObj.getField ( FIELD_NAME_COPYTO ) ;
         PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    FIELD_NAME_COPYTO, jobObj.toString().c_str() ) ;

         BSONObjIterator it( ele.embeddedObject() ) ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            PD_CHECK( e.type() == String, SDB_INVALIDARG, error,
                      PDERROR, "Field invalid in task[%s]",
                      jobObj.toString().c_str() ) ;
            _subCLList.insert( e.valuestr() ) ;
         }

         ele = jobObj.getField ( FIELD_NAME_INDEXNAMES ) ;
         PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    FIELD_NAME_INDEXNAMES, jobObj.toString().c_str() ) ;

         BSONObjIterator itr( ele.embeddedObject() ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            PD_CHECK( e.type() == String, SDB_INVALIDARG, error,
                      PDERROR, "Field invalid in task[%s]",
                      jobObj.toString().c_str() ) ;
            _indexList.insert( e.valuestr() ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _clsCopyIdxTask::_toBson( BSONObjBuilder &builder )
   {
      try
      {

         BSONArrayBuilder arrB1( builder.subarrayStart( FIELD_NAME_COPYTO ) ) ;
         for ( ossPoolSet<ossPoolString>::iterator it = _subCLList.begin() ;
               it != _subCLList.end() ; ++it )
         {
            arrB1.append( it->c_str() ) ;
         }
         arrB1.done() ;

         BSONArrayBuilder arrB2( builder.subarrayStart(FIELD_NAME_INDEXNAMES) ) ;
         for ( ossPoolSet<ossPoolString>::iterator it = _indexList.begin() ;
               it != _indexList.end() ; ++it )
         {
            arrB2.append( it->c_str() ) ;
         }
         arrB2.done() ;

      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }
   }

   const CHAR* _clsCopyIdxTask::commandName() const
   {
      return CMD_NAME_COPY_INDEX ;
   }
}

