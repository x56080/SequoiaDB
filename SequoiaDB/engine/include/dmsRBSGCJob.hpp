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

   Source File Name = dmsRBSGCJob.hpp

   Descriptive Name = Rollback Segment Garbage Collection Job Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains background job to clear
   cached access plans.

   Dependencies: N/A

   Restrictions: N/A
  
   Change Activity:
   defect Date        Who Description 
   ====== =========== === ==============================================
          02/05/2020  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSRBSGCJOB_HPP__
#define DMSRBSGCJOB_HPP__

#include "utilLightJobBase.hpp"
    
namespace engine
{

   // forward class declaration
   class _dmsRBSMgr ;

   /*
      _dmsRBSGCJob define
      Class to trigger RBS GC work in the background
   */
   class _dmsRBSGCJob : public _utilLightJob
   {
      public:
         _dmsRBSGCJob( _dmsRBSMgr  *rbsSUMgr ) ;
         virtual ~_dmsRBSGCJob() ;
         virtual const CHAR*     name() const ;
         virtual INT32   doit( IExecutor *pExe,
                               UTIL_LJOB_DO_RESULT &result,
                               UINT64 &sleepTime ) ;
     
      private:
         _dmsRBSMgr * _rbsSUMgr ;
   } ;
   typedef _dmsRBSGCJob dmsRBSGCJob ;

   INT32  dmsStartAsyncRBSGC( ) ;

}
#endif //DMSRBSGCJOB_HPP__

