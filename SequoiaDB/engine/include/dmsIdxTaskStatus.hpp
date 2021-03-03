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

   Source File Name = dmsIdxTaskStatus.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who      Description
   ====== ========= =======  ==============================================
          2019/8/23 Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_STATUS_HPP_
#define DMS_INDEX_STATUS_HPP_

#include "ossTypes.hpp"
#include "ossMemPool.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "msgDef.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "../bson/bson.hpp"
#include "ixm.hpp"
using namespace bson ;

namespace engine
{
   // It is consistent with DMS_TASK_TYPE.
   // NEVER change the value, because DMS_TASK_TYPE was written in SYSTASKS.
   enum DMS_TASK_TYPE
   {
      DMS_TASK_SPLIT          = 0,  // split task
      DMS_TASK_SEQUENCE       = 1,  // clear sequence cache on coords task
      DMS_TASK_CREATE_IDX     = 2,  // create index task
      DMS_TASK_DROP_IDX       = 3,  // drop index task
      DMS_TASK_COPY_IDX       = 4,  // copy index task

      DMS_TASK_UNKNOWN        = 255
   } ;

   // It is consistent with CLS_TASK_STATUS.
   // NEVER change the value, because CLS_TASK_STATUS was written in SYSTASKS.
   enum DMS_TASK_STATUS
   {
      DMS_TASK_STATUS_READY    = 0, // when it's created
      DMS_TASK_STATUS_RUN      = 1, // when it starts running
      DMS_TASK_STATUS_PAUSE    = 2, // when it's halt
      DMS_TASK_STATUS_CANCELED = 3, // when it's canceled by user
      DMS_TASK_STATUS_META     = 4, // when its meta changed ( ex:catalog info )
      DMS_TASK_STATUS_CLEANUP  = 5, // when it cleans up some data
      DMS_TASK_STATUS_ROLLBACK = 6, // when it rolls back

      DMS_TASK_STATUS_FINISH   = 9, // when it's finished, whether succ or fail
      DMS_TASK_STATUS_END      = 10 // no task should has this status
   } ;

   #define DMS_INVALID_TASKID     ( 0 )

   #define DMS_TASK_MASK_ALL                       (~0)

   #define DMS_TASK_MASK_GROUPNAME                 0x00000001
   #define DMS_TASK_MASK_MAINTASKID                0x00000002
   #define DMS_TASK_MASK_TASKID                    0x00000004
   #define DMS_TASK_MASK_STATUS                    0x00000008
   #define DMS_TASK_MASK_TASKTYPE                  0x00000010
   #define DMS_TASK_MASK_CLNAME                    0x00000020
   #define DMS_TASK_MASK_IDXNAME                   0x00000040
   #define DMS_TASK_MASK_IDXDEF                    0x00000080
   #define DMS_TASK_MASK_SORTBUFSZ                 0x00000100
   #define DMS_TASK_MASK_RESULTCODE                0x00000200
   #define DMS_TASK_MASK_RESULTINFO                0x00000400
   #define DMS_TASK_MASK_OPINFO                    0x00000800
   #define DMS_TASK_MASK_RETRYCNT                  0x00001000
   #define DMS_TASK_MASK_PROGRESS                  0x00002000
   #define DMS_TASK_MASK_SPEED                     0x00004000
   #define DMS_TASK_MASK_TIMESPENT                 0x00008000
   #define DMS_TASK_MASK_TIMELEFT                  0x00010000
   #define DMS_TASK_MASK_TOTALSZ                   0x00020000
   #define DMS_TASK_MASK_PROCESSSZ                 0x00040000

   // for db.snapshot(SDB_SNAP_TASKS)
   #define DMS_TASK_MASK_SNAPSHOT  ( ~DMS_TASK_MASK_GROUPNAME )

