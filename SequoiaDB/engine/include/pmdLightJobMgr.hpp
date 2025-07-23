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

   Source File Name = pmdLightJobMgr.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_LIGHT_JOB_MGR_HPP__
#define PMD_LIGHT_JOB_MGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "utilLightJobBase.hpp"
#include "rtnBackgroundJobBase.hpp"
#include <vector>

namespace engine
{

   /*
      _pmdLightJobMgr define
   */
   class _pmdLightJobMgr : public _utilLightJobMgr
   {
      typedef std::vector<utilLightJobInfo>     LIGHT_JOB_VEC ;
      typedef LIGHT_JOB_VEC::iterator           LIGHT_JOB_VEC_IT ;

      public:
         _pmdLightJobMgr() ;
         virtual ~_pmdLightJobMgr() ;

         void              setMaxExeJob( UINT64 maxExeJob ) ;

         void              exitJob( BOOLEAN isControl ) ;

         ossEvent*         getEvent() { return &_wakeUpEvent ; }

      public:
         virtual void      push( utilLightJob *pJob,
                                 BOOLEAN takeOver = TRUE,
                                 INT32 priority = UTIL_LJOB_PRI_MID,
                                 UINT64 expectAvgCost = UTIL_LJOB_DFT_AVG_COST ) ;

         virtual void      push( const utilLightJobInfo &job ) ;

      public:
         BOOLEAN           dispatchJob( utilLightJobInfo &job ) ;
         void              pushBackJob( utilLightJobInfo &job,
                                        UTIL_LJOB_DO_RESULT result ) ;

         void              checkLoad() ;
         UINT32            processPending() ;

      protected:
         virtual void      _onFini() ;
         void              _checkAndStartJob( BOOLEAN needLock = TRUE ) ;

      protected:
         ossSpinXLatch        _unitLatch ;
         BOOLEAN              _startCtrlJob ;

         UINT32               _curAgent ;
         UINT32               _idleAgent ;
         UINT32               _maxExeJob ;

         ossEvent             _wakeUpEvent ;

         LIGHT_JOB_VEC        _pendingJobVec ;
   } ;
   typedef _pmdLightJobMgr pmdLightJobMgr ;

   /*
      _pmdLightJobExe define
   */
   class _pmdLightJobExe : public _rtnBaseJob
   {
      public:
         _pmdLightJobExe( pmdLightJobMgr *pJobMgr, INT32 timeout = -1 ) ;
         virtual ~_pmdLightJobExe() ;

         BOOLEAN isControlJob() const ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN isSystem() const ;
         virtual BOOLEAN reuseEDU() const { return TRUE ; }

      protected:
         pmdLightJobMgr          *_pJobMgr ;
         INT32                   _timeout ;
   } ;
   typedef _pmdLightJobExe pmdLightJobExe ;

   /*
      Start a Light job executor
   */
   INT32 pmdStartLightJobExe( EDUID *pEDUID, pmdLightJobMgr *pJobMgr,
                              INT32 timeout ) ;

}

#endif //PMD_LIGHT_JOB_MGR_HPP__

