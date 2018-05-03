/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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

#ifndef CLS_TASK_HPP_
#define CLS_TASK_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsBase.hpp"
#include "ossLatch.hpp"
#include "clsCatalogAgent.hpp"
#include <string>
#include <map>

#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   enum CLS_TASK_TYPE
   {
      CLS_TASK_SPLIT          = 0,  //split task

      CLS_TASK_UNKNOW         = 255
   } ;

   enum CLS_TASK_STATUS
   {
      CLS_TASK_STATUS_READY   = 0,  // when initially created
      CLS_TASK_STATUS_RUN     = 1,  // when starts running
      CLS_TASK_STATUS_PAUSE   = 2,  // when is halt
      CLS_TASK_STATUS_CANCELED= 3,  // canceled
      CLS_TASK_STATUS_META    = 4,  // when meta( ex:catalog info ) changed
      CLS_TASK_STATUS_FINISH  = 9,  // when stopped, this should be the last
                                    // status before the task is removed
      CLS_TASK_STATUS_END     = 10  // nothing should have this status
   } ;

   #define CLS_INVALID_TASKID          (0)

   class _clsTask : public SDBObject
   {
      public:
         _clsTask ( UINT64 taskID ) : _taskID ( taskID )
         {
            _status = CLS_TASK_STATUS_READY ;
         }
         virtual ~_clsTask () {}

         UINT64          taskID () const { return _taskID ; }
         CLS_TASK_STATUS status () const { return _status ; }
         void    setStatus( CLS_TASK_STATUS status ) { _status = status ; }

      public:
         virtual CLS_TASK_TYPE   taskType () const = 0 ;
         virtual const CHAR*     taskName () const = 0 ;
         virtual const CHAR*     collectionName() const = 0 ;
         virtual const CHAR*     collectionSpaceName() const = 0 ;

         virtual BOOLEAN         muteXOn ( const _clsTask *pOther ) = 0 ;

      protected:
         UINT64                  _taskID ;
         CLS_TASK_STATUS         _status ;
   };
   typedef _clsTask clsTask ;

   class _clsDummyTask : public _clsTask
   {
      public:
         _clsDummyTask ( UINT64 taskID ) ;
         ~_clsDummyTask () ;
         virtual CLS_TASK_TYPE   taskType () const ;
         virtual const CHAR*     taskName () const ;
         virtual const CHAR*     collectionName() const ;
         virtual const CHAR*     collectionSpaceName() const ;
         virtual BOOLEAN         muteXOn ( const _clsTask *pOther ) ;
   };

   class _clsTaskMgr : public SDBObject
   {
      public:
         _clsTaskMgr ( UINT64 maxTaskID = CLS_INVALID_TASKID ) ;
         ~_clsTaskMgr () ;

         UINT64      getTaskID () ;
         void        setTaskID ( UINT64 taskID ) ;

      public:
         UINT32      taskCount () ;
         UINT32      taskCount( CLS_TASK_TYPE type ) ;

         UINT32      taskCountByCL( const CHAR *pCLName ) ;
         UINT32      taskCountByCS( const CHAR *pCSName ) ;
         INT32       waitTaskEvent( INT64 millisec = OSS_ONE_SEC ) ;

         INT32       addTask ( _clsTask *pTask,
                               UINT64 taskID = CLS_INVALID_TASKID ) ;
         INT32       removeTask ( _clsTask *pTask ) ;
         INT32       removeTask ( UINT64 taskID ) ;
         _clsTask*   findTask ( UINT64 taskID ) ;
         void        stopTask ( UINT64 taskID ) ;

      public:
         void        regCollection( const string &clName ) ;
         void        unregCollection( const string &clName ) ;
         void        lockReg( OSS_LATCH_MODE mode = SHARED ) ;
         void        releaseReg( OSS_LATCH_MODE mode = SHARED ) ;
         UINT32      getRegCount( const string &clName,
                                  BOOLEAN noLatch = FALSE ) ;

         string      dumpTasks( CLS_TASK_TYPE type ) ;

      private:
         std::map<UINT64, _clsTask*>         _taskMap ;
         ossSpinSLatch                       _taskLatch ;
         ossAutoEvent                        _taskEvent ;

         std::map<string, UINT32>            _mapRegister ;
         ossSpinSLatch                       _regLatch ;

         UINT64                              _taskID ;
         UINT64                              _maxID ;

   };
   typedef _clsTaskMgr clsTaskMgr ;

