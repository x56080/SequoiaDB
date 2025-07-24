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

   Source File Name = rtnContextQGM.hpp

   Descriptive Name = RunTime QGM Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_QGM_HPP_
#define RTN_CONTEXT_QGM_HPP_

#include "rtnContext.hpp"

namespace engine
{
   class _qgmPlanContainer ;
   
   /*
      _rtnContextQGM define
   */
   class _rtnContextQGM : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextQGM )
      public:
         _rtnContextQGM ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextQGM () ;

      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () { return NULL ; }

         INT32 open( _qgmPlanContainer *accPlan ) ;

      protected:
         virtual INT32   _prepareData( _pmdEDUCB *cb ) ;

      private:
         _qgmPlanContainer          *_accPlan ;

   } ;
   typedef _rtnContextQGM rtnContextQGM ;

   class _qgmPlan ;

   class _rtnContextQgmSort : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextQgmSort )
   public:
      _rtnContextQgmSort( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextQgmSort() ;

   public:
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const ;
      virtual _dmsStorageUnit* getSU () { return NULL ; }

      INT32 open( _qgmPlan *qp ) ;

   protected:
      virtual INT32   _prepareData( _pmdEDUCB *cb ) ;

   private:
      _qgmPlan *_qp ;
   } ;
   typedef class _rtnContextQgmSort rtnContextQgmSort ;
}

#endif /* RTN_CONTEXT_QGM_HPP_ */

