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

   Source File Name = rtnLocalTaskMgr.hpp

   Descriptive Name = Local Task Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2020  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOCAL_TASK_MGR_HPP__
#define RTN_LOCAL_TASK_MGR_HPP__

#include "rtnLocalTaskFactory.hpp"
#include "pmdEDU.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _rtnLocalTaskMgr define
   */
   class _rtnLocalTaskMgr : public SDBObject
   {
      public:
         typedef ossPoolMap<UINT64, rtnLocalTaskPtr>     MAP_TASK ;
         typedef MAP_TASK::iterator                      MAP_TASK_IT ;

      public:
         _rtnLocalTaskMgr () ;
         ~_rtnLocalTaskMgr () ;

         INT32       reload( pmdEDUCB *cb ) ;
         void        fini() ;
         void        clear() ;

      public:
         UINT32      taskCount () ;
         UINT32      taskCount( RTN_LOCAL_TASK_TYPE type ) ;

         INT32       waitTaskEvent( INT64 millisec = OSS_ONE_SEC ) ;

         INT32       addTask ( rtnLocalTaskPtr &ptr,
                               pmdEDUCB *cb,
                               _dpsLogWrapper *dpsCB ) ;
         void        removeTask ( UINT64 taskID,
                                  pmdEDUCB *cb,
                                  _dpsLogWrapper *dpsCB ) ;
         void        removeTask( const rtnLocalTaskPtr &ptr,
                                 pmdEDUCB *cb,
                                 _dpsLogWrapper *dpsCB ) ;

         INT32       dumpTask( MAP_TASK &mapTask ) ;

         INT32       dumpTask( RTN_LOCAL_TASK_TYPE type,
                               MAP_TASK &mapTask ) ;

         rtnLocalTaskPtr getTask( UINT64 taskID ) ;

      protected:
         void        _clear() ;

      private:
         MAP_TASK                            _taskMap ;
         ossSpinSLatch                       _taskLatch ;
         ossAutoEvent                        _taskEvent ;

         UINT64                              _taskID ;

   };
   typedef _rtnLocalTaskMgr rtnLocalTaskMgr ;

}

#endif //RTN_LOCAL_TASK_MGR_HPP__