   // for reporting task to catalog
   #define DMS_TASK_MASK_REPORT    ( DMS_TASK_MASK_GROUPNAME   | \
                                     DMS_TASK_MASK_TASKID      | \
                                     DMS_TASK_MASK_STATUS      | \
                                     DMS_TASK_MASK_RESULTCODE  | \
                                     DMS_TASK_MASK_RESULTINFO  | \
                                     DMS_TASK_MASK_OPINFO      | \
                                     DMS_TASK_MASK_RETRYCNT    | \
                                     DMS_TASK_MASK_PROGRESS    | \
                                     DMS_TASK_MASK_SPEED       | \
                                     DMS_TASK_MASK_TIMESPENT   | \
                                     DMS_TASK_MASK_TIMELEFT    | \
                                     DMS_TASK_MASK_TOTALSZ     | \
                                     DMS_TASK_MASK_PROCESSSZ )

   class _dmsIdxTaskStatus : public SDBObject
   {
      public:
         _dmsIdxTaskStatus( DMS_TASK_TYPE taskType,
                            UINT64 cataTaskID,
                            UINT64 dataTaskID,
                            UINT64 mainTaskID = DMS_INVALID_TASKID ) ;
         _dmsIdxTaskStatus& operator=( const _dmsIdxTaskStatus &rhs ) ;

         DMS_TASK_TYPE taskType() const     { return _taskType ; }

         DMS_TASK_STATUS status() const     { return _dataTaskStatus ; }
         DMS_TASK_STATUS cataStatus() const { return _cataTaskStatus ; }

         UINT64  cataTaskID() const         { return _cataTaskID ; }
         UINT64  dataTaskID() const         { return _dataTaskID ; }
         UINT64  mainTaskID() const         { return _mainTaskID ; }

         void    setStatus( DMS_TASK_STATUS status ) ;
         void    setStatus2Finish( INT32 resultCode,
                                   const BSONObj &resultInfo = BSONObj() ) ;
         void    setCataStatus2Finish() ;

         INT32   init( const CHAR* collectionName,
                       const BSONObj& index,
                       INT32 sortBufSize = -1 ) ;
         BSONObj toBSON( UINT32 mask = DMS_TASK_MASK_SNAPSHOT ) const ;

         const CHAR* collectionName() { return _clFullName ; }
         const CHAR* indexName()      { return _indexName ; }

         void    calculate() ;

         INT32   sortBufSize() { return _sortBufSize ; }

         void    setTotalPageNum( UINT32 pages ) ;
         void    setPageSizeSquareRoot( UINT32 num ) ;

         UINT32* processedPageNumPtr() { return &_processedPageNum ; }
         void    incProcessedPageNum( INT32 delta ) ;
         void    resetProcessedPageNum() ;

         void    incRetryCnt() ;

         void    setOpInfo( const CHAR* opInfo ) ;
         void    resetOpInfo() ;

         INT32   resultCode() const { return _resultCode ; }

         BOOLEAN isStandaloneIdx() const ;
         BOOLEAN isGlobalIdx() const ;
         INT32 globalIdxCL( const CHAR *&clName, UINT64 &clUniqID ) const ;

         INT32 setIndexDef( const BSONObj &indexDef ) ;

         BOOLEAN isInitialized() const { return _isInitialized ; }

      private:
         DMS_TASK_TYPE   _taskType ;
         UINT64          _cataTaskID ;     // taskID in catalog
         UINT64          _dataTaskID ;     // taskID in data
         UINT64          _mainTaskID ;     // main taskID
         DMS_TASK_STATUS _cataTaskStatus ;
         DMS_TASK_STATUS _dataTaskStatus ;

         CHAR          _clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         BSONObj       _indexDef ;
         CHAR          _indexName[ IXM_INDEX_NAME_SIZE + 1 ] ;
         INT32         _sortBufSize ;

         BOOLEAN       _isStandaloneIdx ;
         BOOLEAN       _isGlobalIdx ;

         UINT32        _totalPageNum ;
         UINT32        _processedPageNum ;
         UINT32        _pageSizeSquareRoot ;
         UINT32        _pcsPageNumLastTime ;

         ossTimestamp  _beginTimestamp ;
         ossTimestamp  _endTimestamp ;
         ossTimestamp  _calculateTimestamp ;

         UINT32        _retryCnt ;
         INT32         _resultCode ;
         BSONObj       _resultInfo ;

         ossPoolString _opInfo ;

         // need to calculate
         UINT32        _progress ;
         FLOAT64       _speed ;
         FLOAT64       _timeSpent ;
         FLOAT64       _timeLeft ;

         BOOLEAN       _isInitialized ;
   } ;
   typedef struct _dmsIdxTaskStatus dmsIdxTaskStatus ;

   #define DMS_IDX_ALL                 (~0)
   #define DMS_IDX_NORMAL              0x00000001
   #define DMS_IDX_STANDALONE          0x00000002

   /*
      The first bit ditinguish between real catalog taskid and dummy taskid.
      Task id generated by dms is in range of ( min, max ]
   */
   #define DMS_DUMMY_CATTASKID_MIN      0x7FFFFFFFFFFFFFFF
   #define DMS_DUMMY_CATTASKID_MAX      0xFFFFFFFFFFFFFFFF

   #define DMS_IS_DUMMY_CATTASKID( id ) ( id & 0x8000000000000000 )

   typedef boost::shared_ptr<dmsIdxTaskStatus> dmsIdxTaskStatusPtr ;

   typedef ossPoolMap<UINT64, dmsIdxTaskStatusPtr> MAP_IDSTATUS ;
   typedef MAP_IDSTATUS::iterator MAP_IDSTATUS_IT ;

   /*
    * dmsIdxTaskStatusMgr
    */
   class _dmsIdxTaskStatusMgr: public SDBObject
   {
      public:
         _dmsIdxTaskStatusMgr() : _dummyTaskHWM( DMS_DUMMY_CATTASKID_MIN )
         {}
         ~_dmsIdxTaskStatusMgr() {}

         INT32 createItem( DMS_TASK_TYPE type,
                           dmsIdxTaskStatusPtr& statusPtr,
                           UINT64 cataTaskID = DMS_INVALID_TASKID,
                           UINT64 dataTaskID = DMS_INVALID_TASKID,
                           UINT64 mainTaskID = DMS_INVALID_TASKID ) ;

         BOOLEAN findItem( UINT64 cataTaskID,
                           dmsIdxTaskStatusPtr& statusPtr ) ;

         INT32 dumpInfo( ossPoolMap<UINT64, _dmsIdxTaskStatus>& statusMap,
                         UINT32 idxType = DMS_IDX_ALL,
                         BOOLEAN excludeDummy = FALSE,
                         BOOLEAN excludeCataFinished = FALSE ) ;
         INT32 count( UINT32 idxType = DMS_IDX_ALL,
                      BOOLEAN excludeDummy = FALSE,
                      BOOLEAN excludeCataFinished = FALSE ) ;

         void cleanOutOfDate( BOOLEAN isPrimary ) ;

      private:
         MAP_IDSTATUS  _mapIdxStatus  ; // map< taskID in cata, status >
         ossSpinSLatch _mapLatch ;

         UINT64        _dummyTaskHWM ; // high water mark of dummy task id
         ossSpinSLatch _hwmLatch ;
   } ;
   typedef class _dmsIdxTaskStatusMgr dmsIdxTaskStatusMgr ;
}

#endif /* DMS_INDEX_STATUS_HPP_ */

