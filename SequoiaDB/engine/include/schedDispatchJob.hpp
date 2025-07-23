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

   Source File Name = schedDispatch.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/28/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_DISPATCH_JOB_HPP__
#define SCHED_DISPATCH_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"

namespace engine
{

   class _schedTaskAdapterBase ;
   class _pmdAsycSessionMgr ;

   /*
      _schedDispatchJob define
   */
   class _schedDispatchJob : public _rtnBaseJob
   {
      public:
         _schedDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                            _pmdAsycSessionMgr *pSessionMgr ) ;
         virtual ~_schedDispatchJob() ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN isSystem() const { return TRUE ; }

      private:
         _schedTaskAdapterBase   *_pTaskAdapter ;
         _pmdAsycSessionMgr      *_pSessionMgr ;

   } ;
   typedef _schedDispatchJob schedDispatchJob ;

   /*
      Gloable Functions Define
   */
   INT32 schedStartDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                                _pmdAsycSessionMgr *pSessionMgr,
                                EDUID *pEduID ) ;

}

#endif // SCHED_DISPATCH_JOB_HPP__
