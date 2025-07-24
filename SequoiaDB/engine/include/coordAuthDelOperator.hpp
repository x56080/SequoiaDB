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

   Source File Name = coordAuthDelOperator.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_AUTHDEL_OPERATOR_HPP__
#define COORD_AUTHDEL_OPERATOR_HPP__

#include "coordAuthBase.hpp"

namespace engine
{
   /*
      _coordAuthDelOperator define
   */
   class _coordAuthDelOperator : public _coordAuthBase
   {
      public:
         _coordAuthDelOperator() ;
         virtual ~_coordAuthDelOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32        execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

         virtual BOOLEAN      isReadOnly() const ;

   } ;
   typedef _coordAuthDelOperator coordAuthDelOperator ;

}

#endif // COORD_AUTHDEL_OPERATOR_HPP__

