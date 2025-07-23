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

   Source File Name = clsUniqueIDCheckJob.hpp

   Descriptive Name = CS/CL UniqueID Checking Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          06/08/2018  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_UNIQUEID_CHECK_JOB_HPP__
#define CLS_UNIQUEID_CHECK_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "utilLightJobBase.hpp"
#include "rtnLocalTaskFactory.hpp"
#include "monDMS.hpp"
#include "clsMgr.hpp"

using namespace std ;

namespace engine
{
   /*
    *  _clsUniqueIDCheckJob define
    */
   class _clsUniqueIDCheckJob : public _rtnBaseJob
   {
      public:
      _clsUniqueIDCheckJob ( BOOLEAN needPrimary  ) ;
      virtual ~_clsUniqueIDCheckJob () ;

   public:
      virtual RTN_JOB_TYPE type () const { return RTN_JOB_CLS_UNIQUEID_CHECK ; }

      virtual const CHAR* name () const { return "UniqueID-Check-By-Name" ; }

      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;

      virtual INT32 doit () ;

   private:
      BOOLEAN _needPrimary ;
   } ;

   typedef _clsUniqueIDCheckJob clsUniqueIDCheckJob ;

   INT32 startUniqueIDCheckJob ( BOOLEAN needPrimary = TRUE,
                                 EDUID* pEDUID = NULL ) ;

   /*
      _clsRenameCheckJob
   */
   class _clsRenameCheckJob : public _utilLightJob
   {
      public:
         _clsRenameCheckJob( const rtnLocalTaskPtr &taskPtr, UINT64 opID ) ;
         virtual ~_clsRenameCheckJob() ;

         INT32                   init() ;

         virtual const CHAR*     name() const ;
         virtual INT32           doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) ;

      protected:
         void                    _release() ;

      protected:
         UINT64                  _tick ;
         clsFreezingWindow*      _pFreezeWindow ;
         rtnLocalTaskPtr         _taskPtr ;
         UINT64                  _opID ;

   } ;
   typedef _clsRenameCheckJob clsRenameCheckJob ;

   INT32 clsStartRenameCheckJob( const rtnLocalTaskPtr &taskPtr, UINT64 opID ) ;

   INT32 clsStartRenameCheckJobs() ;

}

#endif

