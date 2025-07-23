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

   Source File Name = rtnLobMetricsSubmitor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/29/2022  TZB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOBMETRICSSUBMITOR_HPP_
#define RTN_LOBMETRICSSUBMITOR_HPP_

#include "ossTypes.h"
#include "monInterface.hpp"
#include "monCB.hpp"

namespace engine
{

   class _pmdEDUCB ;

   class _rtnLobMetricsSubmitor: public SDBObject
   {
   public:
      _rtnLobMetricsSubmitor( _pmdEDUCB *cb, IMonSubmitEvent *pEvent = NULL ) ;
      ~_rtnLobMetricsSubmitor() ;
      void   submit() ;
      void   discard() ;

   private:
      _pmdEDUCB*          _eduCB ;
      IMonSubmitEvent*    _pEvent ;
      monAppCB            _monAppBegin ;
      BOOLEAN             _hasSubmit ;
      BOOLEAN             _hasDiscard ;
   } ;

   typedef _rtnLobMetricsSubmitor rtnLobMetricsSubmitor ;

}

#endif