#define CLS_MASK_ALL                      (~0)

#define CLS_SPLIT_MASK_ID                 0x00000001
#define CLS_SPLIT_MASK_TYPE               0x00000002
#define CLS_SPLIT_MASK_STATUS             0x00000004
#define CLS_SPLIT_MASK_CLNAME             0x00000008
#define CLS_SPLIT_MASK_SOURCEID           0x00000010
#define CLS_SPLIT_MASK_SOURCENAME         0x00000020
#define CLS_SPLIT_MASK_DSTID              0x00000040
#define CLS_SPLIT_MASK_DSTNAME            0x00000080
#define CLS_SPLIT_MASK_BKEY               0x00000100
#define CLS_SPLIT_MASK_EKEY               0x00000200
#define CLS_SPLIT_MASK_SHARDINGKEY        0x00000400
#define CLS_SPLIT_MASK_SHARDINGTYPE       0x00000800
#define CLS_SPLIT_MASK_PERCENT            0x00001000
//#define CLS_SPLIT_MASK_LOCKEND            0x00002000

//#define CLS_SPLIT_TASK_LOCK_END           "LockEnd"

   class _clsSplitTask : public _clsTask
   {
      public:
         _clsSplitTask ( UINT64 taskID ) ;
         virtual ~_clsSplitTask () ;

         INT32   init ( const CHAR *objdata ) ;

         INT32   init ( const CHAR *clFullName, INT32 sourceID,
                        const CHAR *sourceName, INT32 dstID,
                        const CHAR *dstName, const BSONObj &bKey,
                        const BSONObj &eKey, FLOAT64 percent,
                        clsCatalogSet &cataSet ) ;

         INT32   calcHashPartition ( clsCatalogSet &cataSet, INT32 groupID,
                                     FLOAT64 percent, BSONObj &bKey,
                                     BSONObj &eKey ) ;

         BSONObj toBson ( UINT32 mask = CLS_MASK_ALL ) const ;

         BOOLEAN                 isHashSharding() const ;

      public:
         virtual CLS_TASK_TYPE   taskType () const ;
         virtual const CHAR*     taskName () const ;
         virtual const CHAR*     collectionName() const ;
         virtual const CHAR*     collectionSpaceName() const ;

         virtual BOOLEAN         muteXOn ( const _clsTask *pOther ) ;

      public:
         const CHAR*             clFullName () const ;
         const CHAR*             shardingType () const ;
         const CHAR*             sourceName () const ;
         const CHAR*             dstName () const ;
         UINT32                  sourceID () const ;
         UINT32                  dstID () const ;
         BSONObj                 splitKeyObj () const ;
         BSONObj                 splitEndKeyObj () const ;
         BSONObj                 shardingKey () const ;

      protected:
         BSONObj                 _getOrdering () const ;
         void                    _makeName() ;

      protected:
         std::string             _clFullName ;
         std::string             _csName ;
         std::string             _sourceName ;
         std::string             _dstName ;
         std::string             _shardingType ;
         UINT32                  _sourceID ;
         UINT32                  _dstID ;
         BSONObj                 _splitKeyObj ;
         BSONObj                 _splitEndKeyObj ;
         BSONObj                 _shardingKey ;
         CLS_TASK_TYPE           _taskType ;
         FLOAT64                 _percent ;
         //BOOLEAN                 _lockEnd ;

      private:
         std::string             _taskName ;

   };
   typedef _clsSplitTask clsSplitTask ;

}

#endif //CLS_TASK_HPP_

