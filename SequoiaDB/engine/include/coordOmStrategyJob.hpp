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

   Source File Name = coordOmStrategyJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_OM_STRATEGY_JOB_HPP__
#define COORD_OM_STRATEGY_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"

namespace engine
{

   /*
      _coordOmStrategyJob define
   */
   class _coordOmStrategyJob : public _rtnBaseJob
   {
      public:
         _coordOmStrategyJob() ;
         virtual ~_coordOmStrategyJob() ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN isSystem() const ;

      protected:
         virtual void _onAttach() ;
         virtual void _onDetach() ;

      private:

   } ;
   typedef _coordOmStrategyJob coordOmStrategyJob ;

   /*
      Gloable Functions Define
   */
   INT32 coordStartOmStrategyJob( EDUID *pEduID ) ;

}

#endif // COORD_OM_STRATEGY_JOB_HPP__
