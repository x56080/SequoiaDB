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

   Source File Name = schedTaskMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/29/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_MGR_HPP__
#define SCHED_TASK_MGR_HPP__

#include "monCB.hpp"
#include "monLatch.hpp"
#include "schedDef.hpp"
#include <boost/shared_ptr.hpp>
#include "ossMemPool.hpp"

using namespace std ;

namespace engine
{

   typedef boost::shared_ptr<monSvcTaskInfo>       monSvcTaskInfoPtr ;
   typedef ossPoolMap< UINT64, monSvcTaskInfoPtr > MAP_SVCTASKINFO_PTR ;
   typedef MAP_SVCTASKINFO_PTR::iterator           MAP_SVCTASKINFO_PTR_IT ;

   /*
      _schedItem define
   */
   struct _schedItem
   {
      schedInfo            _info ;
      monSvcTaskInfoPtr    _ptr ;

      void reset()
      {
         _info.reset() ;
         _ptr = monSvcTaskInfoPtr() ;
      }
   } ;
   typedef _schedItem schedItem ;

   /*
      _schedTaskMgr define
   */
   class _schedTaskMgr : public SDBObject
   {
      public:
         _schedTaskMgr() ;
         ~_schedTaskMgr() ;

         INT32       init() ;
         void        fini() ;

         monSvcTaskInfoPtr    getTaskInfoPtr( UINT64 taskID,
                                              const CHAR *taskName ) ;

         void        reset() ;

         MAP_SVCTASKINFO_PTR  getTaskInfos() ;

      private:
         monSvcTaskInfoPtr                   _defaultPtr ;
         MAP_SVCTASKINFO_PTR                 _mapTaskInfo ;
         monSpinXLatch                       _latch ;

   } ;
   typedef _schedTaskMgr schedTaskMgr ;

}

#endif // SCHED_TASK_MGR_HPP__
