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

   Source File Name = omagentJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_JOB_HPP_
#define OMAGENT_JOB_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "pmd.hpp"
#include "omagent.hpp"
#include "omagentTask.hpp"
#include "omagentSubTask.hpp"
#include "omagentBackgroundCmd.hpp"
#include "rtnBackgroundJob.hpp"
#include <string>
#include <vector>

using namespace bson ;

namespace engine
{

   /*
      omagent job
   */
   class _omagentJob : public _rtnBaseJob
   {
      public:
         _omagentJob ( omaTaskPtr pTask, const BSONObj &info, void *ptr ) ;
         virtual ~_omagentJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR*  name () const ;
         virtual BOOLEAN      muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32        doit () ;

      private:
         omaTaskPtr _taskPtr ;
         BSONObj    _info ;
         void       *_pointer ;
         string     _jobName ;
   } ;


   // start omagent job
   INT32 startOmagentJob ( OMA_TASK_TYPE taskType, INT64 taskID,
                           const BSONObj &info, omaTaskPtr &taskPtr,
                           void *ptr = NULL ) ;
   
   _omaTask* getTaskByType( OMA_TASK_TYPE taskType, INT64 taskID ) ;


}



#endif // OMAGENT_JOB_HPP_