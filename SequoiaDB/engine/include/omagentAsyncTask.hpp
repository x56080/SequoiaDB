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

   Source File Name = omagentBackgroundCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/09/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMAGENT_ASYNC_TASK_HPP_
#define OMAGENT_ASYNC_TASK_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "ossEvent.hpp"
#include "omagentTaskBase.hpp"

namespace engine
{

   class _omaAsyncTask : public _omaTask
   {
   public:
      _omaAsyncTask( INT64 taskID, const CHAR* command ) ;
      virtual ~_omaAsyncTask() ;
      INT32 init( const BSONObj &info, void *ptr = NULL ) ;
      INT32 doit() ;

   public:
      INT32 setTaskRunning( _omaCommand* cmd, const BSONObj& itemInfo ) ;
      INT32 updateTaskInfo( _omaCommand* cmd, const BSONObj& itemInfo ) ;
      void notifyUpdateProgress() ;
      INT32 updateProgressToOM() ;
      _omaCommand* createOmaCmd() ;
      void deleteOmaCmd( _omaCommand* cmd ) ;
      const CHAR* getOmaCmdName() ;

      BOOLEAN getSubTaskArg( UINT32& planTaskIndex, BSONObj& subTaskArg ) ;

      void setPlanTaskStatus( OMA_TASK_STATUS status ) ;
      OMA_TASK_STATUS getPlanTaskStatus() ;

   private:
      INT32 _generateTaskPlan( BSONObj& planInfo ) ;
      INT32 _execTaskPlan( const BSONObj& planInfo ) ;
      void _setTaskResultInfoStatus( OMA_TASK_STATUS status ) ;
      INT32 _rollbackPlan() ;

      INT32 _createSubTask() ;
      INT32 _waitSubTask() ;

      void _initSubTaskArg() ;
      void _appendSubTaskArg( BSONObj& subTaskArg ) ;

   private:
      INT32           _errno ;
      BOOLEAN         _isSetErrInfo ;
      string          _command ;
      string          _detail ;
      BSONObj         _taskInfo ;

      UINT32          _planTaskIndex ;
      UINT64          _planTaskNum ;
      ossEvent        _planEvent ;
      ossSpinSLatch   _planLatch ;
      vector<BSONObj> _planTaskArgList ;
      //vector<BSONObj> _planTaskResultList ;
   } ;
   typedef _omaAsyncTask omaAsyncTask ;

   class _omaAsyncSubTask : public _omaTask
   {
   public:
      _omaAsyncSubTask( INT64 taskID ) ;
      virtual ~_omaAsyncSubTask() ;

   public:
      INT32 init( const BSONObj& info, void* ptr = NULL ) ;
      INT32 doit() ;

   private:
      _omaAsyncTask* _task ;
      BSONObj        _taskInfo ;
   } ;
   typedef _omaAsyncSubTask omaAsyncSubTask ;

}

#endif